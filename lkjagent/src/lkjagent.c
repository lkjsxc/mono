#include "lkjagent.h"

static __attribute__((warn_unused_result)) result_t lkjagent_config_init(pool_t* pool, config_t* config) {
    string_t* config_string;
    if (string_create(pool, &config_string) != RESULT_OK) {
        RETURN_ERR("Failed to create config string");
    }

    if (file_read(pool, CONFIG_PATH, &config_string) != RESULT_OK) {
        if (string_destroy(pool, config_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy config string");
        }
        RETURN_ERR("Failed to read config file");
    }

    if (object_parse_json(pool, &config->data, config_string) != RESULT_OK) {
        if (string_destroy(pool, config_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy config string");
        }
        RETURN_ERR("Failed to parse config JSON");
    }

    if (string_destroy(pool, config_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy config string");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_agent_init(pool_t* pool, agent_t* agent) {
    string_t* agent_string;
    if (string_create(pool, &agent_string) != RESULT_OK) {
        RETURN_ERR("Failed to create agent string");
    }

    if (file_read(pool, MEMORY_PATH, &agent_string) != RESULT_OK) {
        if (string_destroy(pool, agent_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy agent string");
        }
        RETURN_ERR("Failed to read agent file");
    }

    if (object_parse_json(pool, &agent->data, agent_string) != RESULT_OK) {
        if (string_destroy(pool, agent_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy agent string");
        }
        RETURN_ERR("Failed to parse agent JSON");
    }

    if (string_destroy(pool, agent_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy agent string");
    }

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
    for (int i = 0; i < 5; i++) {
        if (lkjagent_agent(&lkjagent->pool, &lkjagent->config, &lkjagent->agent) != RESULT_OK) {
            RETURN_ERR("Failed to run lkjagent step");
        }
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_deinit(lkjagent_t* lkjagent) {
    if (object_destroy(&lkjagent->pool, lkjagent->agent.data) != RESULT_OK) {
        RETURN_ERR("Failed to destroy agent data");
    }
    if (object_destroy(&lkjagent->pool, lkjagent->config.data) != RESULT_OK) {
        RETURN_ERR("Failed to destroy config data");
    }
    printf("string16 freelist: %lu\n", lkjagent->pool.string16_freelist_count);
    printf("string256 freelist: %lu\n", lkjagent->pool.string256_freelist_count);
    printf("string4096 freelist: %lu\n", lkjagent->pool.string4096_freelist_count);
    printf("string65536 freelist: %lu\n", lkjagent->pool.string65536_freelist_count);
    printf("string1048576 freelist: %lu\n", lkjagent->pool.string1048576_freelist_count);
    printf("object freelist: %lu\n", lkjagent->pool.object_freelist_count);
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

    if (lkjagent_deinit(lkjagent) != RESULT_OK) {
        RETURN_ERR("Failed to deinitialize lkjagent");
    }

    return RESULT_OK;
}
