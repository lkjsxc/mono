#include "lkjagent.h"

typedef struct {
    pool_t* pool;
    lkjagent_t* agent;
    data_t* config_raw;
    data_t* memory_raw;
} runtime_ctx_t;

static __attribute__((warn_unused_result)) result_t load_config(runtime_ctx_t* ctx) {
    if (file_read(ctx->pool, CONFIG_PATH, &ctx->config_raw) != RESULT_OK) {
        RETURN_ERR("Failed to read configuration file");
    }

    if (object_parse_json(ctx->pool, &ctx->agent->config, ctx->config_raw) != RESULT_OK) {
        RETURN_ERR("Failed to parse configuration JSON");
    }

    if (data_destroy(ctx->pool, ctx->config_raw) != RESULT_OK) {
        RETURN_ERR("Failed to destroy temporary config data");
    }

    ctx->config_raw = NULL;
    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t ensure_memory_default(runtime_ctx_t* ctx) {
    static const char default_memory[] =
        "{\"state\":\"analyzing\",\"working_memory\":{},\"storage\":{}}";

    if (file_read(ctx->pool, MEMORY_PATH, &ctx->memory_raw) == RESULT_OK) {
        return RESULT_OK;
    }

    if (data_create_str(ctx->pool, &ctx->memory_raw, default_memory) != RESULT_OK) {
        RETURN_ERR("Failed to create default memory");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t load_memory(runtime_ctx_t* ctx) {
    if (ensure_memory_default(ctx) != RESULT_OK) {
        RETURN_ERR("Failed to obtain memory buffer");
    }

    if (object_parse_json(ctx->pool, &ctx->agent->memory, ctx->memory_raw) != RESULT_OK) {
        RETURN_ERR("Failed to parse memory JSON");
    }

    if (data_destroy(ctx->pool, ctx->memory_raw) != RESULT_OK) {
        RETURN_ERR("Failed to destroy temporary memory data");
    }

    ctx->memory_raw = NULL;
    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_init(runtime_ctx_t* ctx) {
    ctx->agent->config = NULL;
    ctx->agent->memory = NULL;
    ctx->config_raw = NULL;
    ctx->memory_raw = NULL;

    if (load_config(ctx) != RESULT_OK) {
        RETURN_ERR("Agent initialization failed during config load");
    }

    if (load_memory(ctx) != RESULT_OK) {
        RETURN_ERR("Agent initialization failed during memory load");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_save(pool_t* pool, lkjagent_t* agent) {
    data_t* memory = NULL;

    if (object_todata_json(pool, &memory, agent->memory) != RESULT_OK) {
        RETURN_ERR("Failed to convert memory to JSON");
    }

    if (file_write(MEMORY_PATH, memory) != RESULT_OK) {
        if (data_destroy(pool, memory) != RESULT_OK) {
            PRINT_ERR("Failed to destroy memory data");
        }
        RETURN_ERR("Failed to write memory to file");
    }

    if (data_destroy(pool, memory) != RESULT_OK) {
        RETURN_ERR("Failed to destroy memory data");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_step(pool_t* pool, lkjagent_t* agent, uint64_t iteration) {
    data_t* response = NULL;

    if (lkjagent_request(pool, agent, &response) != RESULT_OK) {
        PRINT_ERR("Request to LLM endpoint failed");
        return RESULT_OK;
    }

    if (lkjagent_process(pool, agent, response, iteration) != RESULT_OK) {
        PRINT_ERR("Processing of LLM response failed");
        if (data_destroy(pool, response) != RESULT_OK) {
            PRINT_ERR("Failed to destroy response payload after processing error");
        }
        return RESULT_OK;
    }

    if (data_destroy(pool, response) != RESULT_OK) {
        RETURN_ERR("Failed to destroy response payload");
    }

    if (lkjagent_save(pool, agent) != RESULT_OK) {
        RETURN_ERR("Failed to persist agent state");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_run(pool_t* pool, lkjagent_t* agent) {
    for (uint64_t iteration = 0; iteration < UINT64_C(100000); iteration++) {
        if (lkjagent_step(pool, agent, iteration) != RESULT_OK) {
            sleep(5);
        }
    }

    return RESULT_OK;
}

int main(void) {
    pool_t pool;
    lkjagent_t agent;
    runtime_ctx_t ctx = {.pool = &pool, .agent = &agent};

    if (pool_init(&pool) != RESULT_OK) {
        fprintf(stderr, "Failed to initialize memory pool\n");
        return 1;
    }

    if (lkjagent_init(&ctx) != RESULT_OK) {
        fprintf(stderr, "Failed to initialize agent\n");
        return 1;
    }

    if (lkjagent_run(&pool, &agent) != RESULT_OK) {
        fprintf(stderr, "Agent execution failed\n");
        return 1;
    }

    return 0;
}