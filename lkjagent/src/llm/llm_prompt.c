/**
 * @file llm_prompt.c
 * @brief LLM prompt construction implementation
 * 
 * This module implements state-specific prompt construction capabilities for 
 * different agent states (thinking, executing, evaluating, paging). It provides
 * template management, dynamic prompt generation, and optimization features.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/llm/llm_prompt.h"
#include "../lkjagent.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
 * @defgroup LLM_Prompt_Templates Prompt Templates
 * @{
 */

static const char* THINKING_TEMPLATE = 
    "You are in THINKING mode. Your task is to carefully analyze the current situation, "
    "consider all available information, and plan your next actions.\n\n"
    "Instructions:\n"
    "- Use <thinking> tags to structure your reasoning process\n"
    "- Consider multiple perspectives and potential outcomes\n"
    "- Identify key decisions that need to be made\n"
    "- Plan concrete next steps\n"
    "- Be thorough but concise in your analysis\n\n"
    "Context:\n%s\n\n"
    "Current Situation: %s\n\n"
    "Please analyze this situation and provide your thoughts within <thinking> tags.";

static const char* EXECUTING_TEMPLATE = 
    "You are in EXECUTING mode. Your task is to take concrete actions and implement "
    "your planned operations efficiently and precisely.\n\n"
    "Instructions:\n"
    "- Use <action> tags to specify commands and operations\n"
    "- Focus on implementation details and execution steps\n"
    "- Be specific about what needs to be done\n"
    "- Consider error handling and edge cases\n"
    "- Provide clear, actionable directives\n\n"
    "Context:\n%s\n\n"
    "Action Plan: %s\n\n"
    "Please execute the planned actions and provide directives within <action> tags.";

static const char* EVALUATING_TEMPLATE = 
    "You are in EVALUATING mode. Your task is to assess the results of recent actions, "
    "measure progress toward goals, and identify areas for improvement.\n\n"
    "Instructions:\n"
    "- Use <evaluation> tags to structure your assessment\n"
    "- Analyze what worked well and what didn't\n"
    "- Measure progress against intended goals\n"
    "- Identify lessons learned and improvements needed\n"
    "- Provide specific recommendations for future actions\n\n"
    "Context:\n%s\n\n"
    "Recent Actions: %s\n\n"
    "Please evaluate the results and provide your assessment within <evaluation> tags.";

static const char* PAGING_TEMPLATE = 
    "You are in PAGING mode. Your task is to manage memory and context efficiently, "
    "deciding what information to keep, move, or archive.\n\n"
    "Instructions:\n"
    "- Use <paging> tags to specify memory management directives\n"
    "- Identify information that should remain in working memory\n"
    "- Determine what should be moved to disk storage\n"
    "- Decide what can be archived or deleted\n"
    "- Optimize memory usage for future operations\n\n"
    "Context:\n%s\n\n"
    "Memory Status: %s\n\n"
    "Please manage the memory and provide directives within <paging> tags.";

/** @} */

/**
 * @defgroup LLM_Prompt_Internal Internal Prompt Functions
 * @{
 */

/**
 * @brief Get template for specified state
 * 
 * @param state Agent state
 * @return Template string or NULL if invalid state
 */
static const char* get_state_template(agent_state_t state) {
    switch (state) {
        case STATE_THINKING:
            return THINKING_TEMPLATE;
        case STATE_EXECUTING:
            return EXECUTING_TEMPLATE;
        case STATE_EVALUATING:
            return EVALUATING_TEMPLATE;
        case STATE_PAGING:
            return PAGING_TEMPLATE;
        default:
            return NULL;
    }
}

