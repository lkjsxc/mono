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
    .disk_file = "agent_memory.json",
    .max_history = 100
};

/**
 * @brief Load configuration from JSON file
 * @param config_file Path to the configuration file
 * @param config Pointer to configuration structure to populate
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t config_load(const char* config_file, full_config_t* config) {
    if (!config_file || !config) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    char config_buffer[4096];
    token_t config_token;
    char string_buffer[512];
    token_t string_result;
    double number_result;

    // Initialize tokens
    if (token_init(&config_token, config_buffer, sizeof(config_buffer)) != RESULT_OK ||
        token_init(&string_result, string_buffer, sizeof(string_buffer)) != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize tokens");
        return RESULT_ERR;
    }

    // Read configuration file
    if (file_read(config_file, &config_token) != RESULT_OK) {
        lkj_log_error(__func__, "failed to read configuration file");
        return RESULT_ERR;
    }

    // Validate JSON
    if (json_validate(&config_token) != RESULT_OK) {
        lkj_log_error(__func__, "invalid JSON in configuration file");
        return RESULT_ERR;
    }

    // Load LMStudio configuration
    if (json_get_string(&config_token, "lmstudio.endpoint", &string_result) == RESULT_OK) {
        strncpy(config->lmstudio.endpoint, string_result.data, sizeof(config->lmstudio.endpoint) - 1);
        config->lmstudio.endpoint[sizeof(config->lmstudio.endpoint) - 1] = '\0';
    } else {
        strcpy(config->lmstudio.endpoint, "http://host.docker.internal:1234/v1/chat/completions");
    }

    if (json_get_string(&config_token, "lmstudio.model", &string_result) == RESULT_OK) {
        strncpy(config->lmstudio.model, string_result.data, sizeof(config->lmstudio.model) - 1);
        config->lmstudio.model[sizeof(config->lmstudio.model) - 1] = '\0';
        printf("DEBUG: Loaded model from config: %s\n", config->lmstudio.model);
    } else {
        strcpy(config->lmstudio.model, "default");
        printf("DEBUG: Failed to load model from config, using default\n");
    }

    if (json_get_number(&config_token, "lmstudio.temperature", &number_result) == RESULT_OK) {
        config->lmstudio.temperature = number_result;
    } else {
        config->lmstudio.temperature = 0.7;
    }

    if (json_get_number(&config_token, "lmstudio.max_tokens", &number_result) == RESULT_OK) {
        config->lmstudio.max_tokens = (int)number_result;
    } else {
        config->lmstudio.max_tokens = -1;
    }

    config->lmstudio.stream = 0; // Default to false

    // Load agent configuration
    if (json_get_number(&config_token, "agent.max_iterations", &number_result) == RESULT_OK) {
        config->agent.max_iterations = (int)number_result;
    } else {
        config->agent.max_iterations = 50;
    }

    if (json_get_number(&config_token, "agent.evaluation_threshold", &number_result) == RESULT_OK) {
        config->agent.evaluation_threshold = number_result;
    } else {
        config->agent.evaluation_threshold = 0.8;
    }

    if (json_get_string(&config_token, "agent.memory_file", &string_result) == RESULT_OK) {
        strncpy(config->agent.memory_file, string_result.data, sizeof(config->agent.memory_file) - 1);
        config->agent.memory_file[sizeof(config->agent.memory_file) - 1] = '\0';
    } else {
        strcpy(config->agent.memory_file, "agent_memory.json");
    }

    if (json_get_number(&config_token, "agent.ram_size", &number_result) == RESULT_OK) {
        config->agent.ram_size = (size_t)number_result;
    } else {
        config->agent.ram_size = 2048;
    }

    if (json_get_number(&config_token, "agent.max_history", &number_result) == RESULT_OK) {
        config->agent.max_history = (size_t)number_result;
    } else {
        config->agent.max_history = 100;
    }

    // Load autonomous mode configuration
    if (json_get_number(&config_token, "agent.autonomous_mode", &number_result) == RESULT_OK) {
        config->agent.autonomous_mode = (int)number_result;
    } else {
        config->agent.autonomous_mode = 0;
    }

    if (json_get_number(&config_token, "agent.continuous_thinking", &number_result) == RESULT_OK) {
        config->agent.continuous_thinking = (int)number_result;
    } else {
        config->agent.continuous_thinking = 0;
    }

    if (json_get_number(&config_token, "agent.self_directed", &number_result) == RESULT_OK) {
        config->agent.self_directed = (int)number_result;
    } else {
        config->agent.self_directed = 0;
    }

    // Load HTTP configuration
    if (json_get_number(&config_token, "http.timeout_seconds", &number_result) == RESULT_OK) {
        config->http.timeout_seconds = (int)number_result;
    } else {
        config->http.timeout_seconds = 30;
    }

    if (json_get_number(&config_token, "http.max_request_size", &number_result) == RESULT_OK) {
        config->http.max_request_size = (int)number_result;
    } else {
        config->http.max_request_size = 8192;
    }

    if (json_get_number(&config_token, "http.max_response_size", &number_result) == RESULT_OK) {
        config->http.max_response_size = (int)number_result;
    } else {
        config->http.max_response_size = 4096;
    }

    if (json_get_string(&config_token, "http.user_agent", &string_result) == RESULT_OK) {
        strncpy(config->http.user_agent, string_result.data, sizeof(config->http.user_agent) - 1);
        config->http.user_agent[sizeof(config->http.user_agent) - 1] = '\0';
    } else {
        strcpy(config->http.user_agent, "lkjagent/1.0");
    }

    // Load system prompt configuration
    if (json_get_string(&config_token, "system_prompt.role", &string_result) == RESULT_OK) {
        strncpy(config->system_prompt.role, string_result.data, sizeof(config->system_prompt.role) - 1);
        config->system_prompt.role[sizeof(config->system_prompt.role) - 1] = '\0';
    } else {
        strcpy(config->system_prompt.role, "system");
    }

    if (json_get_string(&config_token, "system_prompt.content", &string_result) == RESULT_OK) {
        strncpy(config->system_prompt.content, string_result.data, sizeof(config->system_prompt.content) - 1);
        config->system_prompt.content[sizeof(config->system_prompt.content) - 1] = '\0';
    } else {
        strcpy(config->system_prompt.content, "You are an autonomous AI agent. Analyze tasks methodically and provide detailed responses.");
    }

    return RESULT_OK;
}

/**
 * @brief Apply loaded configuration to agent
 * @param agent Pointer to agent structure
 * @param config Pointer to loaded configuration
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t config_apply_to_agent(agent_t* agent, const full_config_t* config) {
    if (!agent || !config) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    // Apply LMStudio configuration
    strncpy(agent->lmstudio_endpoint, config->lmstudio.endpoint, sizeof(agent->lmstudio_endpoint) - 1);
    agent->lmstudio_endpoint[sizeof(agent->lmstudio_endpoint) - 1] = '\0';

    strncpy(agent->model_name, config->lmstudio.model, sizeof(agent->model_name) - 1);
    agent->model_name[sizeof(agent->model_name) - 1] = '\0';

    // Apply agent configuration
    agent->config.max_iterations = config->agent.max_iterations;
    agent->config.evaluation_threshold = config->agent.evaluation_threshold;
    agent->config.ram_size = config->agent.ram_size;
    agent->config.max_history = config->agent.max_history;

    strncpy(agent->config.disk_file, config->agent.memory_file, sizeof(agent->config.disk_file) - 1);
    agent->config.disk_file[sizeof(agent->config.disk_file) - 1] = '\0';

    // Store full config for later reference
    agent->loaded_config = *config;

    return RESULT_OK;
}

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
 * @param config_file Path to configuration file
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

    // Load configuration from file if provided
    if (config_file) {
        full_config_t loaded_config;
        if (config_load(config_file, &loaded_config) == RESULT_OK) {
            if (config_apply_to_agent(agent, &loaded_config) == RESULT_OK) {
                // Configuration loaded successfully - don't log as error
                printf("Configuration loaded and applied successfully from %s\n", config_file);
            } else {
                lkj_log_error(__func__, "failed to apply loaded configuration, using defaults");
            }
        } else {
            lkj_log_error(__func__, "failed to load configuration file, using defaults");
        }
    }

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

    // Prevent self-transitions
    if (agent->state == new_state) {
        lkj_log_error(__func__, "attempted self-transition to same state");
        return RESULT_ERR;
    }

    // Validate state transition
    if (!agent_is_valid_transition(agent->state, new_state)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), 
                "invalid state transition: %s -> %s", old_state_str, new_state_str);
        lkj_log_error(__func__, error_msg);
        return RESULT_ERR;
    }

    // Create detailed transition log entry
    char transition_log[512];
    time_t now;
    time(&now);
    struct tm* tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    snprintf(transition_log, sizeof(transition_log), 
             "[%s] State transition: %s -> %s (iteration %d, reason: %s)", 
             timestamp, old_state_str, new_state_str, agent->iteration_count,
             agent_get_transition_reason(agent->state, new_state));
    
    // Log state transition in recent history
    if (token_append(&agent->memory.recent_history, transition_log) != RESULT_OK ||
        token_append(&agent->memory.recent_history, "\n") != RESULT_OK) {
        lkj_log_error(__func__, "failed to log state transition in history");
        return RESULT_ERR;
    }

    // Update agent state
    agent_state_t previous_state = agent->state;
    agent->state = new_state;
    
    // Update current state in memory
    if (token_set(&agent->memory.current_state, new_state_str) != RESULT_OK) {
        lkj_log_error(__func__, "failed to update current state in memory");
        // Rollback state change
        agent->state = previous_state;
        return RESULT_ERR;
    }

    // Perform state-specific initialization
    result_t init_result = agent_initialize_state(agent, new_state);
    if (init_result != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize new state");
        // Rollback state change
        agent->state = previous_state;
        if (token_set(&agent->memory.current_state, old_state_str) != RESULT_OK) {
            lkj_log_error(__func__, "failed to rollback state in memory");
        }
        return RESULT_ERR;
    }

    printf("  State transition: %s -> %s\n", old_state_str, new_state_str);
    return RESULT_OK;
}

/**
 * @brief Check if agent should transition to paging state
 * @param agent Pointer to agent structure
 * @return 1 if paging needed, 0 otherwise
 */
