#include "agent/state.h"

// Helper function to safely get boolean config value
static result_t get_config_bool(pool_t* pool, config_t* config, const char* path, uint64_t* result) {
    object_t* config_obj = NULL;
    
    *result = 0; // Default to false
    
    if (object_provide_str(pool, &config_obj, config->data, path) != RESULT_OK) {
        RETURN_ERR("Failed to provide config object");
    }
    
    if (config_obj == NULL || config_obj->string == NULL || config_obj->string->data == NULL) {
        RETURN_ERR("Invalid config object");
    }
    
    // Safely parse boolean - check for "true" (case insensitive) or non-zero number
    if (string_equal_str(config_obj->string, "true")) {
        *result = 1;
    } else {
        // Try to parse as number
        char temp_str[32];
        size_t copy_size = config_obj->string->size < sizeof(temp_str) - 1 ? 
                          config_obj->string->size : sizeof(temp_str) - 1;
        memcpy(temp_str, config_obj->string->data, copy_size);
        temp_str[copy_size] = '\0';
        
        uint64_t num_val = strtoull(temp_str, NULL, 10);
        *result = (num_val != 0) ? 1 : 0;
    }
    
    return RESULT_OK;
}

// Helper function to safely get uint64 config value
static result_t get_config_uint64(pool_t* pool, config_t* config, const char* path, uint64_t* result, uint64_t default_value) {
    object_t* config_obj = NULL;
    
    *result = default_value;
    
    if (config == NULL || config->data == NULL) {
        return RESULT_OK; // Return default value
    }
    
    if (object_provide_str(pool, &config_obj, config->data, path) != RESULT_OK) {
        return RESULT_OK; // Return default value if path doesn't exist
    }
    
    if (config_obj == NULL || config_obj->string == NULL || config_obj->string->data == NULL) {
        return RESULT_OK; // Return default value if invalid
    }
    
    // Safely parse number
    char temp_str[32];
    size_t copy_size = config_obj->string->size < sizeof(temp_str) - 1 ? 
                      config_obj->string->size : sizeof(temp_str) - 1;
    memcpy(temp_str, config_obj->string->data, copy_size);
    temp_str[copy_size] = '\0';
    
    uint64_t num_val = strtoull(temp_str, NULL, 10);
    if (num_val > 0) {
        *result = num_val;
    }
    
    return RESULT_OK;
}

// Helper function to safely get string config value
static result_t get_config_string(pool_t* pool, config_t* config, const char* path, char* result, size_t result_size, const char* default_value) {
    object_t* config_obj = NULL;
    
    // Set default value
    strncpy(result, default_value, result_size - 1);
    result[result_size - 1] = '\0';
    
    if (config == NULL || config->data == NULL) {
        return RESULT_OK; // Return default value
    }
    
    if (object_provide_str(pool, &config_obj, config->data, path) != RESULT_OK) {
        return RESULT_OK; // Return default value if path doesn't exist
    }
    
    if (config_obj == NULL || config_obj->string == NULL || config_obj->string->data == NULL) {
        return RESULT_OK; // Return default value if invalid
    }
    
    // Safely copy string
    size_t copy_size = config_obj->string->size < result_size - 1 ? 
                      config_obj->string->size : result_size - 1;
    memcpy(result, config_obj->string->data, copy_size);
    result[copy_size] = '\0';
    
    return RESULT_OK;
}

// Automatic transition from executing state to evaluating state
result_t agent_state_auto_transition(pool_t* pool, config_t* config, agent_t* agent) {
    (void)config; // Mark parameter as intentionally unused
    
    // After any action execution, automatically transition to evaluating state
    if (agent_state_update_state(pool, agent, "evaluating") != RESULT_OK) {
        RETURN_ERR("Failed to transition to evaluating state");
    }
    
    return RESULT_OK;
}

