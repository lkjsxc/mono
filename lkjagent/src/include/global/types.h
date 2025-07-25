#ifndef LKJAGENT_TYPES_H
#define LKJAGENT_TYPES_H

#include "const.h"
#include "std.h"

typedef enum {
    RESULT_OK = 0,
    RESULT_ERR = 1,
} result_t;

typedef enum {
    JSON_TYPE_NULL,
    JSON_TYPE_BOOL,
    JSON_TYPE_NUMBER,
    JSON_TYPE_STRING,
    JSON_TYPE_ARRAY,
    JSON_TYPE_OBJECT
} json_type_t;

typedef struct json_value_s json_value_t;
typedef struct json_object_s json_object_t;
typedef struct json_array_s json_array_t;

typedef struct {
    char* data;
    uint64_t capacity;
    uint64_t size;
} string_t;

typedef struct json_object_element_s {
    string_t* key;
    json_value_t* value;
    struct json_object_element_s* next;
} json_object_element_t;

typedef struct json_array_element_s {
    json_value_t* value;
    struct json_array_element_s* next;
} json_array_element_t;

struct json_object_s {
    json_object_element_t* head;
    uint64_t length;
};

struct json_array_s {
    json_array_element_t* head;
    uint64_t length;
};

struct json_value_s {
    json_type_t type;
    union {
        int bool_value;
        double number_value;
        string_t* string_value;
        json_object_t* object_value;
        json_array_t* array_value;
    } u;
};

typedef struct {
    char pool_string16_data[POOL_STRING16_MAXCOUNT][16];
    string_t pool_string16[POOL_STRING16_MAXCOUNT];
    string_t* pool_string16_freelist_data[POOL_STRING16_MAXCOUNT];
    uint64_t pool_string16_freelist_count;
    char pool_string256_data[POOL_STRING256_MAXCOUNT][256];
    string_t pool_string256[POOL_STRING256_MAXCOUNT];
    string_t* pool_string256_freelist_data[POOL_STRING256_MAXCOUNT];
    uint64_t pool_string256_freelist_count;
    char pool_string4096_data[POOL_STRING4096_MAXCOUNT][4096];
    string_t pool_string4096[POOL_STRING4096_MAXCOUNT];
    string_t* pool_string4096_freelist_data[POOL_STRING4096_MAXCOUNT];
    uint64_t pool_string4096_freelist_count;
    char pool_string65536_data[POOL_STRING65536_MAXCOUNT][65536];
    string_t pool_string65536[POOL_STRING65536_MAXCOUNT];
    string_t* pool_string65536_freelist_data[POOL_STRING65536_MAXCOUNT];
    uint64_t pool_string65536_freelist_count;
    char pool_string1048576_data[POOL_STRING1048576_MAXCOUNT][1048576];
    string_t pool_string1048576[POOL_STRING1048576_MAXCOUNT];
    string_t* pool_string1048576_freelist_data[POOL_STRING1048576_MAXCOUNT];
    uint64_t pool_string1048576_freelist_count;
    json_value_t pool_json_value[POOL_JSON_VALUE_MAXCOUNT];
    json_value_t* pool_json_value_freelist_data[POOL_JSON_VALUE_MAXCOUNT];
    uint64_t pool_json_value_freelist_count;
    json_object_t pool_json_object[POOL_JSON_OBJECT_MAXCOUNT];
    json_object_t* pool_json_object_freelist_data[POOL_JSON_OBJECT_MAXCOUNT];
    uint64_t pool_json_object_freelist_count;
    json_array_t pool_json_array[POOL_JSON_ARRAY_MAXCOUNT];
    json_array_t* pool_json_array_freelist_data[POOL_JSON_ARRAY_MAXCOUNT];
    uint64_t pool_json_array_freelist_count;
    json_object_element_t pool_json_object_element[POOL_JSON_OBJECT_ELEMENT_MAXCOUNT];
    json_object_element_t* pool_json_object_element_freelist_data[POOL_JSON_OBJECT_ELEMENT_MAXCOUNT];
    uint64_t pool_json_object_element_freelist_count;
    json_array_element_t pool_json_array_element[POOL_JSON_ARRAY_ELEMENT_MAXCOUNT];
    json_array_element_t* pool_json_array_element_freelist_data[POOL_JSON_ARRAY_ELEMENT_MAXCOUNT];
    uint64_t pool_json_array_element_freelist_count;
} pool_t;

typedef struct {
    string_t* version;

    string_t* llm_endpoint;
    string_t* llm_model;
    double llm_temperature;

    uint64_t agent_paging_limit;
    uint64_t agent_hard_limit;
    uint64_t agent_max_iterate;
    string_t* agent_default_state;
    string_t* agent_prompt_system;
    string_t* agent_prompt_thinking;
    string_t* agent_prompt_paging;
    string_t* agent_prompt_evaluating;
    string_t* agent_prompt_executing;
} config_t;

typedef struct {
    json_value_t* data;
} agent_t;

typedef struct {
    pool_t pool;
    config_t config;
    agent_t agent;
} lkjagent_t;

#endif
