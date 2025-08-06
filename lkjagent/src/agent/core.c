#include "agent/core.h"
#include "utils/file.h"

// Forward declarations for internal functions
static __attribute__((warn_unused_result)) result_t extract_config_objects(pool_t* pool, config_t* config, agent_t* agent, object_t** agent_workingmemory, object_t** agent_state, object_t** config_agent_state, object_t** config_agent_state_base, object_t** config_agent_state_base_prompt, object_t** config_agent_state_main, object_t** config_agent_state_main_prompt);

static __attribute__((warn_unused_result)) result_t build_json_request_header(pool_t* pool, string_t** dst);

static __attribute__((warn_unused_result)) result_t append_base_prompt(pool_t* pool, string_t** dst, object_t* config_agent_state_base_prompt);

static __attribute__((warn_unused_result)) result_t append_state_prompt(pool_t* pool, string_t** dst, object_t* config_agent_state_main_prompt);

static __attribute__((warn_unused_result)) result_t append_working_memory(pool_t* pool, string_t** dst, object_t* agent_workingmemory);

static __attribute__((warn_unused_result)) result_t append_json_request_footer(pool_t* pool, string_t** dst, config_t* config);

static __attribute__((warn_unused_result)) result_t create_http_request_resources(pool_t* pool, string_t** send_string, string_t** content_type, string_t** recv_http_string);

static __attribute__((warn_unused_result)) result_t extract_llm_response_content(pool_t* pool, object_t* recv_http_object, object_t** recv_content_object);

static __attribute__((warn_unused_result)) result_t cleanup_http_resources(pool_t* pool, string_t* send_string, string_t* content_type, string_t* recv_http_string, object_t* recv_http_object);

// Execute function declarations
static __attribute__((warn_unused_result)) result_t lkjagent_agent_execute(pool_t* pool, agent_t* agent, const string_t* recv);

static __attribute__((warn_unused_result)) result_t parse_llm_response(pool_t* pool, const string_t* recv, object_t** response_obj);

static __attribute__((warn_unused_result)) result_t execute_action_working_memory_add(pool_t* pool, agent_t* agent, object_t* action);

static __attribute__((warn_unused_result)) result_t execute_action_working_memory_remove(pool_t* pool, agent_t* agent, object_t* action);

static __attribute__((warn_unused_result)) result_t execute_action_storage_load(pool_t* pool, agent_t* agent, object_t* action);

static __attribute__((warn_unused_result)) result_t execute_action_storage_save(pool_t* pool, agent_t* agent, object_t* action);

static __attribute__((warn_unused_result)) result_t update_agent_state_and_thinking_log(pool_t* pool, agent_t* agent, object_t* response_obj);

static __attribute__((warn_unused_result)) result_t save_agent_memory(pool_t* pool, agent_t* agent);

static __attribute__((warn_unused_result)) result_t add_thinking_log_entry(pool_t* pool, agent_t* agent, const string_t* thinking_log);

