#ifndef LKJAGENT_TYPES_H
#define LKJAGENT_TYPES_H

#include <stdint.h>
#include "const.h"

typedef enum {
    RESULT_OK = 0,
    RESULT_ERR = 1,
} result_t;

typedef struct {
    char* data;
    uint64_t size;
    uint64_t capacity;
} string_t;

typedef struct tree_t {
    string_t* string;
    struct tree_t* next;
    struct tree_t* child;
    struct tree_t* parent;
} tree_t;

typedef struct {
    char string_data[STRING_COUNT][STRING_CAPACITY];
    string_t string[STRING_COUNT];
    string_t* string_freelist[STRING_COUNT];
    tree_t tree[TREE_COUNT];
    tree_t* tree_freelist[TREE_COUNT];
} lkjagent_t;

#endif
