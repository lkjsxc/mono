/**
 * @file config_loader.c
 * @brief Configuration loading and management implementation for LKJAgent
 * 
 * This module provides comprehensive configuration management including
 * loading from files, validation, defaults, and state-specific prompts
 * with robust error handling.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/config_loader.h"
#include "../include/file_io.h"
#include "../include/json_parser.h"
#include "../include/json_builder.h"
#include "../lkjagent.h"
#include <string.h>
#include <time.h>

/**
 * @defgroup Config_Internal Internal Configuration Functions
 * @{
 */

/**
 * @brief Default system prompts for each agent state
 */
static const char* DEFAULT_THINKING_PROMPT = 
    "You are an autonomous AI agent in thinking mode. Analyze the current situation, "
    "consider available information, and plan your next actions. Use <thinking> tags "
    "to structure your internal reasoning process.";

static const char* DEFAULT_EXECUTING_PROMPT = 
    "You are an autonomous AI agent in execution mode. Carry out the planned actions "
    "systematically and efficiently. Use <action> tags to specify commands or operations "
    "to execute.";

static const char* DEFAULT_EVALUATING_PROMPT = 
    "You are an autonomous AI agent in evaluation mode. Assess the results of recent "
    "actions, determine their success or failure, and decide on next steps. Use "
    "<evaluation> tags to structure your assessment.";

static const char* DEFAULT_PAGING_PROMPT = 
    "You are an autonomous AI agent in memory paging mode. Manage context and memory "
    "efficiently by deciding what information to keep active, archive, or retrieve. "
    "Use <paging> tags to specify memory management directives.";

/**
 * @brief Set default string value in configuration
 * 
 * @param dest Destination buffer
 * @param default_value Default value to set
 * @param max_size Maximum size for the field
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t set_default_string(char* dest, const char* default_value, size_t max_size) {
    if (!dest || !default_value) {
        RETURN_ERR("Null pointer in set_default_string");
        return RESULT_ERR;
    }
    
    size_t len = strlen(default_value);
    if (len >= max_size) {
        RETURN_ERR("Default string value too long");
        return RESULT_ERR;
    }
    
    strcpy(dest, default_value);
    return RESULT_OK;
}

/**
 * @brief Load string value from JSON with validation
 * 
 * @param json_content JSON content to parse
 * @param key Key to look for
 * @param dest Destination buffer
 * @param max_size Maximum size for the field
 * @param default_value Default value if key not found
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t load_string_value(const char* json_content, const char* key, 
                                  char* dest, size_t max_size, const char* default_value) {
    if (!json_content || !key || !dest || !default_value) {
        RETURN_ERR("Null pointer in load_string_value");
        return RESULT_ERR;
    }
    
    data_t value;
    if (data_init(&value, max_size) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (json_find_key(json_content, key, &value) == RESULT_OK) {
        /* Parse the string value */
        data_t parsed_string;
        if (data_init(&parsed_string, max_size) != RESULT_OK) {
            data_destroy(&value);
            return RESULT_ERR;
        }
        
        if (json_parse_string(value.data, &parsed_string) == RESULT_OK) {
            /* Use parsed string value */
            if (parsed_string.size < max_size) {
                strcpy(dest, parsed_string.data);
            } else {
                strncpy(dest, parsed_string.data, max_size - 1);
                dest[max_size - 1] = '\0';
            }
        } else {
            /* Use raw value if string parsing fails */
            if (value.size < max_size) {
                strcpy(dest, value.data);
            } else {
                strncpy(dest, value.data, max_size - 1);
                dest[max_size - 1] = '\0';
            }
        }
        
        data_destroy(&parsed_string);
        data_destroy(&value);
    } else {
        /* Key not found, use default */
        data_destroy(&value);
        return set_default_string(dest, default_value, max_size);
    }
    
    return RESULT_OK;
}

