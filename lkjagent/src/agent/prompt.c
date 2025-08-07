#include "agent/prompt.h"

// Helper function to extract and validate configuration objects
result_t agent_prompt_extract_config_objects(pool_t* pool, config_t* config, agent_t* agent, object_t** agent_workingmemory, object_t** agent_state, object_t** config_agent_state, object_t** config_agent_state_base, object_t** config_agent_state_base_prompt, object_t** config_agent_state_main, object_t** config_agent_state_main_prompt) {
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
result_t agent_prompt_build_header(pool_t* pool, string_t** dst) {
    if (string_copy_str(pool, dst, "{\"messages\":[{\"role\":\"user\",\"content\":\"") != RESULT_OK) {
        RETURN_ERR("Failed to copy initial prompt string");
    }
    return RESULT_OK;
}

// Helper function to append and escape the base prompt
result_t agent_prompt_append_base(pool_t* pool, string_t** dst, object_t* config_agent_state_base_prompt) {
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
result_t agent_prompt_append_state(pool_t* pool, string_t** dst, object_t* config_agent_state_main_prompt) {
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
result_t agent_prompt_append_memory(pool_t* pool, string_t** dst, object_t* agent_workingmemory) {
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
result_t agent_prompt_append_footer(pool_t* pool, string_t** dst, config_t* config) {
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

// Main prompt generation function that orchestrates the building process
result_t agent_prompt_generate(pool_t* pool, config_t* config, agent_t* agent, string_t** dst) {
    object_t* agent_workingmemory;
    object_t* agent_state;
    object_t* config_agent_state;
    object_t* config_agent_state_base;
    object_t* config_agent_state_base_prompt;
    object_t* config_agent_state_main;
    object_t* config_agent_state_main_prompt;

    // Extract all configuration objects first
    if (agent_prompt_extract_config_objects(pool, config, agent, &agent_workingmemory, &agent_state, &config_agent_state,
                               &config_agent_state_base, &config_agent_state_base_prompt,
                               &config_agent_state_main, &config_agent_state_main_prompt) != RESULT_OK) {
        RETURN_ERR("Failed to extract configuration objects");
    }

    // Build the JSON request incrementally
    if (agent_prompt_build_header(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to build JSON request header");
    }

    if (agent_prompt_append_base(pool, dst, config_agent_state_base_prompt) != RESULT_OK) {
        RETURN_ERR("Failed to append base prompt");
    }

    if (agent_prompt_append_state(pool, dst, config_agent_state_main_prompt) != RESULT_OK) {
        RETURN_ERR("Failed to append state prompt");
    }

    if (agent_prompt_append_memory(pool, dst, agent_workingmemory) != RESULT_OK) {
        RETURN_ERR("Failed to append working memory");
    }

    if (agent_prompt_append_footer(pool, dst, config) != RESULT_OK) {
        RETURN_ERR("Failed to append JSON request footer");
    }

    return RESULT_OK;
}
