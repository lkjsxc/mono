#include "lkjagent.h"

static __attribute__((warn_unused_result)) result_t lkjagent_init(lkjagent_t* lkjagent) {
    if (pool_init(&lkjagent->pool) != RESULT_OK) {
        RETURN_ERR("Failed to initialize memory pool");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_run(lkjagent_t* lkjagent) {
    string_t* string;

    if (string_create(&lkjagent->pool, &string) != RESULT_OK) {
        RETURN_ERR("Failed to create string");
    }

    if (file_read(&lkjagent->pool, "/data/config.json", &string) != RESULT_OK) {
        RETURN_ERR("Failed to read configuration file");
    }

    printf("Configuration: %.*s\n", (int)string->size, string->data);

    if (string_destroy(&lkjagent->pool, string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy string");
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
