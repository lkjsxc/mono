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
result_t agent_state_update_and_log(pool_t* pool, agent_t* agent, object_t* response_obj) {
    object_t* next_state_obj = NULL;
    
    // Extract next state from response
    if (agent_state_extract_next_state(pool, response_obj, &next_state_obj) == RESULT_OK) {
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
    }
    
    // Handle thinking log if present
    object_t* thinking_log_obj = NULL;
    if (object_provide_str(pool, &thinking_log_obj, response_obj, "thinking_log") == RESULT_OK) {
        if (agent_state_manage_thinking_log(pool, NULL, agent, response_obj) != RESULT_OK) {
            RETURN_ERR("Failed to manage thinking log");
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
    if (agent_state_manage_evaluation_log(pool, agent, response_obj) != RESULT_OK) {
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
    
    // Add estimation for thinking logs
    uint64_t log_count = 0;
    char log_key[64];
    for (uint64_t i = 1; i <= 10; i++) {  // Check up to 10 thinking logs
        snprintf(log_key, sizeof(log_key), "thinking_log_%03lu", (unsigned long)i);
        object_t* log_obj = NULL;
        if (object_provide_str(pool, &log_obj, agent->data, log_key) == RESULT_OK) {
            log_count++;
            *token_count += log_obj->string->size / 4;
        }
    }
    
    if (string_destroy(pool, memory_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy memory string after token estimation");
    }
    
    return RESULT_OK;
}

// Extract next state from LLM response
result_t agent_state_extract_next_state(pool_t* pool, object_t* response_obj, object_t** next_state_obj) {
    if (object_provide_str(pool, next_state_obj, response_obj, "next_state") != RESULT_OK) {
        RETURN_ERR("Failed to extract next_state from response");
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

// Handle thinking log entries with rotation
result_t agent_state_manage_thinking_log(pool_t* pool, config_t* config, agent_t* agent, object_t* response_obj) {
    object_t* thinking_log_obj = NULL;
    object_t* config_thinking_log = NULL;
    object_t* max_entries_obj = NULL;
    uint64_t max_entries = 4;  // Default value
    
    // Extract thinking log from response
    if (object_provide_str(pool, &thinking_log_obj, response_obj, "thinking_log") != RESULT_OK) {
        RETURN_ERR("Failed to extract thinking_log from response");
    }
    
    // Get max entries from config if available
    if (config != NULL) {
        if (object_provide_str(pool, &config_thinking_log, config->data, "agent.thinking_log") == RESULT_OK) {
            if (object_provide_str(pool, &max_entries_obj, config_thinking_log, "max_entries") == RESULT_OK) {
                max_entries = strtoull(max_entries_obj->string->data, NULL, 10);
            }
        }
    }
    
    // Find the next available thinking log slot
    char log_key[64];
    uint64_t next_index = 1;
    
    // Find the highest existing index
    for (uint64_t i = 1; i <= max_entries; i++) {
        snprintf(log_key, sizeof(log_key), "thinking_log_%03lu", (unsigned long)i);
        object_t* existing_log = NULL;
        if (object_provide_str(pool, &existing_log, agent->data, log_key) == RESULT_OK) {
            next_index = i + 1;
        }
    }
    
    // If we've exceeded max entries, rotate (remove first, shift others)
    if (next_index > max_entries) {
        // Shift all logs down by one position
        for (uint64_t i = 1; i < max_entries; i++) {
            char old_key[64], new_key[64];
            snprintf(old_key, sizeof(old_key), "thinking_log_%03lu", (unsigned long)(i + 1));
            snprintf(new_key, sizeof(new_key), "thinking_log_%03lu", (unsigned long)i);
            
            object_t* log_to_move = NULL;
            if (object_provide_str(pool, &log_to_move, agent->data, old_key) == RESULT_OK) {
                string_t* new_key_string = NULL;
                if (string_create_str(pool, &new_key_string, new_key) != RESULT_OK) {
                    RETURN_ERR("Failed to create new key string for log rotation");
                }
                
                if (object_set_string(pool, agent->data, new_key_string, log_to_move->string) != RESULT_OK) {
                    if (string_destroy(pool, new_key_string) != RESULT_OK) {
                        RETURN_ERR("Failed to destroy new key string after set failure");
                    }
                    RETURN_ERR("Failed to rotate thinking log entry");
                }
                
                if (string_destroy(pool, new_key_string) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy new key string after rotation");
                }
            }
        }
        next_index = max_entries;
    }
    
    // Add the new thinking log entry
    snprintf(log_key, sizeof(log_key), "thinking_log_%03lu", (unsigned long)next_index);
    string_t* log_key_string = NULL;
    if (string_create_str(pool, &log_key_string, log_key) != RESULT_OK) {
        RETURN_ERR("Failed to create log key string");
    }
    
    if (object_set_string(pool, agent->data, log_key_string, thinking_log_obj->string) != RESULT_OK) {
        if (string_destroy(pool, log_key_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy log key string after set failure");
        }
        RETURN_ERR("Failed to set thinking log entry");
    }
    
    if (string_destroy(pool, log_key_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy log key string");
    }
    
    return RESULT_OK;
}

// Handle evaluation log entries
result_t agent_state_manage_evaluation_log(pool_t* pool, agent_t* agent, object_t* response_obj) {
    object_t* evaluation_log_obj = NULL;
    
    // Extract evaluation log from response
    if (object_provide_str(pool, &evaluation_log_obj, response_obj, "evaluation_log") != RESULT_OK) {
        // Evaluation log is optional, don't treat as error
        return RESULT_OK;
    }
    
    // Find the next available evaluation log slot (simple increment)
    char log_key[64];
    uint64_t next_index = 1;
    
    // Find the highest existing index
    for (uint64_t i = 1; i <= 10; i++) {  // Check up to 10 evaluation logs
        snprintf(log_key, sizeof(log_key), "evaluation_log_%03lu", (unsigned long)i);
        object_t* existing_log = NULL;
        if (object_provide_str(pool, &existing_log, agent->data, log_key) == RESULT_OK) {
            next_index = i + 1;
        }
    }
    
    // Limit evaluation logs to prevent memory overflow
    if (next_index > 10) {
        // Replace the last entry
        next_index = 10;
    }
    
    // Add the new evaluation log entry
    snprintf(log_key, sizeof(log_key), "evaluation_log_%03lu", (unsigned long)next_index);
    string_t* log_key_string = NULL;
    if (string_create_str(pool, &log_key_string, log_key) != RESULT_OK) {
        RETURN_ERR("Failed to create evaluation log key string");
    }
    
    if (object_set_string(pool, agent->data, log_key_string, evaluation_log_obj->string) != RESULT_OK) {
        if (string_destroy(pool, log_key_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy evaluation log key string after set failure");
        }
        RETURN_ERR("Failed to set evaluation log entry");
    }
    
    if (string_destroy(pool, log_key_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy evaluation log key string");
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
    (void)config; // Mark parameter as intentionally unused
    object_t* working_memory = NULL;
    object_t* storage = NULL;
    
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
    
    // Find items to page out (example: old thinking logs)
    for (uint64_t i = 1; i <= 5; i++) {  // Page out older thinking logs
        char old_log_key[64];
        snprintf(old_log_key, sizeof(old_log_key), "thinking_log_%03lu", (unsigned long)i);
        
        object_t* log_to_page = NULL;
        if (object_provide_str(pool, &log_to_page, agent->data, old_log_key) == RESULT_OK) {
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
            string_t* agent_key_string = NULL;
            string_t* empty_string = NULL;
            if (string_create_str(pool, &agent_key_string, old_log_key) != RESULT_OK ||
                string_create_str(pool, &empty_string, "") != RESULT_OK) {
                if (string_destroy(pool, storage_key_string) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy storage key string after removal failure");
                }
                RETURN_ERR("Failed to create strings for log removal");
            }
            
            if (object_set_string(pool, agent->data, agent_key_string, empty_string) != RESULT_OK) {
                if (string_destroy(pool, storage_key_string) != RESULT_OK ||
                    string_destroy(pool, agent_key_string) != RESULT_OK ||
                    string_destroy(pool, empty_string) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy strings after removal failure");
                }
                RETURN_ERR("Failed to remove log from agent memory during paging");
            }
            
            if (string_destroy(pool, storage_key_string) != RESULT_OK ||
                string_destroy(pool, agent_key_string) != RESULT_OK ||
                string_destroy(pool, empty_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy strings after paging operation");
            }
        }
    }
    
    return RESULT_OK;
}