// Helper function to extract and validate configuration objects
static __attribute__((warn_unused_result)) result_t extract_config_objects(pool_t* pool, config_t* config, agent_t* agent, object_t** agent_workingmemory, object_t** agent_state, object_t** config_agent_state, object_t** config_agent_state_base, object_t** config_agent_state_base_prompt, object_t** config_agent_state_main, object_t** config_agent_state_main_prompt) {
    if (object_provide_str(pool, agent_workingmemory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory from agent");
    }
    if (object_provide_str(pool, agent_state, agent->data, "state") != RESULT_OK) {
        RETURN_ERR("Failed to get state from agent");
    }
    if (object_provide_str(pool, config_agent_state, config->data, "agent.state") != RESULT_OK) {
        RETURN_ERR("Failed to get agent config from configuration");
    }
    if (object_provide_str(pool, config_agent_state_base, *config_agent_state, "base") != RESULT_OK) {
        RETURN_ERR("Failed to get base config from agent configuration");
    }
    if (object_provide_str(pool, config_agent_state_base_prompt, *config_agent_state_base, "prompt") != RESULT_OK) {
        RETURN_ERR("Failed to get base prompt from agent configuration");
    }
    if (object_provide_string(config_agent_state_main, *config_agent_state, (*agent_state)->string) != RESULT_OK) {
        RETURN_ERR("Failed to get state config from agent configuration");
    }
    if (object_provide_str(pool, config_agent_state_main_prompt, *config_agent_state_main, "prompt") != RESULT_OK) {
        RETURN_ERR("Failed to get state prompt from agent configuration");
    }

    return RESULT_OK;
}

// Helper function to build the JSON request header
static __attribute__((warn_unused_result)) result_t build_json_request_header(pool_t* pool, string_t** dst) {
    if (string_copy_str(pool, dst, "{\"messages\":[{\"role\":\"user\",\"content\":\"") != RESULT_OK) {
        RETURN_ERR("Failed to copy initial prompt string");
    }
    return RESULT_OK;
}

// Helper function to append and escape the base prompt
static __attribute__((warn_unused_result)) result_t append_base_prompt(pool_t* pool, string_t** dst, object_t* config_agent_state_base_prompt) {
    string_t* tmp_string;

    if (string_create(pool, &tmp_string) != RESULT_OK) {
        RETURN_ERR("Failed to create temporary string for base prompt");
    }

    if (object_tostring_xml(pool, &tmp_string, config_agent_state_base_prompt) != RESULT_OK) {
        if (string_destroy(pool, tmp_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy temporary string after XML conversion failure");
        }
        RETURN_ERR("Failed to convert base prompt to XML string");
    }
    if (string_escape(pool, &tmp_string) != RESULT_OK) {
        if (string_destroy(pool, tmp_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy temporary string after escape failure");
        }
        RETURN_ERR("Failed to escape base prompt string");
    }
    if (string_append_string(pool, dst, tmp_string) != RESULT_OK) {
        if (string_destroy(pool, tmp_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy temporary string after append failure");
        }
        RETURN_ERR("Failed to append base prompt string");
    }

    if (string_destroy(pool, tmp_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy temporary string for base prompt");
    }

    return RESULT_OK;
}

// Helper function to append and escape the state-specific prompt
static __attribute__((warn_unused_result)) result_t append_state_prompt(pool_t* pool, string_t** dst, object_t* config_agent_state_main_prompt) {
    string_t* tmp_string;

    if (string_create(pool, &tmp_string) != RESULT_OK) {
        RETURN_ERR("Failed to create temporary string for state prompt");
    }

    if (object_tostring_xml(pool, &tmp_string, config_agent_state_main_prompt) != RESULT_OK) {
        if (string_destroy(pool, tmp_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy temporary string after XML conversion failure");
        }
        RETURN_ERR("Failed to convert state prompt to XML string");
    }
    if (string_escape(pool, &tmp_string) != RESULT_OK) {
        if (string_destroy(pool, tmp_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy temporary string after escape failure");
        }
        RETURN_ERR("Failed to escape state prompt string");
    }
    if (string_append_string(pool, dst, tmp_string) != RESULT_OK) {
        if (string_destroy(pool, tmp_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy temporary string after append failure");
        }
        RETURN_ERR("Failed to append state prompt string");
    }

    if (string_destroy(pool, tmp_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy temporary string for state prompt");
    }

    return RESULT_OK;
}

// Helper function to append working memory with XML tags
static __attribute__((warn_unused_result)) result_t append_working_memory(pool_t* pool, string_t** dst, object_t* agent_workingmemory) {
    string_t* tmp_string;

    if (string_create(pool, &tmp_string) != RESULT_OK) {
        RETURN_ERR("Failed to create temporary string for working memory");
    }

    if (object_tostring_xml(pool, &tmp_string, agent_workingmemory) != RESULT_OK) {
        if (string_destroy(pool, tmp_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy temporary string after XML conversion failure");
        }
        RETURN_ERR("Failed to convert working memory to XML string");
    }
    if (string_escape(pool, &tmp_string) != RESULT_OK) {
        if (string_destroy(pool, tmp_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy temporary string after escape failure");
        }
        RETURN_ERR("Failed to escape working memory string");
    }

    if (string_append_str(pool, dst, "<working_memory>") != RESULT_OK) {
        if (string_destroy(pool, tmp_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy temporary string after tag append failure");
        }
        RETURN_ERR("Failed to append working memory opening tag");
    }
    if (string_append_string(pool, dst, tmp_string) != RESULT_OK) {
        if (string_destroy(pool, tmp_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy temporary string after content append failure");
        }
        RETURN_ERR("Failed to append working memory string");
    }
    if (string_append_str(pool, dst, "</working_memory>") != RESULT_OK) {
        if (string_destroy(pool, tmp_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy temporary string after closing tag failure");
        }
        RETURN_ERR("Failed to append working memory closing tag");
    }

    if (string_destroy(pool, tmp_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy temporary string for working memory");
    }

    return RESULT_OK;
}

// Helper function to append JSON request footer with model configuration
static __attribute__((warn_unused_result)) result_t append_json_request_footer(pool_t* pool, string_t** dst, config_t* config) {
    object_t* tmp_object;

    if (string_append_str(pool, dst, "\"}], \"model\":\"") != RESULT_OK) {
        RETURN_ERR("Failed to append model field start");
    }

    if (object_provide_str(pool, &tmp_object, config->data, "llm.model") != RESULT_OK) {
        RETURN_ERR("Failed to get model from configuration");
    }
    if (string_append_string(pool, dst, tmp_object->string) != RESULT_OK) {
        RETURN_ERR("Failed to append model string");
    }

    if (string_append_str(pool, dst, "\", \"temperature\":") != RESULT_OK) {
        RETURN_ERR("Failed to append temperature field start");
    }

    if (object_provide_str(pool, &tmp_object, config->data, "llm.temperature") != RESULT_OK) {
        RETURN_ERR("Failed to get temperature from configuration");
    }
    if (string_append_string(pool, dst, tmp_object->string) != RESULT_OK) {
        RETURN_ERR("Failed to append temperature string");
    }

    if (string_append_str(pool, dst, ", \"max_tokens\":-1,\"stream\":false}") != RESULT_OK) {
        RETURN_ERR("Failed to append closing JSON string");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_agent_makeprompt(pool_t* pool, config_t* config, agent_t* agent, string_t** dst) {
    object_t* agent_workingmemory;
    object_t* agent_state;
    object_t* config_agent_state;
    object_t* config_agent_state_base;
    object_t* config_agent_state_base_prompt;
    object_t* config_agent_state_main;
    object_t* config_agent_state_main_prompt;

    // Extract all configuration objects first
    if (extract_config_objects(pool, config, agent, &agent_workingmemory, &agent_state, &config_agent_state,
                               &config_agent_state_base, &config_agent_state_base_prompt,
                               &config_agent_state_main, &config_agent_state_main_prompt) != RESULT_OK) {
        RETURN_ERR("Failed to extract configuration objects");
    }

    // Build the JSON request incrementally
    if (build_json_request_header(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to build JSON request header");
    }

    if (append_base_prompt(pool, dst, config_agent_state_base_prompt) != RESULT_OK) {
        RETURN_ERR("Failed to append base prompt");
    }

    if (append_state_prompt(pool, dst, config_agent_state_main_prompt) != RESULT_OK) {
        RETURN_ERR("Failed to append state prompt");
    }

    if (append_working_memory(pool, dst, agent_workingmemory) != RESULT_OK) {
        RETURN_ERR("Failed to append working memory");
    }

    if (append_json_request_footer(pool, dst, config) != RESULT_OK) {
        RETURN_ERR("Failed to append JSON request footer");
    }

    return RESULT_OK;
}

// Helper function to create and initialize HTTP request resources
static __attribute__((warn_unused_result)) result_t create_http_request_resources(pool_t* pool, string_t** send_string, string_t** content_type, string_t** recv_http_string) {
    // Create send string
    if (string_create(pool, send_string) != RESULT_OK) {
        RETURN_ERR("Failed to create send string");
    }

    // Create content type string
    if (string_create_str(pool, content_type, "application/json") != RESULT_OK) {
        if (string_destroy(pool, *send_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy send string after content_type failure");
        }
        RETURN_ERR("Failed to create content type string");
    }

    // Create response string
    if (string_create(pool, recv_http_string) != RESULT_OK) {
        if (string_destroy(pool, *send_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy send string after response creation failure");
        }
        if (string_destroy(pool, *content_type) != RESULT_OK) {
            RETURN_ERR("Failed to destroy content type after response creation failure");
        }
        RETURN_ERR("Failed to create response string");
    }

    return RESULT_OK;
}

// Helper function to extract content from LLM response
static __attribute__((warn_unused_result)) result_t extract_llm_response_content(pool_t* pool, object_t* recv_http_object, object_t** recv_content_object) {
    if (object_provide_str(pool, recv_content_object, recv_http_object, "choices.[0].message.content") != RESULT_OK) {
        RETURN_ERR("Failed to get content from HTTP response");
    }
    return RESULT_OK;
}

// Helper function to clean up all HTTP-related resources
static __attribute__((warn_unused_result)) result_t cleanup_http_resources(pool_t* pool, string_t* send_string, string_t* content_type, string_t* recv_http_string, object_t* recv_http_object) {
    result_t cleanup_result = RESULT_OK;

    if (send_string && string_destroy(pool, send_string) != RESULT_OK) {
        cleanup_result = RESULT_ERR;
    }
    if (content_type && string_destroy(pool, content_type) != RESULT_OK) {
        cleanup_result = RESULT_ERR;
    }
    if (recv_http_string && string_destroy(pool, recv_http_string) != RESULT_OK) {
        cleanup_result = RESULT_ERR;
    }
    if (recv_http_object && object_destroy(pool, recv_http_object) != RESULT_OK) {
        cleanup_result = RESULT_ERR;
    }

    if (cleanup_result != RESULT_OK) {
        RETURN_ERR("Failed to clean up one or more HTTP resources");
    }

    return RESULT_OK;
}

// Core agent processing function - orchestrates the full LLM interaction cycle
result_t lkjagent_agent(pool_t* pool, config_t* config, agent_t* agent) {
    // Resource declarations
    string_t* send_string = NULL;
    string_t* content_type = NULL;
    string_t* recv_http_string = NULL;
    object_t* recv_http_object = NULL;
    object_t* recv_content_object = NULL;
    object_t* url_object = NULL;

    if (create_http_request_resources(pool, &send_string, &content_type, &recv_http_string) != RESULT_OK) {
        RETURN_ERR("Failed to create HTTP request resources");
    }

    if (lkjagent_agent_makeprompt(pool, config, agent, &send_string) != RESULT_OK) {
        if (cleanup_http_resources(pool, send_string, content_type, recv_http_string, NULL) != RESULT_OK) {
            RETURN_ERR("Failed to cleanup HTTP resources after prompt generation failure");
        }
        RETURN_ERR("Failed to create prompt for agent");
    }

    printf("Send: \n%.*s\n", (int)send_string->size, send_string->data);

    if (object_provide_str(pool, &url_object, config->data, "llm.endpoint") != RESULT_OK) {
        if (cleanup_http_resources(pool, send_string, content_type, recv_http_string, NULL) != RESULT_OK) {
            RETURN_ERR("Failed to cleanup HTTP resources after URL retrieval failure");
        }
        RETURN_ERR("Failed to get URL from config");
    }

    if (http_post(pool, url_object->string, content_type, send_string, &recv_http_string) != RESULT_OK) {
        if (cleanup_http_resources(pool, send_string, content_type, recv_http_string, NULL) != RESULT_OK) {
            RETURN_ERR("Failed to cleanup HTTP resources after HTTP POST failure");
        }
        RETURN_ERR("Failed to send HTTP POST request");
    }

    if (object_parse_json(pool, &recv_http_object, recv_http_string) != RESULT_OK) {
        if (cleanup_http_resources(pool, send_string, content_type, recv_http_string, NULL) != RESULT_OK) {
            RETURN_ERR("Failed to cleanup HTTP resources after JSON parse failure");
        }
        RETURN_ERR("Failed to parse HTTP response JSON");
    }

    if (extract_llm_response_content(pool, recv_http_object, &recv_content_object) != RESULT_OK) {
        if (cleanup_http_resources(pool, send_string, content_type, recv_http_string, recv_http_object) != RESULT_OK) {
            RETURN_ERR("Failed to cleanup HTTP resources after content extraction failure");
        }
        RETURN_ERR("Failed to extract content from LLM response");
    }

    printf("Content: \n%.*s\n", (int)recv_content_object->string->size, recv_content_object->string->data);

    // Phase 7: Execute agent actions based on LLM response
    if (lkjagent_agent_execute(pool, agent, recv_content_object->string) != RESULT_OK) {
        if (cleanup_http_resources(pool, send_string, content_type, recv_http_string, recv_http_object) != RESULT_OK) {
            RETURN_ERR("Failed to cleanup HTTP resources after agent execution failure");
        }
        RETURN_ERR("Failed to execute agent with received content");
    }

    // Phase 8: Clean up all allocated resources
    if (cleanup_http_resources(pool, send_string, content_type, recv_http_string, recv_http_object) != RESULT_OK) {
        RETURN_ERR("Failed to cleanup HTTP resources");
    }

    return RESULT_OK;
}

// ============================================================================
// EXECUTE FUNCTIONS - Consolidated from execute.c
// ============================================================================

// Helper function to add thinking log entry to working memory with rotation
static __attribute__((warn_unused_result)) result_t add_thinking_log_entry(pool_t* pool, agent_t* agent, const string_t* thinking_log) {
    object_t* working_memory;
    object_t* config_thinking_log;
    object_t* max_entries_obj;
    string_t* key_string;
    char key_buffer[64];
    uint64_t max_entries = 10;  // default

    // Get working memory
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory from agent");
    }

    // Try to get max_entries from config (optional)
    if (object_provide_str(pool, &config_thinking_log, agent->data, "config.agent.thinking_log") == RESULT_OK) {
        if (object_provide_str(pool, &max_entries_obj, config_thinking_log, "max_entries") == RESULT_OK) {
            // Convert string to number (simple implementation)
            max_entries = 0;
            for (uint64_t i = 0; i < max_entries_obj->string->size; i++) {
                char c = max_entries_obj->string->data[i];
                if (c >= '0' && c <= '9') {
                    max_entries = max_entries * 10 + (c - '0');
                }
            }
            if (max_entries == 0)
                max_entries = 10;  // fallback
        }
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
                RETURN_ERR("Failed to add thinking log entry");
            }

            if (string_destroy(pool, key_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy key string");
            }

            printf("Added thinking log [%s]: %.*s\n", key_buffer, (int)thinking_log->size, thinking_log->data);
            return RESULT_OK;
        }

        if (string_destroy(pool, key_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy key string in loop");
        }
    }

    // All slots full, rotate by overwriting the first one
    snprintf(key_buffer, sizeof(key_buffer), "thinking_log_00");
    if (string_create_str(pool, &key_string, key_buffer) != RESULT_OK) {
        RETURN_ERR("Failed to create thinking log rotation key string");
    }

    if (object_set_string(pool, working_memory, key_string, thinking_log) != RESULT_OK) {
        if (string_destroy(pool, key_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy rotation key string after set failure");
        }
        RETURN_ERR("Failed to rotate thinking log entry");
    }

    if (string_destroy(pool, key_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy rotation key string");
    }

    printf("Rotated thinking log [%s]: %.*s\n", key_buffer, (int)thinking_log->size, thinking_log->data);
    return RESULT_OK;
}

// Helper function to parse LLM response XML
static __attribute__((warn_unused_result)) result_t parse_llm_response(pool_t* pool, const string_t* recv, object_t** response_obj) {
    if (object_parse_xml(pool, response_obj, recv) != RESULT_OK) {
        RETURN_ERR("Failed to parse LLM response as XML");
    }
    return RESULT_OK;
}

// Helper function to execute working_memory_add action
static __attribute__((warn_unused_result)) result_t execute_action_working_memory_add(pool_t* pool, agent_t* agent, object_t* action) {
    object_t* tags_obj;
    object_t* value_obj;
    object_t* working_memory;
    string_t* key_string;

    // Get working memory object
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory from agent");
    }

    // Extract tags and value from action
    if (object_provide_str(pool, &tags_obj, action, "tags") != RESULT_OK) {
        RETURN_ERR("Failed to get tags from working_memory_add action");
    }
    if (object_provide_str(pool, &value_obj, action, "value") != RESULT_OK) {
        RETURN_ERR("Failed to get value from working_memory_add action");
    }

    // Use tags as the key
    if (string_create_string(pool, &key_string, tags_obj->string) != RESULT_OK) {
        RETURN_ERR("Failed to create key string from tags");
    }

    // Replace spaces with underscores in the key to make it a valid identifier
    for (uint64_t i = 0; i < key_string->size; i++) {
        if (key_string->data[i] == ' ') {
            key_string->data[i] = '_';
        }
    }

    // Add the key-value pair to working memory
    if (object_set_string(pool, working_memory, key_string, value_obj->string) != RESULT_OK) {
        if (string_destroy(pool, key_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy key string after set failure");
        }
        RETURN_ERR("Failed to add value to working memory");
    }

    if (string_destroy(pool, key_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy key string");
    }

    printf("Added to working memory: %.*s = %.*s\n",
           (int)tags_obj->string->size, tags_obj->string->data,
           (int)value_obj->string->size, value_obj->string->data);

    return RESULT_OK;
}

// Helper function to execute working_memory_remove action
static __attribute__((warn_unused_result)) result_t execute_action_working_memory_remove(pool_t* pool, agent_t* agent, object_t* action) {
    object_t* tags_obj;
    object_t* working_memory;
    object_t* target_obj;
    string_t* key_string;

    // Get working memory object
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory from agent");
    }

    // Extract tags from action
    if (object_provide_str(pool, &tags_obj, action, "tags") != RESULT_OK) {
        RETURN_ERR("Failed to get tags from working_memory_remove action");
    }

    // Use tags as the key
    if (string_create_string(pool, &key_string, tags_obj->string) != RESULT_OK) {
        RETURN_ERR("Failed to create key string from tags");
    }

    // Replace spaces with underscores in the key
    for (uint64_t i = 0; i < key_string->size; i++) {
        if (key_string->data[i] == ' ') {
            key_string->data[i] = '_';
        }
    }

    // Check if the key exists before trying to remove it
    if (object_get(&target_obj, working_memory, key_string) == RESULT_OK && target_obj) {
        // For simplicity, we'll set the value to an empty object (effectively removing it)
        object_t* empty_obj;
        if (object_create(pool, &empty_obj) != RESULT_OK) {
            if (string_destroy(pool, key_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy key string after empty object creation failure");
            }
            RETURN_ERR("Failed to create empty object for removal");
        }

        if (object_set(pool, working_memory, key_string, empty_obj) != RESULT_OK) {
            if (string_destroy(pool, key_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy key string after set failure");
            }
            RETURN_ERR("Failed to remove value from working memory");
        }

        printf("Removed from working memory: %.*s\n",
               (int)tags_obj->string->size, tags_obj->string->data);
    } else {
        printf("Key not found in working memory: %.*s\n",
               (int)tags_obj->string->size, tags_obj->string->data);
    }

    if (string_destroy(pool, key_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy key string");
    }

    return RESULT_OK;
}

// Helper function to execute storage_load action
static __attribute__((warn_unused_result)) result_t execute_action_storage_load(pool_t* pool, agent_t* agent, object_t* action) {
    object_t* tags_obj;
    object_t* working_memory;
    object_t* storage;
    object_t* stored_value;
    string_t* key_string;

    // Get working memory and storage objects
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory from agent");
    }
    if (object_provide_str(pool, &storage, agent->data, "storage") != RESULT_OK) {
        RETURN_ERR("Failed to get storage from agent");
    }

    // Extract tags from action
    if (object_provide_str(pool, &tags_obj, action, "tags") != RESULT_OK) {
        RETURN_ERR("Failed to get tags from storage_load action");
    }

    // Use tags as the key
    if (string_create_string(pool, &key_string, tags_obj->string) != RESULT_OK) {
        RETURN_ERR("Failed to create key string from tags");
    }

    // Replace spaces with underscores in the key
    for (uint64_t i = 0; i < key_string->size; i++) {
        if (key_string->data[i] == ' ') {
            key_string->data[i] = '_';
        }
    }

    // Try to find the value in storage
    if (object_get(&stored_value, storage, key_string) == RESULT_OK && stored_value && stored_value->string) {
        // Load the value from storage into working memory
        if (object_set_string(pool, working_memory, key_string, stored_value->string) != RESULT_OK) {
            if (string_destroy(pool, key_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy key string after set failure");
            }
            RETURN_ERR("Failed to load value from storage to working memory");
        }

        printf("Loaded from storage to working memory: %.*s = %.*s\n",
               (int)tags_obj->string->size, tags_obj->string->data,
               (int)stored_value->string->size, stored_value->string->data);
    } else {
        printf("Key not found in storage: %.*s\n",
               (int)tags_obj->string->size, tags_obj->string->data);
    }

    if (string_destroy(pool, key_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy key string");
    }

    return RESULT_OK;
}

// Helper function to execute storage_save action
static __attribute__((warn_unused_result)) result_t execute_action_storage_save(pool_t* pool, agent_t* agent, object_t* action) {
    object_t* tags_obj;
    object_t* value_obj;
    object_t* storage;
    string_t* key_string;

    // Get storage object
    if (object_provide_str(pool, &storage, agent->data, "storage") != RESULT_OK) {
        RETURN_ERR("Failed to get storage from agent");
    }

    // Extract tags and value from action
    if (object_provide_str(pool, &tags_obj, action, "tags") != RESULT_OK) {
        RETURN_ERR("Failed to get tags from storage_save action");
    }
    if (object_provide_str(pool, &value_obj, action, "value") != RESULT_OK) {
        RETURN_ERR("Failed to get value from storage_save action");
    }

    // Use tags as the key
    if (string_create_string(pool, &key_string, tags_obj->string) != RESULT_OK) {
        RETURN_ERR("Failed to create key string from tags");
    }

    // Replace spaces with underscores in the key
    for (uint64_t i = 0; i < key_string->size; i++) {
        if (key_string->data[i] == ' ') {
            key_string->data[i] = '_';
        }
    }

    // Save the key-value pair to storage
    if (object_set_string(pool, storage, key_string, value_obj->string) != RESULT_OK) {
        if (string_destroy(pool, key_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy key string after set failure");
        }
        RETURN_ERR("Failed to save value to storage");
    }

    if (string_destroy(pool, key_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy key string");
    }

    printf("Saved to storage: %.*s = %.*s\n",
           (int)tags_obj->string->size, tags_obj->string->data,
           (int)value_obj->string->size, value_obj->string->data);

    return RESULT_OK;
}

// Helper function to update agent state and handle thinking logs
static __attribute__((warn_unused_result)) result_t update_agent_state_and_thinking_log(pool_t* pool, agent_t* agent, object_t* response_obj) {
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

// Helper function to save agent memory to file
static __attribute__((warn_unused_result)) result_t save_agent_memory(pool_t* pool, agent_t* agent) {
    string_t* json_output;

    // Convert agent data to JSON string
    if (object_tostring_json(pool, &json_output, agent->data) != RESULT_OK) {
        RETURN_ERR("Failed to convert agent data to JSON");
    }

    // Write to memory file
    if (file_write(MEMORY_PATH, json_output) != RESULT_OK) {
        if (string_destroy(pool, json_output) != RESULT_OK) {
            RETURN_ERR("Failed to destroy JSON output after file write failure");
        }
        RETURN_ERR("Failed to write agent memory to file");
    }

    if (string_destroy(pool, json_output) != RESULT_OK) {
        RETURN_ERR("Failed to destroy JSON output string");
    }

    return RESULT_OK;
}

// Main execution function that processes LLM responses and executes agent actions
static __attribute__((warn_unused_result)) result_t lkjagent_agent_execute(pool_t* pool, agent_t* agent, const string_t* recv) {
    object_t* response_obj = NULL;
    object_t* agent_response = NULL;
    object_t* action_obj = NULL;
    object_t* action_type_obj = NULL;

    // Phase 1: Parse the LLM response
    if (parse_llm_response(pool, recv, &response_obj) != RESULT_OK) {
        RETURN_ERR("Failed to parse LLM response");
    }

    // Debug
    string_t* debug_string;
    if (string_create(pool, &debug_string) != RESULT_OK) {
        RETURN_ERR("Failed to create debug string for LLM response");
    }
    if(object_tostring_json(pool, &debug_string, response_obj) != RESULT_OK) {
        printf("Failed to convert response object to JSON string");
    }
    printf("test2:\n%.*s\n", (int)debug_string->size, debug_string->data);
    fflush(stdout);

    // Phase 2: Extract agent response
    if (object_provide_str(pool, &agent_response, response_obj, "agent") != RESULT_OK) {
        if (object_destroy(pool, response_obj) != RESULT_OK) {
            RETURN_ERR("Failed to destroy response object after agent extraction failure");
        }
        RETURN_ERR("Failed to get agent object from LLM response");
    }

    // Phase 3: Check if this is an action (executing state) or state transition
    if (object_provide_str(pool, &action_obj, agent_response, "action") == RESULT_OK) {
        // This is an action - extract action type
        if (object_provide_str(pool, &action_type_obj, action_obj, "type") != RESULT_OK) {
            if (object_destroy(pool, response_obj) != RESULT_OK) {
                RETURN_ERR("Failed to destroy response object after action type extraction failure");
            }
            RETURN_ERR("Failed to get action type from agent response");
        }

        // Execute the appropriate action based on type
        if (strncmp(action_type_obj->string->data, "working_memory_add", action_type_obj->string->size) == 0) {
            if (execute_action_working_memory_add(pool, agent, action_obj) != RESULT_OK) {
                if (object_destroy(pool, response_obj) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy response object after working_memory_add failure");
                }
                RETURN_ERR("Failed to execute working_memory_add action");
            }
        } else if (strncmp(action_type_obj->string->data, "working_memory_remove", action_type_obj->string->size) == 0) {
            if (execute_action_working_memory_remove(pool, agent, action_obj) != RESULT_OK) {
                if (object_destroy(pool, response_obj) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy response object after working_memory_remove failure");
                }
                RETURN_ERR("Failed to execute working_memory_remove action");
            }
        } else if (strncmp(action_type_obj->string->data, "storage_load", action_type_obj->string->size) == 0) {
            if (execute_action_storage_load(pool, agent, action_obj) != RESULT_OK) {
                if (object_destroy(pool, response_obj) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy response object after storage_load failure");
                }
                RETURN_ERR("Failed to execute storage_load action");
            }
        } else if (strncmp(action_type_obj->string->data, "storage_save", action_type_obj->string->size) == 0) {
            if (execute_action_storage_save(pool, agent, action_obj) != RESULT_OK) {
                if (object_destroy(pool, response_obj) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy response object after storage_save failure");
                }
                RETURN_ERR("Failed to execute storage_save action");
            }
        } else {
            printf("Unknown action type: %.*s\n",
                   (int)action_type_obj->string->size, action_type_obj->string->data);
        }
    } else {
        // This might be a state transition - handle it
        if (update_agent_state_and_thinking_log(pool, agent, response_obj) != RESULT_OK) {
            if (object_destroy(pool, response_obj) != RESULT_OK) {
                RETURN_ERR("Failed to destroy response object after state update failure");
            }
            RETURN_ERR("Failed to update agent state");
        }
    }

    // Phase 4: Save the updated agent memory to file
    if (save_agent_memory(pool, agent) != RESULT_OK) {
        if (object_destroy(pool, response_obj) != RESULT_OK) {
            RETURN_ERR("Failed to destroy response object after memory save failure");
        }
        RETURN_ERR("Failed to save agent memory");
    }

    // Phase 5: Clean up
    if (object_destroy(pool, response_obj) != RESULT_OK) {
        RETURN_ERR("Failed to destroy response object");
    }

    return RESULT_OK;
}