int agent_should_page(const agent_t* agent) {
    if (!agent) {
        return 0;
    }
    
    // Check if scratchpad is getting full (over 80% capacity)
    int scratchpad_usage = (int)((agent->memory.scratchpad.size * 100) / agent->memory.scratchpad.capacity);
    
    // Check if recent history is getting full
    int history_usage = (int)((agent->memory.recent_history.size * 100) / agent->memory.recent_history.capacity);
    
    // Trigger paging if any memory area is over 80% full
    return (scratchpad_usage > 80 || history_usage > 80);
}

/**
 * @brief Check if a state transition is valid
 * @param current_state Current agent state
 * @param new_state Proposed new state
 * @return 1 if valid, 0 if invalid
 */
int agent_is_valid_transition(agent_state_t current_state, agent_state_t new_state) {
    // No transition needed if already in target state
    if (current_state == new_state) {
        return 0; // Invalid - no self-transitions allowed
    }
    
    // Define valid state transitions based on agent lifecycle
    switch (current_state) {
        case AGENT_STATE_THINKING:
            // From thinking, can go to executing or paging
            // Thinking -> Executing: Normal flow after planning
            // Thinking -> Paging: If memory is full during planning
            return (new_state == AGENT_STATE_EXECUTING || new_state == AGENT_STATE_PAGING);
            
        case AGENT_STATE_EXECUTING:
            // From executing, can go to evaluating or paging
            // Executing -> Evaluating: Normal flow after action completion
            // Executing -> Paging: If memory fills up during execution
            return (new_state == AGENT_STATE_EVALUATING || new_state == AGENT_STATE_PAGING);
            
        case AGENT_STATE_EVALUATING:
            // From evaluating, can go to thinking (continue) or paging
            // Evaluating -> Thinking: Need more work, return to planning
            // Evaluating -> Paging: If memory needs management before continuing
            return (new_state == AGENT_STATE_THINKING || new_state == AGENT_STATE_PAGING);
            
        case AGENT_STATE_PAGING:
            // From paging, can return to any operational state
            // The target state depends on where we were interrupted for paging
            return (new_state == AGENT_STATE_THINKING || 
                   new_state == AGENT_STATE_EXECUTING || 
                   new_state == AGENT_STATE_EVALUATING);
            
        default:
            return 0; // Unknown state - invalid
    }
}

