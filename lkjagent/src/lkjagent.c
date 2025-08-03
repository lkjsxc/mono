#include "lkjagent.h"
#include <stdbool.h>

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

static __attribute__((warn_unused_result)) result_t lkjagent_step(pool_t* pool, config_t* config, agent_t* agent) {
    
    string_t* send_string;
    string_t* content_type;
    // object_t* send_object;
    object_t* url_object;

    string_t* recv_http_string;
    string_t* recv_content_string;
    object_t* recv_http_object;
    object_t* recv_content_object;

    if(string_create_str(pool, &send_string, "{\"model\":\"deepseek/deepseek-r1-0528-qwen3-8b\",\"messages\":[{\"role\":\"user\",\"content\":\"Hello! Please respond with a simple greeting.\"}],\"temperature\":0.70,\"max_tokens\":-1,\"stream\":false}") != RESULT_OK) {
        RETURN_ERR("Failed to create send string");
    }
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

    printf("HTTP Response: %.*s\n", (int)recv_http_string->size, recv_http_string->data);

    if(object_parse_json(pool, &recv_http_object, recv_http_string) != RESULT_OK) {
        RETURN_ERR("Failed to parse HTTP response JSON");
    }

    if(object_provide_str(pool, &recv_content_object, recv_http_object, "choices.[0].message.content") != RESULT_OK) {
        RETURN_ERR("Failed to get content from HTTP response");
    }

    printf("Response: \n%.*s\n", (int)recv_content_object->string->size, recv_content_object->string->data);

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_init(lkjagent_t* lkjagent) {
    if (pool_init(&lkjagent->pool) != RESULT_OK) {
        RETURN_ERR("Failed to initialize memory pool");
    }

    if (lkjagent_config_init(&lkjagent->pool, &lkjagent->config) != RESULT_OK) {
        RETURN_ERR("Failed to initialize configuration");
    }

    // if (lkjagent_agent_init(&lkjagent->pool, &lkjagent->agent) != RESULT_OK) {
    //     RETURN_ERR("Failed to initialize agent");
    // }

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
