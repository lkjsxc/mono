#include "agent/state.h"
#include "utils/file.h"

// Helper function to estimate token count for working memory
size_t agent_state_estimate_tokens(const object_t* working_memory) {
    if (!working_memory)
        return 0;

    size_t total_tokens = 0;
    object_t* child = working_memory->child;

    while (child) {
        if (child->string) {
            // Rough estimation: ~4 characters per token
            total_tokens += (child->string->size / 4) + 1;
        }
        if (child->child && child->child->string) {
            // Add value tokens
            total_tokens += (child->child->string->size / 4) + 1;
        }
        child = child->next;
    }

    return total_tokens;
}

// Helper function to automatically transition state after action execution
result_t agent_state_auto_transition(pool_t* pool, config_t* config, agent_t* agent) {
    object_t* working_memory;
    object_t* paging_limit_config;
    object_t* hard_limit_config;
    object_t* max_tokens_obj;
    string_t* state_path;

    // Get working memory
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory for state transition");
    }

    // Estimate current token count
    size_t current_tokens = agent_state_estimate_tokens(working_memory);

    // Get paging and hard limits from config
    uint64_t paging_limit = 4096;  // Default
    uint64_t hard_limit = 8192;    // Default

    if (object_provide_str(pool, &paging_limit_config, config->data, "agent.paging_limit") == RESULT_OK) {
        if (object_provide_str(pool, &max_tokens_obj, paging_limit_config, "max_tokens") == RESULT_OK) {
            if (max_tokens_obj->string && max_tokens_obj->string->data) {
                paging_limit = strtoull(max_tokens_obj->string->data, NULL, 10);
            }
        }
    }

    if (object_provide_str(pool, &hard_limit_config, config->data, "agent.hard_limit") == RESULT_OK) {
        if (object_provide_str(pool, &max_tokens_obj, hard_limit_config, "max_tokens") == RESULT_OK) {
            if (max_tokens_obj->string && max_tokens_obj->string->data) {
                hard_limit = strtoull(max_tokens_obj->string->data, NULL, 10);
            }
        }
    }

    // Create path string for state update
    if (string_create_str(pool, &state_path, "state") != RESULT_OK) {
        RETURN_ERR("Failed to create state path string");
    }

    // Determine next state based on token count
    const char* next_state;
    if (current_tokens >= hard_limit) {
        next_state = "paging";
        printf("Auto-transition: executing -> paging (tokens: %zu >= hard_limit: %lu)\n",
               current_tokens, hard_limit);
    } else if (current_tokens >= paging_limit) {
        next_state = "paging";
        printf("Auto-transition: executing -> paging (tokens: %zu >= paging_limit: %lu)\n",
               current_tokens, paging_limit);
    } else {
        next_state = "thinking";
        printf("Auto-transition: executing -> thinking (tokens: %zu < paging_limit: %lu)\n",
               current_tokens, paging_limit);
    }

    // Set the new state
    string_t* next_state_str;
    if (string_create_str(pool, &next_state_str, next_state) != RESULT_OK) {
        if (string_destroy(pool, state_path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy state path string");
        }
        RETURN_ERR("Failed to create next state string");
    }

    if (object_set_string(pool, agent->data, state_path, next_state_str) != RESULT_OK) {
        if (string_destroy(pool, next_state_str) != RESULT_OK) {
            RETURN_ERR("Failed to destroy next state string");
        }
        if (string_destroy(pool, state_path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy state path string");
        }
        RETURN_ERR("Failed to set new agent state");
    }

    if (string_destroy(pool, next_state_str) != RESULT_OK) {
        if (string_destroy(pool, state_path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy state path string");
        }
        RETURN_ERR("Failed to destroy next state string");
    }

    if (string_destroy(pool, state_path) != RESULT_OK) {
        RETURN_ERR("Failed to destroy state path string");
    }

    return RESULT_OK;
}

// Helper function to add thinking log entry to working memory with rotation
static result_t add_thinking_log_entry(pool_t* pool, agent_t* agent, const string_t* thinking_log) {
    object_t* working_memory;
    string_t* key_string;
    char key_buffer[64];
    uint64_t max_entries = 10;  // default, could be made configurable later

    // Get working memory
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory from agent");
    }

    // Find next available thinking log slot (rotate if needed)
    for (uint64_t i = 0; i < max_entries; i++) {
        snprintf(key_buffer, sizeof(key_buffer), "thinking_log_%02lu", i);

        if (string_create_str(pool, &key_string, key_buffer) != RESULT_OK) {
            RETURN_ERR("Failed to create thinking log key string");
        }

        object_t* existing_entry;
        if (object_get(&existing_entry, working_memory, key_string) != RESULT_OK || !existing_entry || !existing_entry->string) {
            // Found empty slot, use it
            if (object_set_string(pool, working_memory, key_string, thinking_log) != RESULT_OK) {
                if (string_destroy(pool, key_string) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy key string after set failure");
                }
                RETURN_ERR("Failed to set thinking log entry");
            }

            printf("Added thinking log [%s]: %.*s\n", key_buffer,
                   (int)thinking_log->size, thinking_log->data);

            if (string_destroy(pool, key_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy key string after successful set");
            }
            return RESULT_OK;
        }

        if (string_destroy(pool, key_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy key string in loop");
        }
    }

    // All slots are full, rotate (overwrite oldest entry, which is slot 0)
    snprintf(key_buffer, sizeof(key_buffer), "thinking_log_%02d", 0);
    if (string_create_str(pool, &key_string, key_buffer) != RESULT_OK) {
        RETURN_ERR("Failed to create rotation key string");
    }

    if (object_set_string(pool, working_memory, key_string, thinking_log) != RESULT_OK) {
        if (string_destroy(pool, key_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy rotation key string after set failure");
        }
        RETURN_ERR("Failed to set rotated thinking log entry");
    }

    printf("Rotated thinking log [%s]: %.*s\n", key_buffer,
           (int)thinking_log->size, thinking_log->data);

    if (string_destroy(pool, key_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy rotation key string");
    }

    return RESULT_OK;
}

// Helper function to update agent state and handle thinking logs
result_t agent_state_update_and_log(pool_t* pool, agent_t* agent, object_t* response_obj) {
    object_t* agent_response;
    object_t* next_state_obj;
    object_t* thinking_log_obj;
    object_t* current_state;

    // Extract agent response from LLM output
    if (object_provide_str(pool, &agent_response, response_obj, "agent") != RESULT_OK) {
        RETURN_ERR("Failed to get agent object from LLM response");
    }

    // Check if this is a state transition (thinking state)
    if (object_provide_str(pool, &next_state_obj, agent_response, "next_state") == RESULT_OK) {
        // Get current state for logging
        if (object_provide_str(pool, &current_state, agent->data, "state") != RESULT_OK) {
            RETURN_ERR("Failed to get current state from agent");
        }

        // Update the agent's state
        string_t* state_path;
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

        printf("State transition: %.*s -> %.*s\n",
               (int)current_state->string->size, current_state->string->data,
               (int)next_state_obj->string->size, next_state_obj->string->data);

        // Handle thinking log if present
        if (object_provide_str(pool, &thinking_log_obj, agent_response, "thinking_log") == RESULT_OK) {
            printf("Thinking: %.*s\n",
                   (int)thinking_log_obj->string->size, thinking_log_obj->string->data);

            // Add thinking log to working memory with rotation
            if (add_thinking_log_entry(pool, agent, thinking_log_obj->string) != RESULT_OK) {
                RETURN_ERR("Failed to add thinking log to working memory");
            }
        }
    }

    return RESULT_OK;
}
