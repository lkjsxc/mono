/**
 * @file agent.c
 * @brief Agent management system implementation
 * 
 * This file implements the core agent management functionality including:
 * - Agent state management and transitions
 * - Memory management (RAM and persistent disk storage)
 * - Tool execution system
 * - LMStudio integration for AI inference
 * 
 * The agent operates in four states: thinking, executing, evaluating, and paging.
 * It maintains both volatile RAM memory and persistent disk storage using JSON.
 */

#define _GNU_SOURCE  // For usleep
#include "lkjagent.h"

// Default system prompt for the agent
static const char* DEFAULT_SYSTEM_PROMPT = 
    "You are an autonomous AI agent designed to complete tasks through structured reasoning.\n"
    "You operate in four states: thinking, executing, evaluating, and paging.\n"
    "Available tools: search, retrieve, write, execute_code, forget.\n"
    "Always respond with valid JSON containing your next action and state transition.\n"
    "Format: {\"state\": \"next_state\", \"action\": \"action_name\", \"args\": \"arguments\", \"reasoning\": \"explanation\"}";

// Default configuration values
static const agent_config_t DEFAULT_CONFIG = {
    .max_iterations = 50,
    .evaluation_threshold = 0.8,
    .ram_size = 8192,
    .disk_file = "data/memory.json",
    .max_history = 100
};

/**
 * @brief Convert agent state enum to string representation
 * @param state The agent state to convert
 * @return String representation of the state
 */
const char* agent_state_to_string(agent_state_t state) {
    switch (state) {
        case AGENT_STATE_THINKING:   return "thinking";
        case AGENT_STATE_EXECUTING:  return "executing";
        case AGENT_STATE_EVALUATING: return "evaluating";
        case AGENT_STATE_PAGING:     return "paging";
        default:                     return "unknown";
    }
}

