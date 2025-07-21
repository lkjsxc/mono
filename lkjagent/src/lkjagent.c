#include "lkjagent.h"

result_t lkjagent_init(lkjagent_t* lkjagent) {
    if (pool_init(&lkjagent->pool) != RESULT_OK) {
        RETURN_ERR("Failed to initialize pools");
    }
    return RESULT_OK;
}

result_t lkjagent_loadconfig(lkjagent_t* lkjagent) {
    string_t* config_string;
    if (pool_string4096_alloc(&lkjagent->pool, &config_string) != RESULT_OK) {
        RETURN_ERR("Failed to allocate string for config");
    }

    if (file_read("data/config.json", config_string) != RESULT_OK) {
        if (pool_string4096_free(&lkjagent->pool, config_string) != RESULT_OK) {
            RETURN_ERR("Failed to free config string");
        }
        RETURN_ERR("Failed to read config file");
    }

    json_value_t* root;
    if (json_parse(&lkjagent->pool, config_string, &root) != RESULT_OK) {
        if (pool_string4096_free(&lkjagent->pool, config_string) != RESULT_OK) {
            RETURN_ERR("Failed to free config string");
        }
        RETURN_ERR("Failed to parse config JSON");
    }

    // Example: Extract version
    json_value_t* version = json_object_get(root, "version");
    if (version && version->type == JSON_TYPE_STRING) {
        printf("Config version: %s\n", version->u.string_value->data);
    }

    // Example: Extract LM Studio endpoint
    json_value_t* lmstudio = json_object_get(root, "lmstudio");
    if (lmstudio && lmstudio->type == JSON_TYPE_OBJECT) {
        json_value_t* endpoint = json_object_get(lmstudio, "endpoint");
        if (endpoint && endpoint->type == JSON_TYPE_STRING) {
            printf("LM Studio endpoint: %s\n", endpoint->u.string_value->data);
        }
    }

    // Example: Extract agent settings
    json_value_t* agent = json_object_get(root, "agent");
    if (agent && agent->type == JSON_TYPE_OBJECT) {
        json_value_t* soft_limit = json_object_get(agent, "soft_limit");
        json_value_t* hard_limit = json_object_get(agent, "hard_limit");

        if (soft_limit && soft_limit->type == JSON_TYPE_NUMBER) {
            printf("Agent soft limit: %.0f\n", soft_limit->u.number_value);
        }

        if (hard_limit && hard_limit->type == JSON_TYPE_NUMBER) {
            printf("Agent hard limit: %.0f\n", hard_limit->u.number_value);
        }
    }

    if (pool_string4096_free(&lkjagent->pool, config_string) != RESULT_OK) {
        RETURN_ERR("Failed to free config string");
    }
    return RESULT_OK;
}

result_t lkjagent_run(lkjagent_t* lkjagent) {
    if (lkjagent_loadconfig(lkjagent) != RESULT_OK) {
        RETURN_ERR("Failed to load configuration");
    }

    return RESULT_OK;
}

int main() {
    lkjagent_t* lkjagent = malloc(sizeof(lkjagent_t));
    if (!lkjagent) {
        RETURN_ERR("Failed to allocate memory for lkjagent");
    }

    if (lkjagent_init(lkjagent) != RESULT_OK) {
        free(lkjagent);
        RETURN_ERR("Failed to initialize lkjagent");
    }

    if (lkjagent_run(lkjagent) != RESULT_OK) {
        free(lkjagent);
        RETURN_ERR("Failed to run lkjagent");
    }

    free(lkjagent);
    return 0;
}