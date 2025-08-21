#include "lkjscript.h"

int main() {
    static pool_t pool;
    data_t* src = NULL;
    object_t* token = NULL;
    if(pool_init(&pool) != RESULT_OK) {
        RETURN_ERR("Failed to initialize memory pool");
        return 1;
    }
    if(file_read(&pool, SRC_PATH, &src) != RESULT_OK) {
        RETURN_ERR("Failed to read source file");
        return 1;
    }
    if(lkjscript_tokenize(&pool, token, src) != RESULT_OK) {
        RETURN_ERR("Failed to tokenize source code");
        return 1;
    }
    return 0;
}
