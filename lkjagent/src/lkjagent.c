#include "lkjagent.h"

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
            PRINT_ERR("Failed to destroy temporary data");
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
            PRINT_ERR("Failed to destroy temporary data");
        }
        RETURN_ERR("Failed to parse memory JSON");
    }
    if (data_destroy(pool, memory_tmp) != RESULT_OK) {
        RETURN_ERR("Failed to destroy temporary data");
    }
    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_step(pool_t* pool, lkjagent_t* lkjagent, uint64_t iteration) {
    data_t* recv = NULL;
    printf("debug: lkjagent_step called with iteration %lu\n", iteration);
    if (lkjagent_request(pool, lkjagent, &recv) != RESULT_OK) {
        RETURN_ERR("Failed to make request");
    }
    printf("Debug\n%.*s\n", (int)recv->size, recv->data);
    if (data_destroy(pool, recv) != RESULT_OK) {
        RETURN_ERR("Failed to destroy received data");
    }
    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_run(pool_t* pool, lkjagent_t* lkjagent) {
    object_t* iteration_limit_enable = NULL;
    object_t* iteration_limit_value = NULL;
    int64_t iteration_limit;
    if (object_provide_str(&iteration_limit_enable, lkjagent->config, "agent.iteration_limit.enable") != RESULT_OK) {
        RETURN_ERR("Failed to provide iteration_limit_enable");
    }
    if (object_provide_str(&iteration_limit_value, lkjagent->config, "agent.iteration_limit.value") != RESULT_OK) {
        RETURN_ERR("Failed to provide iteration_limit_value");
    }
    if (data_equal_str(iteration_limit_enable->data, "true")) {
        iteration_limit = UINT64_MAX;
    } else if (data_equal_str(iteration_limit_enable->data, "false")) {
        if (data_toint(iteration_limit_value->data, &iteration_limit) != RESULT_OK) {
            RETURN_ERR("Failed to convert iteration_limit_value to int");
        }
    } else {
        RETURN_ERR("Invalid value for agent.iteration_limit.enable");
    }
    for (int64_t i = 0; i < iteration_limit; i++) {
        if (lkjagent_step(pool, lkjagent, i) != RESULT_OK) {
            RETURN_ERR("Failed to execute lkjagent step");
        }
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
