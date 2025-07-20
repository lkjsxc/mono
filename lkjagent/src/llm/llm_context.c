/**
 * @file llm_context.c
 * @brief LLM context preparation and management implementation
 * 
 * This module implements context preparation capabilities for LLM interactions,
 * including memory integration, context window management, and intelligent
 * context sizing for optimal LLM performance.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/llm/llm_context.h"
#include "../lkjagent.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

/**
 * @defgroup LLM_Context_Internal Internal LLM Context Functions
 * @{
 */

/**
 * @brief Estimate token count from character count
 * 
 * @param char_count Number of characters
 * @return Estimated token count
 */
static size_t estimate_tokens_from_chars(size_t char_count) {
    /* Conservative estimate: 4 characters per token on average */
    return (char_count + 3) / 4;
}

/**
 * @brief Get state-specific system prompt
 * 
 * @param state Agent state
 * @param system_prompt Buffer to store system prompt
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t get_state_system_prompt(agent_state_t state, data_t* system_prompt) {
    const char* prompt_text = "";
    
    switch (state) {
        case STATE_THINKING:
            prompt_text = "You are an autonomous AI agent in THINKING mode. Analyze the situation carefully, "
                         "consider all available information, and plan your next actions. Use <thinking> tags "
                         "to show your reasoning process. Be thorough and consider multiple perspectives.";
            break;
            
        case STATE_EXECUTING:
            prompt_text = "You are an autonomous AI agent in EXECUTING mode. Focus on taking concrete actions "
                         "and implementing your plans. Use <action> tags to specify commands and operations. "
                         "Be precise and efficient in your execution.";
            break;
            
        case STATE_EVALUATING:
            prompt_text = "You are an autonomous AI agent in EVALUATING mode. Assess the results of recent "
                         "actions, measure progress toward goals, and identify areas for improvement. Use "
                         "<evaluation> tags to structure your assessment.";
            break;
            
        case STATE_PAGING:
            prompt_text = "You are an autonomous AI agent in PAGING mode. Manage memory and context efficiently. "
                         "Decide which information to keep in working memory, move to disk, or archive. Use "
                         "<paging> tags to specify memory management directives.";
            break;
            
        default:
            prompt_text = "You are an autonomous AI agent. Respond appropriately to the given context and instructions.";
            break;
    }
    
    return data_set(system_prompt, prompt_text, 0);
}

/**
 * @brief Sort components by priority
 * 
 * @param components Array of components to sort
 * @param count Number of components
 */
static void sort_components_by_priority(llm_context_component_t* components, size_t count) {
    if (!components || count <= 1) return;
    
    /* Simple bubble sort by priority (descending) */
    for (size_t i = 0; i < count - 1; i++) {
        for (size_t j = 0; j < count - 1 - i; j++) {
            if (components[j].priority < components[j + 1].priority) {
                /* Swap components */
                llm_context_component_t temp = components[j];
                components[j] = components[j + 1];
                components[j + 1] = temp;
            }
        }
    }
}

/**
 * @brief Calculate component priority based on state and metadata
 * 
 * @param component Component to calculate priority for
 * @param current_state Current agent state
 * @param config Context configuration
 * @return Calculated priority score
 */
static size_t calculate_component_priority(const llm_context_component_t* component, 
                                         agent_state_t current_state,
                                         const llm_context_config_t* config) {
    if (!component || !config) return 0;
    
    size_t base_priority = component->priority;
    
    /* Apply state-specific weights */
    if (strcmp(component->component_type, "system") == 0) {
        base_priority = (size_t)(base_priority * config->priority_weights.system_prompt_weight);
    } else if (strcmp(component->component_type, "memory_recent") == 0) {
        base_priority = (size_t)(base_priority * config->priority_weights.recent_memory_weight);
    } else if (strcmp(component->component_type, "memory_important") == 0) {
        base_priority = (size_t)(base_priority * config->priority_weights.important_memory_weight);
    } else if (strcmp(component->component_type, "state") == 0) {
        base_priority = (size_t)(base_priority * config->priority_weights.current_state_weight);
    }
    
    /* Time-based decay for older components */
    time_t now = time(NULL);
    if (component->timestamp > 0) {
        time_t age = now - component->timestamp;
        if (age > 3600) { /* Older than 1 hour */
            base_priority = (size_t)(base_priority * 0.8f);
        }
        if (age > 86400) { /* Older than 1 day */
            base_priority = (size_t)(base_priority * 0.6f);
        }
    }
    
    return MIN(base_priority, 100);
}

