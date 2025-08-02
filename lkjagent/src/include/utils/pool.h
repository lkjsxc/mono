#ifndef LKJAGENT_UTILS_POOL_H
#define LKJAGENT_UTILS_POOL_H

#include "global/const.h"
#include "global/macro.h"
#include "global/std.h"
#include "global/types.h"

__attribute__((warn_unused_result)) result_t pool_init(pool_t* pool);

__attribute__((warn_unused_result)) result_t pool_string16_alloc(pool_t* pool, string_t** string);

__attribute__((warn_unused_result)) result_t pool_string256_alloc(pool_t* pool, string_t** string);

__attribute__((warn_unused_result)) result_t pool_string4096_alloc(pool_t* pool, string_t** string);

__attribute__((warn_unused_result)) result_t pool_string65536_alloc(pool_t* pool, string_t** string);

__attribute__((warn_unused_result)) result_t pool_string1048576_alloc(pool_t* pool, string_t** string);

__attribute__((warn_unused_result)) result_t pool_string_alloc(pool_t* pool, string_t** string, uint64_t capacity);

__attribute__((warn_unused_result)) result_t pool_string_free(pool_t* pool, string_t* string);

__attribute__((warn_unused_result)) result_t pool_string_realloc(pool_t* pool, string_t** string, uint64_t capacity);

__attribute__((warn_unused_result)) result_t pool_json_value_alloc(pool_t* pool, json_value_t** value);

__attribute__((warn_unused_result)) result_t pool_json_value_free(pool_t* pool, json_value_t* value);

__attribute__((warn_unused_result)) result_t pool_json_object_alloc(pool_t* pool, json_object_t** object);

__attribute__((warn_unused_result)) result_t pool_json_object_free(pool_t* pool, json_object_t* object);

__attribute__((warn_unused_result)) result_t pool_json_object_element_alloc(pool_t* pool, json_object_element_t** elem);

__attribute__((warn_unused_result)) result_t pool_json_object_element_free(pool_t* pool, json_object_element_t* elem);

#endif