/**
 * @brief Get descriptive reason for state transition
 * @param current_state Current agent state
 * @param new_state New state to transition to
 * @return String describing the reason for transition
 */
const char* agent_get_transition_reason(agent_state_t current_state, agent_state_t new_state) {
    switch (current_state) {
        case AGENT_STATE_THINKING:
            if (new_state == AGENT_STATE_EXECUTING) {
                return "plan complete, beginning execution";
            } else if (new_state == AGENT_STATE_PAGING) {
                return "memory full during planning";
            }
            break;
            
        case AGENT_STATE_EXECUTING:
            if (new_state == AGENT_STATE_EVALUATING) {
                return "actions completed, evaluating results";
            } else if (new_state == AGENT_STATE_PAGING) {
                return "memory full during execution";
            }
            break;
            
        case AGENT_STATE_EVALUATING:
            if (new_state == AGENT_STATE_THINKING) {
                return "more work needed, replanning";
            } else if (new_state == AGENT_STATE_PAGING) {
                return "memory management required";
            }
            break;
            
        case AGENT_STATE_PAGING:
            if (new_state == AGENT_STATE_THINKING) {
                return "memory optimized, resuming planning";
            } else if (new_state == AGENT_STATE_EXECUTING) {
                return "memory optimized, resuming execution";
            } else if (new_state == AGENT_STATE_EVALUATING) {
                return "memory optimized, resuming evaluation";
            }
            break;
    }
    
    return "state machine transition";
}

/**
 * @brief Initialize state-specific context when entering a new state
 * @param agent Pointer to agent structure
 * @param new_state State being entered
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_initialize_state(agent_t* agent, agent_state_t new_state) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }
    
    // State-specific initialization
    switch (new_state) {
        case AGENT_STATE_THINKING:
            // Clear any previous planning notes and start fresh
            if (token_append(&agent->memory.scratchpad, "=== THINKING PHASE ===\n") != RESULT_OK) {
                lkj_log_error(__func__, "failed to initialize thinking state");
                return RESULT_ERR;
            }
            break;
            
        case AGENT_STATE_EXECUTING:
            // Mark the beginning of execution phase
            if (token_append(&agent->memory.scratchpad, "=== EXECUTION PHASE ===\n") != RESULT_OK) {
                lkj_log_error(__func__, "failed to initialize executing state");
                return RESULT_ERR;
            }
            break;
            
        case AGENT_STATE_EVALUATING:
            // Mark the beginning of evaluation phase
            if (token_append(&agent->memory.scratchpad, "=== EVALUATION PHASE ===\n") != RESULT_OK) {
                lkj_log_error(__func__, "failed to initialize evaluating state");
                return RESULT_ERR;
            }
            break;
            
        case AGENT_STATE_PAGING:
            // Log memory management activity
            if (token_append(&agent->memory.scratchpad, "=== MEMORY PAGING ===\n") != RESULT_OK) {
                lkj_log_error(__func__, "failed to initialize paging state");
                return RESULT_ERR;
            }
            break;
            
        default:
            return RESULT_ERR;
    }
    
    return RESULT_OK;
}

/**
 * @brief Let the AI decide what to process next using LMStudio
 * @param agent Pointer to agent structure
 * @param next_action Token to store the AI's decision
 * @return RESULT_OK if decision made, RESULT_ERR on failure
 */
