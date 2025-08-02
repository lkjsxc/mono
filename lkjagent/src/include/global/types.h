#ifndef LKJAGENT_TYPES_H
#define LKJAGENT_TYPES_H

#include "const.h"
#include "std.h"

typedef enum {
    RESULT_OK = 0,
    RESULT_ERR = 1,
} result_t;

typedef struct string_s {
    char* data;
    uint64_t capacity;
    uint64_t size;
} string_t;

typedef struct object_t {
    string_t* string;
    struct object_t* child;
    struct object_t* next;
} object_t;

typedef struct {
    char string16_data[POOL_STRING16_MAXCOUNT * 16];
    string_t string16[POOL_STRING16_MAXCOUNT];
    string_t* string16_freelist_data[POOL_STRING16_MAXCOUNT];
    uint64_t string16_freelist_count;
    char string256_data[POOL_STRING256_MAXCOUNT * 256];
    string_t string256[POOL_STRING256_MAXCOUNT];
    string_t* string256_freelist_data[POOL_STRING256_MAXCOUNT];
    uint64_t string256_freelist_count;
    char string4096_data[POOL_STRING4096_MAXCOUNT * 4096];
    string_t string4096[POOL_STRING4096_MAXCOUNT];
    string_t* string4096_freelist_data[POOL_STRING4096_MAXCOUNT];
    uint64_t string4096_freelist_count;
    char string65536_data[POOL_STRING65536_MAXCOUNT * 65536];
    string_t string65536[POOL_STRING65536_MAXCOUNT];
    string_t* string65536_freelist_data[POOL_STRING65536_MAXCOUNT];
    uint64_t string65536_freelist_count;
    char string1048576_data[POOL_STRING1048576_MAXCOUNT * 1048576];
    string_t string1048576[POOL_STRING1048576_MAXCOUNT];
    string_t* string1048576_freelist_data[POOL_STRING1048576_MAXCOUNT];
    uint64_t string1048576_freelist_count;
    object_t object_data[POOL_OBJECT_MAXCOUNT];
    object_t* object_freelist_data[POOL_OBJECT_MAXCOUNT];
    uint64_t object_freelist_count;
} pool_t;

typedef struct {
    object_t* data;
} agent_t;

typedef struct {
    pool_t pool;
    config_t config;
    agent_t agent;
} lkjagent_t;

#endif
