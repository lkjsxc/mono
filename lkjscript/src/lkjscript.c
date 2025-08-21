#include "lkjlib/lkjlib.h"

#define SRC_PATH "/data/main.lkjscript"

int main() {
    static pool_t pool;
    data_t* src = NULL;
    if(pool_init(&pool) != RESULT_OK) {
        RETURN_ERR("Failed to initialize memory pool");
        return 1;
    }
    if(file_read(&pool, SRC_PATH, &src) != RESULT_OK) {
        RETURN_ERR("Failed to read source file");
        return 1;
    }
    printf("debug: %.*s\n", (int)src->size, src->data);
    fflush(stdout);
    return 0;
}