#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SRC_PATH "/data/main.lkjscript"
#define SRC_CAPACITY (1024 * 1024 * 256)

int main() {
    // Read the source file
    FILE* file = fopen(SRC_PATH, "r");
    src_size = fread(src_data, 1, SRC_CAPACITY, file);
    fclose(file);

    // Tokenize the source code
    object_t* token = NULL;
    if (lkjscript_tokenize(&pool, token, src_data, src_size) != 0) {
        printf("Failed to tokenize source code\n");
        return 1;
    }

    return 0;
}