/**
 * @brief Load numeric value from JSON with validation
 * 
 * @param json_content JSON content to parse
 * @param key Key to look for
 * @param dest Pointer to destination variable
 * @param default_value Default value if key not found
 * @param min_value Minimum allowed value
 * @param max_value Maximum allowed value
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t load_numeric_value(const char* json_content, const char* key,
                                   double* dest, double default_value,
                                   double min_value, double max_value) {
    if (!json_content || !key || !dest) {
        RETURN_ERR("Null pointer in load_numeric_value");
        return RESULT_ERR;
    }
    
    data_t value;
    if (data_init(&value, 64) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (json_find_key(json_content, key, &value) == RESULT_OK) {
        double parsed_value;
        if (json_parse_number(value.data, &parsed_value) == RESULT_OK) {
            /* Validate range */
            if (parsed_value >= min_value && parsed_value <= max_value) {
                *dest = parsed_value;
            } else {
                /* Out of range, use default */
                *dest = default_value;
            }
        } else {
            /* Parsing failed, use default */
            *dest = default_value;
        }
    } else {
        /* Key not found, use default */
        *dest = default_value;
    }
    
    data_destroy(&value);
    return RESULT_OK;
}

/** @} */

result_t config_load(const char* filename, config_t* config) {
    if (!filename) {
        RETURN_ERR("Null filename in config_load");
        return RESULT_ERR;
    }
    
    if (!config) {
        RETURN_ERR("Null config in config_load");
        return RESULT_ERR;
    }
    
    /* First load defaults to ensure consistent state */
    if (config_load_defaults(config) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Check if file exists */
    if (file_exists(filename) != RESULT_OK) {
        /* File doesn't exist, keep defaults but mark as invalid */
        config->is_valid = false;
        return RESULT_OK;
    }
    
    /* Get file modification time */
    time_t file_mtime;
    if (file_get_mtime(filename, &file_mtime) == RESULT_OK) {
        config->config_mtime = file_mtime;
    }
    
    /* Read configuration file */
    data_t file_content;
    if (data_init(&file_content, FILE_BUFFER_SIZE) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (file_read_all(filename, &file_content, MAX_DATA_SIZE) != RESULT_OK) {
        data_destroy(&file_content);
        config->is_valid = false;
        return RESULT_OK; /* Keep defaults */
    }
    
    /* Validate JSON structure */
    if (json_validate_structure(file_content.data) != RESULT_OK) {
        data_destroy(&file_content);
        config->is_valid = false;
        return RESULT_OK; /* Keep defaults */
    }
    
    /* Load LLM settings */
    load_string_value(file_content.data, "llm_endpoint", config->llm_endpoint, 
                      sizeof(config->llm_endpoint), "http://localhost:8080/v1/chat/completions");
    load_string_value(file_content.data, "llm_model", config->llm_model,
                      sizeof(config->llm_model), "default");
    load_string_value(file_content.data, "llm_api_key", config->llm_api_key,
                      sizeof(config->llm_api_key), "");
    
    double temp_value;
    load_numeric_value(file_content.data, "llm_max_context", &temp_value, 4096.0, 1024.0, 128000.0);
    config->llm_max_context = (size_t)temp_value;
    
    load_numeric_value(file_content.data, "llm_timeout", &temp_value, 30.0, 1.0, 300.0);
    config->llm_timeout = (int)temp_value;
    
    /* Load memory settings */
    load_numeric_value(file_content.data, "memory_max_working_size", &temp_value, 1048576.0, 1024.0, MAX_DATA_SIZE);
    config->memory_max_working_size = (size_t)temp_value;
    
    load_numeric_value(file_content.data, "memory_max_disk_size", &temp_value, 10485760.0, 10240.0, MAX_DATA_SIZE * 10);
    config->memory_max_disk_size = (size_t)temp_value;
    
    load_numeric_value(file_content.data, "memory_cleanup_threshold", &temp_value, 80.0, 50.0, 95.0);
    config->memory_cleanup_threshold = (size_t)temp_value;
    
    data_destroy(&file_content);
    
    /* Validate the loaded configuration */
    return config_validate(config);
}

result_t config_load_defaults(config_t* config) {
    if (!config) {
        RETURN_ERR("Null config in config_load_defaults");
        return RESULT_ERR;
    }
    
    /* Initialize all string fields to safe defaults */
    if (set_default_string(config->llm_endpoint, "http://localhost:8080/v1/chat/completions", 
                          sizeof(config->llm_endpoint)) != RESULT_OK ||
        set_default_string(config->llm_model, "default", sizeof(config->llm_model)) != RESULT_OK ||
        set_default_string(config->llm_api_key, "", sizeof(config->llm_api_key)) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Set numeric defaults */
    config->llm_max_context = 4096;
    config->llm_timeout = 30;
    config->memory_max_working_size = 1024 * 1024; /* 1MB */
    config->memory_max_disk_size = 10 * 1024 * 1024; /* 10MB */
    config->memory_cleanup_threshold = 80; /* 80% */
    
    /* Initialize state prompts */
    if (data_init(&config->thinking_prompt, 1024) != RESULT_OK ||
        data_init(&config->executing_prompt, 1024) != RESULT_OK ||
        data_init(&config->evaluating_prompt, 1024) != RESULT_OK ||
        data_init(&config->paging_prompt, 1024) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Set default prompts */
    if (data_set(&config->thinking_prompt, DEFAULT_THINKING_PROMPT, 0) != RESULT_OK ||
        data_set(&config->executing_prompt, DEFAULT_EXECUTING_PROMPT, 0) != RESULT_OK ||
        data_set(&config->evaluating_prompt, DEFAULT_EVALUATING_PROMPT, 0) != RESULT_OK ||
        data_set(&config->paging_prompt, DEFAULT_PAGING_PROMPT, 0) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Set timestamps and validation */
    config->config_mtime = 0;
    config->is_valid = true;
    
    return RESULT_OK;
}

result_t config_validate(config_t* config) {
    if (!config) {
        RETURN_ERR("Null config in config_validate");
        return RESULT_ERR;
    }
    
    bool is_valid = true;
    
    /* Validate LLM endpoint */
    if (strlen(config->llm_endpoint) == 0 || strlen(config->llm_endpoint) >= MAX_CONFIG_VALUE_SIZE) {
        is_valid = false;
    }
    
    /* Validate model name */
    if (strlen(config->llm_model) == 0 || strlen(config->llm_model) >= MAX_CONFIG_VALUE_SIZE) {
        is_valid = false;
    }
    
    /* Validate numeric ranges */
    if (config->llm_max_context < 1024 || config->llm_max_context > 128000) {
        is_valid = false;
    }
    
    if (config->llm_timeout < 1 || config->llm_timeout > 300) {
        is_valid = false;
    }
    
    if (config->memory_max_working_size < 1024 || config->memory_max_working_size > MAX_DATA_SIZE) {
        is_valid = false;
    }
    
    if (config->memory_max_disk_size < 10240 || config->memory_max_disk_size > MAX_DATA_SIZE * 10) {
        is_valid = false;
    }
    
    if (config->memory_cleanup_threshold < 50 || config->memory_cleanup_threshold > 95) {
        is_valid = false;
    }
    
    /* Validate prompts */
    if (data_validate(&config->thinking_prompt) != RESULT_OK ||
        data_validate(&config->executing_prompt) != RESULT_OK ||
        data_validate(&config->evaluating_prompt) != RESULT_OK ||
        data_validate(&config->paging_prompt) != RESULT_OK) {
        is_valid = false;
    }
    
    /* Check that prompts are not empty */
    if (config->thinking_prompt.size == 0 || config->executing_prompt.size == 0 ||
        config->evaluating_prompt.size == 0 || config->paging_prompt.size == 0) {
        is_valid = false;
    }
    
    config->is_valid = is_valid;
    return is_valid ? RESULT_OK : RESULT_ERR;
}

result_t config_get_state_prompt(const config_t* config, agent_state_t state, data_t* prompt) {
    if (!config) {
        RETURN_ERR("Null config in config_get_state_prompt");
        return RESULT_ERR;
    }
    
    if (!STATE_IS_VALID(state)) {
        RETURN_ERR("Invalid agent state in config_get_state_prompt");
        return RESULT_ERR;
    }
    
    if (!prompt) {
        RETURN_ERR("Null prompt buffer in config_get_state_prompt");
        return RESULT_ERR;
    }
    
    const data_t* source_prompt;
    switch (state) {
        case STATE_THINKING:
            source_prompt = &config->thinking_prompt;
            break;
        case STATE_EXECUTING:
            source_prompt = &config->executing_prompt;
            break;
        case STATE_EVALUATING:
            source_prompt = &config->evaluating_prompt;
            break;
        case STATE_PAGING:
            source_prompt = &config->paging_prompt;
            break;
        default:
            RETURN_ERR("Unknown agent state in config_get_state_prompt");
            return RESULT_ERR;
    }
    
    return data_copy(prompt, source_prompt);
}

result_t config_get_llm_settings(const config_t* config, data_t* endpoint, data_t* model, 
                                  data_t* api_key, size_t* max_context, int* timeout) {
    if (!config) {
        RETURN_ERR("Null config in config_get_llm_settings");
        return RESULT_ERR;
    }
    
    if (!endpoint || !model || !api_key || !max_context || !timeout) {
        RETURN_ERR("Null parameter in config_get_llm_settings");
        return RESULT_ERR;
    }
    
    /* Copy string values */
    if (data_set(endpoint, config->llm_endpoint, 0) != RESULT_OK ||
        data_set(model, config->llm_model, 0) != RESULT_OK ||
        data_set(api_key, config->llm_api_key, 0) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Copy numeric values */
    *max_context = config->llm_max_context;
    *timeout = config->llm_timeout;
    
    return RESULT_OK;
}

result_t config_get_memory_settings(const config_t* config, size_t* max_working_size, 
                                     size_t* max_disk_size, size_t* cleanup_threshold) {
    if (!config) {
        RETURN_ERR("Null config in config_get_memory_settings");
        return RESULT_ERR;
    }
    
    if (!max_working_size || !max_disk_size || !cleanup_threshold) {
        RETURN_ERR("Null parameter in config_get_memory_settings");
        return RESULT_ERR;
    }
    
    *max_working_size = config->memory_max_working_size;
    *max_disk_size = config->memory_max_disk_size;
    *cleanup_threshold = config->memory_cleanup_threshold;
    
    return RESULT_OK;
}

result_t config_save(const config_t* config, const char* filename) {
    if (!config) {
        RETURN_ERR("Null config in config_save");
        return RESULT_ERR;
    }
    
    if (!filename) {
        RETURN_ERR("Null filename in config_save");
        return RESULT_ERR;
    }
    
    /* Build configuration JSON */
    data_t config_json;
    if (data_init(&config_json, 2048) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (json_build_config(config, &config_json) != RESULT_OK) {
        data_destroy(&config_json);
        return RESULT_ERR;
    }
    
    /* Write to file atomically */
    result_t result = file_write_atomic(filename, &config_json, true);
    data_destroy(&config_json);
    
    return result;
}

result_t config_has_changed(const config_t* config, const char* filename, bool* has_changed) {
    if (!config) {
        RETURN_ERR("Null config in config_has_changed");
        return RESULT_ERR;
    }
    
    if (!filename) {
        RETURN_ERR("Null filename in config_has_changed");
        return RESULT_ERR;
    }
    
    if (!has_changed) {
        RETURN_ERR("Null has_changed pointer in config_has_changed");
        return RESULT_ERR;
    }
    
    time_t current_mtime;
    if (file_get_mtime(filename, &current_mtime) != RESULT_OK) {
        /* File doesn't exist or can't be accessed */
        *has_changed = false;
        return RESULT_OK;
    }
    
    *has_changed = (current_mtime > config->config_mtime);
    return RESULT_OK;
}

void config_destroy(config_t* config) {
    if (!config) {
        return;
    }
    
    /* Clear string fields */
    memset(config->llm_endpoint, 0, sizeof(config->llm_endpoint));
    memset(config->llm_model, 0, sizeof(config->llm_model));
    memset(config->llm_api_key, 0, sizeof(config->llm_api_key));
    
    /* Destroy data fields */
    data_destroy(&config->thinking_prompt);
    data_destroy(&config->executing_prompt);
    data_destroy(&config->evaluating_prompt);
    data_destroy(&config->paging_prompt);
    
    /* Reset numeric fields */
    config->llm_max_context = 0;
    config->llm_timeout = 0;
    config->memory_max_working_size = 0;
    config->memory_max_disk_size = 0;
    config->memory_cleanup_threshold = 0;
    config->config_mtime = 0;
    config->is_valid = false;
}