// Regular state update with thinking log management
result_t agent_state_update_and_log(pool_t* pool, config_t* config, agent_t* agent, object_t* response_obj) {
    object_t* next_state_obj = NULL;
    uint64_t successful_operations = 0;
    
    if (pool == NULL || agent == NULL || response_obj == NULL) {
        RETURN_ERR("Invalid parameters for state update and log");
    }
    
    // Extract next state from response (if present)
    if (agent_state_extract_next_state(pool, response_obj, &next_state_obj) == RESULT_OK && next_state_obj != NULL) {
        // Update agent state
        string_t* state_path = NULL;
        if (string_create_str(pool, &state_path, "state") != RESULT_OK) {
            RETURN_ERR("Failed to create state path string");
        }
        
        if (object_set_string(pool, agent->data, state_path, next_state_obj->string) != RESULT_OK) {
            if (string_destroy(pool, state_path) != RESULT_OK) {
                RETURN_ERR("Failed to destroy state path after set failure");
            }
            RETURN_ERR("Failed to update agent state");
        }
        
        if (string_destroy(pool, state_path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy state path string");
        }
        
        successful_operations++;
    }
    
    // Handle thinking log if present and enabled
    object_t* thinking_log_obj = NULL;
    if (object_provide_str(pool, &thinking_log_obj, response_obj, "thinking_log") == RESULT_OK) {
        uint64_t thinking_log_enabled = 0;
        if (get_config_bool(pool, config, "agent.thinking_log.enable", &thinking_log_enabled) == RESULT_OK && thinking_log_enabled) {
            result_t thinking_log_result = agent_state_manage_thinking_log(pool, config, agent, response_obj);
            if (thinking_log_result == RESULT_OK) {
                successful_operations++;
            }
        }
    }
    
    // Handle evaluation log if present and enabled
    object_t* evaluation_log_obj = NULL;
    if (object_provide_str(pool, &evaluation_log_obj, response_obj, "evaluation_log") == RESULT_OK) {
        uint64_t evaluation_log_enabled = 0;
        if (get_config_bool(pool, config, "agent.evaluation_log.enable", &evaluation_log_enabled) == RESULT_OK && evaluation_log_enabled) {
            result_t evaluation_log_result = agent_state_manage_evaluation_log(pool, config, agent, response_obj);
            if (evaluation_log_result == RESULT_OK) {
                successful_operations++;
            }
        }
    }
    
    // If no operations succeeded, force a state reset to prevent getting stuck
    if (successful_operations == 0) {
        if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
            RETURN_ERR("Failed to reset agent state to thinking after no successful operations");
        }
    }
    
    return RESULT_OK;
}

// Memory-aware transition handling for evaluating state
result_t agent_state_handle_evaluation_transition(pool_t* pool, config_t* config, agent_t* agent, object_t* response_obj) {
    uint64_t requires_paging = 0;
    
    if (pool == NULL || config == NULL || agent == NULL || response_obj == NULL) {
        RETURN_ERR("Invalid parameters for evaluation transition");
    }
    
    // Check if memory limits require paging
    if (agent_state_check_memory_limits(pool, config, agent, &requires_paging) != RESULT_OK) {
        RETURN_ERR("Failed to check memory limits");
    }
    
    // Handle evaluation log if enabled
    uint64_t evaluation_log_enabled = 0;
    if (get_config_bool(pool, config, "agent.evaluation_log.enable", &evaluation_log_enabled) == RESULT_OK && evaluation_log_enabled) {
        if (agent_state_manage_evaluation_log(pool, config, agent, response_obj) != RESULT_OK) {
            RETURN_ERR("Failed to manage evaluation log");
        }
    }
    
    if (requires_paging) {
        // Force transition to paging state instead of thinking
        if (agent_state_update_state(pool, agent, "paging") != RESULT_OK) {
            RETURN_ERR("Failed to transition to paging state");
        }
        
        // Execute paging operation
        if (agent_state_execute_paging(pool, config, agent) != RESULT_OK) {
            RETURN_ERR("Failed to execute paging operation");
        }
        
        // After paging, transition back to thinking
        if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
            RETURN_ERR("Failed to transition back to thinking after paging");
        }
    } else {
        // Normal transition - extract next state from response and update
        object_t* next_state_obj = NULL;
        if (agent_state_extract_next_state(pool, response_obj, &next_state_obj) == RESULT_OK && next_state_obj != NULL) {
            string_t* state_path = NULL;
            if (string_create_str(pool, &state_path, "state") != RESULT_OK) {
                RETURN_ERR("Failed to create state path string for evaluation transition");
            }
            
            if (object_set_string(pool, agent->data, state_path, next_state_obj->string) != RESULT_OK) {
                if (string_destroy(pool, state_path) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy state path after set failure");
                }
                RETURN_ERR("Failed to update agent state from response");
            }
            
            if (string_destroy(pool, state_path) != RESULT_OK) {
                RETURN_ERR("Failed to destroy state path string");
            }
        } else {
            // Default to thinking if no next state specified
            if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
                RETURN_ERR("Failed to default to thinking state");
            }
        }
    }
    
    return RESULT_OK;
}