/** @} */

result_t llm_context_prepare(void* memory, agent_state_t current_state, const llm_context_config_t* config, llm_context_t* context) {
    if (!config || !context) {
        RETURN_ERR("Invalid parameters for context preparation");
        return RESULT_ERR;
    }
    
    /* Clear context structure */
    context->memory_count = 0;
    context->total_tokens = 0;
    context->within_limits = false;
    context->preparation_time = time(NULL);
    
    /* Add system prompt */
    if (get_state_system_prompt(current_state, &context->system_prompt.content) != RESULT_OK) {
        RETURN_ERR("Failed to get system prompt for state");
        return RESULT_ERR;
    }
    
    strcpy(context->system_prompt.component_type, "system");
    context->system_prompt.priority = 100; /* Highest priority */
    context->system_prompt.token_count = estimate_tokens_from_chars(context->system_prompt.content.size);
    context->system_prompt.timestamp = time(NULL);
    
    /* Add current state information */
    char state_info[512];
    const char* state_name = "";
    switch (current_state) {
        case STATE_THINKING: state_name = "THINKING"; break;
        case STATE_EXECUTING: state_name = "EXECUTING"; break;
        case STATE_EVALUATING: state_name = "EVALUATING"; break;
        case STATE_PAGING: state_name = "PAGING"; break;
        default: state_name = "UNKNOWN"; break;
    }
    
    snprintf(state_info, sizeof(state_info), 
             "Current agent state: %s\nTimestamp: %lu\n", 
             state_name, (unsigned long)time(NULL));
    
    if (data_set(&context->current_state.content, state_info, 0) != RESULT_OK) {
        RETURN_ERR("Failed to set current state information");
        return RESULT_ERR;
    }
    
    strcpy(context->current_state.component_type, "state");
    context->current_state.priority = 80;
    context->current_state.token_count = estimate_tokens_from_chars(context->current_state.content.size);
    context->current_state.timestamp = time(NULL);
    
    /* Add memory components if memory system is available */
    if (memory && llm_context_add_memory(context, memory, 32) != RESULT_OK) {
        /* Non-fatal - continue without memory components */
    }
    
    /* Calculate total token count */
    context->total_tokens = context->system_prompt.token_count + context->current_state.token_count;
    for (size_t i = 0; i < context->memory_count; i++) {
        context->total_tokens += context->memory_components[i].token_count;
    }
    
    /* Check if within limits */
    size_t available_tokens = config->max_context_tokens - config->response_token_reserve;
    context->within_limits = (context->total_tokens <= available_tokens);
    
    /* Trim if necessary */
    if (!context->within_limits) {
        if (llm_context_trim_size(context, config) != RESULT_OK) {
            RETURN_ERR("Failed to trim context to fit within limits");
            return RESULT_ERR;
        }
    }
    
    /* Apply prioritization */
    if (llm_context_prioritize(context, current_state, config) != RESULT_OK) {
        /* Non-fatal - continue with default prioritization */
    }
    
    return RESULT_OK;
}

