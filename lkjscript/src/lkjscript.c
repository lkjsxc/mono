#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define LKJSCRIPT_SIZE (1024 * 1024 * 16)
#define LKJSCRIPT_SRCPATH "/data/main.lkjscript"

typedef enum node_type_t node_type_t;
typedef union uni_t unit_t;
typedef struct node_t node_t;

enum node_type_t {
    NODE_TYPE_NULL,
    NODE_TYPE_BLOCK,
    NODE_TYPE_IDENT,
    NODE_TYPE_NUM,
    NODE_TYPE_STR,
};

union uni_t {
    uint64_t u64;
    int64_t i64;
    double f64;
    char* str;
    node_t* node;
};

struct node_t {
    uint32_t type;
    node_t* next;
    node_t* parent;
    node_t* child_begin;
    node_t* child_rbegin;
};

int8_t* mem;

void mem_init() {
    mem = mmap(NULL, LKJSCRIPT_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mem == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed\n");
        exit(1);
    }
    if(mprotect(mem, LKJSCRIPT_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC) == -1) {
        fprintf(stderr, "Error: mprotect failed\n");
        munmap(mem, LKJSCRIPT_SIZE);
        exit(1);
    }
}

void mem_deinit() {
    if (munmap(mem, LKJSCRIPT_SIZE) == -1) {
        fprintf(stderr, "Error: munmap failed\n");
        exit(1);
    }
}

void readsrc() {
    FILE* f = fopen(LKJSCRIPT_SRCPATH, "rb");
}

void lkjscript_init() {
    mem_init();
    readsrc();
    parse();
    gencode();
}

void lkjscript_run() {
    int64_t (*fn)() = (int64_t (*)())mem;
    int64_t result = fn();
    printf("Result: %ld\n", result);
    fflush(stdout);
}

void lkjscript_deinit() {
    mem_deinit();
}

int main() {
    lkjscript_init();
    lkjscript_run();
    lkjscript_deinit();
    return 0;
}