result_t agent_ai_decide_next_action(agent_t* agent, token_t* next_action) {
    if (!agent || !next_action) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }
    
    // Build a prompt asking the AI what it wants to do next
    static char decision_prompt[4096];
    token_t prompt;
    if (token_init(&prompt, decision_prompt, sizeof(decision_prompt)) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Create an AI-driven decision prompt
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
        token_append(&prompt, "\\nWhat would you like to explore or work on next?\"}") != RESULT_OK ||
        token_append(&prompt, "], \"temperature\": 0.8, \"max_tokens\": 150}") != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Call LMStudio for AI decision
    static char ai_response[2048];
    token_t response;
    if (token_init(&response, ai_response, sizeof(ai_response)) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    static char method_buffer[16];
    static char url_buffer[256];
    token_t method, url;
    
    if (token_init(&method, method_buffer, sizeof(method_buffer)) != RESULT_OK ||
        token_init(&url, url_buffer, sizeof(url_buffer)) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (token_set(&method, "POST") != RESULT_OK ||
        token_set(&url, agent->lmstudio_endpoint) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (http_request(&method, &url, &prompt, &response) == RESULT_OK) {
        // Try to extract the AI's decision from the response
        static char content_buffer[1024];
        token_t content;
        if (token_init(&content, content_buffer, sizeof(content_buffer)) == RESULT_OK) {
            if (json_get_string(&response, "choices.0.message.content", &content) == RESULT_OK) {
                if (token_copy(next_action, &content) == RESULT_OK) {
                    return RESULT_OK;
                }
            }
        }
        
        // Fallback: use part of the response if JSON parsing fails
        if (response.size > 0) {
            size_t copy_len = (response.size < next_action->capacity - 1) ? 
                              response.size : next_action->capacity - 1;
            if (token_set_length(next_action, response.data, copy_len) == RESULT_OK) {
                return RESULT_OK;
            }
        }
    }
    
    // Fallback decision if LMStudio call fails
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

/**
 * @brief Make an intelligent decision about the next state transition
 * @param agent Pointer to agent structure
 * @param next_state Pointer to store the recommended next state
 * @return RESULT_OK if a transition is recommended, RESULT_ERR if should stay in current state
 */
result_t agent_decide_next_state(agent_t* agent, agent_state_t* next_state) {
    if (!agent || !next_state) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }
    
    // Check for memory pressure first
    if (agent_should_page(agent) && agent->state != AGENT_STATE_PAGING) {
        *next_state = AGENT_STATE_PAGING;
        return RESULT_OK;
    }
    
    // State-specific decision logic for continuous thinking
    switch (agent->state) {
        case AGENT_STATE_THINKING:
            // Always move to execution after thinking phase
            *next_state = AGENT_STATE_EXECUTING;
            return RESULT_OK;
            
        case AGENT_STATE_EXECUTING:
            // Always move to evaluation after execution
            *next_state = AGENT_STATE_EVALUATING;
            return RESULT_OK;
            
        case AGENT_STATE_EVALUATING:
            // Modified for continuous thinking: Always return to thinking unless explicitly complete
            // Only stop if task is explicitly marked as complete
            if (agent_is_task_complete(agent)) {
                // Task is explicitly complete, no more transitions
                return RESULT_ERR; // Stay in evaluating state
            } else {
                // Continue the thinking cycle - there's always more to explore!
                *next_state = AGENT_STATE_THINKING;
                return RESULT_OK;
            }
            
        case AGENT_STATE_PAGING:
            // After paging, return to thinking to reassess
            *next_state = AGENT_STATE_THINKING;
            return RESULT_OK;
            
        default:
            lkj_log_error(__func__, "unknown current state");
            return RESULT_ERR;
    }
}

/**
 * @brief Check if the agent's current task is complete
 * @param agent Pointer to agent structure
 * @return 1 if task is complete, 0 if more work needed
 */
int agent_is_task_complete(const agent_t* agent) {
    if (!agent) {
        return 0;
    }
    
    // Modified for continuous thinking: Only complete task in very specific scenarios
    // 1. Agent has been running for a very long time (100+ iterations)
    // 2. AND agent explicitly indicates completion in scratchpad
    // 3. AND agent is in evaluating state
    
    // Check for explicit completion markers in scratchpad
    int has_completion_marker = 0;
    if (agent->memory.scratchpad.data) {
        has_completion_marker = (strstr(agent->memory.scratchpad.data, "TASK_EXPLICITLY_COMPLETE") != NULL ||
                               strstr(agent->memory.scratchpad.data, "FINAL_CONCLUSION_REACHED") != NULL);
    }
    
    // Only complete if we have explicit markers AND have done significant work
    if (agent->iteration_count >= 100 && 
        agent->state == AGENT_STATE_EVALUATING &&
        has_completion_marker &&
        agent->memory.scratchpad.size > 500) { // Substantial evidence of work
        return 1;
    }
    
    // Never complete tasks early - keep thinking!
    return 0;
}

/**
 * @brief Enhanced AI-driven agent step where AI decides what to process
 * @param agent Pointer to agent structure
 * @return RESULT_OK to continue, RESULT_TASK_COMPLETE when AI decides to stop, RESULT_ERR on failure
 */
result_t agent_step_ai_driven(agent_t* agent) {
    if (!agent) {
        return RESULT_ERR;
    }

    // No iteration limit - let the AI think as long as it wants!
    agent->iteration_count++;
    printf("AI-Driven Step %d (State: %s)\n", agent->iteration_count, agent_state_to_string(agent->state));
    
    // Let the AI decide what it wants to work on next
    static char ai_decision_buffer[1024];
    token_t ai_decision;
    if (token_init(&ai_decision, ai_decision_buffer, sizeof(ai_decision_buffer)) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Get AI's decision about what to process next
    if (agent_ai_decide_next_action(agent, &ai_decision) == RESULT_OK) {
        printf("  AI Decision: %s\n", ai_decision.data);
        
        // Log the AI's autonomous decision
        if (token_append(&agent->memory.scratchpad, "AI_AUTONOMOUS_DECISION: ") != RESULT_OK ||
            token_append(&agent->memory.scratchpad, ai_decision.data) != RESULT_OK ||
            token_append(&agent->memory.scratchpad, "\n") != RESULT_OK) {
            lkj_log_error(__func__, "Failed to log AI decision");
        }
        
        // Check if AI wants to stop thinking
        if (strstr(ai_decision.data, "stop") != NULL || 
            strstr(ai_decision.data, "complete") != NULL ||
            strstr(ai_decision.data, "finished") != NULL ||
            strstr(ai_decision.data, "done") != NULL) {
            printf("  AI has decided to conclude its work\n");
            if (token_append(&agent->memory.scratchpad, "I_CHOOSE_TO_STOP_THINKING\n") != RESULT_OK) {
                lkj_log_error(__func__, "Failed to log AI completion decision");
            }
            return RESULT_TASK_COMPLETE;
        }
    } else {
        // Fallback: continue with interesting autonomous work
        printf("  AI generating autonomous work...\n");
        char autonomous_work[256];
        snprintf(autonomous_work, sizeof(autonomous_work), 
                "Autonomous exploration #%d: Investigating new patterns and connections", 
                agent->iteration_count);
        
        if (token_set(&ai_decision, autonomous_work) != RESULT_OK ||
            token_append(&agent->memory.scratchpad, "AUTONOMOUS_WORK: ") != RESULT_OK ||
            token_append(&agent->memory.scratchpad, ai_decision.data) != RESULT_OK ||
            token_append(&agent->memory.scratchpad, "\n") != RESULT_OK) {
            lkj_log_error(__func__, "Failed to log autonomous work");
        }
    }
    
    // Perform state-specific AI-driven work
    switch (agent->state) {
        case AGENT_STATE_THINKING:
            printf("  AI deep thinking and exploration...\n");
            if (token_append(&agent->memory.scratchpad, "DEEP_AI_THINKING: Exploring new dimensions and possibilities.\n") != RESULT_OK) {
                lkj_log_error(__func__, "Failed to update scratchpad");
                return RESULT_ERR;
            }
            break;
            
        case AGENT_STATE_EXECUTING:
            printf("  AI-directed execution and investigation...\n");
            if (token_append(&agent->memory.scratchpad, "AI_DIRECTED_EXECUTION: Following autonomous research plan.\n") != RESULT_OK) {
                lkj_log_error(__func__, "Failed to update scratchpad");
                return RESULT_ERR;
            }
            
            // Let AI explore through tools
            static char tool_buffer[512];
            token_t tool_result;
            if (token_init(&tool_result, tool_buffer, sizeof(tool_buffer)) == RESULT_OK) {
                char search_query[256];
                snprintf(search_query, sizeof(search_query), "autonomous investigation topic %d", agent->iteration_count);
                
                if (agent_tool_search(agent, search_query, &tool_result) == RESULT_OK) {
                    if (token_append(&agent->memory.scratchpad, "AI_TOOL_EXPLORATION: ") != RESULT_OK ||
                        token_append(&agent->memory.scratchpad, tool_result.data) != RESULT_OK ||
                        token_append(&agent->memory.scratchpad, "\n") != RESULT_OK) {
                        lkj_log_error(__func__, "Failed to log tool exploration");
                    }
                }
            }
            break;
            
        case AGENT_STATE_EVALUATING:
            printf("  AI autonomous evaluation and reflection...\n");
            if (token_append(&agent->memory.scratchpad, "AI_REFLECTION: Analyzing progress and considering new directions.\n") != RESULT_OK) {
                lkj_log_error(__func__, "Failed to update scratchpad");
                return RESULT_ERR;
            }
            break;
            
        case AGENT_STATE_PAGING:
            printf("  AI memory optimization...\n");
            if (agent_memory_save_to_disk(agent) == RESULT_OK) {
                printf("  AI completed memory optimization\n");
            }
            break;
    }
    
    // Use AI-driven state transitions (but don't force completion)
    agent_state_t next_state;
    if (agent_decide_next_state(agent, &next_state) == RESULT_OK) {
        if (agent_transition_state(agent, next_state) != RESULT_OK) {
            lkj_log_error(__func__, "Failed AI-driven state transition");
            return RESULT_ERR;
        }
    }
    
    return RESULT_OK;
}

/**
 * @brief Check if the agent should continue thinking autonomously
 * @param agent Pointer to agent structure  
 * @return 1 if should continue, 0 if should stop
 */
int agent_should_continue_thinking(const agent_t* agent) {
    if (!agent) {
        return 0;
    }
    
    // Continue thinking if:
    // 1. Autonomous mode is enabled
    // 2. Max iterations is -1 (unlimited) or we haven't reached the limit
    // 3. Agent is actively processing or has more to explore
    
    int autonomous_enabled = (agent->loaded_config.agent.autonomous_mode && 
                             agent->loaded_config.agent.continuous_thinking);
    
    int unlimited_iterations = (agent->config.max_iterations == -1);
    int within_limits = (agent->iteration_count < agent->config.max_iterations);
    
    int can_continue = unlimited_iterations || within_limits;
    
    return autonomous_enabled && can_continue;
}

/**
 * @brief Let the AI agent decide what new task to work on
 * @param agent Pointer to agent structure
 * @param new_task Token to store the new task description
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_decide_new_task(agent_t* agent, token_t* new_task) {
    if (!agent || token_validate(new_task) != RESULT_OK) {
        lkj_log_error(__func__, "invalid parameters");
        return RESULT_ERR;
    }
    
    // Create a prompt for the AI to decide what to work on next
    static char prompt_buffer[4096];
    token_t prompt;
    
    if (token_init(&prompt, prompt_buffer, sizeof(prompt_buffer)) != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize prompt");
        return RESULT_ERR;
    }
    
    // Build autonomous task decision prompt
    if (token_set(&prompt, "AUTONOMOUS AGENT TASK SELECTION\n\n") != RESULT_OK ||
        token_append(&prompt, "Current Context:\n") != RESULT_OK ||
        token_append(&prompt, "- State: ") != RESULT_OK ||
        token_append(&prompt, agent_state_to_string(agent->state)) != RESULT_OK ||
        token_append(&prompt, "\n- Iteration: ") != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Add iteration count
    char iter_str[32];
    snprintf(iter_str, sizeof(iter_str), "%d", agent->iteration_count);
    if (token_append(&prompt, iter_str) != RESULT_OK ||
        token_append(&prompt, "\n- Previous work:\n") != RESULT_OK ||
        token_append(&prompt, agent->memory.scratchpad.data) != RESULT_OK ||
        token_append(&prompt, "\n\nAs an autonomous AI agent, decide what meaningful task you want to work on next. ") != RESULT_OK ||
        token_append(&prompt, "Consider: research topics, system analysis, creative projects, problem solving, ") != RESULT_OK ||
        token_append(&prompt, "learning new concepts, or exploring interesting questions.\n\n") != RESULT_OK ||
        token_append(&prompt, "Respond with just the task description (one clear sentence):") != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Call LMStudio to get AI's decision
    static char response_buffer[2048];
    token_t response;
    
    if (token_init(&response, response_buffer, sizeof(response_buffer)) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (agent_call_lmstudio(agent, &prompt, &response) == RESULT_OK && !token_is_empty(&response)) {
        // Extract just the task from the response (remove any extra formatting)
        if (token_copy(new_task, &response) != RESULT_OK) {
            return RESULT_ERR;
        }
        
        // Trim whitespace
        if (token_trim(new_task) != RESULT_OK) {
            return RESULT_ERR;
        }
        
        printf("🤖 AI Agent decided on new task: %s\n", new_task->data);
        return RESULT_OK;
    }
    
    // Fallback tasks if LMStudio is not available
    const char* fallback_tasks[] = {
        "Explore the nature of consciousness and artificial intelligence",
        "Analyze patterns in human communication and language",
        "Research optimal algorithms for problem-solving",
        "Investigate the relationship between memory and learning",
        "Examine the structure and efficiency of information systems",
        "Study the principles of autonomous decision-making",
        "Analyze the balance between exploration and exploitation in learning",
        "Research methods for continuous self-improvement"
    };
    
    int task_index = agent->iteration_count % (sizeof(fallback_tasks) / sizeof(fallback_tasks[0]));
    if (token_set(new_task, fallback_tasks[task_index]) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    printf("🤖 AI Agent selected fallback task: %s\n", new_task->data);
    return RESULT_OK;
}

/**
 * @brief Run agent in fully autonomous mode with continuous thinking
 * @param agent Pointer to agent structure
 * @return RESULT_OK on completion, RESULT_ERR on failure
 */
result_t agent_run_autonomous(agent_t* agent) {
    if (!agent) {
        return RESULT_ERR;
    }
    
    printf("🚀 Starting Autonomous AI Agent (Continuous Thinking Mode)\n");
    printf("Agent will decide its own tasks and continue thinking indefinitely...\n");
    printf("Press Ctrl+C to stop the agent.\n\n");
    
    static char task_buffer[512];
    token_t current_task;
    
    if (token_init(&current_task, task_buffer, sizeof(task_buffer)) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Start with an initial autonomous task
    if (agent_decide_new_task(agent, &current_task) != RESULT_OK) {
        if (token_set(&current_task, "Begin autonomous exploration and analysis of available systems and data") != RESULT_OK) {
            return RESULT_ERR;
        }
    }
    
    if (agent_set_task(agent, current_task.data) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    int task_cycles = 0;
    
    while (agent_should_continue_thinking(agent)) {
        // Run current task for multiple iterations
        result_t step_result = RESULT_OK;
        int task_iterations = 0;
        
        printf("\n=== Task Cycle %d ===\n", ++task_cycles);
        printf("Current Task: %s\n", agent->memory.task_goal.data);
        
        // Work on current task for a while
        while (step_result == RESULT_OK && task_iterations < 20 && agent_should_continue_thinking(agent)) {
            step_result = agent_step_intelligent(agent);
            task_iterations++;
            
            if (step_result == RESULT_TASK_COMPLETE) {
                printf("✅ Task completed after %d iterations\n", task_iterations);
                break;
            } else if (step_result == RESULT_ERR) {
                printf("❌ Error in task execution\n");
                break;
            }
            
            // Brief pause between steps
            usleep(100000); // 100ms
        }
        
        // Save progress
        if (agent_memory_save_to_disk(agent) == RESULT_OK) {
            printf("💾 Progress saved to disk\n");
        }
        
        // Decide on next task autonomously
        if (agent_should_continue_thinking(agent)) {
            if (agent_decide_new_task(agent, &current_task) == RESULT_OK) {
                if (agent_set_task(agent, current_task.data) != RESULT_OK) {
                    printf("Failed to set new task, continuing with current one\n");
                }
            }
            
            // Brief pause between task cycles
            usleep(500000); // 500ms
        }
    }
    
    printf("\n🏁 Autonomous agent session completed\n");
    printf("Total iterations: %d\n", agent->iteration_count);
    printf("Task cycles completed: %d\n", task_cycles);
    
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

    // Create JSON request body using loaded configuration
    snprintf(request_body, sizeof(request_body),
        "{"
        "\"model\":\"%s\","
        "\"messages\":["
        "{\"role\":\"%s\",\"content\":\"%s\"},"
        "{\"role\":\"user\",\"content\":\"%s\"}"
        "],"
        "\"max_tokens\":%d,"
        "\"temperature\":%.2f,"
        "\"stream\":%s"
        "}",
        agent->loaded_config.lmstudio.model,
        agent->loaded_config.system_prompt.role,
        agent->loaded_config.system_prompt.content,
        prompt->data,
        agent->loaded_config.lmstudio.max_tokens,
        agent->loaded_config.lmstudio.temperature,
        agent->loaded_config.lmstudio.stream ? "true" : "false");
    
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

    printf("Agent Step %d (State: %s)\n", agent->iteration_count, agent_state_to_string(agent->state));
    
    // Check if memory paging is needed before any state transition
    if (agent_should_page(agent) && agent->state != AGENT_STATE_PAGING) {
        printf("  Memory usage high, transitioning to paging state\n");
        if (agent_transition_state(agent, AGENT_STATE_PAGING) != RESULT_OK) {
            lkj_log_error(__func__, "Failed to transition to paging state");
            return RESULT_ERR;
        }
        return RESULT_OK; // Let paging happen in next step
    }
    
    // Enhanced state transition logic with autonomous decision making
    switch (agent->state) {
        case AGENT_STATE_THINKING:
            printf("  Analyzing task and formulating plan...\n");
            
            // Add task analysis to scratchpad
            if (token_append(&agent->memory.scratchpad, "THINKING: Analyzing system requirements.\n") != RESULT_OK) {
                lkj_log_error(__func__, "Failed to update scratchpad");
                return RESULT_ERR;
            }
            
            // Always transition to executing after thinking
            if (agent_transition_state(agent, AGENT_STATE_EXECUTING) != RESULT_OK) {
                lkj_log_error(__func__, "Failed to transition to executing state");
                return RESULT_ERR;
            }
            break;
            
        case AGENT_STATE_EXECUTING:
            printf("  Executing planned actions...\n");
            
            // Simulate tool execution and data gathering
            if (token_append(&agent->memory.scratchpad, "EXECUTING: Running system analysis tools.\n") != RESULT_OK) {
                lkj_log_error(__func__, "Failed to update scratchpad");
                return RESULT_ERR;
            }
            
            // Execute some tools to gather information
            static char tool_result_buffer[512];
            token_t tool_result;
            if (token_init(&tool_result, tool_result_buffer, sizeof(tool_result_buffer)) == RESULT_OK) {
                // Search for system information
                if (agent_tool_search(agent, "system status", &tool_result) == RESULT_OK) {
                    if (token_append(&agent->memory.scratchpad, "TOOL_RESULT: ") != RESULT_OK ||
                        token_append(&agent->memory.scratchpad, tool_result.data) != RESULT_OK ||
                        token_append(&agent->memory.scratchpad, "\n") != RESULT_OK) {
                        lkj_log_error(__func__, "Failed to log tool result");
                    }
                }
            }
            
            // Transition to evaluation after execution
            if (agent_transition_state(agent, AGENT_STATE_EVALUATING) != RESULT_OK) {
                lkj_log_error(__func__, "Failed to transition to evaluating state");
                return RESULT_ERR;
            }
            break;
            
        case AGENT_STATE_EVALUATING:
            printf("  Evaluating results and determining next action...\n");
            
            // Add evaluation to scratchpad
            if (token_append(&agent->memory.scratchpad, "EVALUATING: Assessing gathered data and task progress.\n") != RESULT_OK) {
                lkj_log_error(__func__, "Failed to update scratchpad");
                return RESULT_ERR;
            }
            
            // Modified for continuous thinking: Only complete if explicitly marked
            if (agent_is_task_complete(agent)) {
                printf("  Task explicitly marked as complete after %d iterations\n", agent->iteration_count);
                if (token_append(&agent->memory.scratchpad, "FINAL_CONCLUSION_REACHED: Analysis cycle complete.\n") != RESULT_OK) {
                    lkj_log_error(__func__, "Failed to update scratchpad");
                }
                return RESULT_TASK_COMPLETE; // Task complete
            } else {
                printf("  Continuing analysis, returning to thinking phase (iteration %d)\n", agent->iteration_count);
                if (token_append(&agent->memory.scratchpad, "CONTINUING: More analysis needed, deepening understanding.\n") != RESULT_OK) {
                    lkj_log_error(__func__, "Failed to update scratchpad");
                }
                if (agent_transition_state(agent, AGENT_STATE_THINKING) != RESULT_OK) {
                    lkj_log_error(__func__, "Failed to transition to thinking state");
                    return RESULT_ERR;
                }
            }
            break;
            
        case AGENT_STATE_PAGING:
            printf("  Managing memory and optimizing storage...\n");
            
            // Save current state to disk
            if (agent_memory_save_to_disk(agent) == RESULT_OK) {
                printf("  Memory successfully paged to disk\n");
            } else {
                printf("  Warning: Memory paging failed\n");
            }
            
            // Clear some working memory to free up space
            if (agent_memory_clear_ram(agent) == RESULT_OK) {
                printf("  RAM cleared for optimization\n");
            }
            
            // Return to thinking after paging
            if (agent_transition_state(agent, AGENT_STATE_THINKING) != RESULT_OK) {
                lkj_log_error(__func__, "Failed to transition to thinking state");
                return RESULT_ERR;
            }
            break;
    }

    return RESULT_OK;
}

/**
 * @brief Enhanced agent step with intelligent state transition decisions
 * @param agent Pointer to agent structure
 * @return RESULT_OK to continue, RESULT_TASK_COMPLETE when done, RESULT_ERR on failure
 */
result_t agent_step_intelligent(agent_t* agent) {
    if (!agent) {
        return RESULT_ERR;
    }

    // Check iteration limit
    if (agent->iteration_count >= agent->config.max_iterations) {
        printf("Agent reached maximum iterations (%d)\n", agent->config.max_iterations);
        return RESULT_ERR;
    }

    agent->iteration_count++;
    printf("Intelligent Step %d (State: %s)\n", agent->iteration_count, agent_state_to_string(agent->state));
    
    // Check for task completion first
    if (agent_is_task_complete(agent)) {
        printf("  Task analysis complete - all objectives achieved\n");
        if (token_append(&agent->memory.scratchpad, "TASK_COMPLETE: All objectives successfully achieved.\n") != RESULT_OK) {
            lkj_log_error(__func__, "Failed to log task completion");
        }
        return RESULT_TASK_COMPLETE;
    }
    
    // Perform state-specific work
    switch (agent->state) {
        case AGENT_STATE_THINKING:
            printf("  Deep analysis and strategic planning...\n");
            if (token_append(&agent->memory.scratchpad, "STRATEGIC_THINKING: Comprehensive task analysis.\n") != RESULT_OK) {
                lkj_log_error(__func__, "Failed to update scratchpad");
                return RESULT_ERR;
            }
            break;
            
        case AGENT_STATE_EXECUTING:
            printf("  Executing planned actions with tool integration...\n");
            if (token_append(&agent->memory.scratchpad, "ACTIVE_EXECUTION: Running system analysis tools.\n") != RESULT_OK) {
                lkj_log_error(__func__, "Failed to update scratchpad");
                return RESULT_ERR;
            }
            
            // Execute tools for information gathering
            static char tool_buffer[512];
            token_t tool_result;
            if (token_init(&tool_result, tool_buffer, sizeof(tool_buffer)) == RESULT_OK) {
                if (agent_tool_search(agent, "comprehensive system status", &tool_result) == RESULT_OK) {
                    if (token_append(&agent->memory.scratchpad, "TOOL_ANALYSIS: ") != RESULT_OK ||
                        token_append(&agent->memory.scratchpad, tool_result.data) != RESULT_OK ||
                        token_append(&agent->memory.scratchpad, "\n") != RESULT_OK) {
                        lkj_log_error(__func__, "Failed to log tool results");
                    }
                }
            }
            break;
            
        case AGENT_STATE_EVALUATING:
            printf("  Intelligent evaluation and decision making...\n");
            if (token_append(&agent->memory.scratchpad, "INTELLIGENT_EVAL: Analyzing outcomes and planning next phase.\n") != RESULT_OK) {
                lkj_log_error(__func__, "Failed to update scratchpad");
                return RESULT_ERR;
            }
            break;
            
        case AGENT_STATE_PAGING:
            printf("  Optimizing memory and knowledge management...\n");
            if (agent_memory_save_to_disk(agent) == RESULT_OK) {
                printf("  Memory optimization completed successfully\n");
            }
            if (agent_memory_clear_ram(agent) == RESULT_OK) {
                printf("  RAM optimization completed\n");
            }
            break;
    }
    
    // Use intelligent decision making for next state
    agent_state_t next_state;
    if (agent_decide_next_state(agent, &next_state) == RESULT_OK) {
        if (agent_transition_state(agent, next_state) != RESULT_OK) {
            lkj_log_error(__func__, "Failed intelligent state transition");
            return RESULT_ERR;
        }
    } else {
        // No transition recommended, task might be complete or needs to stay in current state
        if (agent->state == AGENT_STATE_EVALUATING && agent_is_task_complete(agent)) {
            return RESULT_TASK_COMPLETE;
        }
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

    printf("Starting autonomous agent execution...\n");
    printf("Task: %s\n", agent->memory.task_goal.data);
    printf("Initial state: %s\n", agent_state_to_string(agent->state));

    result_t step_result = RESULT_OK;
    int unlimited = (agent->config.max_iterations == -1);
    
    while ((unlimited || agent->iteration_count < agent->config.max_iterations) && step_result == RESULT_OK) {
        step_result = agent_step(agent);
        
        if (step_result != RESULT_OK) {
            if (!unlimited && agent->iteration_count >= agent->config.max_iterations) {
                printf("Agent completed maximum iterations (%d)\n", agent->config.max_iterations);
            } else {
                printf("Agent step completed at iteration %d\n", agent->iteration_count);
            }
            break;
        }

        // Brief pause between steps for demonstration purposes
        usleep(200000); // 200ms
    }

    printf("Agent execution completed after %d iterations\n", agent->iteration_count);
    printf("Final state: %s\n", agent_state_to_string(agent->state));
    
    // Save final state to memory
    if (agent_memory_save_to_disk(agent) == RESULT_OK) {
        printf("Final agent state saved to disk\n");
    }
    
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
