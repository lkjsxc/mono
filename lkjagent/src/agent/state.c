#include "agent/state.h"

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
    
    // Handle thinking log if present (optional)
    object_t* thinking_log_obj = NULL;
    if (object_provide_str(pool, &thinking_log_obj, response_obj, "thinking_log") == RESULT_OK) {
        // Thinking log management is optional, don't fail if it doesn't work
        result_t thinking_log_result = agent_state_manage_thinking_log(pool, config, agent, response_obj);
        if (thinking_log_result == RESULT_OK) {
            successful_operations++;
        }
        (void)thinking_log_result; // Explicitly ignore the result
    }
    
    // Handle evaluation log if present (optional)
    object_t* evaluation_log_obj = NULL;
    if (object_provide_str(pool, &evaluation_log_obj, response_obj, "evaluation_log") == RESULT_OK) {
        // Evaluation log management is optional, don't fail if it doesn't work
        result_t evaluation_log_result = agent_state_manage_evaluation_log(pool, config, agent, response_obj);
        if (evaluation_log_result == RESULT_OK) {
            successful_operations++;
        }
        (void)evaluation_log_result; // Explicitly ignore the result
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
    
    // Check if memory limits require paging
    if (agent_state_check_memory_limits(pool, config, agent, &requires_paging) != RESULT_OK) {
        RETURN_ERR("Failed to check memory limits");
    }
    
    // Handle evaluation log
    if (agent_state_manage_evaluation_log(pool, config, agent, response_obj) != RESULT_OK) {
        RETURN_ERR("Failed to manage evaluation log");
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
        if (agent_state_extract_next_state(pool, response_obj, &next_state_obj) == RESULT_OK) {
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
    
    // Note: thinking logs and evaluation logs are now included in working memory JSON,
    // so no need for separate counting - they're already included in the above calculation
    
    if (string_destroy(pool, memory_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy memory string after token estimation");
    }
    
    return RESULT_OK;
}

// Extract next state from LLM response
result_t agent_state_extract_next_state(pool_t* pool, object_t* response_obj, object_t** next_state_obj) {
    if (response_obj == NULL) {
        RETURN_ERR("Response object is NULL");
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
    
    if (string_create_str(pool, &state_string, new_state) != RESULT_OK) {
        RETURN_ERR("Failed to create state string");
    }
    
    string_t* state_path = NULL;
    if (string_create_str(pool, &state_path, "state") != RESULT_OK) {
        if (string_destroy(pool, state_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy state string after path creation failure");
        }
        RETURN_ERR("Failed to create state path string");
    }
    
    if (object_set_string(pool, agent->data, state_path, state_string) != RESULT_OK) {
        if (string_destroy(pool, state_string) != RESULT_OK ||
            string_destroy(pool, state_path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy strings after set failure");
        }
        RETURN_ERR("Failed to set agent state");
    }
    
    if (string_destroy(pool, state_string) != RESULT_OK ||
        string_destroy(pool, state_path) != RESULT_OK) {
        RETURN_ERR("Failed to destroy strings after state update");
    }
    
    return RESULT_OK;
}

// Handle thinking log entries with rotation and working memory integration
result_t agent_state_manage_thinking_log(pool_t* pool, config_t* config, agent_t* agent, object_t* response_obj) {
    object_t* thinking_log_obj = NULL;
    object_t* config_thinking_log = NULL;
    object_t* max_entries_obj = NULL;
    uint64_t max_entries = 4;  // Default value
    
    // Extract thinking log from response
    if (object_provide_str(pool, &thinking_log_obj, response_obj, "thinking_log") != RESULT_OK) {
        // thinking_log is optional, return success if not found
        return RESULT_OK;
    }
    
    // Validate thinking log object
    if (thinking_log_obj == NULL || thinking_log_obj->string == NULL) {
        return RESULT_OK; // Don't fail on invalid thinking log
    }
    
    // Get max entries from config if available
    if (config != NULL) {
        if (object_provide_str(pool, &config_thinking_log, config->data, "agent.thinking_log") == RESULT_OK) {
            if (object_provide_str(pool, &max_entries_obj, config_thinking_log, "max_entries") == RESULT_OK) {
                max_entries = strtoull(max_entries_obj->string->data, NULL, 10);
                if (max_entries == 0) max_entries = 4; // Fallback
            }
        }
    }
    
    // Find the next available thinking log slot in working memory
    char log_key[64];
    uint64_t next_index = 1;
    
    // Get working memory to check existing logs
    object_t* working_memory = NULL;
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        printf("Error: Failed to get working memory for thinking log management\n");
        return RESULT_OK;
    }
    
    // Find the highest existing index in working memory
    for (uint64_t i = 1; i <= max_entries; i++) {
        snprintf(log_key, sizeof(log_key), "thinking_log_%03lu", (unsigned long)i);
        object_t* existing_log = NULL;
        if (object_provide_str(pool, &existing_log, working_memory, log_key) == RESULT_OK) {
            next_index = i + 1;
        }
    }
    
    // If we've exceeded max entries, rotate (remove first, shift others)
    if (next_index > max_entries) {
        // Remove oldest entry from working memory before rotation
        char oldest_wm_key[64];
        snprintf(oldest_wm_key, sizeof(oldest_wm_key), "thinking_log_%03lu", (unsigned long)1);
        string_t* oldest_wm_key_string = NULL;
        string_t* empty_string = NULL;
        if (string_create_str(pool, &oldest_wm_key_string, oldest_wm_key) == RESULT_OK &&
            string_create_str(pool, &empty_string, "") == RESULT_OK) {
            // Remove from working memory (ignore result as this is cleanup)
            if (object_set_string(pool, working_memory, oldest_wm_key_string, empty_string) != RESULT_OK) {
                printf("Error: Failed to clear old thinking log from working memory\n");
            }
            if (string_destroy(pool, oldest_wm_key_string) != RESULT_OK) {
                printf("Error: Failed to destroy oldest working memory key string\n");
            }
            if (string_destroy(pool, empty_string) != RESULT_OK) {
                printf("Error: Failed to destroy empty string\n");
            }
        }
        
        // Shift all logs down by one position - only in working memory
        for (uint64_t i = 1; i < max_entries; i++) {
            char old_key[64], new_key[64];
            snprintf(old_key, sizeof(old_key), "thinking_log_%03lu", (unsigned long)(i + 1));
            snprintf(new_key, sizeof(new_key), "thinking_log_%03lu", (unsigned long)i);
            
            object_t* log_to_move = NULL;
            if (object_provide_str(pool, &log_to_move, working_memory, old_key) == RESULT_OK) {
                string_t* new_key_string = NULL;
                if (string_create_str(pool, &new_key_string, new_key) != RESULT_OK) {
                    continue; // Skip this rotation on error
                }
                
                // Move within working memory only
                if (object_set_string(pool, working_memory, new_key_string, log_to_move->string) != RESULT_OK) {
                    printf("Error: Failed to rotate thinking log in working memory\n");
                }
                
                if (string_destroy(pool, new_key_string) != RESULT_OK) {
                    printf("Error: Failed to destroy new key string during rotation\n");
                }
            }
        }
        next_index = max_entries;
    }
    
    // Add the new thinking log entry - only to working memory
    snprintf(log_key, sizeof(log_key), "thinking_log_%03lu", (unsigned long)next_index);
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
    object_t* config_evaluation_log = NULL;
    object_t* max_entries_obj = NULL;
    object_t* enable_obj = NULL;
    object_t* key_prefix_obj = NULL;
    uint64_t max_entries = 10;  // Default value
    char key_prefix[32] = "evaluation_log_";  // Default prefix
    uint64_t enable = 1;  // Default enabled
    
    // Extract evaluation log from response
    if (object_provide_str(pool, &evaluation_log_obj, response_obj, "evaluation_log") != RESULT_OK) {
        // Evaluation log is optional, don't treat as error
        return RESULT_OK;
    }
    
    // Validate evaluation log object
    if (evaluation_log_obj == NULL || evaluation_log_obj->string == NULL) {
        return RESULT_OK; // Don't fail on invalid evaluation log
    }
    
    // Get configuration values if available
    if (config != NULL) {
        if (object_provide_str(pool, &config_evaluation_log, config->data, "agent.evaluation_log") == RESULT_OK) {
            // Check if enabled
            if (object_provide_str(pool, &enable_obj, config_evaluation_log, "enable") == RESULT_OK) {
                enable = strtoull(enable_obj->string->data, NULL, 10);
            }
            
            // If disabled, skip processing
            if (!enable) {
                return RESULT_OK;
            }
            
            // Get max entries
            if (object_provide_str(pool, &max_entries_obj, config_evaluation_log, "max_entries") == RESULT_OK) {
                max_entries = strtoull(max_entries_obj->string->data, NULL, 10);
                if (max_entries == 0) max_entries = 10; // Fallback
            }
            
            // Get key prefix
            if (object_provide_str(pool, &key_prefix_obj, config_evaluation_log, "key_prefix") == RESULT_OK) {
                if (key_prefix_obj->string != NULL && key_prefix_obj->string->size > 0) {
                    snprintf(key_prefix, sizeof(key_prefix), "%s", key_prefix_obj->string->data);
                }
            }
        }
    }
    
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
        if (object_provide_str(pool, &existing_log, working_memory, log_key) == RESULT_OK) {
            next_index = i + 1;
        }
    }
    
    // If we've exceeded max entries, rotate (remove first, shift others)
    if (next_index > max_entries) {
        // Remove oldest entry from working memory before rotation
        char oldest_wm_key[64];
        snprintf(oldest_wm_key, sizeof(oldest_wm_key), "%s%03lu", key_prefix, (unsigned long)1);
        string_t* oldest_wm_key_string = NULL;
        string_t* empty_string = NULL;
        if (string_create_str(pool, &oldest_wm_key_string, oldest_wm_key) == RESULT_OK &&
            string_create_str(pool, &empty_string, "") == RESULT_OK) {
            // Remove from working memory (ignore result as this is cleanup)
            if (object_set_string(pool, working_memory, oldest_wm_key_string, empty_string) != RESULT_OK) {
                printf("Error: Failed to clear old evaluation log from working memory\n");
            }
            if (string_destroy(pool, oldest_wm_key_string) != RESULT_OK) {
                printf("Error: Failed to destroy oldest evaluation working memory key string\n");
            }
            if (string_destroy(pool, empty_string) != RESULT_OK) {
                printf("Error: Failed to destroy empty string for evaluation log\n");
            }
        }
        
        // Rotate existing logs (shift them down)
        for (uint64_t i = 1; i < max_entries; i++) {
            char old_log_key[64];
            char new_log_key[64];
            snprintf(old_log_key, sizeof(old_log_key), "%s%03lu", key_prefix, (unsigned long)(i + 1));
            snprintf(new_log_key, sizeof(new_log_key), "%s%03lu", key_prefix, (unsigned long)i);
            
            // Get the value from the next slot
            object_t* old_log_obj = NULL;
            if (object_provide_str(pool, &old_log_obj, working_memory, old_log_key) == RESULT_OK) {
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
    object_t* config_execution_log = NULL;
    object_t* max_entries_obj = NULL;
    object_t* enable_obj = NULL;
    object_t* key_prefix_obj = NULL;
    uint64_t max_entries = 4;  // Default value
    char key_prefix[32] = "execution_log_";  // Default prefix
    uint64_t enable = 1;  // Default enabled
    
    // Get configuration values if available
    if (config != NULL) {
        if (object_provide_str(pool, &config_execution_log, config->data, "agent.execution_log") == RESULT_OK) {
            // Check if enabled
            if (object_provide_str(pool, &enable_obj, config_execution_log, "enable") == RESULT_OK) {
                enable = strtoull(enable_obj->string->data, NULL, 10);
            }
            
            // If disabled, skip processing
            if (!enable) {
                return RESULT_OK;
            }
            
            // Get max entries
            if (object_provide_str(pool, &max_entries_obj, config_execution_log, "max_entries") == RESULT_OK) {
                max_entries = strtoull(max_entries_obj->string->data, NULL, 10);
                if (max_entries == 0) max_entries = 4; // Fallback
            }
            
            // Get key prefix
            if (object_provide_str(pool, &key_prefix_obj, config_execution_log, "key_prefix") == RESULT_OK) {
                if (key_prefix_obj->string != NULL && key_prefix_obj->string->size > 0) {
                    snprintf(key_prefix, sizeof(key_prefix), "%s", key_prefix_obj->string->data);
                }
            }
        }
    }
    
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
        if (object_provide_str(pool, &existing_log, working_memory, log_key) == RESULT_OK) {
            next_index = i + 1;
        }
    }
    
    // If we've exceeded max entries, rotate (remove first, shift others)
    if (next_index > max_entries) {
        // Remove oldest entry from working memory before rotation
        char oldest_wm_key[64];
        snprintf(oldest_wm_key, sizeof(oldest_wm_key), "%s%03lu", key_prefix, (unsigned long)1);
        string_t* oldest_wm_key_string = NULL;
        string_t* empty_string = NULL;
        if (string_create_str(pool, &oldest_wm_key_string, oldest_wm_key) == RESULT_OK &&
            string_create_str(pool, &empty_string, "") == RESULT_OK) {
            // Remove from working memory (ignore result as this is cleanup)
            if (object_set_string(pool, working_memory, oldest_wm_key_string, empty_string) != RESULT_OK) {
                printf("Error: Failed to clear old execution log from working memory\n");
            }
            if (string_destroy(pool, oldest_wm_key_string) != RESULT_OK) {
                printf("Error: Failed to destroy oldest execution working memory key string\n");
            }
            if (string_destroy(pool, empty_string) != RESULT_OK) {
                printf("Error: Failed to destroy empty string for execution log\n");
            }
        }
        
        // Rotate existing logs (shift them down)
        for (uint64_t i = 1; i < max_entries; i++) {
            char old_log_key[64];
            char new_log_key[64];
            snprintf(old_log_key, sizeof(old_log_key), "%s%03lu", key_prefix, (unsigned long)(i + 1));
            snprintf(new_log_key, sizeof(new_log_key), "%s%03lu", key_prefix, (unsigned long)i);
            
            // Get the value from the next slot
            object_t* old_log_obj = NULL;
            if (object_provide_str(pool, &old_log_obj, working_memory, old_log_key) == RESULT_OK) {
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
    object_t* paging_config = NULL;
    object_t* max_tokens_obj = NULL;
    uint64_t paging_limit = 1024;  // Default value
    
    *requires_paging = 0;
    
    // Get current token count
    if (agent_state_estimate_tokens(pool, agent, &token_count) != RESULT_OK) {
        RETURN_ERR("Failed to estimate token count");
    }
    
    // Get paging limit from config
    if (object_provide_str(pool, &paging_config, config->data, "agent.paging_limit") == RESULT_OK) {
        if (object_provide_str(pool, &max_tokens_obj, paging_config, "max_tokens") == RESULT_OK) {
            paging_limit = strtoull(max_tokens_obj->string->data, NULL, 10);
        }
    }
    
    // Check if we need paging
    if (token_count >= paging_limit) {
        *requires_paging = 1;
    }
    
    return RESULT_OK;
}

// Execute paging operation to manage memory overflow
result_t agent_state_execute_paging(pool_t* pool, config_t* config, agent_t* agent) {
    object_t* working_memory = NULL;
    object_t* storage = NULL;
    object_t* config_thinking_log = NULL;
    object_t* config_evaluation_log = NULL;
    object_t* thinking_prefix_obj = NULL;
    object_t* evaluation_prefix_obj = NULL;
    object_t* thinking_max_entries_obj = NULL;
    object_t* evaluation_max_entries_obj = NULL;
    
    char thinking_prefix[32] = "thinking_log_";  // Default prefix
    char evaluation_prefix[32] = "evaluation_log_";  // Default prefix
    uint64_t thinking_max_entries = 4;  // Default value
    uint64_t evaluation_max_entries = 10;  // Default value
    
    // Get configuration values if available
    if (config != NULL) {
        // Get thinking log configuration
        if (object_provide_str(pool, &config_thinking_log, config->data, "agent.thinking_log") == RESULT_OK) {
            if (object_provide_str(pool, &thinking_prefix_obj, config_thinking_log, "key_prefix") == RESULT_OK) {
                if (thinking_prefix_obj->string != NULL && thinking_prefix_obj->string->size > 0) {
                    snprintf(thinking_prefix, sizeof(thinking_prefix), "%s", thinking_prefix_obj->string->data);
                }
            }
            if (object_provide_str(pool, &thinking_max_entries_obj, config_thinking_log, "max_entries") == RESULT_OK) {
                thinking_max_entries = strtoull(thinking_max_entries_obj->string->data, NULL, 10);
                if (thinking_max_entries == 0) thinking_max_entries = 4; // Fallback
            }
        }
        
        // Get evaluation log configuration
        if (object_provide_str(pool, &config_evaluation_log, config->data, "agent.evaluation_log") == RESULT_OK) {
            if (object_provide_str(pool, &evaluation_prefix_obj, config_evaluation_log, "key_prefix") == RESULT_OK) {
                if (evaluation_prefix_obj->string != NULL && evaluation_prefix_obj->string->size > 0) {
                    snprintf(evaluation_prefix, sizeof(evaluation_prefix), "%s", evaluation_prefix_obj->string->data);
                }
            }
            if (object_provide_str(pool, &evaluation_max_entries_obj, config_evaluation_log, "max_entries") == RESULT_OK) {
                evaluation_max_entries = strtoull(evaluation_max_entries_obj->string->data, NULL, 10);
                if (evaluation_max_entries == 0) evaluation_max_entries = 10; // Fallback
            }
        }
    }
    
    // Get working memory and storage
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory for paging");
    }
    
    if (object_provide_str(pool, &storage, agent->data, "storage") != RESULT_OK) {
        // Create storage if it doesn't exist
        object_t* new_storage = NULL;
        if (object_create(pool, &new_storage) != RESULT_OK) {
            RETURN_ERR("Failed to create storage object for paging");
        }
        
        string_t* storage_path = NULL;
        if (string_create_str(pool, &storage_path, "storage") != RESULT_OK) {
            if (object_destroy(pool, new_storage) != RESULT_OK) {
                RETURN_ERR("Failed to destroy new storage after path creation failure");
            }
            RETURN_ERR("Failed to create storage path string");
        }
        
        if (object_set(pool, agent->data, storage_path, new_storage) != RESULT_OK) {
            if (string_destroy(pool, storage_path) != RESULT_OK ||
                object_destroy(pool, new_storage) != RESULT_OK) {
                RETURN_ERR("Failed to destroy resources after storage set failure");
            }
            RETURN_ERR("Failed to set storage object");
        }
        
        if (string_destroy(pool, storage_path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy storage path string");
        }
        
        storage = new_storage;
    }
    
    // Simple paging strategy: move least recently used items from working memory to storage
    // For this implementation, we'll move items with certain prefixes to storage
    
    // Find items to page out (example: old thinking logs and evaluation logs)
    for (uint64_t i = 1; i <= (thinking_max_entries / 2); i++) {  // Page out older thinking logs
        char old_log_key[64];
        snprintf(old_log_key, sizeof(old_log_key), "%s%03lu", thinking_prefix, (unsigned long)i);
        
        object_t* log_to_page = NULL;
        if (object_provide_str(pool, &log_to_page, working_memory, old_log_key) == RESULT_OK) {
            // Move to storage with archived prefix
            char storage_key[128]; // Increased buffer size to prevent truncation
            snprintf(storage_key, sizeof(storage_key), "archived_%s", old_log_key);
            
            string_t* storage_key_string = NULL;
            if (string_create_str(pool, &storage_key_string, storage_key) != RESULT_OK) {
                RETURN_ERR("Failed to create storage key string for paging");
            }
            
            if (object_set_string(pool, storage, storage_key_string, log_to_page->string) != RESULT_OK) {
                if (string_destroy(pool, storage_key_string) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy storage key string after set failure");
                }
                RETURN_ERR("Failed to move log to storage during paging");
            }
            
            // Remove from working memory by setting to empty
            string_t* wm_key_string = NULL;
            string_t* empty_string = NULL;
            if (string_create_str(pool, &wm_key_string, old_log_key) != RESULT_OK ||
                string_create_str(pool, &empty_string, "") != RESULT_OK) {
                if (string_destroy(pool, storage_key_string) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy storage key string after removal failure");
                }
                RETURN_ERR("Failed to create strings for log removal");
            }
            
            if (object_set_string(pool, working_memory, wm_key_string, empty_string) != RESULT_OK) {
                if (string_destroy(pool, storage_key_string) != RESULT_OK ||
                    string_destroy(pool, wm_key_string) != RESULT_OK ||
                    string_destroy(pool, empty_string) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy strings after removal failure");
                }
                RETURN_ERR("Failed to remove log from working memory during paging");
            }
            
            if (string_destroy(pool, storage_key_string) != RESULT_OK ||
                string_destroy(pool, wm_key_string) != RESULT_OK ||
                string_destroy(pool, empty_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy strings after paging operation");
            }
        }
    }
    
    // Also page out older evaluation logs
    for (uint64_t i = 1; i <= (evaluation_max_entries / 2); i++) {  // Page out older evaluation logs
        char old_log_key[64];
        snprintf(old_log_key, sizeof(old_log_key), "%s%03lu", evaluation_prefix, (unsigned long)i);
        
        object_t* log_to_page = NULL;
        if (object_provide_str(pool, &log_to_page, working_memory, old_log_key) == RESULT_OK) {
            // Move to storage with archived prefix
            char storage_key[128];
            snprintf(storage_key, sizeof(storage_key), "archived_%s", old_log_key);
            
            string_t* storage_key_string = NULL;
            if (string_create_str(pool, &storage_key_string, storage_key) != RESULT_OK) {
                continue; // Skip this entry on error
            }
            
            if (object_set_string(pool, storage, storage_key_string, log_to_page->string) == RESULT_OK) {
                // Remove from working memory only
                string_t* wm_key_string = NULL;
                string_t* empty_string = NULL;
                if (string_create_str(pool, &wm_key_string, old_log_key) == RESULT_OK &&
                    string_create_str(pool, &empty_string, "") == RESULT_OK) {
                    
                    if (object_set_string(pool, working_memory, wm_key_string, empty_string) != RESULT_OK) {
                        printf("Error: Failed to remove evaluation log from working memory during paging\n");
                    }
                    
                    if (string_destroy(pool, wm_key_string) != RESULT_OK) {
                        printf("Error: Failed to destroy working memory key string during evaluation log paging\n");
                    }
                    if (string_destroy(pool, empty_string) != RESULT_OK) {
                        printf("Error: Failed to destroy empty string during evaluation log paging\n");
                    }
                }
            }
            
            if (string_destroy(pool, storage_key_string) != RESULT_OK) {
                printf("Error: Failed to destroy storage key string during evaluation log paging\n");
            }
        }
    }
    
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
