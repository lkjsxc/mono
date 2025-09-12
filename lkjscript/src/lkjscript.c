#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define LKJSCRIPT_SIZE (1024 * 1024 * 16)

typedef enum result_t result_t;
typedef struct node_t node_t;

enum result_t {
    RESULT_OK,
    RESULT_ERROR
};

struct node_t {
    uint32_t type;
    node_t* next;
    node_t* parent;
    node_t* child_begin;
    node_t* child_rbegin;
};

int8_t* lkjscript;

int main() {
    lkjscript = mmap(NULL, LKJSCRIPT_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (lkjscript == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed\n");
        return 1;
    }
    if(mprotect(lkjscript, LKJSCRIPT_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC) == -1) {
        fprintf(stderr, "Error: mprotect failed\n");
        munmap(lkjscript, LKJSCRIPT_SIZE);
        return 1;
    }
}