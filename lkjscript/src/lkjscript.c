#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LKJSCRIPT_SIZE (1024 * 1024 * 16)
#define LKJSCRIPT_SRCPATH "/data/0001.lkjscript"

typedef enum {
    RESULT_OK,
    RESULT_ERROR
} result_t;

typedef enum {
    INST_NULL,
    INST_END,
    INST_DEBUG
} inst_t;

typedef struct {
    uint8_t data[LKJSCRIPT_SIZE];
} lkjscript_t;

static lkjscript_t lkjscript;

int token_size(const char* token) {
    for(int i = 0; 1; i++) {
        if(token[i] == ' ' || token[i] == '\n' || token[i] == '\r' || token[i] == '\t' || token[i] == '\0') {
            return i;
        }
    }
}

const char* token_next(const char* token) {
    int size = token_size(token);
    token += size;
    while(*token == ' ' || *token == '\n' || *token == '\r' || *token == '\t') {
        token++;
    }
    return token;
}

int token_equal(const char* a, const char* b) {
    int a_size = token_size(a);
    int b_size = token_size(b);
    if(a_size != b_size) {
        return 0;
    }
    for(int i = 0; i < a_size; i++) {
        if(a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

result_t compile() {
    // Load source file
    FILE *src_file = fopen(LKJSCRIPT_SRCPATH, "rb");
    if (!src_file) {
        fprintf(stderr, "Error: Unable to open source file %s\n", LKJSCRIPT_SRCPATH);
        return RESULT_ERROR;
    }
    int32_t src_size = fread(lkjscript.data, 1, LKJSCRIPT_SIZE, src_file);
    fclose(src_file);
    lkjscript.data[src_size] = '\0';

    // Tokenize and compile
    int32_t* inst_data = (int32_t*)(lkjscript.data + src_size + 1);
    int inst_size = 0;
    const char* token_itr = (const char*)lkjscript.data;
    while(*token_itr != '\0') {
        if(token_equal(token_itr, "debug")) {
            inst_data[inst_size++] = INST_DEBUG;
        } else {
            fprintf(stderr, "Error: Unknown token '%.*s'\n", token_size(token_itr), token_itr);
            return RESULT_ERROR;
        }
        token_itr = token_next(token_itr);
    }
    inst_data[inst_size++] = INST_END;

    // Move instructions to the start of lkjscript.data
    memmove(lkjscript.data, inst_data, inst_size * sizeof(int32_t));

    return RESULT_OK;
}

result_t run() {
    int32_t* pc = (int32_t*)lkjscript.data;
    while(1) {
        int32_t inst = *pc++;
        switch(inst) {
            case INST_NULL:
                return RESULT_ERROR;
            case INST_END:
                return RESULT_OK;
            case INST_DEBUG:
                printf("Debug instruction executed.\n");
                break;
        }
    }
}

int main() {
    if(compile() != RESULT_OK) {
        return 1;
    }
    if(run() != RESULT_OK) {
        return 1;
    }
    return 0;
}
