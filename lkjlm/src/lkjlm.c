#include "lkjlib.h"

static result_t lkjlm_run(pool_t* pool) {
    data_t* file_data;

    if (data_create(pool, &file_data) != RESULT_OK) {
        RETURN_ERR("Failed to create data for file");
    }

    if (file_read(pool, &file_data, "/data/input.txt") != RESULT_OK) {
        RETURN_ERR("Failed to read input file");
    }

    printf("%.*s", (int)file_data->size, file_data->data);

    if (data_destroy(pool, file_data) != RESULT_OK) {
        RETURN_ERR("Failed to destroy file data");
    }

    return RESULT_OK;
}

int main() {
    static pool_t pool;
    if (pool_init(&pool) != RESULT_OK) {
        RETURN_ERR("Failed to initialize memory pool");
    }

    if (lkjlm_run(&pool) != RESULT_OK) {
        RETURN_ERR("Failed to run lkjlm");
    }

    return 0;
}
