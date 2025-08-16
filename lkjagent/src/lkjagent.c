#include "lkjlib/lkjlib.h"

#define CONFIG_PATH "/data/config.json"
#define MEMORY_PATH "/data/memory.json"

typedef struct lkjagent_t {
    object_t* config;
    object_t* memory;
} lkjagent_t;

static __attribute__((warn_unused_result)) result_t lkjagent_init(pool_t* pool, lkjagent_t* lkjagent) {
    data_t* config_tmp = NULL;
    data_t* memory_tmp = NULL;
    lkjagent->config = NULL;
    lkjagent->memory = NULL;
    if (file_read(pool, CONFIG_PATH, &config_tmp) != RESULT_OK) {
        RETURN_ERR("Failed to read configuration file");
    }
    if (object_parse_json(pool, &lkjagent->config, config_tmp) != RESULT_OK) {
        if (data_destroy(pool, config_tmp) != RESULT_OK) {
            RETURN_ERR("Failed to destroy temporary data");
        }
        RETURN_ERR("Failed to parse configuration JSON");
    }
    if (data_destroy(pool, config_tmp) != RESULT_OK) {
        RETURN_ERR("Failed to destroy temporary data");
    }
    if (file_read(pool, MEMORY_PATH, &memory_tmp) != RESULT_OK) {
        RETURN_ERR("Failed to read memory file");
    }
    if (object_parse_json(pool, &lkjagent->memory, memory_tmp) != RESULT_OK) {
        if (data_destroy(pool, memory_tmp) != RESULT_OK) {
            RETURN_ERR("Failed to destroy temporary data");
        }
        RETURN_ERR("Failed to parse memory JSON");
    }
    if (data_destroy(pool, memory_tmp) != RESULT_OK) {
        RETURN_ERR("Failed to destroy temporary data");
    }
    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_run(pool_t* pool, lkjagent_t* lkjagent) {
    data_t* tmp = NULL;
    if (object_todata_json(pool, &tmp, lkjagent->config) != RESULT_OK) {
        RETURN_ERR("Failed to convert config object to JSON");
    }
    printf("%.*s\n", (int)tmp->size, tmp->data);
    if (data_destroy(pool, tmp) != RESULT_OK) {
        RETURN_ERR("Failed to destroy temporary data");
    }
    return RESULT_OK;
}

int main() {
    static pool_t pool;
    static lkjagent_t lkjagent;
    if (pool_init(&pool) != RESULT_OK) {
        RETURN_ERR("Failed to initialize memory pool");
    }
    if (lkjagent_init(&pool, &lkjagent) != RESULT_OK) {
        RETURN_ERR("Failed to initialize lkjagent");
    }
    if (lkjagent_run(&pool, &lkjagent) != RESULT_OK) {
        RETURN_ERR("Failed to run lkjagent");
    }
    return 0;
}