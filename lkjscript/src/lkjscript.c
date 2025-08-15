#include "lkjlib.h"

#define SOURCECODE_PATH "/data/index.txt"

static __attribute__((warn_unused_result)) result_t lkjscript_compile1(pool_t* pool, data_t* sourcecode, data_t** bytecode) {
    if (!pool || !sourcecode || !bytecode) {
        return RESULT_ERR;
    }
    if (data_create(pool, bytecode) != RESULT_OK) {
        RETURN_ERR("Failed to create bytecode");
    }
    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjscript_run1(pool_t* pool, data_t* bytecode) {
    if (!pool || !bytecode) {
        return RESULT_ERR;
    }
    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjscript_run(pool_t* pool, const char* path) {
    data_t* sourcecode = NULL;
    data_t* bytecode = NULL;
    if (file_read(pool, &sourcecode, path) != RESULT_OK) {
        RETURN_ERR("Failed to read source file");
    }
    if (lkjscript_compile1(pool, sourcecode, &bytecode) != RESULT_OK) {
        if (data_destroy(pool, sourcecode) != RESULT_OK) {
            PRINT_ERR("Failed to destroy source code data");
        }
        RETURN_ERR("Failed to compile source code");
    }
    if (lkjscript_run1(pool, bytecode) != RESULT_OK) {
        if (data_destroy(pool, sourcecode) != RESULT_OK) {
            PRINT_ERR("Failed to destroy source code data");
        }
        if (data_destroy(pool, bytecode) != RESULT_OK) {
            PRINT_ERR("Failed to destroy bytecode data");
        }
        RETURN_ERR("Failed to run bytecode");
    }
    if (data_destroy(pool, sourcecode) != RESULT_OK) {
        PRINT_ERR("Failed to destroy source code data");
    }
    if (data_destroy(pool, bytecode) != RESULT_OK) {
        PRINT_ERR("Failed to destroy bytecode data");
    }
    return RESULT_OK;
}

int main() {
    static pool_t pool;
    if (pool_init(&pool) != RESULT_OK) {
        RETURN_ERR("Failed to initialize memory pool");
    }
    if (lkjscript_run(&pool, SOURCECODE_PATH) != RESULT_OK) {
        RETURN_ERR("Failed to run lkjcript");
    }
    return 0;
}
