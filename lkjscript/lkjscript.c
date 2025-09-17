#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define LKJSCRIPT_SIZE (1024 * 1024 * 16)
#define LKJSCRIPT_SRCPATH "/data/main.lkjscript"

typedef enum result_t result_t;
typedef enum inst_t inst_t;
typedef enum node_type_t node_type_t;
typedef union uni64_t uni64_t;
typedef struct token_t token_t;
typedef struct node_t node_t;
typedef struct result_parse_t result_parse_t;

enum result_t {
    RESULT_OK,
    RESULT_ERROR
};

enum inst_t {
    INST_NULL,
    INST_IMM,
    INST_COPY,
    INST_ADD,
    INST_SUB,
    INST_LOAD,
    INST_STORE,
    INST_JMP,
    INST_JZE,
};

enum node_type_t {
    NODE_TYPE_NULL,
    NODE_TYPE_BLOCK,
    NODE_TYPE_IDENT,
    NODE_TYPE_NUM,
    NODE_TYPE_STR,
};

union uni64_t {
    uint64_t u64;
    int64_t i64;
    double f64;
    char* str;
    token_t* token;
    node_t* node;
};

struct token_t {
    char* data;
    uint64_t size;
};

struct node_t {
    uint64_t type;
    uni64_t value;
    node_t* next;
    node_t* parent;
    node_t* child_begin;
    node_t* child_rbegin;
};

struct result_parse_t {
    result_t result;
    token_t* token_itr;
    node_t* node_itr;
};

uint8_t* mem = NULL;

uint64_t token_equal(token_t* a, token_t* b) {
    if (a->size != b->size) {
        return 0;
    }
    for (uint64_t i = 0; i < a->size; i++) {
        if (a->data[i] != b->data[i]) {
            return 0;
        }
    }
    return 1;
}

