#ifndef LKJAGENT_TYPES_H
#define LKJAGENT_TYPES_H

#include "const.h"
#include "std.h"

typedef enum {
    RESULT_OK = 0,
    RESULT_ERR = 1,
} result_t;

typedef struct {
    char* data;
    uint64_t size;
    uint64_t capacity;
} string_t;

typedef struct {
    char pool_string256_data[POOL_STRING256_MAXCOUNT][256];
    string_t pool_string256[POOL_STRING256_MAXCOUNT];
    string_t* pool_string256_freelist_data[POOL_STRING256_MAXCOUNT];
    uint64_t pool_string256_freelist_count;
    char pool_string4096_data[POOL_STRING4096_MAXCOUNT][4096];
    string_t pool_string4096[POOL_STRING4096_MAXCOUNT];
    string_t* pool_string4096_freelist_data[POOL_STRING4096_MAXCOUNT];
    uint64_t pool_string4096_freelist_count;
    char pool_string1048576_data[POOL_STRING1048576_MAXCOUNT][1048576];
    string_t pool_string1048576[POOL_STRING1048576_MAXCOUNT];
    string_t* pool_string1048576_freelist_data[POOL_STRING1048576_MAXCOUNT];
    uint64_t pool_string1048576_freelist_count;
} pool_t;

typedef struct {
    pool_t pool;
} lkjagent_t;

#endif
