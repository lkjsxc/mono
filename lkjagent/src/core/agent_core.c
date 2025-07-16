/**
 * @file agent_core.c
 * @brief Core agent management and coordination
 * 
 * This file contains the main agent structure management,
 * initialization, cleanup, and high-level coordination functions.
 */

#include "../lkjagent.h"

/**
 * @brief Create and initialize a new agent
 * @param config_file Path to configuration file
 * @return Pointer to agent or NULL on failure
 */
agent_t* agent_create(const char* config_file) {
    if (!config_file) {
        lkj_log_error(__func__, "config_file is NULL");
        return NULL;
    }

    // Allocate agent structure
    agent_t* agent = malloc(sizeof(agent_t));
    if (!agent) {
        lkj_log_error(__func__, "failed to allocate agent");
        return NULL;
    }

    // Initialize to zero
    memset(agent, 0, sizeof(agent_t));

    // Load configuration
    if (config_load(config_file, &agent->loaded_config) != RESULT_OK) {
        lkj_log_error(__func__, "failed to load configuration");
        free(agent);
        return NULL;
    }

    // Apply configuration to agent
    if (config_apply_to_agent(agent, &agent->loaded_config) != RESULT_OK) {
        lkj_log_error(__func__, "failed to apply configuration");
        free(agent);
        return NULL;
    }

    // Initialize state to thinking
    agent->state = AGENT_STATE_THINKING;
    agent->iteration_count = 0;

    printf("Agent created successfully\n");
    return agent;
}

/**
 * @brief Destroy agent and free resources
 * @param agent Pointer to agent to destroy
 */
void agent_destroy(agent_t* agent) {
    if (!agent) {
        return;
    }

    // Save memory to disk before destroying
    if (agent->memory.scratchpad.data) {
        agent_memory_save_to_disk(agent);
    }

    free(agent);
    printf("Agent destroyed\n");
}

/**
 * @brief Set a task for the agent
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
 * @brief Check if the agent's current task is complete
 * @param agent Pointer to agent structure
 * @return 1 if task is complete, 0 if more work needed
 */
int agent_is_task_complete(const agent_t* agent) {
    if (!agent) {
        return 0;
    }
    
    // Check for explicit completion markers in scratchpad
    int has_completion_marker = 0;
    if (agent->memory.scratchpad.data) {
        has_completion_marker = (strstr(agent->memory.scratchpad.data, "TASK_EXPLICITLY_COMPLETE") != NULL ||
                               strstr(agent->memory.scratchpad.data, "FINAL_CONCLUSION_REACHED") != NULL);
    }
    
    // Only complete if we have explicit markers AND have done significant work
    if (agent->iteration_count >= 10 && 
        agent->state == AGENT_STATE_EVALUATING &&
        has_completion_marker &&
        agent->memory.scratchpad.size > 500) { // Substantial evidence of work
        return 1;
    }
    
    return 0;
}

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
    } else {
        strcpy(config->lmstudio.model, "qwen/qwen3-8b");
    }

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

    if (json_get_number(&config_token, "agent.ram_size", &number_result) == RESULT_OK) {
        config->agent.ram_size = (size_t)number_result;
    } else {
        config->agent.ram_size = 2048;
    }

    return RESULT_OK;
}

/**
 * @brief Apply loaded configuration to agent structure
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
