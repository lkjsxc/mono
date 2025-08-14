#include "lkjscript.h"

result_t lkjscript_run(pool_t* pool, const char* path) {
    data_t* sourcecode = NULL;

    if (file_read(pool, &sourcecode, path) != RESULT_OK) {
        RETURN_ERR("Failed to read source file");
    }

    printf("%.*s\n", (int)sourcecode->size, sourcecode->data);
    fflush(stdout);

    if (data_destroy(pool, sourcecode) != RESULT_OK) {
        RETURN_ERR("Failed to destroy source code data");
    }

    return RESULT_OK;
}

int main() {
    static pool_t pool;

    if (pool_init(&pool) != RESULT_OK) {
        RETURN_ERR("Failed to initialize memory pool");
    }

    if (lkjscript_run(&pool, SOURCECODE_PATH) != RESULT_OK) {
        RETURN_ERR("Failed to run LScript");
    }

    return 0;
}
