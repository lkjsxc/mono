#include "lkjagent.h"

static __attribute__((warn_unused_result)) result_t lkjagent_config_init(pool_t* pool, config_t* config) {
    string_t* config_string;
    if(string_create(pool, &config_string) != RESULT_OK) {
        RETURN_ERR("Failed to create config string");
    }

    if(file_read(pool, CONFIG_PATH, &config_string) != RESULT_OK) {
        if(string_destroy(pool, config_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy config string");
        }
        RETURN_ERR("Failed to read config file");
    }

    if(object_parse_json(pool, &config->data, config_string) != RESULT_OK) {
        if(string_destroy(pool, config_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy config string");
        }
        RETURN_ERR("Failed to parse config JSON");
    }

    if(string_destroy(pool, config_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy config string");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_agent_init(pool_t* pool, agent_t* agent) {
    string_t* agent_string;
    if(string_create(pool, &agent_string) != RESULT_OK) {
        RETURN_ERR("Failed to create agent string");
    }

    if(file_read(pool, MEMORY_PATH, &agent_string) != RESULT_OK) {
        if(string_destroy(pool, agent_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy agent string");
        }
        RETURN_ERR("Failed to read agent file");
    }

    if(object_parse_json(pool, &agent->data, agent_string) != RESULT_OK) {
        if(string_destroy(pool, agent_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy agent string");
        }
        RETURN_ERR("Failed to parse agent JSON");
    }

    if(string_destroy(pool, agent_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy agent string");
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
    object_t* tmp_object;
    string_t* tmp_string;

    if(object_provide_str(pool, &agent_workingmemory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory from agent");
    }
    if(object_provide_str(pool, &agent_state, agent->data, "state") != RESULT_OK) {
        RETURN_ERR("Failed to get state from agent");
    }
    if(object_provide_str(pool, &config_agent_state, config->data, "agent.state") != RESULT_OK) {
        RETURN_ERR("Failed to get agent config from configuration");
    }
    if(object_provide_str(pool, &config_agent_state_base, config_agent_state, "base") != RESULT_OK) {
        RETURN_ERR("Failed to get base config from agent configuration");
    }
    if(object_provide_str(pool, &config_agent_state_base_prompt, config_agent_state_base, "prompt") != RESULT_OK) {
        RETURN_ERR("Failed to get base prompt from agent configuration");
    }
    if(object_provide_string(&config_agent_state_main, config_agent_state, agent_state->string) != RESULT_OK) {
        RETURN_ERR("Failed to get state config from agent configuration");
    }
    if(object_provide_str(pool, &config_agent_state_main_prompt, config_agent_state_main, "prompt") != RESULT_OK) {
        RETURN_ERR("Failed to get state prompt from agent configuration");
    }
    if(string_create(pool, &tmp_string) != RESULT_OK) {
        RETURN_ERR("Failed to create buffer string");
    }

    if(string_copy_str(pool, dst, "{\"messages\":[{\"role\":\"user\",\"content\":\"") != RESULT_OK) {
        RETURN_ERR("Failed to copy initial prompt string");
    }

    if(object_tostring_xml(pool, &tmp_string, config_agent_state_base_prompt) != RESULT_OK) {
        RETURN_ERR("Failed to parse working memory JSON");
    }
    if(string_escape(pool, &tmp_string) != RESULT_OK) {
        RETURN_ERR("Failed to escape base prompt string");
    }
    if(string_append_string(pool, dst, tmp_string) != RESULT_OK) {
        RETURN_ERR("Failed to append base prompt string");
    }

    if(object_tostring_xml(pool, &tmp_string, config_agent_state_main_prompt) != RESULT_OK) {
        RETURN_ERR("Failed to parse state prompt JSON");
    }
    if(string_escape(pool, &tmp_string) != RESULT_OK) {
        RETURN_ERR("Failed to escape base prompt string");
    }
    if(string_append_string(pool, dst, tmp_string) != RESULT_OK) {
        RETURN_ERR("Failed to append state prompt string");
    }

    if(object_tostring_xml(pool, &tmp_string, agent_workingmemory) != RESULT_OK) {
        RETURN_ERR("Failed to parse working memory JSON");
    }
    if(string_escape(pool, &tmp_string) != RESULT_OK) {
        RETURN_ERR("Failed to escape base prompt string");
    }
    if(string_append_str(pool, dst, "<working_memory>") != RESULT_OK) {
        RETURN_ERR("Failed to append working memory opening tag");
    }
    if(string_append_string(pool, dst, tmp_string) != RESULT_OK) {
        RETURN_ERR("Failed to append working memory string");
    }
    if(string_append_str(pool, dst, "</working_memory>") != RESULT_OK) {
        RETURN_ERR("Failed to append working memory closing tag");
    }

    if(string_append_str(pool, dst, "\"}], \"model\":\"") != RESULT_OK) {
        RETURN_ERR("Failed to append closing JSON string");
    }
    if(object_provide_str(pool, &tmp_object, config->data, "llm.model") != RESULT_OK) {
        RETURN_ERR("Failed to get model from configuration");
    }
    if(string_append_string(pool, dst, tmp_object->string) != RESULT_OK) {
        RETURN_ERR("Failed to append model string");
    }
    if(string_append_str(pool, dst, "\", \"temperature\":") != RESULT_OK) {
        RETURN_ERR("Failed to append temperature string");
    }
    if(object_provide_str(pool, &tmp_object, config->data, "llm.temperature") != RESULT_OK) {
        RETURN_ERR("Failed to get temperature from configuration");
    }
    if(string_append_string(pool, dst, tmp_object->string) != RESULT_OK) {
        RETURN_ERR("Failed to append temperature string");
    }
    if(string_append_str(pool, dst, ", \"max_tokens\":-1,\"stream\":false}") != RESULT_OK) {
        RETURN_ERR("Failed to append closing JSON string");
    }

    if(string_destroy(pool, tmp_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy buffer string");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_step(pool_t* pool, config_t* config, agent_t* agent) {
    
    string_t* send_string;
    string_t* content_type;
    // object_t* send_object;
    object_t* url_object;

    string_t* recv_http_string;
    string_t* recv_content_string;
    object_t* recv_http_object;
    object_t* recv_content_object;

    if(string_create(pool, &send_string) != RESULT_OK) {
        RETURN_ERR("Failed to create send string");
    }
    if(lkjagent_agent_makeprompt(pool, config, agent, &send_string) != RESULT_OK) {
        RETURN_ERR("Failed to create prompt for agent");
    }
    printf("Send: \n%.*s\n", (int)send_string->size, send_string->data);
    if(string_create_str(pool, &content_type, "application/json") != RESULT_OK) {
        RETURN_ERR("Failed to create content type string");
    }
    if(object_provide_str(pool, &url_object, config->data, "llm.endpoint") != RESULT_OK) {
        RETURN_ERR("Failed to get URL from config");
    }
    if(string_create(pool, &recv_http_string) != RESULT_OK) {
        RETURN_ERR("Failed to create response string");
    }
    if(string_create(pool, &recv_content_string) != RESULT_OK) {
        RETURN_ERR("Failed to create content string");
    }

    if(agent == NULL) {
        RETURN_ERR("Agent is not initialized");
    }

    if(http_post(pool, url_object->string, content_type, send_string, &recv_http_string) != RESULT_OK) {
        RETURN_ERR("Failed to send HTTP POST request");
    }

    if(object_parse_json(pool, &recv_http_object, recv_http_string) != RESULT_OK) {
        RETURN_ERR("Failed to parse HTTP response JSON");
    }

    if(object_provide_str(pool, &recv_content_object, recv_http_object, "choices.[0].message.content") != RESULT_OK) {
        RETURN_ERR("Failed to get content from HTTP response");
    }

    printf("Content: \n%.*s\n", (int)recv_content_object->string->size, recv_content_object->string->data);

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_init(lkjagent_t* lkjagent) {
    if (pool_init(&lkjagent->pool) != RESULT_OK) {
        RETURN_ERR("Failed to initialize memory pool");
    }

    if (lkjagent_config_init(&lkjagent->pool, &lkjagent->config) != RESULT_OK) {
        RETURN_ERR("Failed to initialize configuration");
    }

    if (lkjagent_agent_init(&lkjagent->pool, &lkjagent->agent) != RESULT_OK) {
        RETURN_ERR("Failed to initialize agent");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_run(lkjagent_t* lkjagent) {

    if(lkjagent_step(&lkjagent->pool, &lkjagent->config, &lkjagent->agent) != RESULT_OK) {
        RETURN_ERR("Failed to run lkjagent step");
    }

    return RESULT_OK;
}

int main() {
    lkjagent_t* lkjagent = malloc(sizeof(lkjagent_t));

    if (!lkjagent) {
        RETURN_ERR("Failed to allocate memory for lkjagent");
    }

    if (lkjagent_init(lkjagent) != RESULT_OK) {
        RETURN_ERR("Failed to initialize lkjagent");
    }

    if (lkjagent_run(lkjagent) != RESULT_OK) {
        RETURN_ERR("Failed to run lkjagent");
    }

    return RESULT_OK;
}