// Token counting and memory estimation
result_t agent_state_estimate_tokens(pool_t* pool, agent_t* agent, uint64_t* token_count) {
    string_t* memory_string = NULL;
    object_t* working_memory = NULL;
    
    if (pool == NULL || agent == NULL || token_count == NULL) {
        RETURN_ERR("Invalid parameters for token estimation");
    }
    
    *token_count = 0;
    
    // Get working memory
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory for token estimation");
    }
    
    // Convert working memory to string for estimation
    if (string_create(pool, &memory_string) != RESULT_OK) {
        RETURN_ERR("Failed to create string for token estimation");
    }
    
    if (object_tostring_json(pool, &memory_string, working_memory) != RESULT_OK) {
        if (string_destroy(pool, memory_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy memory string after JSON conversion failure");
        }
        RETURN_ERR("Failed to convert working memory to JSON for token estimation");
    }
    
    // Rough token estimation: 1 token per 4 characters (common approximation)
    *token_count = memory_string->size / 4;
    
    if (string_destroy(pool, memory_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy memory string after token estimation");
    }
    
    return RESULT_OK;
}

// Extract next state from LLM response
result_t agent_state_extract_next_state(pool_t* pool, object_t* response_obj, object_t** next_state_obj) {
    if (pool == NULL || response_obj == NULL || next_state_obj == NULL) {
        RETURN_ERR("Invalid parameters for next state extraction");
    }
    
    if (object_provide_str(pool, next_state_obj, response_obj, "next_state") != RESULT_OK) {
        // next_state is optional in some responses, don't treat as critical error
        *next_state_obj = NULL;
        return RESULT_ERR;
    }
    
    if (*next_state_obj == NULL || (*next_state_obj)->string == NULL) {
        RETURN_ERR("Next state object is invalid");
    }
    
    return RESULT_OK;
}

// Update agent state with new value
result_t agent_state_update_state(pool_t* pool, agent_t* agent, const char* new_state) {
    string_t* state_string = NULL;
    string_t* state_path = NULL;
    
    if (pool == NULL || agent == NULL || new_state == NULL) {
        RETURN_ERR("Invalid parameters for state update");
    }
    
    if (string_create_str(pool, &state_string, new_state) != RESULT_OK) {
        RETURN_ERR("Failed to create state string");
    }
    
    if (string_create_str(pool, &state_path, "state") != RESULT_OK) {
        if (string_destroy(pool, state_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy state string after path creation failure");
        }
        RETURN_ERR("Failed to create state path string");
    }
    
    if (object_set_string(pool, agent->data, state_path, state_string) != RESULT_OK) {
        if (string_destroy(pool, state_string) != RESULT_OK ||
            string_destroy(pool, state_path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy strings after state set failure");
        }
        RETURN_ERR("Failed to set agent state");
    }
    
    if (string_destroy(pool, state_string) != RESULT_OK ||
        string_destroy(pool, state_path) != RESULT_OK) {
        RETURN_ERR("Failed to destroy strings after state update");
    }
    
    return RESULT_OK;
}

// Handle thinking log entries with working memory integration
result_t agent_state_manage_thinking_log(pool_t* pool, config_t* config, agent_t* agent, object_t* response_obj) {
    object_t* thinking_log_obj = NULL;
    uint64_t max_entries = 10;  // Default value
    char key_prefix[32] = "thinking_log_";  // Default prefix
    uint64_t enable = 1;  // Default enabled
    
    if (pool == NULL || agent == NULL || response_obj == NULL) {
        RETURN_ERR("Invalid parameters for thinking log management");
    }
    
    // Check if thinking log is enabled
    if (get_config_bool(pool, config, "agent.thinking_log.enable", &enable) != RESULT_OK || !enable) {
        return RESULT_OK; // Skip if disabled
    }
    
    // Extract thinking log from response
    if (object_provide_str(pool, &thinking_log_obj, response_obj, "thinking_log") != RESULT_OK) {
        return RESULT_OK; // Thinking log is optional
    }
    
    if (thinking_log_obj == NULL || thinking_log_obj->string == NULL) {
        return RESULT_OK; // Don't fail on invalid thinking log
    }
    
    // Get configuration values
    get_config_uint64(pool, config, "agent.thinking_log.max_entries", &max_entries, 10);
    get_config_string(pool, config, "agent.thinking_log.key_prefix", key_prefix, sizeof(key_prefix), "thinking_log_");
    
    // Find the next available thinking log slot in working memory
    char log_key[64];
    uint64_t next_index = 1;
    
    // Get working memory to check existing logs
    object_t* working_memory = NULL;
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory for thinking log management");
    }
    
    // Find the highest existing index in working memory
    for (uint64_t i = 1; i <= max_entries; i++) {
        snprintf(log_key, sizeof(log_key), "%s%03lu", key_prefix, (unsigned long)i);
        object_t* existing_log = NULL;
        if (object_provide_str(pool, &existing_log, working_memory, log_key) == RESULT_OK && 
            existing_log != NULL && existing_log->string != NULL && existing_log->string->size > 0) {
            next_index = i + 1;
        }
    }
    
    // If we've exceeded max entries, rotate (remove first, shift others)
    if (next_index > max_entries) {
        // Rotate existing logs (shift them down)
        for (uint64_t i = 1; i < max_entries; i++) {
            char old_log_key[64];
            char new_log_key[64];
            snprintf(old_log_key, sizeof(old_log_key), "%s%03lu", key_prefix, (unsigned long)(i + 1));
            snprintf(new_log_key, sizeof(new_log_key), "%s%03lu", key_prefix, (unsigned long)i);
            
            // Get the value from the next slot
            object_t* log_to_move = NULL;
            if (object_provide_str(pool, &log_to_move, working_memory, old_log_key) == RESULT_OK &&
                log_to_move != NULL && log_to_move->string != NULL) {
                
                string_t* new_key_string = NULL;
                if (string_create_str(pool, &new_key_string, new_log_key) == RESULT_OK) {
                    // Move within working memory only
                    if (object_set_string(pool, working_memory, new_key_string, log_to_move->string) != RESULT_OK) {
                        printf("Error: Failed to rotate thinking log in working memory\n");
                    }
                    
                    if (string_destroy(pool, new_key_string) != RESULT_OK) {
                        printf("Error: Failed to destroy new key string during rotation\n");
                    }
                }
            }
        }
        next_index = max_entries;
    }
    
    // Add the new thinking log entry - only to working memory
    snprintf(log_key, sizeof(log_key), "%s%03lu", key_prefix, (unsigned long)next_index);
    string_t* log_key_string = NULL;
    if (string_create_str(pool, &log_key_string, log_key) != RESULT_OK) {
        return RESULT_OK; // Don't fail on key creation error
    }
    
    // Add to working memory
    if (object_set_string(pool, working_memory, log_key_string, thinking_log_obj->string) != RESULT_OK) {
        printf("Error: Failed to add thinking log to working memory\n");
    }
    
    if (string_destroy(pool, log_key_string) != RESULT_OK) {
        return RESULT_OK; // Don't fail on cleanup error
    }
    
    return RESULT_OK;
}

// Handle evaluation log entries with working memory integration
result_t agent_state_manage_evaluation_log(pool_t* pool, config_t* config, agent_t* agent, object_t* response_obj) {
    object_t* evaluation_log_obj = NULL;
    uint64_t max_entries = 10;  // Default value
    char key_prefix[32] = "evaluation_log_";  // Default prefix
    uint64_t enable = 1;  // Default enabled
    
    if (pool == NULL || agent == NULL || response_obj == NULL) {
        RETURN_ERR("Invalid parameters for evaluation log management");
    }
    
    // Check if evaluation log is enabled
    if (get_config_bool(pool, config, "agent.evaluation_log.enable", &enable) != RESULT_OK || !enable) {
        return RESULT_OK; // Skip if disabled
    }
    
    // Extract evaluation log from response
    if (object_provide_str(pool, &evaluation_log_obj, response_obj, "evaluation_log") != RESULT_OK) {
        return RESULT_OK; // Evaluation log is optional
    }
    
    if (evaluation_log_obj == NULL || evaluation_log_obj->string == NULL) {
        return RESULT_OK; // Don't fail on invalid evaluation log
    }
    
    // Get configuration values
    get_config_uint64(pool, config, "agent.evaluation_log.max_entries", &max_entries, 10);
    get_config_string(pool, config, "agent.evaluation_log.key_prefix", key_prefix, sizeof(key_prefix), "evaluation_log_");
    
    // Find the next available evaluation log slot in working memory
    char log_key[64];
    uint64_t next_index = 1;
    
    // Get working memory to check existing logs
    object_t* working_memory = NULL;
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory for evaluation log management");
    }
    
    // Find the highest existing index in working memory
    for (uint64_t i = 1; i <= max_entries; i++) {
        snprintf(log_key, sizeof(log_key), "%s%03lu", key_prefix, (unsigned long)i);
        object_t* existing_log = NULL;
        if (object_provide_str(pool, &existing_log, working_memory, log_key) == RESULT_OK &&
            existing_log != NULL && existing_log->string != NULL && existing_log->string->size > 0) {
            next_index = i + 1;
        }
    }
    
    // If we've exceeded max entries, rotate (remove first, shift others)
    if (next_index > max_entries) {
        // Rotate existing logs (shift them down)
        for (uint64_t i = 1; i < max_entries; i++) {
            char old_log_key[64];
            char new_log_key[64];
            snprintf(old_log_key, sizeof(old_log_key), "%s%03lu", key_prefix, (unsigned long)(i + 1));
            snprintf(new_log_key, sizeof(new_log_key), "%s%03lu", key_prefix, (unsigned long)i);
            
            // Get the value from the next slot
            object_t* old_log_obj = NULL;
            if (object_provide_str(pool, &old_log_obj, working_memory, old_log_key) == RESULT_OK &&
                old_log_obj != NULL && old_log_obj->string != NULL) {
                string_t* new_log_key_string = NULL;
                if (string_create_str(pool, &new_log_key_string, new_log_key) == RESULT_OK) {
                    // Move the log entry
                    if (object_set_string(pool, working_memory, new_log_key_string, old_log_obj->string) != RESULT_OK) {
                        printf("Error: Failed to rotate evaluation log %s to %s\n", old_log_key, new_log_key);
                    }
                    if (string_destroy(pool, new_log_key_string) != RESULT_OK) {
                        printf("Error: Failed to destroy new evaluation log key string during rotation\n");
                    }
                }
            }
        }
        
        // Replace the last entry
        next_index = max_entries;
    }
    
    // Add the new evaluation log entry - only to working memory
    snprintf(log_key, sizeof(log_key), "%s%03lu", key_prefix, (unsigned long)next_index);
    string_t* log_key_string = NULL;
    if (string_create_str(pool, &log_key_string, log_key) != RESULT_OK) {
        RETURN_ERR("Failed to create evaluation log key string");
    }
    
    // Add to working memory only (working_memory already available from above)
    if (object_set_string(pool, working_memory, log_key_string, evaluation_log_obj->string) != RESULT_OK) {
        if (string_destroy(pool, log_key_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy evaluation log key string after set failure");
        }
        RETURN_ERR("Failed to add evaluation log to working memory");
    }
    
    if (string_destroy(pool, log_key_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy evaluation log key string");
    }
    
    return RESULT_OK;
}

// Handle execution log entries with working memory integration
result_t agent_state_manage_execution_log(pool_t* pool, config_t* config, agent_t* agent, const char* action_type, const char* tags, const char* result_message) {

    uint64_t max_entries = 4;  // Default value
    char key_prefix[32] = "execution_log_";  // Default prefix
    uint64_t enable = 1;  // Default enabled
    
    if (pool == NULL || agent == NULL) {
        RETURN_ERR("Invalid parameters for execution log management");
    }
    
    // Check if execution log is enabled
    if (get_config_bool(pool, config, "agent.execution_log.enable", &enable) != RESULT_OK || !enable) {
        return RESULT_OK; // Skip if disabled
    }
    
    // Get configuration values
    get_config_uint64(pool, config, "agent.execution_log.max_entries", &max_entries, 4);
    get_config_string(pool, config, "agent.execution_log.key_prefix", key_prefix, sizeof(key_prefix), "execution_log_");
    
    // Format execution log message
    char execution_log_buffer[512];
    snprintf(execution_log_buffer, sizeof(execution_log_buffer), 
             "Action: %s, Tags: %s, Result: %s", 
             action_type ? action_type : "unknown", 
             tags ? tags : "none", 
             result_message ? result_message : "no result");
    
    // Find the next available execution log slot in working memory
    char log_key[64];
    uint64_t next_index = 1;
    
    // Get working memory to check existing logs
    object_t* working_memory = NULL;
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory for execution log management");
    }
    
    // Find the highest existing index in working memory
    for (uint64_t i = 1; i <= max_entries; i++) {
        snprintf(log_key, sizeof(log_key), "%s%03lu", key_prefix, (unsigned long)i);
        object_t* existing_log = NULL;
        if (object_provide_str(pool, &existing_log, working_memory, log_key) == RESULT_OK &&
            existing_log != NULL && existing_log->string != NULL && existing_log->string->size > 0) {
            next_index = i + 1;
        }
    }
    
    // If we've exceeded max entries, rotate (remove first, shift others)
    if (next_index > max_entries) {
        // Rotate existing logs (shift them down)
        for (uint64_t i = 1; i < max_entries; i++) {
            char old_log_key[64];
            char new_log_key[64];
            snprintf(old_log_key, sizeof(old_log_key), "%s%03lu", key_prefix, (unsigned long)(i + 1));
            snprintf(new_log_key, sizeof(new_log_key), "%s%03lu", key_prefix, (unsigned long)i);
            
            // Get the value from the next slot
            object_t* old_log_obj = NULL;
            if (object_provide_str(pool, &old_log_obj, working_memory, old_log_key) == RESULT_OK &&
                old_log_obj != NULL && old_log_obj->string != NULL) {
                string_t* new_log_key_string = NULL;
                if (string_create_str(pool, &new_log_key_string, new_log_key) == RESULT_OK) {
                    // Move the log entry
                    if (object_set_string(pool, working_memory, new_log_key_string, old_log_obj->string) != RESULT_OK) {
                        printf("Error: Failed to rotate execution log %s to %s\n", old_log_key, new_log_key);
                    }
                    if (string_destroy(pool, new_log_key_string) != RESULT_OK) {
                        printf("Error: Failed to destroy new execution log key string during rotation\n");
                    }
                }
            }
        }
        
        // Replace the last entry
        next_index = max_entries;
    }
    
    // Add the new execution log entry - only to working memory
    snprintf(log_key, sizeof(log_key), "%s%03lu", key_prefix, (unsigned long)next_index);
    string_t* log_key_string = NULL;
    string_t* log_value_string = NULL;
    
    if (string_create_str(pool, &log_key_string, log_key) != RESULT_OK) {
        return RESULT_OK; // Don't fail on logging errors
    }
    
    if (string_create_str(pool, &log_value_string, execution_log_buffer) != RESULT_OK) {
        if (string_destroy(pool, log_key_string) != RESULT_OK) {
            printf("Error: Failed to destroy execution log key string after value creation failure\n");
        }
        return RESULT_OK; // Don't fail on logging errors
    }
    
    // Add to working memory only (working_memory already available from above)
    if (object_set_string(pool, working_memory, log_key_string, log_value_string) != RESULT_OK) {
        printf("Error: Failed to add execution log to working memory\n");
    }
    
    // Clean up strings
    if (string_destroy(pool, log_key_string) != RESULT_OK) {
        printf("Error: Failed to destroy execution log key string\n");
    }
    if (string_destroy(pool, log_value_string) != RESULT_OK) {
        printf("Error: Failed to destroy execution log value string\n");
    }
    
    return RESULT_OK;
}

// Check if memory limits require paging
result_t agent_state_check_memory_limits(pool_t* pool, config_t* config, agent_t* agent, uint64_t* requires_paging) {
    uint64_t token_count = 0;
    uint64_t paging_limit = 1024;  // Default value
    uint64_t enable = 1;  // Default enabled
    
    if (pool == NULL || agent == NULL || requires_paging == NULL) {
        RETURN_ERR("Invalid parameters for memory limit check");
    }
    
    *requires_paging = 0;
    
    // Check if paging is enabled
    if (get_config_bool(pool, config, "agent.paging_limit.enable", &enable) != RESULT_OK || !enable) {
        return RESULT_OK; // Skip if disabled
    }
    
    // Get current token count
    if (agent_state_estimate_tokens(pool, agent, &token_count) != RESULT_OK) {
        RETURN_ERR("Failed to estimate token count");
    }
    
    // Get paging limit from config
    get_config_uint64(pool, config, "agent.paging_limit.max_tokens", &paging_limit, 1024);
    
    // Check if we need paging
    if (token_count >= paging_limit) {
        *requires_paging = 1;
    }
    
    return RESULT_OK;
}

// Execute paging operation to manage memory overflow
result_t agent_state_execute_paging(pool_t* pool, config_t* config, agent_t* agent) {
    (void)pool;   // Mark as unused
    (void)config; // Mark as unused
    (void)agent;  // Mark as unused
    
    // TODO: Implement intelligent paging strategy
    // For now, this is a placeholder
    printf("Paging operation executed (placeholder implementation)\n");
    
    return RESULT_OK;
}

// Synchronize all logs with working memory for consistent access
result_t agent_state_sync_logs_to_working_memory(pool_t* pool, agent_t* agent) {
    (void)pool;  // Mark as unused
    (void)agent; // Mark as unused
    
    // Since logs are now stored exclusively in working memory,
    // no synchronization is needed - they're already where they should be
    return RESULT_OK;
}