result_t llm_context_build_prompt(const llm_context_t* context, data_t* prompt_buffer) {
    if (!context || !prompt_buffer) {
        RETURN_ERR("Invalid parameters for prompt building");
        return RESULT_ERR;
    }
    
    /* Clear prompt buffer */
    if (data_set(prompt_buffer, "", 0) != RESULT_OK) {
        RETURN_ERR("Failed to clear prompt buffer");
        return RESULT_ERR;
    }
    
    /* Add system prompt */
    if (context->system_prompt.content.size > 0) {
        if (data_append(prompt_buffer, context->system_prompt.content.data, 0) != RESULT_OK) {
            RETURN_ERR("Failed to add system prompt to prompt buffer");
            return RESULT_ERR;
        }
        if (data_append(prompt_buffer, "\n\n", 0) != RESULT_OK) {
            RETURN_ERR("Failed to add separator after system prompt");
            return RESULT_ERR;
        }
    }
    
    /* Add memory components in priority order */
    if (context->memory_count > 0) {
        if (data_append(prompt_buffer, "Relevant Memory:\n", 0) != RESULT_OK) {
            RETURN_ERR("Failed to add memory section header");
            return RESULT_ERR;
        }
        
        for (size_t i = 0; i < context->memory_count; i++) {
            const llm_context_component_t* component = &context->memory_components[i];
            
            if (component->content.size > 0) {
                /* Add context key if available */
                if (component->context_key[0] != '\0') {
                    char key_header[128];
                    snprintf(key_header, sizeof(key_header), "[%s]: ", component->context_key);
                    if (data_append(prompt_buffer, key_header, 0) != RESULT_OK) {
                        RETURN_ERR("Failed to add context key header");
                        return RESULT_ERR;
                    }
                }
                
                if (data_append(prompt_buffer, component->content.data, 0) != RESULT_OK) {
                    RETURN_ERR("Failed to add memory component to prompt");
                    return RESULT_ERR;
                }
                if (data_append(prompt_buffer, "\n", 0) != RESULT_OK) {
                    RETURN_ERR("Failed to add newline after memory component");
                    return RESULT_ERR;
                }
            }
        }
        
        if (data_append(prompt_buffer, "\n", 0) != RESULT_OK) {
            RETURN_ERR("Failed to add separator after memory section");
            return RESULT_ERR;
        }
    }
    
    /* Add current state information */
    if (context->current_state.content.size > 0) {
        if (data_append(prompt_buffer, context->current_state.content.data, 0) != RESULT_OK) {
            RETURN_ERR("Failed to add current state to prompt buffer");
            return RESULT_ERR;
        }
        if (data_append(prompt_buffer, "\n\n", 0) != RESULT_OK) {
            RETURN_ERR("Failed to add separator after current state");
            return RESULT_ERR;
        }
    }
    
    /* Add final instruction */
    const char* final_instruction = "Please respond appropriately to the above context and your current state.";
    if (data_append(prompt_buffer, final_instruction, 0) != RESULT_OK) {
        RETURN_ERR("Failed to add final instruction to prompt");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t llm_context_add_system(llm_context_t* context, agent_state_t current_state, const char* system_prompt) {
    if (!context || !system_prompt) {
        RETURN_ERR("Invalid parameters for adding system prompt");
        return RESULT_ERR;
    }
    
    /* Set system prompt content */
    if (data_set(&context->system_prompt.content, system_prompt, 0) != RESULT_OK) {
        RETURN_ERR("Failed to set system prompt content");
        return RESULT_ERR;
    }
    
    /* Set metadata */
    strcpy(context->system_prompt.component_type, "system");
    context->system_prompt.priority = 100;
    context->system_prompt.token_count = estimate_tokens_from_chars(context->system_prompt.content.size);
    context->system_prompt.timestamp = time(NULL);
    
    return RESULT_OK;
}

result_t llm_context_add_memory(llm_context_t* context, void* memory, size_t max_memory_components) {
    if (!context || !memory) {
        RETURN_ERR("Invalid parameters for adding memory context");
        return RESULT_ERR;
    }
    
    /* For now, add some placeholder memory components */
    /* In the full implementation, this would integrate with the memory system */
    
    size_t components_added = 0;
    max_memory_components = MIN(max_memory_components, 64);
    
    /* Add a sample recent memory component */
    if (components_added < max_memory_components) {
        llm_context_component_t* component = &context->memory_components[components_added];
        
        if (data_set(&component->content, "Recent context: Working on LLM integration", 0) == RESULT_OK) {
            strcpy(component->component_type, "memory_recent");
            strcpy(component->context_key, "llm_integration");
            component->priority = 70;
            component->token_count = estimate_tokens_from_chars(component->content.size);
            component->timestamp = time(NULL) - 300; /* 5 minutes ago */
            components_added++;
        }
    }
    
    /* Add a sample important memory component */
    if (components_added < max_memory_components) {
        llm_context_component_t* component = &context->memory_components[components_added];
        
        if (data_set(&component->content, "Important context: System architecture decisions", 0) == RESULT_OK) {
            strcpy(component->component_type, "memory_important");
            strcpy(component->context_key, "architecture");
            component->priority = 90;
            component->token_count = estimate_tokens_from_chars(component->content.size);
            component->timestamp = time(NULL) - 3600; /* 1 hour ago */
            components_added++;
        }
    }
    
    context->memory_count = components_added;
    return RESULT_OK;
}

result_t llm_context_add_state(llm_context_t* context, agent_state_t current_state, const char* state_info) {
    if (!context) {
        RETURN_ERR("Invalid context parameter for adding state");
        return RESULT_ERR;
    }
    
    char state_content[1024];
    const char* state_name = "";
    
    switch (current_state) {
        case STATE_THINKING: state_name = "THINKING"; break;
        case STATE_EXECUTING: state_name = "EXECUTING"; break;
        case STATE_EVALUATING: state_name = "EVALUATING"; break;
        case STATE_PAGING: state_name = "PAGING"; break;
        default: state_name = "UNKNOWN"; break;
    }
    
    if (state_info) {
        snprintf(state_content, sizeof(state_content), 
                "Current State: %s\nState Information: %s\nTimestamp: %lu", 
                state_name, state_info, (unsigned long)time(NULL));
    } else {
        snprintf(state_content, sizeof(state_content), 
                "Current State: %s\nTimestamp: %lu", 
                state_name, (unsigned long)time(NULL));
    }
    
    if (data_set(&context->current_state.content, state_content, 0) != RESULT_OK) {
        RETURN_ERR("Failed to set current state content");
        return RESULT_ERR;
    }
    
    strcpy(context->current_state.component_type, "state");
    context->current_state.priority = 80;
    context->current_state.token_count = estimate_tokens_from_chars(context->current_state.content.size);
    context->current_state.timestamp = time(NULL);
    
    return RESULT_OK;
}

result_t llm_context_trim_size(llm_context_t* context, const llm_context_config_t* config) {
    if (!context || !config) {
        RETURN_ERR("Invalid parameters for context trimming");
        return RESULT_ERR;
    }
    
    size_t target_tokens = config->max_context_tokens - config->response_token_reserve;
    
    /* Sort memory components by priority (ascending for removal) */
    sort_components_by_priority(context->memory_components, context->memory_count);
    
    /* Remove lowest priority components until we fit */
    while (context->total_tokens > target_tokens && context->memory_count > 0) {
        /* Remove the last (lowest priority) component */
        context->memory_count--;
        llm_context_component_t* removed = &context->memory_components[context->memory_count];
        
        context->total_tokens -= removed->token_count;
        
        /* Clear the removed component */
        data_clear(&removed->content);
        memset(removed, 0, sizeof(llm_context_component_t));
    }
    
    /* Recalculate total tokens */
    context->total_tokens = context->system_prompt.token_count + context->current_state.token_count;
    for (size_t i = 0; i < context->memory_count; i++) {
        context->total_tokens += context->memory_components[i].token_count;
    }
    
    context->within_limits = (context->total_tokens <= target_tokens);
    
    return RESULT_OK;
}

result_t llm_context_prioritize(llm_context_t* context, agent_state_t current_state, const llm_context_config_t* config) {
    if (!context || !config) {
        RETURN_ERR("Invalid parameters for context prioritization");
        return RESULT_ERR;
    }
    
    /* Recalculate priorities for all components */
    context->system_prompt.priority = calculate_component_priority(&context->system_prompt, current_state, config);
    context->current_state.priority = calculate_component_priority(&context->current_state, current_state, config);
    
    for (size_t i = 0; i < context->memory_count; i++) {
        context->memory_components[i].priority = calculate_component_priority(&context->memory_components[i], current_state, config);
    }
    
    /* Sort memory components by priority (descending) */
    sort_components_by_priority(context->memory_components, context->memory_count);
    
    return RESULT_OK;
}

result_t llm_context_calculate_size(const llm_context_t* context, size_t* token_count) {
    if (!context || !token_count) {
        RETURN_ERR("Invalid parameters for context size calculation");
        return RESULT_ERR;
    }
    
    *token_count = context->system_prompt.token_count + context->current_state.token_count;
    
    for (size_t i = 0; i < context->memory_count; i++) {
        *token_count += context->memory_components[i].token_count;
    }
    
    /* Add overhead for formatting (approximately 10%) */
    *token_count = (size_t)(*token_count * 1.1f);
    
    return RESULT_OK;
}

result_t llm_context_fit_window(llm_context_t* context, size_t available_tokens, const llm_context_config_t* config) {
    if (!context || !config) {
        RETURN_ERR("Invalid parameters for context window fitting");
        return RESULT_ERR;
    }
    
    /* Update config with available tokens */
    llm_context_config_t temp_config = *config;
    temp_config.max_context_tokens = available_tokens;
    
    return llm_context_trim_size(context, &temp_config);
}

result_t llm_context_summarize_old(llm_context_t* context, size_t component_index) {
    if (!context || component_index >= context->memory_count) {
        RETURN_ERR("Invalid parameters for context summarization");
        return RESULT_ERR;
    }
    
    llm_context_component_t* component = &context->memory_components[component_index];
    
    /* Simple summarization - truncate to first 100 characters and add ellipsis */
    if (component->content.size > 100) {
        char summary[128];
        strncpy(summary, component->content.data, 97);
        summary[97] = '\0';
        strcat(summary, "...");
        
        if (data_set(&component->content, summary, 0) == RESULT_OK) {
            component->token_count = estimate_tokens_from_chars(component->content.size);
            return RESULT_OK;
        }
    }
    
    return RESULT_ERR;
}

result_t llm_context_preserve_important(llm_context_t* context, size_t importance_threshold) {
    if (!context) {
        RETURN_ERR("Invalid context parameter for importance preservation");
        return RESULT_ERR;
    }
    
    /* Mark important components as high priority */
    for (size_t i = 0; i < context->memory_count; i++) {
        if (context->memory_components[i].priority >= importance_threshold) {
            /* Boost priority to ensure preservation */
            context->memory_components[i].priority = MIN(context->memory_components[i].priority + 20, 100);
        }
    }
    
    return RESULT_OK;
}

result_t llm_context_manage_overflow(llm_context_t* context, const llm_context_config_t* config) {
    if (!context || !config) {
        RETURN_ERR("Invalid parameters for overflow management");
        return RESULT_ERR;
    }
    
    /* Try trimming first */
    if (llm_context_trim_size(context, config) == RESULT_OK && context->within_limits) {
        return RESULT_OK;
    }
    
    /* Try summarizing old components */
    for (size_t i = context->memory_count; i > 0; i--) {
        if (llm_context_summarize_old(context, i - 1) == RESULT_OK) {
            /* Recalculate total tokens */
            if (llm_context_calculate_size(context, &context->total_tokens) == RESULT_OK) {
                size_t target_tokens = config->max_context_tokens - config->response_token_reserve;
                if (context->total_tokens <= target_tokens) {
                    context->within_limits = true;
                    return RESULT_OK;
                }
            }
        }
    }
    
    /* Final trimming attempt */
    return llm_context_trim_size(context, config);
}

result_t llm_context_init(llm_context_t* context) {
    if (!context) {
        RETURN_ERR("Context pointer is NULL");
        return RESULT_ERR;
    }
    
    /* Initialize structure */
    memset(context, 0, sizeof(llm_context_t));
    
    /* Initialize system prompt component */
    if (data_init(&context->system_prompt.content, 1024) != RESULT_OK) {
        RETURN_ERR("Failed to initialize system prompt content buffer");
        return RESULT_ERR;
    }
    
    /* Initialize current state component */
    if (data_init(&context->current_state.content, 512) != RESULT_OK) {
        data_clear(&context->system_prompt.content);
        RETURN_ERR("Failed to initialize current state content buffer");
        return RESULT_ERR;
    }
    
    /* Initialize memory components */
    for (size_t i = 0; i < 64; i++) {
        if (data_init(&context->memory_components[i].content, 256) != RESULT_OK) {
            /* Clean up previously initialized components */
            for (size_t j = 0; j < i; j++) {
                data_clear(&context->memory_components[j].content);
            }
            data_clear(&context->system_prompt.content);
            data_clear(&context->current_state.content);
            RETURN_ERR("Failed to initialize memory component content buffer");
            return RESULT_ERR;
        }
    }
    
    return RESULT_OK;
}

result_t llm_context_cleanup(llm_context_t* context) {
    if (!context) {
        RETURN_ERR("Context pointer is NULL");
        return RESULT_ERR;
    }
    
    /* Clean up system prompt component */
    data_clear(&context->system_prompt.content);
    
    /* Clean up current state component */
    data_clear(&context->current_state.content);
    
    /* Clean up memory components */
    for (size_t i = 0; i < 64; i++) {
        data_clear(&context->memory_components[i].content);
    }
    
    /* Reset structure */
    memset(context, 0, sizeof(llm_context_t));
    
    return RESULT_OK;
}
