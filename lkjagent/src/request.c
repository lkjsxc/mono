#include "lkjagent.h"

result_t extract_config_objects( object_t* config, object_t* agent, object_t** agent_workingmemory, object_t** agent_state, object_t** config_agent_state, object_t** config_agent_state_base, object_t** config_agent_state_base_prompt, object_t** config_agent_state_main, object_t** config_agent_state_main_prompt) {
    if (object_provide_str(agent_workingmemory, agent, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory from agent");
    }
    if (object_provide_str(agent_state, agent, "state") != RESULT_OK) {
        RETURN_ERR("Failed to get state from agent");
    }
    if (object_provide_str(config_agent_state, config, "agent.state") != RESULT_OK) {
        RETURN_ERR("Failed to get agent config from configuration");
    }
    if (object_provide_str(config_agent_state_base, *config_agent_state, "base") != RESULT_OK) {
        RETURN_ERR("Failed to get base config from agent configuration");
    }
    if (object_provide_str(config_agent_state_base_prompt, *config_agent_state_base, "prompt") != RESULT_OK) {
        RETURN_ERR("Failed to get base prompt from agent configuration");
    }
    if (object_provide_data(config_agent_state_main, *config_agent_state, (*agent_state)->data) != RESULT_OK) {
        RETURN_ERR("Failed to get state config from agent configuration");
    }
    if (object_provide_str(config_agent_state_main_prompt, *config_agent_state_main, "prompt") != RESULT_OK) {
        RETURN_ERR("Failed to get state prompt from agent configuration");
    }

    return RESULT_OK;
}

result_t build_header(pool_t* pool, data_t** dst) {
    if (data_create_str(pool, dst, "{\"messages\":[{\"role\":\"user\",\"content\":\"") != RESULT_OK) {
        RETURN_ERR("Failed to copy initial prompt data");
    }
    return RESULT_OK;
}

result_t append_base(pool_t* pool, data_t** dst, object_t* config_agent_state_base_prompt) {
    data_t* tmp_data = NULL;

    if (data_create(pool, &tmp_data) != RESULT_OK) {
        RETURN_ERR("Failed to create temporary data for base prompt");
    }

    if (object_todata_xml(pool, &tmp_data, config_agent_state_base_prompt) != RESULT_OK) {
        result_t tmp = data_destroy(pool, tmp_data);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy tmp_data after base XML conversion error\n");
        }
        RETURN_ERR("Failed to convert base prompt to XML data");
    }
    if (data_escape_json(pool, &tmp_data) != RESULT_OK) {
        result_t tmp = data_destroy(pool, tmp_data);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy tmp_data after base escape error\n");
        }
        RETURN_ERR("Failed to escape base prompt data");
    }
    if (data_append_data(pool, dst, tmp_data) != RESULT_OK) {
        result_t tmp = data_destroy(pool, tmp_data);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy tmp_data after base append error\n");
        }
        RETURN_ERR("Failed to append base prompt data");
    }

    if (data_destroy(pool, tmp_data) != RESULT_OK) {
        RETURN_ERR("Failed to destroy temporary data for base prompt");
    }

    return RESULT_OK;
}

result_t append_state(pool_t* pool, data_t** dst, object_t* config_agent_state_main_prompt) {
    data_t* tmp_data = NULL;

    if (data_create(pool, &tmp_data) != RESULT_OK) {
        RETURN_ERR("Failed to create temporary data for state prompt");
    }

    if (object_todata_xml(pool, &tmp_data, config_agent_state_main_prompt) != RESULT_OK) {
        result_t tmp = data_destroy(pool, tmp_data);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy tmp_data after state XML conversion error\n");
        }
        RETURN_ERR("Failed to convert state prompt to XML data");
    }
    if (data_escape_json(pool, &tmp_data) != RESULT_OK) {
        result_t tmp = data_destroy(pool, tmp_data);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy tmp_data after state escape error\n");
        }
        RETURN_ERR("Failed to escape state prompt data");
    }
    if (data_append_data(pool, dst, tmp_data) != RESULT_OK) {
        result_t tmp = data_destroy(pool, tmp_data);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy tmp_data after state append error\n");
        }
        RETURN_ERR("Failed to append state prompt data");
    }

    if (data_destroy(pool, tmp_data) != RESULT_OK) {
        RETURN_ERR("Failed to destroy temporary data for state prompt");
    }

    return RESULT_OK;
}

