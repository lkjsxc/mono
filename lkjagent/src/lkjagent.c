#include "lkjagent.h"

result_t lkjagent_init(lkjagent_t* lkjagent) {
    if (pool_init(&lkjagent->pool) != RESULT_OK) {
        RETURN_ERR("Failed to initialize pools");
    }

    if (config_init(&lkjagent->pool, &lkjagent->config) != RESULT_OK) {
        RETURN_ERR("Failed to load configuration");
    }

    if(config_load(&lkjagent->pool, &lkjagent->config, CONFIG_PATH) != RESULT_OK) {
        RETURN_ERR("Failed to load configuration from file");
    }

    return RESULT_OK;
}

result_t lkjagent_run(lkjagent_t* lkjagent) {
    printf("version: %s\n", lkjagent->config.version->data);
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

    return 0;
}