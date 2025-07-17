/**
 * @file config.c
 * @brief Configuration management for the agent
 * 
 * This file handles loading and applying configuration from JSON files,
 * providing default values, and managing configuration state.
 */

#include "lkjagent.h"

// Default configuration values
static const full_config_t default_config = {
    .lmstudio = {
        .endpoint = "http://host.docker.internal:1234/v1/chat/completions",
        .model = "qwen/qwen3-8b",
        .temperature = 0.7,
        .max_tokens = -1,
        .stream = 0
    },
    .agent = {
        .max_iterations = 50,
        .evaluation_threshold = 0.8,
        .memory_file = "data/memory.json",
        .ram_size = 2048,
        .max_history = 100,
        .autonomous_mode = 1,
        .continuous_thinking = 1,
        .self_directed = 1
    },
    .http = {
        .timeout_seconds = 30,
        .max_request_size = 8192,
        .max_response_size = 4096,
        .user_agent = "lkjagent/1.0"
    },
    .system_prompt = {
        .role = "system",
        .content = "You are an autonomous AI agent designed to complete tasks through structured reasoning."
    }
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

    // Start with default configuration
    *config = default_config;

    // Read configuration file
    if (file_read(config_file, &config_token) != RESULT_OK) {
        lkj_log_error(__func__, "failed to read configuration file, using defaults");
        return RESULT_OK; // Use defaults
    }

    // Validate JSON
    if (json_validate(&config_token) != RESULT_OK) {
        lkj_log_error(__func__, "invalid JSON in configuration file, using defaults");
        return RESULT_OK; // Use defaults
    }

    // Load LMStudio configuration
    if (json_get_string(&config_token, "lmstudio.endpoint", &string_result) == RESULT_OK) {
        strncpy(config->lmstudio.endpoint, string_result.data, sizeof(config->lmstudio.endpoint) - 1);
        config->lmstudio.endpoint[sizeof(config->lmstudio.endpoint) - 1] = '\0';
    }

    if (json_get_string(&config_token, "lmstudio.model", &string_result) == RESULT_OK) {
        strncpy(config->lmstudio.model, string_result.data, sizeof(config->lmstudio.model) - 1);
        config->lmstudio.model[sizeof(config->lmstudio.model) - 1] = '\0';
    }

    if (json_get_number(&config_token, "lmstudio.temperature", &number_result) == RESULT_OK) {
        config->lmstudio.temperature = number_result;
    }

    if (json_get_number(&config_token, "lmstudio.max_tokens", &number_result) == RESULT_OK) {
        config->lmstudio.max_tokens = (int)number_result;
    }

    // Load agent configuration
    if (json_get_number(&config_token, "agent.max_iterations", &number_result) == RESULT_OK) {
        config->agent.max_iterations = (int)number_result;
    }

    if (json_get_number(&config_token, "agent.evaluation_threshold", &number_result) == RESULT_OK) {
        config->agent.evaluation_threshold = number_result;
    }

    if (json_get_string(&config_token, "agent.memory_file", &string_result) == RESULT_OK) {
        strncpy(config->agent.memory_file, string_result.data, sizeof(config->agent.memory_file) - 1);
        config->agent.memory_file[sizeof(config->agent.memory_file) - 1] = '\0';
    }

    if (json_get_number(&config_token, "agent.ram_size", &number_result) == RESULT_OK) {
        config->agent.ram_size = (size_t)number_result;
    }

    if (json_get_number(&config_token, "agent.max_history", &number_result) == RESULT_OK) {
        config->agent.max_history = (size_t)number_result;
    }

    // Load boolean flags
    if (json_get_number(&config_token, "agent.autonomous_mode", &number_result) == RESULT_OK) {
        config->agent.autonomous_mode = (int)number_result;
    }

    if (json_get_number(&config_token, "agent.continuous_thinking", &number_result) == RESULT_OK) {
        config->agent.continuous_thinking = (int)number_result;
    }

    if (json_get_number(&config_token, "agent.self_directed", &number_result) == RESULT_OK) {
        config->agent.self_directed = (int)number_result;
    }

    // Load HTTP configuration
    if (json_get_number(&config_token, "http.timeout_seconds", &number_result) == RESULT_OK) {
        config->http.timeout_seconds = (int)number_result;
    }

    if (json_get_number(&config_token, "http.max_request_size", &number_result) == RESULT_OK) {
        config->http.max_request_size = (int)number_result;
    }

    if (json_get_number(&config_token, "http.max_response_size", &number_result) == RESULT_OK) {
        config->http.max_response_size = (int)number_result;
    }

    if (json_get_string(&config_token, "http.user_agent", &string_result) == RESULT_OK) {
        strncpy(config->http.user_agent, string_result.data, sizeof(config->http.user_agent) - 1);
        config->http.user_agent[sizeof(config->http.user_agent) - 1] = '\0';
    }

    // Load system prompt
    if (json_get_string(&config_token, "system_prompt.content", &string_result) == RESULT_OK) {
        strncpy(config->system_prompt.content, string_result.data, sizeof(config->system_prompt.content) - 1);
        config->system_prompt.content[sizeof(config->system_prompt.content) - 1] = '\0';
    }

    printf("Configuration loaded successfully\n");
    return RESULT_OK;
}

/**
 * @brief Apply configuration to an agent structure
 * @param agent Pointer to agent structure
 * @param config Pointer to configuration to apply
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