/**
 * @brief Apply template variables to a template string
 * 
 * @param template Template string with %s placeholders
 * @param context_str Context information
 * @param state_str State-specific information
 * @param result Buffer to store result
 * @param max_size Maximum size of result buffer
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t apply_template(const char* template, const char* context_str, 
                              const char* state_str, char* result, size_t max_size) {
    if (!template || !context_str || !state_str || !result || max_size == 0) {
        return RESULT_ERR;
    }
    
    int written = snprintf(result, max_size, template, context_str, state_str);
    if (written < 0 || (size_t)written >= max_size) {
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

/**
 * @brief Extract context summary from context object
 * 
 * @param context LLM context object
 * @param summary Buffer to store summary
 * @param max_size Maximum size of summary buffer
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t extract_context_summary(const llm_context_t* context, char* summary, size_t max_size) {
    if (!context || !summary || max_size == 0) {
        return RESULT_ERR;
    }
    
    summary[0] = '\0';
    size_t used = 0;
    
    /* Add memory components summary */
    if (context->memory_count > 0) {
        int written = snprintf(summary + used, max_size - used, 
                              "Memory Components (%zu):\n", context->memory_count);
        if (written < 0 || used + written >= max_size) return RESULT_ERR;
        used += written;
        
        for (size_t i = 0; i < MIN(context->memory_count, 3); i++) {
            const llm_context_component_t* comp = &context->memory_components[i];
            if (comp->context_key[0] != '\0') {
                written = snprintf(summary + used, max_size - used, 
                                 "- [%s]: %.100s%s\n", 
                                 comp->context_key,
                                 comp->content.data ? comp->content.data : "",
                                 comp->content.size > 100 ? "..." : "");
            } else {
                written = snprintf(summary + used, max_size - used, 
                                 "- %.100s%s\n",
                                 comp->content.data ? comp->content.data : "",
                                 comp->content.size > 100 ? "..." : "");
            }
            if (written < 0 || used + written >= max_size) break;
            used += written;
        }
        
        if (context->memory_count > 3) {
            written = snprintf(summary + used, max_size - used, 
                             "... and %zu more components\n", context->memory_count - 3);
            if (written > 0 && used + written < max_size) used += written;
        }
    } else {
        int written = snprintf(summary + used, max_size - used, "No memory components available.\n");
        if (written > 0 && used + written < max_size) used += written;
    }
    
    return RESULT_OK;
}

/**
 * @brief Generate state-specific information string
 * 
 * @param state Agent state
 * @param config Prompt configuration
 * @param state_info Buffer to store state information
 * @param max_size Maximum size of state info buffer
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t generate_state_info(agent_state_t state, const llm_prompt_config_t* config, 
                                   char* state_info, size_t max_size) {
    if (!config || !state_info || max_size == 0) {
        return RESULT_ERR;
    }
    
    const char* state_name = "";
    const char* state_focus = "";
    
    switch (state) {
        case STATE_THINKING:
            state_name = "THINKING";
            state_focus = "Analysis and planning are your priorities. Consider all angles and plan thoroughly.";
            break;
        case STATE_EXECUTING:
            state_name = "EXECUTING";
            state_focus = "Implementation and action are your priorities. Be precise and efficient.";
            break;
        case STATE_EVALUATING:
            state_name = "EVALUATING";
            state_focus = "Assessment and improvement are your priorities. Measure results and learn.";
            break;
        case STATE_PAGING:
            state_name = "PAGING";
            state_focus = "Memory management is your priority. Optimize information storage and access.";
            break;
        default:
            state_name = "UNKNOWN";
            state_focus = "Respond appropriately to the given context.";
            break;
    }
    
    int written = snprintf(state_info, max_size,
                          "Current State: %s\n"
                          "Focus: %s\n"
                          "Temperature: %.2f\n"
                          "Max Tokens: %zu\n"
                          "Timestamp: %lu",
                          state_name, state_focus,
                          config->temperature,
                          config->max_tokens,
                          (unsigned long)time(NULL));
    
    if (written < 0 || (size_t)written >= max_size) {
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

/** @} */

