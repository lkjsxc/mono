/**
 * @file lmstudio_api.c
 * @brief LMStudio API integration for AI inference
 * 
 * This file contains all LMStudio API communication functionality including:
 * - HTTP request building for LMStudio endpoints
 * - JSON payload construction and parsing
 * - AI decision making and response processing
 */

#include "../lkjagent.h"

/**
 * @brief Build a prompt for LMStudio API call
 * @param agent Pointer to agent structure
 * @param prompt Pointer to token to store the built prompt
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_build_prompt(const agent_t* agent, token_t* prompt) {
    if (!agent || !prompt) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    // Clear the prompt token
    if (token_clear(prompt) != RESULT_OK) {
        lkj_log_error(__func__, "failed to clear prompt token");
        return RESULT_ERR;
    }

    // Build JSON request for LMStudio
    if (token_set(prompt, "{\"model\": \"") != RESULT_OK ||
        token_append(prompt, agent->model_name) != RESULT_OK ||
        token_append(prompt, "\", \"messages\": [") != RESULT_OK ||
        token_append(prompt, "{\"role\": \"system\", \"content\": \"") != RESULT_OK ||
        token_append(prompt, agent->memory.system_prompt.data) != RESULT_OK ||
        token_append(prompt, "\"}, ") != RESULT_OK ||
        token_append(prompt, "{\"role\": \"user\", \"content\": \"Current state: ") != RESULT_OK ||
        token_append(prompt, agent_state_to_string(agent->state)) != RESULT_OK ||
        token_append(prompt, "\\nTask: ") != RESULT_OK ||
        token_append(prompt, agent->memory.task_goal.data) != RESULT_OK ||
        token_append(prompt, "\\nPlan: ") != RESULT_OK ||
        token_append(prompt, agent->memory.plan.data) != RESULT_OK ||
        token_append(prompt, "\\nScratchpad: ") != RESULT_OK ||
        token_append(prompt, agent->memory.scratchpad.data) != RESULT_OK ||
        token_append(prompt, "\\nWhat should I do next?\"}") != RESULT_OK ||
        token_append(prompt, "], \"temperature\": 0.7, \"stream\": false}") != RESULT_OK) {
        lkj_log_error(__func__, "failed to build prompt");
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * @brief Call LMStudio API with a prompt
 * @param agent Pointer to agent structure
 * @param prompt Pointer to prompt token
 * @param response Pointer to token to store the response
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_call_lmstudio(agent_t* agent, const token_t* prompt, token_t* response) {
    if (!agent || !prompt || !response) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    static char url_buffer[256];
    token_t url;
    
    if (token_init(&url, url_buffer, sizeof(url_buffer)) != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize URL token");
        return RESULT_ERR;
    }

    if (token_set(&url, agent->lmstudio_endpoint) != RESULT_OK) {
        lkj_log_error(__func__, "failed to set LMStudio endpoint");
        return RESULT_ERR;
    }

    // Make HTTP POST request to LMStudio
    if (http_post(&url, prompt, response) != RESULT_OK) {
        lkj_log_error(__func__, "LMStudio API call failed");
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * @brief Parse LMStudio API response
 * @param agent Pointer to agent structure
 * @param response Pointer to response token
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_parse_response(agent_t* agent, const token_t* response) {
    if (!agent || !response) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    // Validate JSON response
    if (json_validate(response) != RESULT_OK) {
        lkj_log_error(__func__, "invalid JSON response from LMStudio");
        return RESULT_ERR;
    }

    // Extract content from response
    static char content_buffer[1024];
    token_t content;
    
    if (token_init(&content, content_buffer, sizeof(content_buffer)) != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize content token");
        return RESULT_ERR;
    }

    if (json_get_string(response, "choices.0.message.content", &content) == RESULT_OK) {
        // Add AI response to scratchpad
        if (token_append(&agent->memory.scratchpad, "AI_RESPONSE: ") != RESULT_OK ||
            token_append(&agent->memory.scratchpad, content.data) != RESULT_OK ||
            token_append(&agent->memory.scratchpad, "\n") != RESULT_OK) {
            lkj_log_error(__func__, "failed to add AI response to scratchpad");
            return RESULT_ERR;
        }
    }

    return RESULT_OK;
}

/**
 * @brief Get AI decision for autonomous thinking
 * @param agent Pointer to agent structure
 * @param next_action Pointer to token to store the AI's decision
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_ai_decide_next_action(agent_t* agent, token_t* next_action) {
    if (!agent || !next_action) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    static char prompt_buffer[4096];
    static char response_buffer[2048];
    token_t prompt, response;

    if (token_init(&prompt, prompt_buffer, sizeof(prompt_buffer)) != RESULT_OK ||
        token_init(&response, response_buffer, sizeof(response_buffer)) != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize tokens");
        return RESULT_ERR;
    }

    // Build autonomous thinking prompt
    if (token_set(&prompt, "{\"model\": \"") != RESULT_OK ||
        token_append(&prompt, agent->model_name) != RESULT_OK ||
        token_append(&prompt, "\", \"messages\": [") != RESULT_OK ||
        token_append(&prompt, "{\"role\": \"system\", \"content\": \"You are an autonomous AI agent. ") != RESULT_OK ||
        token_append(&prompt, "Decide what to explore, analyze, or work on next. ") != RESULT_OK ||
        token_append(&prompt, "Be creative and curious. You can: think deeper, explore new angles, ") != RESULT_OK ||
        token_append(&prompt, "investigate patterns, make connections, or pursue interesting tangents. ") != RESULT_OK ||
        token_append(&prompt, "Respond with just your decision in 1-2 sentences.\"}, ") != RESULT_OK ||
        token_append(&prompt, "{\"role\": \"user\", \"content\": \"Current state: ") != RESULT_OK ||
        token_append(&prompt, agent_state_to_string(agent->state)) != RESULT_OK ||
        token_append(&prompt, "\\nTask: ") != RESULT_OK ||
        token_append(&prompt, agent->memory.task_goal.data) != RESULT_OK ||
        token_append(&prompt, "\\nRecent work: ") != RESULT_OK ||
        token_append(&prompt, agent->memory.scratchpad.data) != RESULT_OK ||
        token_append(&prompt, "\\nWhat should I explore or think about next?\"}") != RESULT_OK ||
        token_append(&prompt, "], \"temperature\": 0.8, \"stream\": false}") != RESULT_OK) {
        lkj_log_error(__func__, "failed to build AI decision prompt");
        return RESULT_ERR;
    }

    // Call LMStudio API
    if (agent_call_lmstudio(agent, &prompt, &response) == RESULT_OK) {
        // Extract content from response
        static char content_buffer[1024];
        token_t content;
        
        if (token_init(&content, content_buffer, sizeof(content_buffer)) == RESULT_OK &&
            json_get_string(&response, "choices.0.message.content", &content) == RESULT_OK) {
            if (token_copy(next_action, &content) != RESULT_OK) {
                lkj_log_error(__func__, "failed to copy AI decision");
                return RESULT_ERR;
            }
            return RESULT_OK;
        }
    }

    // Fallback decisions if LMStudio is not available
    const char* fallback_decisions[] = {
        "Continue deep analysis and explore new perspectives",
        "Investigate interesting patterns and connections", 
        "Think creatively about alternative approaches",
        "Explore the implications and consequences",
        "Consider the broader context and relationships"
    };
    
    int decision_index = agent->iteration_count % 5;
    return token_set(next_action, fallback_decisions[decision_index]);
}
