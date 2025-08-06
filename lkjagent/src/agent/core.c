#include "agent/core.h"

// Forward declarations for internal functions
static __attribute__((warn_unused_result)) result_t extract_config_objects(pool_t* pool, config_t* config, agent_t* agent, 
    object_t** agent_workingmemory, object_t** agent_state, object_t** config_agent_state, 
    object_t** config_agent_state_base, object_t** config_agent_state_base_prompt, 
    object_t** config_agent_state_main, object_t** config_agent_state_main_prompt);

static __attribute__((warn_unused_result)) result_t build_json_request_header(pool_t* pool, string_t** dst);

static __attribute__((warn_unused_result)) result_t append_base_prompt(pool_t* pool, string_t** dst, object_t* config_agent_state_base_prompt);

static __attribute__((warn_unused_result)) result_t append_state_prompt(pool_t* pool, string_t** dst, object_t* config_agent_state_main_prompt);

static __attribute__((warn_unused_result)) result_t append_working_memory(pool_t* pool, string_t** dst, object_t* agent_workingmemory);

static __attribute__((warn_unused_result)) result_t append_json_request_footer(pool_t* pool, string_t** dst, config_t* config);

static __attribute__((warn_unused_result)) result_t create_http_request_resources(pool_t* pool, string_t** send_string, string_t** content_type, string_t** recv_http_string);

static __attribute__((warn_unused_result)) result_t extract_llm_response_content(pool_t* pool, object_t* recv_http_object, object_t** recv_content_object);

static __attribute__((warn_unused_result)) result_t cleanup_http_resources(pool_t* pool, string_t* send_string, string_t* content_type, string_t* recv_http_string, object_t* recv_http_object);

// Helper function to extract and validate configuration objects
static __attribute__((warn_unused_result)) result_t extract_config_objects(pool_t* pool, config_t* config, agent_t* agent, 
    object_t** agent_workingmemory, object_t** agent_state, object_t** config_agent_state, 
    object_t** config_agent_state_base, object_t** config_agent_state_base_prompt, 
    object_t** config_agent_state_main, object_t** config_agent_state_main_prompt) {
    
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

// static __attribute__((warn_unused_result)) result_t lkjagent_agent_execute(pool_t* pool, agent_t* agent, const string_t* recv) {
//     return RESULT_OK;
// }

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

    // if(lkjagent_agent_execute(pool, agent, recv_content_object->string) != RESULT_OK) {
    //     if (cleanup_http_resources(pool, send_string, content_type, recv_http_string, recv_http_object) != RESULT_OK) {
    //         RETURN_ERR("Failed to cleanup HTTP resources after agent execution failure");
    //     }
    //     RETURN_ERR("Failed to execute agent with received content");
    // }

    if (cleanup_http_resources(pool, send_string, content_type, recv_http_string, recv_http_object) != RESULT_OK) {
        RETURN_ERR("Failed to cleanup HTTP resources");
    }

    return RESULT_OK;
}