result_t append_memory(pool_t* pool, data_t** dst, object_t* agent_workingmemory) {
    data_t* tmp_data = NULL;

    if (data_create(pool, &tmp_data) != RESULT_OK) {
        RETURN_ERR("Failed to create temporary data for working memory");
    }

    if (object_todata_xml(pool, &tmp_data, agent_workingmemory) != RESULT_OK) {
        result_t tmp = data_destroy(pool, tmp_data);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy tmp_data after memory XML conversion error\n");
        }
        RETURN_ERR("Failed to convert working memory to XML data");
    }
    if (data_escape_json(pool, &tmp_data) != RESULT_OK) {
        result_t tmp = data_destroy(pool, tmp_data);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy tmp_data after memory escape error\n");
        }
        RETURN_ERR("Failed to escape working memory data");
    }

    if (data_append_str(pool, dst, "<working_memory>") != RESULT_OK) {
        result_t tmp = data_destroy(pool, tmp_data);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy tmp_data after memory opening tag append error\n");
        }
        RETURN_ERR("Failed to append working memory opening tag");
    }
    if (data_append_data(pool, dst, tmp_data) != RESULT_OK) {
        result_t tmp = data_destroy(pool, tmp_data);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy tmp_data after memory content append error\n");
        }
        RETURN_ERR("Failed to append working memory data");
    }
    if (data_append_str(pool, dst, "</working_memory>") != RESULT_OK) {
        result_t tmp = data_destroy(pool, tmp_data);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy tmp_data after memory closing tag append error\n");
        }
        RETURN_ERR("Failed to append working memory closing tag");
    }

    if (data_destroy(pool, tmp_data) != RESULT_OK) {
        RETURN_ERR("Failed to destroy temporary data for working memory");
    }

    return RESULT_OK;
}

result_t append_footer(pool_t* pool, data_t** dst, object_t* config) {
    object_t* tmp_object = NULL;

    if (data_append_str(pool, dst, "\"}], \"model\":\"") != RESULT_OK) {
        RETURN_ERR("Failed to append model field start");
    }

    if (object_provide_str(&tmp_object, config, "llm.model") != RESULT_OK) {
        RETURN_ERR("Failed to get model from configuration");
    }
    if (data_append_data(pool, dst, tmp_object->data) != RESULT_OK) {
        RETURN_ERR("Failed to append model data");
    }

    if (data_append_str(pool, dst, "\", \"temperature\":") != RESULT_OK) {
        RETURN_ERR("Failed to append temperature field start");
    }

    if (object_provide_str(&tmp_object, config, "llm.temperature") != RESULT_OK) {
        RETURN_ERR("Failed to get temperature from configuration");
    }
    if (data_append_data(pool, dst, tmp_object->data) != RESULT_OK) {
        RETURN_ERR("Failed to append temperature data");
    }

    if (data_append_str(pool, dst, ", \"max_tokens\":-1,\"stream\":false}") != RESULT_OK) {
        RETURN_ERR("Failed to append closing JSON data");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t makerequest(pool_t* pool, object_t* config, object_t* agent, data_t** dst) {
    object_t* agent_workingmemory = NULL;
    object_t* agent_state = NULL;
    object_t* config_agent_state = NULL;
    object_t* config_agent_state_base = NULL;
    object_t* config_agent_state_base_prompt = NULL;
    object_t* config_agent_state_main = NULL;
    object_t* config_agent_state_main_prompt = NULL;

    if (extract_config_objects(config, agent, &agent_workingmemory, &agent_state, &config_agent_state,
                               &config_agent_state_base, &config_agent_state_base_prompt,
                               &config_agent_state_main, &config_agent_state_main_prompt) != RESULT_OK) {
        RETURN_ERR("Failed to extract configuration objects");
    }

    if (build_header(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to build JSON request header");
    }

    if (append_base(pool, dst, config_agent_state_base_prompt) != RESULT_OK) {
        RETURN_ERR("Failed to append base prompt");
    }

    if (append_state(pool, dst, config_agent_state_main_prompt) != RESULT_OK) {
        RETURN_ERR("Failed to append state prompt");
    }

    if (append_memory(pool, dst, agent_workingmemory) != RESULT_OK) {
        RETURN_ERR("Failed to append working memory");
    }

    if (append_footer(pool, dst, config) != RESULT_OK) {
        RETURN_ERR("Failed to append JSON request footer");
    }

    return RESULT_OK;
}

result_t lkjagent_request(pool_t* pool, lkjagent_t* lkjagent, data_t** dst) {
    object_t* url = NULL;
    data_t* content_type = NULL;
    data_t* request = NULL;
    if(object_provide_str(&url, lkjagent->config, "llm.endpoint") != RESULT_OK) {
        RETURN_ERR("Failed to get URL from configuration");
    }
    if(data_create_str(pool, &content_type, "application/json") != RESULT_OK) {
        RETURN_ERR("Failed to create content type data");
    }
    if (makerequest(pool, lkjagent->config, lkjagent->memory, &request) != RESULT_OK) {
        if(data_destroy(pool, content_type) != RESULT_OK) {
            PRINT_ERR("Failed to destroy content type data");
        }
        RETURN_ERR("Failed to make request");
    }
    if(http_post(pool, url->data, content_type, request, dst) != RESULT_OK) {
        if(data_destroy(pool, content_type) != RESULT_OK) {
            PRINT_ERR("Failed to destroy content type data");
        }
        if(data_destroy(pool, request) != RESULT_OK) {
            PRINT_ERR("Failed to destroy request data");
        }
        RETURN_ERR("Failed to send HTTP POST request");
    }
    return RESULT_OK;
}
