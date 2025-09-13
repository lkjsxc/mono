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
typedef enum node_type_t node_type_t;
typedef union uni64_t uni64_t;
typedef struct token_t token_t;
typedef struct node_t node_t;
typedef struct result_parse_t result_parse_t;

enum result_t {
    RESULT_OK,
    RESULT_ERROR
};

enum node_type_t {
    NODE_TYPE_NULL,
    NODE_TYPE_BLOCK,
    NODE_TYPE_IDENT,
    NODE_TYPE_NUM,
    NODE_TYPE_STR,
    NODE_TYPE_BF_BLOCK_LOOP,
    NODE_TYPE_BF_PLUS,
    NODE_TYPE_BF_MINUS,
    NODE_TYPE_BF_LSHIFT,
    NODE_TYPE_BF_RSHIFT,
    NODE_TYPE_BF_OUTPUT,
    NODE_TYPE_BF_INPUT,
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

int64_t token_equal(token_t* a, token_t* b) {
    if (a->size != b->size) {
        return 0;
    }
    for (int64_t i = 0; i < a->size; i++) {
        if (a->data[i] != b->data[i]) {
            return 0;
        }
    }
    return 1;
}

int64_t token_equal_str(token_t* a, const char* b) {
    int64_t b_size = strlen(b);
    if (a->size != b_size) {
        return 0;
    }
    for (int64_t i = 0; i < a->size; i++) {
        if (a->data[i] != b[i]) {
            return 0;
        }
    }
    return 1;
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
    mem[fsize] = '\0';
    *itr = mem + fsize + 1;
    // mem[fsize] = '\n';
    // mem[fsize + 1] = '\0';
    // *itr = mem + fsize + 2;
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

node_t* compile_parse_push(node_t** node_itr, node_type_t type, node_t* parent) {
    node_t* node = *node_itr;
    (*node_itr) += 1;
    node->type = type;
    node->value.u64 = 0;
    node->next = NULL;
    node->parent = parent;
    node->child_begin = NULL;
    node->child_rbegin = NULL;
    if (parent->child_begin == NULL) {
        parent->child_begin = node;
    } else {
        parent->child_rbegin->next = node;
    }
    parent->child_rbegin = node;
    return node;
}

result_parse_t compile_parse_stmt(node_t* parent, token_t* token_itr, node_t* node_itr) {
    if (token_equal_str(token_itr, "[")) {
        node_t* loop_parent = compile_parse_push(&node_itr, NODE_TYPE_BF_BLOCK_LOOP, parent);
        token_itr += 1;
        while (!token_equal_str(token_itr, "]")) {
            result_parse_t result = compile_parse_stmt(loop_parent, token_itr, node_itr);
            if (result.result != RESULT_ERROR) {
                fprintf(stderr, "Error: I will think about error message.\n");
                return (result_parse_t){.result = RESULT_ERROR, .token_itr = token_itr, .node_itr = node_itr};
            }
            token_itr = result.token_itr;
            node_itr = result.node_itr;
        }
        token_itr += 1;
        return (result_parse_t){.result = RESULT_OK, .token_itr = token_itr, .node_itr = node_itr};
    } else if (token_equal_str(token_itr, "+")) {
        node_t* plus = compile_parse_push(&node_itr, NODE_TYPE_BF_PLUS, parent);
        token_itr += 1;
        return (result_parse_t){.result = RESULT_OK, .token_itr = token_itr, .node_itr = node_itr};
    } else if (token_equal_str(token_itr, "-")) {
        node_t* plus = compile_parse_push(&node_itr, NODE_TYPE_BF_MINUS, parent);
        token_itr += 1;
        return (result_parse_t){.result = RESULT_OK, .token_itr = token_itr, .node_itr = node_itr};
    } else if (token_equal_str(token_itr, "<")) {
        node_t* plus = compile_parse_push(&node_itr, NODE_TYPE_BF_LSHIFT, parent);
        token_itr += 1;
        return (result_parse_t){.result = RESULT_OK, .token_itr = token_itr, .node_itr = node_itr};
    } else if (token_equal_str(token_itr, ">")) {
        node_t* plus = compile_parse_push(&node_itr, NODE_TYPE_BF_RSHIFT, parent);
        token_itr += 1;
        return (result_parse_t){.result = RESULT_OK, .token_itr = token_itr, .node_itr = node_itr};
    } else if (token_equal_str(token_itr, ".")) {
        node_t* plus = compile_parse_push(&node_itr, NODE_TYPE_BF_OUTPUT, parent);
        token_itr += 1;
        return (result_parse_t){.result = RESULT_OK, .token_itr = token_itr, .node_itr = node_itr};
    } else if (token_equal_str(token_itr, ",")) {
        node_t* plus = compile_parse_push(&node_itr, NODE_TYPE_BF_INPUT, parent);
        token_itr += 1;
        return (result_parse_t){.result = RESULT_OK, .token_itr = token_itr, .node_itr = node_itr};
    } else {
        fprintf(stderr, "Error: I will think about error message.\n");
        return (result_parse_t){.result = RESULT_ERROR, .token_itr = token_itr, .node_itr = node_itr};
    }
}

result_t compile_parse(uint8_t** itr, token_t* token) {
    token_t* token_itr = token;
    node_t* node_itr = (node_t*)*itr;
    node_t* root = node_itr++;
    root->type = NODE_TYPE_BLOCK;
    root->value.u64 = 0;
    root->next = NULL;
    root->parent = NULL;
    root->child_begin = NULL;
    root->child_rbegin = NULL;
    while (token_itr->data != NULL) {
        result_parse_t result = compile_parse_stmt(root, token_itr, node_itr);
        if (result.result != RESULT_OK) {
            fprintf(stderr, "Error: I will think about error message.\n");
        }
        token_itr = result.token_itr;
        node_itr = result.node_itr;
    }
    *itr = (uint8_t*)node_itr;
    printf("Parsing step (stub)... OK\n");
    return RESULT_OK;
}

result_t compile_codegen(uint8_t** itr, node_t* node_root) {
    uint32_t* code_itr = (uint32_t*)itr;

    *itr = (uint8_t*)code_itr;
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

void lkjscript_run() {
    if (mem == NULL) {
        fprintf(stderr, "Error: Cannot run script, memory not initialized.\n");
        return;
    }
    printf("Executing generated code...\n");
    int64_t (*fn)() = (int64_t (*)())mem;
    int64_t result = fn();
    printf("Result: %ld\n", result);
    fflush(stdout);
}

result_t lkjscript_deinit() {
    printf("Deinitializing script engine...\n");
    return mem_deinit();
}

int main() {
    FILE* f = fopen(LKJSCRIPT_SRCPATH, "w");
    if (f) {
        fprintf(f, "print('hello world')\n");
        fclose(f);
    }
    printf("Initializing script engine...\n");
    if (lkjscript_init() != RESULT_OK) {
        fprintf(stderr, "\nLKJScript initialization failed. Exiting.\n");
        return EXIT_FAILURE;
    }
    lkjscript_run();
    if (lkjscript_deinit() != RESULT_OK) {
        fprintf(stderr, "\nLKJScript deinitialization failed. Exiting.\n");
        return EXIT_FAILURE;
    }
    printf("Shutdown complete.\n");
    return EXIT_SUCCESS;
}