#include "lkjlib/lkjlib.h"

int main() {

    static pool_t pool;

    if(pool_init(&pool) != RESULT_OK) {
        fprintf(stderr, "Failed to initialize pool\n");
        return EXIT_FAILURE;
    }

    data_t* test;

    if(data_create(&pool, &test) != RESULT_OK) {
        fprintf(stderr, "Failed to create data\n");
        return EXIT_FAILURE;
    }

    if(file_read(&pool, &test, "data/test.txt") != RESULT_OK) {
        fprintf(stderr, "Failed to read file\n");
        return EXIT_FAILURE;
    }

    printf("File content: %.*s\n", (int)test->size, test->data);

    if(data_destroy(&pool, test) != RESULT_OK) {
        fprintf(stderr, "Failed to destroy data\n");
        return EXIT_FAILURE;
    }

    return 0;
}