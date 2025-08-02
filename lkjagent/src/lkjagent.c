#include "lkjagent.h"

static __attribute__((warn_unused_result)) result_t lkjagent_init(lkjagent_t* lkjagent) {
    if (pool_init(&lkjagent->pool) != RESULT_OK) {
        RETURN_ERR("Failed to initialize memory pool");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_run(lkjagent_t* lkjagent) {
    string_t* config_string;
    string_t* prompt_string;
    string_t* prompt_path;
    json_value_t* config_json;
    json_value_t* prompt_json;


    if (string_create(&lkjagent->pool, &config_string) != RESULT_OK) {
        RETURN_ERR("Failed to create string");
    }

    if (string_create(&lkjagent->pool, &prompt_string) != RESULT_OK) {
        RETURN_ERR("Failed to create prompt string");
    }

    if (string_create_str(&lkjagent->pool, &prompt_path, "agent") != RESULT_OK) {
        RETURN_ERR("Failed to create agent key string");
    }

    if (file_read(&lkjagent->pool, "/data/config.json", &config_string) != RESULT_OK) {
        RETURN_ERR("Failed to read configuration file");
    }

    if (json_parse(&lkjagent->pool, &config_json, config_string) != RESULT_OK) {
        RETURN_ERR("Failed to parse configuration JSON");
    }

    if (json_object_get(&prompt_json, config_json, prompt_path) != RESULT_OK) {
        RETURN_ERR("Failed to get prompt from configuration JSON");
    }

    if (json_to_string_xml(&lkjagent->pool, &prompt_string, prompt_json) != RESULT_OK) {
        RETURN_ERR("Failed to convert prompt JSON to string");
    }

    printf("Prompt: %s\n", prompt_string->data);

    if (string_destroy(&lkjagent->pool, config_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy string");
    }

    if (string_destroy(&lkjagent->pool, prompt_path) != RESULT_OK) {
        RETURN_ERR("Failed to destroy agent key string");
    }

    if (json_destroy(&lkjagent->pool, config_json) != RESULT_OK) {
        RETURN_ERR("Failed to destroy JSON value");
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