/**
 * @brief Initialize agent memory with static buffers
 * @param memory Pointer to memory structure to initialize
 * @param buffers Array of static character buffers
 * @param num_buffers Number of buffers available
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_memory_init(agent_memory_t* memory, char buffers[][2048], size_t num_buffers) {
    if (!memory) {
        lkj_log_error(__func__, "memory parameter is NULL");
        return RESULT_ERR;
    }
    if (!buffers) {
        lkj_log_error(__func__, "buffers parameter is NULL");
        return RESULT_ERR;
    }
    if (num_buffers < 7) {
        lkj_log_error(__func__, "insufficient buffers (need at least 7)");
        return RESULT_ERR;
    }

    // Initialize all memory tokens with static buffers
    if (token_init(&memory->system_prompt, buffers[0], 2048) != RESULT_OK ||
        token_init(&memory->current_state, buffers[1], 2048) != RESULT_OK ||
        token_init(&memory->task_goal, buffers[2], 2048) != RESULT_OK ||
        token_init(&memory->plan, buffers[3], 2048) != RESULT_OK ||
        token_init(&memory->scratchpad, buffers[4], 2048) != RESULT_OK ||
        token_init(&memory->recent_history, buffers[5], 2048) != RESULT_OK ||
        token_init(&memory->retrieved_from_disk, buffers[6], 2048) != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize memory tokens");
        return RESULT_ERR;
    }

    // Set default system prompt
    if (token_set(&memory->system_prompt, DEFAULT_SYSTEM_PROMPT) != RESULT_OK) {
        lkj_log_error(__func__, "failed to set default system prompt");
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * @brief Initialize agent with configuration
 * @param agent Pointer to agent structure to initialize
 * @param config_file Path to configuration file (unused in this implementation)
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_init(agent_t* agent, const char* config_file) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    // Initialize with default configuration
    agent->config = DEFAULT_CONFIG;
    agent->state = AGENT_STATE_THINKING;
    agent->iteration_count = 0;
    
    // Set default LMStudio endpoint
    strncpy(agent->lmstudio_endpoint, "http://host.docker.internal:1234/v1/chat/completions", 
            sizeof(agent->lmstudio_endpoint) - 1);
    agent->lmstudio_endpoint[sizeof(agent->lmstudio_endpoint) - 1] = '\0';
    
    strncpy(agent->model_name, "default", sizeof(agent->model_name) - 1);
    agent->model_name[sizeof(agent->model_name) - 1] = '\0';

    // TODO: In future, parse config_file if provided
    (void)config_file; // Suppress unused parameter warning

    return RESULT_OK;
}

/**
 * @brief Set the current task for the agent
 * @param agent Pointer to agent structure
 * @param task Task description string
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_set_task(agent_t* agent, const char* task) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }
    if (!task) {
        lkj_log_error(__func__, "task parameter is NULL");
        return RESULT_ERR;
    }

    if (token_set(&agent->memory.task_goal, task) != RESULT_OK) {
        lkj_log_error(__func__, "failed to set task goal in memory");
        return RESULT_ERR;
    }

    // Reset agent state when new task is set
    agent->state = AGENT_STATE_THINKING;
    agent->iteration_count = 0;
    
    // Clear previous plan and scratchpad
    if (token_clear(&agent->memory.plan) != RESULT_OK) {
        lkj_log_error(__func__, "failed to clear previous plan");
        return RESULT_ERR;
    }
    if (token_clear(&agent->memory.scratchpad) != RESULT_OK) {
        lkj_log_error(__func__, "failed to clear scratchpad");
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * @brief Transition agent to a new state
 * @param agent Pointer to agent structure
 * @param new_state New state to transition to
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_transition_state(agent_t* agent, agent_state_t new_state) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    const char* old_state_str = agent_state_to_string(agent->state);
    const char* new_state_str = agent_state_to_string(new_state);

    // Log state transition in recent history
    char transition_log[256];
    snprintf(transition_log, sizeof(transition_log), 
             "State transition: %s -> %s (iteration %d)", 
             old_state_str, new_state_str, agent->iteration_count);
    
    if (token_append(&agent->memory.recent_history, transition_log) != RESULT_OK ||
        token_append(&agent->memory.recent_history, "\n") != RESULT_OK) {
        lkj_log_error(__func__, "failed to log state transition in history");
        return RESULT_ERR;
    }

    // Update current state
    agent->state = new_state;
    
    if (token_set(&agent->memory.current_state, new_state_str) != RESULT_OK) {
        lkj_log_error(__func__, "failed to update current state in memory");
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * @brief Build prompt for LMStudio from current agent state
 * @param agent Pointer to agent structure
 * @param prompt Token to store the built prompt
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_build_prompt(const agent_t* agent, token_t* prompt) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }
    if (!prompt) {
        lkj_log_error(__func__, "prompt parameter is NULL");
        return RESULT_ERR;
    }

    if (token_clear(prompt) != RESULT_OK) {
        lkj_log_error(__func__, "failed to clear prompt token");
        return RESULT_ERR;
    }

    // Build structured prompt with current context
    if (token_append(prompt, "SYSTEM: ") != RESULT_OK ||
        token_append(prompt, agent->memory.system_prompt.data) != RESULT_OK ||
        token_append(prompt, "\n\nCURRENT STATE: ") != RESULT_OK ||
        token_append(prompt, agent_state_to_string(agent->state)) != RESULT_OK ||
        token_append(prompt, "\n\nTASK GOAL: ") != RESULT_OK ||
        token_append(prompt, agent->memory.task_goal.data) != RESULT_OK ||
        token_append(prompt, "\n\nCURRENT PLAN: ") != RESULT_OK ||
        token_append(prompt, agent->memory.plan.data) != RESULT_OK ||
        token_append(prompt, "\n\nSCRATCHPAD: ") != RESULT_OK ||
        token_append(prompt, agent->memory.scratchpad.data) != RESULT_OK ||
        token_append(prompt, "\n\nRECENT HISTORY: ") != RESULT_OK ||
        token_append(prompt, agent->memory.recent_history.data) != RESULT_OK ||
        token_append(prompt, "\n\nRETRIEVED FROM DISK: ") != RESULT_OK ||
        token_append(prompt, agent->memory.retrieved_from_disk.data) != RESULT_OK ||
        token_append(prompt, "\n\nRespond with your next action in JSON format.") != RESULT_OK) {
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * @brief Call LMStudio API with the given prompt
 * @param agent Pointer to agent structure
 * @param prompt Input prompt token
 * @param response Token to store response
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_call_lmstudio(agent_t* agent, const token_t* prompt, token_t* response) {
    if (!agent || !prompt || !response) {
        return RESULT_ERR;
    }

    // Create JSON request body for LMStudio
    static char request_body[8192];
    static char url_buffer[256];
    static char method_buffer[16];
    
    token_t method, url, body, http_response;
    
    if (token_init(&method, method_buffer, sizeof(method_buffer)) != RESULT_OK ||
        token_init(&url, url_buffer, sizeof(url_buffer)) != RESULT_OK ||
        token_init(&body, request_body, sizeof(request_body)) != RESULT_OK ||
        token_init(&http_response, response->data, response->capacity) != RESULT_OK) {
        return RESULT_ERR;
    }

    // Build request
    if (token_set(&method, "POST") != RESULT_OK ||
        token_set(&url, agent->lmstudio_endpoint) != RESULT_OK) {
        return RESULT_ERR;
    }

    // Create JSON request body
    snprintf(request_body, sizeof(request_body),
        "{"
        "\"model\":\"%s\","
        "\"messages\":["
        "{\"role\":\"user\",\"content\":\"%s\"}"
        "],"
        "\"max_tokens\":2048,"
        "\"temperature\":0.7"
        "}",
        agent->model_name, prompt->data);
    
    body.size = strlen(request_body);

    // Make HTTP request
    if (http_request(&method, &url, &body, &http_response) != RESULT_OK) {
        return RESULT_ERR;
    }

    // Copy response
    if (token_copy(response, &http_response) != RESULT_OK) {
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * @brief Parse LMStudio response and extract action
 * @param agent Pointer to agent structure
 * @param response Response token from LMStudio
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_parse_response(agent_t* agent, const token_t* response) {
    if (!agent || !response) {
        return RESULT_ERR;
    }

    // For now, just log the response in scratchpad
    // In a full implementation, this would parse JSON and extract actions
    if (token_append(&agent->memory.scratchpad, "LMStudio Response: ") != RESULT_OK ||
        token_append(&agent->memory.scratchpad, response->data) != RESULT_OK ||
        token_append(&agent->memory.scratchpad, "\n") != RESULT_OK) {
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * @brief Execute a single step of agent operation
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_step(agent_t* agent) {
    if (!agent) {
        return RESULT_ERR;
    }

    // Check iteration limit
    if (agent->iteration_count >= agent->config.max_iterations) {
        printf("Agent reached maximum iterations (%d)\n", agent->config.max_iterations);
        return RESULT_ERR;
    }

    agent->iteration_count++;

    static char prompt_buffer[8192];
    static char response_buffer[4096];
    
    token_t prompt, response;
    
    if (token_init(&prompt, prompt_buffer, sizeof(prompt_buffer)) != RESULT_OK ||
        token_init(&response, response_buffer, sizeof(response_buffer)) != RESULT_OK) {
        return RESULT_ERR;
    }

    // Build prompt based on current state
    if (agent_build_prompt(agent, &prompt) != RESULT_OK) {
        printf("Failed to build prompt\n");
        return RESULT_ERR;
    }

    printf("Agent Step %d (State: %s)\n", agent->iteration_count, agent_state_to_string(agent->state));
    
    // Call LMStudio for next action
    if (agent_call_lmstudio(agent, &prompt, &response) != RESULT_OK) {
        printf("Failed to call LMStudio\n");
        return RESULT_ERR;
    }

    // Parse response and update state
    if (agent_parse_response(agent, &response) != RESULT_OK) {
        printf("Failed to parse response\n");
        return RESULT_ERR;
    }

    // Simple state transition logic (in a full implementation, this would be based on parsed response)
    switch (agent->state) {
        case AGENT_STATE_THINKING:
            if (agent_transition_state(agent, AGENT_STATE_EXECUTING) != RESULT_OK) {
                lkj_log_error(__func__, "Failed to transition to executing state");
                return RESULT_ERR;
            }
            break;
        case AGENT_STATE_EXECUTING:
            if (agent_transition_state(agent, AGENT_STATE_EVALUATING) != RESULT_OK) {
                lkj_log_error(__func__, "Failed to transition to evaluating state");
                return RESULT_ERR;
            }
            break;
        case AGENT_STATE_EVALUATING:
            // Check if task is complete or continue thinking
            if (agent->iteration_count > 3) {
                printf("Task evaluation complete\n");
                return RESULT_OK;
            } else {
                if (agent_transition_state(agent, AGENT_STATE_THINKING) != RESULT_OK) {
                    lkj_log_error(__func__, "Failed to transition to thinking state");
                    return RESULT_ERR;
                }
            }
            break;
        case AGENT_STATE_PAGING:
            if (agent_transition_state(agent, AGENT_STATE_THINKING) != RESULT_OK) {
                lkj_log_error(__func__, "Failed to transition to thinking state");
                return RESULT_ERR;
            }
            break;
    }

    return RESULT_OK;
}

/**
 * @brief Run agent until task completion or max iterations
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_run(agent_t* agent) {
    if (!agent) {
        return RESULT_ERR;
    }

    printf("Starting agent execution...\n");
    printf("Task: %s\n", agent->memory.task_goal.data);

    while (agent->iteration_count < agent->config.max_iterations) {
        if (agent_step(agent) != RESULT_OK) {
            printf("Agent step failed at iteration %d\n", agent->iteration_count);
            break;
        }

        // Brief pause between steps
        usleep(100000); // 100ms
    }

    printf("Agent execution completed after %d iterations\n", agent->iteration_count);
    return RESULT_OK;
}

/**
 * @brief Escape JSON string - replace quotes and newlines
 * @param src Source string
 * @param dest Destination token for escaped string
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t json_escape_string(const char* src, token_t* dest) {
    if (!src || !dest) {
        lkj_log_error(__func__, "NULL parameter provided");
        return RESULT_ERR;
    }

    if (token_clear(dest) != RESULT_OK) {
        lkj_log_error(__func__, "Failed to clear destination token");
        return RESULT_ERR;
    }
    
    size_t src_len = strlen(src);
    for (size_t i = 0; i < src_len; i++) {
        char c = src[i];
        switch (c) {
            case '"':
                if (token_append(dest, "\\\"") != RESULT_OK) return RESULT_ERR;
                break;
            case '\\':
                if (token_append(dest, "\\\\") != RESULT_OK) return RESULT_ERR;
                break;
            case '\n':
                if (token_append(dest, "\\n") != RESULT_OK) return RESULT_ERR;
                break;
            case '\r':
                if (token_append(dest, "\\r") != RESULT_OK) return RESULT_ERR;
                break;
            case '\t':
                if (token_append(dest, "\\t") != RESULT_OK) return RESULT_ERR;
                break;
            default:
                if (token_append_length(dest, &c, 1) != RESULT_OK) return RESULT_ERR;
                break;
        }
    }
    
    return RESULT_OK;
}

/**
 * @brief Get current timestamp in ISO 8601 format
 * @param timestamp_buffer Buffer to store timestamp
 * @param buffer_size Size of the buffer
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t get_iso_timestamp(char* timestamp_buffer, size_t buffer_size) {
    if (!timestamp_buffer || buffer_size < 32) {
        return RESULT_ERR;
    }

    time_t now = time(NULL);
    struct tm* utc_time = gmtime(&now);
    
    if (!utc_time) {
        return RESULT_ERR;
    }

    size_t result = strftime(timestamp_buffer, buffer_size, "%Y-%m-%dT%H:%M:%SZ", utc_time);
    return (result > 0) ? RESULT_OK : RESULT_ERR;
}

/**
 * @brief Save agent memory to disk (JSON format)
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_memory_save_to_disk(const agent_t* agent) {
    if (!agent) {
        printf("Error: agent_memory_save_to_disk - NULL agent pointer\n");
        return RESULT_ERR;
    }

    // Use larger buffer for complex JSON structure
    static char json_buffer[32768];  // 32KB buffer
    static char escape_buffer[4096]; // Buffer for escaping strings
    static char timestamp_buffer[32];
    
    token_t json_content, escaped_string;
    
    if (token_init(&json_content, json_buffer, sizeof(json_buffer)) != RESULT_OK) {
        printf("Error: Failed to initialize JSON content token\n");
        return RESULT_ERR;
    }

    if (token_init(&escaped_string, escape_buffer, sizeof(escape_buffer)) != RESULT_OK) {
        printf("Error: Failed to initialize escape buffer token\n");
        return RESULT_ERR;
    }

    // Get current timestamp
    if (get_iso_timestamp(timestamp_buffer, sizeof(timestamp_buffer)) != RESULT_OK) {
        printf("Error: Failed to get timestamp\n");
        return RESULT_ERR;
    }

    // Build JSON according to README memory architecture
    if (token_set(&json_content, "{\n") != RESULT_OK) {
        printf("Error: Failed to start JSON object\n");
        return RESULT_ERR;
    }

    // Metadata section
    char iter_str[32];
    snprintf(iter_str, sizeof(iter_str), "%d", agent->iteration_count);
    
    if (token_append(&json_content, "  \"metadata\": {\n") != RESULT_OK ||
        token_append(&json_content, "    \"version\": \"1.0\",\n") != RESULT_OK ||
        token_append(&json_content, "    \"created\": \"") != RESULT_OK ||
        token_append(&json_content, timestamp_buffer) != RESULT_OK ||
        token_append(&json_content, "\",\n") != RESULT_OK ||
        token_append(&json_content, "    \"last_modified\": \"") != RESULT_OK ||
        token_append(&json_content, timestamp_buffer) != RESULT_OK ||
        token_append(&json_content, "\",\n") != RESULT_OK ||
        token_append(&json_content, "    \"state\": \"") != RESULT_OK ||
        token_append(&json_content, agent_state_to_string(agent->state)) != RESULT_OK ||
        token_append(&json_content, "\",\n") != RESULT_OK ||
        token_append(&json_content, "    \"iterations\": ") != RESULT_OK ||
        token_append(&json_content, iter_str) != RESULT_OK ||
        token_append(&json_content, "\n") != RESULT_OK ||
        token_append(&json_content, "  },\n") != RESULT_OK) {
        printf("Error: Failed to build metadata section\n");
        return RESULT_ERR;
    }

    // Working memory section
    if (token_append(&json_content, "  \"working_memory\": {\n") != RESULT_OK) {
        printf("Error: Failed to start working_memory section\n");
        return RESULT_ERR;
    }

    // Task goal
    const char* task_data = agent->memory.task_goal.data ? agent->memory.task_goal.data : "";
    if (strlen(task_data) == 0) {
        // Use a simple placeholder for empty task
        if (token_append(&json_content, "    \"current_task\": \"\",\n") != RESULT_OK) {
            printf("Error: Failed to add empty task field\n");
            return RESULT_ERR;
        }
    } else {
        if (json_escape_string(task_data, &escaped_string) != RESULT_OK) {
            printf("Error: Failed to escape task_goal\n");
            return RESULT_ERR;
        }
        if (token_append(&json_content, "    \"current_task\": \"") != RESULT_OK ||
            token_append(&json_content, escaped_string.data) != RESULT_OK ||
            token_append(&json_content, "\",\n") != RESULT_OK) {
            printf("Error: Failed to add task_goal (task_data='%s', escaped='%s')\n", task_data, escaped_string.data);
            return RESULT_ERR;
        }
    }

    // Plan
    const char* plan_data = agent->memory.plan.data ? agent->memory.plan.data : "";
    if (strlen(plan_data) == 0) {
        // Use a simple placeholder for empty plan
        if (token_append(&json_content, "    \"context\": \"\",\n") != RESULT_OK) {
            printf("Error: Failed to add empty plan field\n");
            return RESULT_ERR;
        }
    } else {
        if (json_escape_string(plan_data, &escaped_string) != RESULT_OK) {
            printf("Error: Failed to escape plan\n");
            return RESULT_ERR;
        }
        if (token_append(&json_content, "    \"context\": \"") != RESULT_OK ||
            token_append(&json_content, escaped_string.data) != RESULT_OK ||
            token_append(&json_content, "\",\n") != RESULT_OK) {
            printf("Error: Failed to add plan (plan_data='%s', escaped='%s')\n", plan_data, escaped_string.data);
            printf("JSON buffer size: %zu, available: %d\n", json_content.size, token_available_space(&json_content));
            return RESULT_ERR;
        }
    }

    // Variables (simplified as scratchpad)
    const char* scratchpad_data = agent->memory.scratchpad.data ? agent->memory.scratchpad.data : "";
    if (strlen(scratchpad_data) == 0) {
        // Use a simple placeholder for empty scratchpad
        if (token_append(&json_content, "    \"variables\": \"\"\n") != RESULT_OK ||
            token_append(&json_content, "  },\n") != RESULT_OK) {
            printf("Error: Failed to add empty scratchpad field\n");
            return RESULT_ERR;
        }
    } else {
        if (json_escape_string(scratchpad_data, &escaped_string) != RESULT_OK) {
            printf("Error: Failed to escape scratchpad\n");
            return RESULT_ERR;
        }
        if (token_append(&json_content, "    \"variables\": \"") != RESULT_OK ||
            token_append(&json_content, escaped_string.data) != RESULT_OK ||
            token_append(&json_content, "\"\n") != RESULT_OK ||
            token_append(&json_content, "  },\n") != RESULT_OK) {
            printf("Error: Failed to add scratchpad (scratchpad_data='%s', escaped='%s')\n", scratchpad_data, escaped_string.data);
            return RESULT_ERR;
        }
    }

    // Knowledge base section (placeholder for future expansion)
    if (token_append(&json_content, "  \"knowledge_base\": {\n") != RESULT_OK ||
        token_append(&json_content, "    \"concepts\": {},\n") != RESULT_OK ||
        token_append(&json_content, "    \"procedures\": {},\n") != RESULT_OK ||
        token_append(&json_content, "    \"facts\": {}\n") != RESULT_OK ||
        token_append(&json_content, "  },\n") != RESULT_OK) {
        printf("Error: Failed to add knowledge_base section\n");
        return RESULT_ERR;
    }

    // Log section (using recent history)
    if (token_append(&json_content, "  \"log\": [\n") != RESULT_OK) {
        printf("Error: Failed to start log section\n");
        return RESULT_ERR;
    }

    if (!token_is_empty(&agent->memory.recent_history)) {
        const char* history_data = agent->memory.recent_history.data ? agent->memory.recent_history.data : "";
        if (json_escape_string(history_data, &escaped_string) != RESULT_OK) {
            printf("Error: Failed to escape recent_history\n");
            return RESULT_ERR;
        }
        if (token_append(&json_content, "    {\n") != RESULT_OK ||
            token_append(&json_content, "      \"timestamp\": \"") != RESULT_OK ||
            token_append(&json_content, timestamp_buffer) != RESULT_OK ||
            token_append(&json_content, "\",\n") != RESULT_OK ||
            token_append(&json_content, "      \"state\": \"") != RESULT_OK ||
            token_append(&json_content, agent_state_to_string(agent->state)) != RESULT_OK ||
            token_append(&json_content, "\",\n") != RESULT_OK ||
            token_append(&json_content, "      \"action\": \"memory_save\",\n") != RESULT_OK ||
            token_append(&json_content, "      \"details\": \"") != RESULT_OK ||
            token_append(&json_content, escaped_string.data) != RESULT_OK ||
            token_append(&json_content, "\"\n") != RESULT_OK ||
            token_append(&json_content, "    }\n") != RESULT_OK) {
            printf("Error: Failed to add log entry (history_data='%s', escaped='%s')\n", history_data, escaped_string.data);
            return RESULT_ERR;
        }
    }

    if (token_append(&json_content, "  ],\n") != RESULT_OK) {
        printf("Error: Failed to close log section\n");
        return RESULT_ERR;
    }

    // File section (placeholder for future expansion)
    if (token_append(&json_content, "  \"file\": {\n") != RESULT_OK ||
        token_append(&json_content, "    \"generated_code\": {},\n") != RESULT_OK ||
        token_append(&json_content, "    \"documents\": {},\n") != RESULT_OK ||
        token_append(&json_content, "    \"data\": {}\n") != RESULT_OK ||
        token_append(&json_content, "  }\n") != RESULT_OK ||
        token_append(&json_content, "}\n") != RESULT_OK) {
        printf("Error: Failed to close JSON object\n");
        return RESULT_ERR;
    }

    printf("Generated JSON memory (%zu bytes):\n", json_content.size);
    printf("Saving to file: %s\n", agent->config.disk_file);

    // Write to disk with atomic operation (write to temp file, then rename)
    static char temp_file[512];
    snprintf(temp_file, sizeof(temp_file), "%s.tmp", agent->config.disk_file);

    if (file_write(temp_file, &json_content) != RESULT_OK) {
        printf("Error: Failed to write to temporary file: %s\n", temp_file);
        return RESULT_ERR;
    }

    // Atomic rename operation
    if (rename(temp_file, agent->config.disk_file) != 0) {
        printf("Error: Failed to rename temp file to final file: %s\n", strerror(errno));
        unlink(temp_file); // Clean up temp file
        return RESULT_ERR;
    }

    printf("Successfully saved agent memory to disk (%zu bytes)\n", json_content.size);
    return RESULT_OK;
}

/**
 * @brief Load agent memory from disk (JSON format)
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_memory_load_from_disk(agent_t* agent) {
    if (!agent) {
        printf("Error: agent_memory_load_from_disk - NULL agent pointer\n");
        return RESULT_ERR;
    }

    static char json_buffer[32768];  // 32KB buffer to match save function
    token_t json_content;
    
    if (token_init(&json_content, json_buffer, sizeof(json_buffer)) != RESULT_OK) {
        printf("Error: Failed to initialize JSON content token for loading\n");
        return RESULT_ERR;
    }

    // Read from disk
    if (file_read(agent->config.disk_file, &json_content) != RESULT_OK) {
        printf("No existing memory file found at '%s', starting fresh\n", agent->config.disk_file);
        return RESULT_OK; // Not an error, just no existing memory
    }

    printf("Successfully loaded memory from disk (%zu bytes)\n", json_content.size);
    
    // Basic JSON validation - check for proper structure
    if (json_content.size < 10 || 
        !strstr(json_content.data, "metadata") || 
        !strstr(json_content.data, "working_memory")) {
        printf("Warning: Loaded JSON appears to be malformed, continuing with default memory\n");
        return RESULT_OK; // Continue with default memory rather than failing
    }

    // In a full implementation, this would parse the JSON and restore agent state
    // For now, we'll extract some basic information and store it in retrieved_from_disk
    
    // Look for current_task in the JSON
    char* task_start = strstr(json_content.data, "\"current_task\":");
    if (task_start) {
        task_start = strchr(task_start, '"');
        if (task_start) {
            task_start++; // Skip opening quote
            char* task_end = strchr(task_start, '"');
            if (task_end && task_end > task_start) {
                size_t task_len = (size_t)(task_end - task_start);
                if (task_len > 0 && task_len < 1024) {
                    char task_buffer[1024];
                    strncpy(task_buffer, task_start, task_len);
                    task_buffer[task_len] = '\0';
                    
                    // Store retrieved task in memory for agent awareness
                    if (token_set(&agent->memory.retrieved_from_disk, "Previous task: ") == RESULT_OK) {
                        if (token_append(&agent->memory.retrieved_from_disk, task_buffer) != RESULT_OK) {
                            lkj_log_error(__func__, "Failed to append task buffer to retrieved memory");
                        } else {
                            printf("Restored previous task from memory: %s\n", task_buffer);
                        }
                    }
                }
            }
        }
    }

    // Look for state information
    char* state_start = strstr(json_content.data, "\"state\":");
    if (state_start) {
        state_start = strchr(state_start, '"');
        if (state_start) {
            state_start++; // Skip opening quote
            char* state_end = strchr(state_start, '"');
            if (state_end && state_end > state_start) {
                size_t state_len = (size_t)(state_end - state_start);
                if (state_len > 0 && state_len < 32) {
                    char state_buffer[32];
                    strncpy(state_buffer, state_start, state_len);
                    state_buffer[state_len] = '\0';
                    printf("Previous agent state was: %s\n", state_buffer);
                }
            }
        }
    }

    printf("Memory loaded successfully - agent can access previous context\n");
    return RESULT_OK;
}

/**
 * @brief Clear RAM memory (keep only system prompt)
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_memory_clear_ram(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "NULL agent parameter");
        return RESULT_ERR;
    }

    // Clear all memory except system prompt
    if (token_clear(&agent->memory.current_state) != RESULT_OK ||
        token_clear(&agent->memory.task_goal) != RESULT_OK ||
        token_clear(&agent->memory.plan) != RESULT_OK ||
        token_clear(&agent->memory.scratchpad) != RESULT_OK ||
        token_clear(&agent->memory.recent_history) != RESULT_OK ||
        token_clear(&agent->memory.retrieved_from_disk) != RESULT_OK) {
        lkj_log_error(__func__, "Failed to clear one or more memory tokens");
        return RESULT_ERR;
    }

    printf("Agent RAM memory cleared\n");
    return RESULT_OK;
}

// Tool execution functions (simplified implementations)

/**
 * @brief Execute a tool with given arguments
 * @param agent Pointer to agent structure
 * @param tool Tool type to execute
 * @param args Tool arguments
 * @param result Token to store tool execution result
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_execute_tool(agent_t* agent, tool_type_t tool, const char* args, token_t* result) {
    if (!agent || !args || !result) {
        return RESULT_ERR;
    }

    switch (tool) {
        case TOOL_SEARCH:
            return agent_tool_search(agent, args, result);
        case TOOL_RETRIEVE:
            return agent_tool_retrieve(agent, args, result);
        case TOOL_WRITE:
            // Note: write tool requires additional parameters in full implementation
            return agent_tool_write(agent, "default_key", args, "default");
        case TOOL_EXECUTE_CODE:
            return agent_tool_execute_code(agent, args, result);
        case TOOL_FORGET:
            if (agent_tool_forget(agent, args) != RESULT_OK) {
                lkj_log_error(__func__, "Tool forget operation failed");
                return token_set(result, "Failed to forget data");
            }
            return token_set(result, "Successfully forgot data");
        default:
            return token_set(result, "Unknown tool");
    }
}

/**
 * @brief Search tool implementation
 * @param agent Pointer to agent structure
 * @param query Search query
 * @param result Token to store search results
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_tool_search(agent_t* agent, const char* query, token_t* result) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }
    if (!query) {
        lkj_log_error(__func__, "query parameter is NULL");
        return RESULT_ERR;
    }
    if (!result) {
        lkj_log_error(__func__, "result parameter is NULL");
        return RESULT_ERR;
    }

    // Simplified search - just echo the query
    if (token_set(result, "Search results for: ") != RESULT_OK) {
        lkj_log_error(__func__, "failed to set search result prefix");
        return RESULT_ERR;
    }
    if (token_append(result, query) != RESULT_OK) {
        lkj_log_error(__func__, "failed to append query to search result");
        return RESULT_ERR;
    }
    return RESULT_OK;
}

/**
 * @brief Retrieve tool implementation
 * @param agent Pointer to agent structure
 * @param key Key to retrieve
 * @param result Token to store retrieved value
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_tool_retrieve(agent_t* agent, const char* key, token_t* result) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }
    if (!key) {
        lkj_log_error(__func__, "key parameter is NULL");
        return RESULT_ERR;
    }
    if (!result) {
        lkj_log_error(__func__, "result parameter is NULL");
        return RESULT_ERR;
    }

    // Simplified retrieve - just echo the key
    if (token_set(result, "Retrieved value for key: ") != RESULT_OK) {
        lkj_log_error(__func__, "failed to set retrieve result prefix");
        return RESULT_ERR;
    }
    if (token_append(result, key) != RESULT_OK) {
        lkj_log_error(__func__, "failed to append key to retrieve result");
        return RESULT_ERR;
    }
    return RESULT_OK;
}

/**
 * @brief Write tool implementation
 * @param agent Pointer to agent structure
 * @param key Key to write
 * @param value Value to write
 * @param tags Tags for the data
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_tool_write(agent_t* agent, const char* key, const char* value, const char* tags) {
    if (!agent || !key || !value) {
        return RESULT_ERR;
    }

    // Simplified write - just log the operation
    printf("Writing to memory: key='%s', value='%s', tags='%s'\n", key, value, tags ? tags : "none");
    return RESULT_OK;
}

/**
 * @brief Execute code tool implementation
 * @param agent Pointer to agent structure
 * @param code Code to execute
 * @param result Token to store execution result
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_tool_execute_code(agent_t* agent, const char* code, token_t* result) {
    if (!agent || !code || !result) {
        return RESULT_ERR;
    }

    // Simplified code execution - just echo the code
    return token_set(result, "Executed code: ") == RESULT_OK &&
           token_append(result, code) == RESULT_OK ? RESULT_OK : RESULT_ERR;
}

/**
 * @brief Forget tool implementation
 * @param agent Pointer to agent structure
 * @param key Key to forget
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_tool_forget(agent_t* agent, const char* key) {
    if (!agent || !key) {
        return RESULT_ERR;
    }

    // Simplified forget - just log the operation
    printf("Forgetting key: %s\n", key);
    return RESULT_OK;
}