uint64_t token_equal_str(token_t* a, const char* b) {
    uint64_t b_size = strlen(b);
    if (a->size != b_size) {
        return 0;
    }
    for (uint64_t i = 0; i < a->size; i++) {
        if (a->data[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

node_t* node_find(node_t* current, node_type_t type) {
    node_t* base;
    if (current->parent == NULL) {
        base = current;
    } else {
        base = current->parent->child_begin;
    }
    for (node_t* base_itr = base; base_itr != NULL; base_itr = base_itr->parent) {
        for (node_t* node_itr = base_itr; node_itr != NULL; node_itr = node_itr->next) {
            if (node_itr->type == type) {
                return node_itr;
            }
        }
    }
    return NULL;
}

result_t mem_init() {
    mem = mmap(NULL, LKJSCRIPT_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed: %s\n", strerror(errno));
        return RESULT_ERROR;
    }
    if (mprotect(mem, LKJSCRIPT_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC) == -1) {
        fprintf(stderr, "Error: mprotect failed: %s\n", strerror(errno));
        munmap(mem, LKJSCRIPT_SIZE);
        mem = NULL;
        return RESULT_ERROR;
    }
    return RESULT_OK;
}

result_t mem_deinit() {
    if (mem != NULL && munmap(mem, LKJSCRIPT_SIZE) == -1) {
        fprintf(stderr, "Error: munmap failed: %s\n", strerror(errno));
        return RESULT_ERROR;
    }
    mem = NULL;
    return RESULT_OK;
}

result_t compile_readsrc(uint8_t** itr) {
    FILE* fp = fopen(LKJSCRIPT_SRCPATH, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open source file '%s': %s\n", LKJSCRIPT_SRCPATH, strerror(errno));
        return RESULT_ERROR;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "Error: Could not seek in file '%s': %s\n", LKJSCRIPT_SRCPATH, strerror(errno));
        fclose(fp);
        return RESULT_ERROR;
    }
    int64_t fsize = (int64_t)ftell(fp);
    if (fsize == -1) {
        fprintf(stderr, "Error: Could not get size of file '%s': %s\n", LKJSCRIPT_SRCPATH, strerror(errno));
        fclose(fp);
        return RESULT_ERROR;
    } else if (fsize >= LKJSCRIPT_SIZE) {
        fprintf(stderr, "Error: Source file '%s' (%ld bytes) is too large for memory buffer (%d bytes).\n", LKJSCRIPT_SRCPATH, fsize, LKJSCRIPT_SIZE);
        fclose(fp);
        return RESULT_ERROR;
    }
    rewind(fp);
    int64_t nread = (int64_t)fread(mem, 1, fsize, fp);
    if (nread < fsize) {
        if (ferror(fp)) {
            fprintf(stderr, "Error: Failed to read from source file '%s': %s\n", LKJSCRIPT_SRCPATH, strerror(errno));
        } else {
            fprintf(stderr, "Error: Unexpected end of file while reading '%s'. Read %zu of %ld bytes.\n", LKJSCRIPT_SRCPATH, nread, fsize);
        }
        fclose(fp);
        return RESULT_ERROR;
    }
    fclose(fp);
    mem[fsize] = '\n';
    mem[fsize + 1] = '\0';
    *itr = mem + fsize + 2;
    return RESULT_OK;
}

void compile_tokenize_push(token_t** token_itr, char* data, uint64_t size) {
    (*token_itr)->data = data;
    (*token_itr)->size = size;
    (*token_itr) += 1;
}

result_t compile_tokenize(uint8_t** itr, char* src) {
    token_t* token_itr = (token_t*)*itr;
    char* src_itr = src;
    while (*src_itr != '\0') {
        char c = *src_itr;
        if (c == '>' || c == '<' || c == '+' || c == '-' || c == '.' || c == ',' || c == '[' || c == ']') {
            compile_tokenize_push(&token_itr, src_itr, 1);
            src_itr += 1;
        } else {
            fprintf(stderr, "Wow amazing char hahaha!!\n");
            return RESULT_ERROR;
        }
    }
    compile_tokenize_push(&token_itr, NULL, 0);
    *itr = (uint8_t*)token_itr;
    return RESULT_OK;
}

result_t lkjscript_init() {
    if (mem_init() != RESULT_OK) {
        return RESULT_ERROR;
    }
    uint8_t* itr = mem;
    char* src = (char*)itr;
    if (compile_readsrc(&itr) != RESULT_OK) {
        fprintf(stderr, "Error: Failed to readsrc\n");
        if (mem_deinit() != RESULT_OK) {
            fprintf(stderr, "Error: Failed to mem_deinit\n");
        }
        return RESULT_ERROR;
    }
    token_t* token = (token_t*)itr;
    if (compile_tokenize(&itr, src) != RESULT_OK) {
        fprintf(stderr, "Error: Failed to tokenize\n");
        if (mem_deinit() != RESULT_OK) {
            fprintf(stderr, "Error: Failed to mem_deinit\n");
        }
        return RESULT_ERROR;
    }
    node_t* node_root = (node_t*)itr;
    if (compile_parse(&itr, token) != RESULT_OK) {
        fprintf(stderr, "Error: Failed to parse\n");
        if (mem_deinit() != RESULT_OK) {
            fprintf(stderr, "Error: Failed to mem_deinit\n");
        }
        return RESULT_ERROR;
    }
    uint8_t* code_start = itr;
    if (compile_codegen(&itr, node_root) != RESULT_OK) {
        fprintf(stderr, "Error: Failed to codegen\n");
        if (mem_deinit() != RESULT_OK) {
            fprintf(stderr, "Error: Failed to mem_deinit\n");
        }
        return RESULT_ERROR;
    }
    uint8_t* code_end = itr;
    size_t code_size = code_end - code_start;
    memmove(mem, code_start, code_size);
    return RESULT_OK;
}

result_t lkjscript_deinit() {
    printf("Deinitializing script engine...\n");
    return mem_deinit();
}

void lkjscript_run() {
}

int main() {
    if (lkjscript_init() != RESULT_OK) {
        fprintf(stderr, "\nLKJScript initialization failed. Exiting.\n");
        return EXIT_FAILURE;
    }
    lkjscript_run();
    if (lkjscript_deinit() != RESULT_OK) {
        fprintf(stderr, "\nLKJScript deinitialization failed. Exiting.\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}