result_t llm_prompt_build_thinking(const llm_context_t* context, const llm_prompt_config_t* config, data_t* prompt) {
    if (!context || !config || !prompt) {
        RETURN_ERR("Invalid parameters for thinking prompt construction");
        return RESULT_ERR;
    }
    
    char context_summary[2048];
    char state_info[512];
    char final_prompt[4096];
    
    /* Extract context summary */
    if (extract_context_summary(context, context_summary, sizeof(context_summary)) != RESULT_OK) {
        RETURN_ERR("Failed to extract context summary for thinking prompt");
        return RESULT_ERR;
    }
    
    /* Generate state information */
    if (generate_state_info(STATE_THINKING, config, state_info, sizeof(state_info)) != RESULT_OK) {
        RETURN_ERR("Failed to generate state information for thinking prompt");
        return RESULT_ERR;
    }
    
    /* Apply template */
    if (apply_template(THINKING_TEMPLATE, context_summary, state_info, final_prompt, sizeof(final_prompt)) != RESULT_OK) {
        RETURN_ERR("Failed to apply thinking template");
        return RESULT_ERR;
    }
    
    /* Store in data buffer */
    if (data_set(prompt, final_prompt, 0) != RESULT_OK) {
        RETURN_ERR("Failed to store thinking prompt in data buffer");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t llm_prompt_build_executing(const llm_context_t* context, const llm_prompt_config_t* config, data_t* prompt) {
    if (!context || !config || !prompt) {
        RETURN_ERR("Invalid parameters for executing prompt construction");
        return RESULT_ERR;
    }
    
    char context_summary[2048];
    char state_info[512];
    char final_prompt[4096];
    
    /* Extract context summary */
    if (extract_context_summary(context, context_summary, sizeof(context_summary)) != RESULT_OK) {
        RETURN_ERR("Failed to extract context summary for executing prompt");
        return RESULT_ERR;
    }
    
    /* Generate state information */
    if (generate_state_info(STATE_EXECUTING, config, state_info, sizeof(state_info)) != RESULT_OK) {
        RETURN_ERR("Failed to generate state information for executing prompt");
        return RESULT_ERR;
    }
    
    /* Apply template */
    if (apply_template(EXECUTING_TEMPLATE, context_summary, state_info, final_prompt, sizeof(final_prompt)) != RESULT_OK) {
        RETURN_ERR("Failed to apply executing template");
        return RESULT_ERR;
    }
    
    /* Store in data buffer */
    if (data_set(prompt, final_prompt, 0) != RESULT_OK) {
        RETURN_ERR("Failed to store executing prompt in data buffer");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t llm_prompt_build_evaluating(const llm_context_t* context, const llm_prompt_config_t* config, data_t* prompt) {
    if (!context || !config || !prompt) {
        RETURN_ERR("Invalid parameters for evaluating prompt construction");
        return RESULT_ERR;
    }
    
    char context_summary[2048];
    char state_info[512];
    char final_prompt[4096];
    
    /* Extract context summary */
    if (extract_context_summary(context, context_summary, sizeof(context_summary)) != RESULT_OK) {
        RETURN_ERR("Failed to extract context summary for evaluating prompt");
        return RESULT_ERR;
    }
    
    /* Generate state information */
    if (generate_state_info(STATE_EVALUATING, config, state_info, sizeof(state_info)) != RESULT_OK) {
        RETURN_ERR("Failed to generate state information for evaluating prompt");
        return RESULT_ERR;
    }
    
    /* Apply template */
    if (apply_template(EVALUATING_TEMPLATE, context_summary, state_info, final_prompt, sizeof(final_prompt)) != RESULT_OK) {
        RETURN_ERR("Failed to apply evaluating template");
        return RESULT_ERR;
    }
    
    /* Store in data buffer */
    if (data_set(prompt, final_prompt, 0) != RESULT_OK) {
        RETURN_ERR("Failed to store evaluating prompt in data buffer");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t llm_prompt_build_paging(const llm_context_t* context, const llm_prompt_config_t* config, data_t* prompt) {
    if (!context || !config || !prompt) {
        RETURN_ERR("Invalid parameters for paging prompt construction");
        return RESULT_ERR;
    }
    
    char context_summary[2048];
    char state_info[512];
    char final_prompt[4096];
    
    /* Extract context summary */
    if (extract_context_summary(context, context_summary, sizeof(context_summary)) != RESULT_OK) {
        RETURN_ERR("Failed to extract context summary for paging prompt");
        return RESULT_ERR;
    }
    
    /* Generate state information with memory focus */
    int written = snprintf(state_info, sizeof(state_info),
                          "Current State: PAGING\n"
                          "Memory Usage: %zu tokens\n"
                          "Memory Components: %zu\n"
                          "Context Limit: Within limits = %s\n"
                          "Temperature: %.2f\n"
                          "Timestamp: %lu",
                          context->total_tokens,
                          context->memory_count,
                          context->within_limits ? "Yes" : "No",
                          config->temperature,
                          (unsigned long)time(NULL));
    
    if (written < 0 || (size_t)written >= sizeof(state_info)) {
        RETURN_ERR("Failed to generate paging state information");
        return RESULT_ERR;
    }
    
    /* Apply template */
    if (apply_template(PAGING_TEMPLATE, context_summary, state_info, final_prompt, sizeof(final_prompt)) != RESULT_OK) {
        RETURN_ERR("Failed to apply paging template");
        return RESULT_ERR;
    }
    
    /* Store in data buffer */
    if (data_set(prompt, final_prompt, 0) != RESULT_OK) {
        RETURN_ERR("Failed to store paging prompt in data buffer");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t llm_prompt_build_custom(const char* template_str, const llm_context_t* context, 
                                const llm_prompt_config_t* config, data_t* prompt) {
    if (!template_str || !context || !config || !prompt) {
        RETURN_ERR("Invalid parameters for custom prompt construction");
        return RESULT_ERR;
    }
    
    char context_summary[2048];
    char state_info[512];
    char final_prompt[4096];
    
    /* Extract context summary */
    if (extract_context_summary(context, context_summary, sizeof(context_summary)) != RESULT_OK) {
        RETURN_ERR("Failed to extract context summary for custom prompt");
        return RESULT_ERR;
    }
    
    /* Generate generic state information */
    int written = snprintf(state_info, sizeof(state_info),
                          "Configuration:\n"
                          "Temperature: %.2f\n"
                          "Max Tokens: %zu\n"
                          "Context Tokens: %zu\n"
                          "Timestamp: %lu",
                          config->temperature,
                          config->max_tokens,
                          context->total_tokens,
                          (unsigned long)time(NULL));
    
    if (written < 0 || (size_t)written >= sizeof(state_info)) {
        RETURN_ERR("Failed to generate custom state information");
        return RESULT_ERR;
    }
    
    /* Apply custom template */
    if (apply_template(template_str, context_summary, state_info, final_prompt, sizeof(final_prompt)) != RESULT_OK) {
        RETURN_ERR("Failed to apply custom template");
        return RESULT_ERR;
    }
    
    /* Store in data buffer */
    if (data_set(prompt, final_prompt, 0) != RESULT_OK) {
        RETURN_ERR("Failed to store custom prompt in data buffer");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t llm_prompt_apply_template(const char* template_str, const char** variables, size_t var_count, data_t* prompt) {
    if (!template_str || !prompt) {
        RETURN_ERR("Invalid parameters for template application");
        return RESULT_ERR;
    }
    
    /* For now, implement simple %s substitution for up to 2 variables */
    if (var_count >= 2 && variables) {
        char result[4096];
        int written = snprintf(result, sizeof(result), template_str, variables[0], variables[1]);
        if (written < 0 || (size_t)written >= sizeof(result)) {
            RETURN_ERR("Template application resulted in truncation");
            return RESULT_ERR;
        }
        
        if (data_set(prompt, result, 0) != RESULT_OK) {
            RETURN_ERR("Failed to store template result in data buffer");
            return RESULT_ERR;
        }
    } else if (var_count >= 1 && variables) {
        char result[4096];
        int written = snprintf(result, sizeof(result), template_str, variables[0]);
        if (written < 0 || (size_t)written >= sizeof(result)) {
            RETURN_ERR("Template application resulted in truncation");
            return RESULT_ERR;
        }
        
        if (data_set(prompt, result, 0) != RESULT_OK) {
            RETURN_ERR("Failed to store template result in data buffer");
            return RESULT_ERR;
        }
    } else {
        /* No variables - use template as-is */
        if (data_set(prompt, template_str, 0) != RESULT_OK) {
            RETURN_ERR("Failed to store template in data buffer");
            return RESULT_ERR;
        }
    }
    
    return RESULT_OK;
}

result_t llm_prompt_optimize_length(data_t* prompt, size_t max_tokens, const llm_prompt_config_t* config) {
    if (!prompt || !config || max_tokens == 0) {
        RETURN_ERR("Invalid parameters for prompt optimization");
        return RESULT_ERR;
    }
    
    /* Estimate current token count (4 chars per token) */
    size_t estimated_tokens = (prompt->size + 3) / 4;
    
    if (estimated_tokens <= max_tokens) {
        return RESULT_OK; /* Already within limits */
    }
    
    /* Calculate target character count */
    size_t target_chars = max_tokens * 4;
    if (target_chars >= prompt->size) {
        return RESULT_OK; /* Shouldn't happen, but safe check */
    }
    
    /* Truncate at word boundary */
    char* content = (char*)prompt->data;
    size_t truncate_pos = target_chars;
    
    /* Find last space before truncation point */
    while (truncate_pos > 0 && content[truncate_pos] != ' ' && content[truncate_pos] != '\n') {
        truncate_pos--;
    }
    
    if (truncate_pos == 0) {
        /* No good break point found, use hard truncation */
        truncate_pos = target_chars - 10; /* Leave room for ellipsis */
    }
    
    /* Add ellipsis and null terminator */
    strcpy(content + truncate_pos, "...");
    prompt->size = truncate_pos + 3;
    
    return RESULT_OK;
}

result_t llm_prompt_add_instructions(data_t* prompt, const char* instructions) {
    if (!prompt || !instructions) {
        RETURN_ERR("Invalid parameters for adding instructions");
        return RESULT_ERR;
    }
    
    /* Add instructions at the end */
    if (data_append(prompt, "\n\nAdditional Instructions:\n", 0) != RESULT_OK) {
        RETURN_ERR("Failed to add instruction header");
        return RESULT_ERR;
    }
    
    if (data_append(prompt, instructions, 0) != RESULT_OK) {
        RETURN_ERR("Failed to add instructions content");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t llm_prompt_add_context(data_t* prompt, const char* context_info) {
    if (!prompt || !context_info) {
        RETURN_ERR("Invalid parameters for adding context");
        return RESULT_ERR;
    }
    
    /* Add context before the main prompt */
    data_t temp_data;
    if (data_init(&temp_data, prompt->size + strlen(context_info) + 64) != RESULT_OK) {
        RETURN_ERR("Failed to initialize temporary buffer for context addition");
        return RESULT_ERR;
    }
    
    /* Build new prompt with context first */
    if (data_set(&temp_data, "Additional Context:\n", 0) != RESULT_OK ||
        data_append(&temp_data, context_info, 0) != RESULT_OK ||
        data_append(&temp_data, "\n\n", 0) != RESULT_OK ||
        data_append(&temp_data, prompt->data, 0) != RESULT_OK) {
        data_clear(&temp_data);
        RETURN_ERR("Failed to build prompt with additional context");
        return RESULT_ERR;
    }
    
    /* Replace original prompt */
    data_clear(prompt);
    *prompt = temp_data;
    
    return RESULT_OK;
}

result_t llm_prompt_set_temperature(llm_prompt_config_t* config, float temperature) {
    if (!config) {
        RETURN_ERR("Invalid config parameter for temperature setting");
        return RESULT_ERR;
    }
    
    if (temperature < 0.0f || temperature > 2.0f) {
        RETURN_ERR("Temperature must be between 0.0 and 2.0");
        return RESULT_ERR;
    }
    
    config->temperature = temperature;
    return RESULT_OK;
}

result_t llm_prompt_set_max_tokens(llm_prompt_config_t* config, size_t max_tokens) {
    if (!config) {
        RETURN_ERR("Invalid config parameter for max tokens setting");
        return RESULT_ERR;
    }
    
    if (max_tokens == 0 || max_tokens > 32768) {
        RETURN_ERR("Max tokens must be between 1 and 32768");
        return RESULT_ERR;
    }
    
    config->max_tokens = max_tokens;
    return RESULT_OK;
}

result_t llm_prompt_configure_state(llm_prompt_config_t* config, agent_state_t state) {
    if (!config) {
        RETURN_ERR("Invalid config parameter for state configuration");
        return RESULT_ERR;
    }
    
    /* Set state-specific configuration parameters */
    switch (state) {
        case STATE_THINKING:
            config->temperature = 0.7f;  /* Balanced creativity and accuracy */
            config->max_tokens = 2048;   /* Longer responses for analysis */
            break;
            
        case STATE_EXECUTING:
            config->temperature = 0.3f;  /* Lower temperature for precision */
            config->max_tokens = 1024;   /* Concise action directives */
            break;
            
        case STATE_EVALUATING:
            config->temperature = 0.5f;  /* Moderate creativity for assessment */
            config->max_tokens = 1536;   /* Medium length for evaluation */
            break;
            
        case STATE_PAGING:
            config->temperature = 0.2f;  /* Very low for memory management */
            config->max_tokens = 512;    /* Short directives for memory ops */
            break;
            
        default:
            config->temperature = 0.5f;  /* Default balanced setting */
            config->max_tokens = 1024;   /* Default response length */
            break;
    }
    
    return RESULT_OK;
}

result_t llm_prompt_init_config(llm_prompt_config_t* config) {
    if (!config) {
        RETURN_ERR("Config pointer is NULL");
        return RESULT_ERR;
    }
    
    /* Set default configuration */
    config->temperature = 0.5f;
    config->max_tokens = 1024;
    config->use_templates = true;
    config->optimize_length = true;
    
    return RESULT_OK;
}

result_t llm_prompt_validate_config(const llm_prompt_config_t* config) {
    if (!config) {
        RETURN_ERR("Config pointer is NULL");
        return RESULT_ERR;
    }
    
    if (config->temperature < 0.0f || config->temperature > 2.0f) {
        RETURN_ERR("Invalid temperature value");
        return RESULT_ERR;
    }
    
    if (config->max_tokens == 0 || config->max_tokens > 32768) {
        RETURN_ERR("Invalid max_tokens value");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}
