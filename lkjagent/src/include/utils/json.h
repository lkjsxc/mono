#ifndef LKJAGENT_JSON_H
#define LKJAGENT_JSON_H

#include "global/macro.h"
#include "global/std.h"
#include "global/types.h"
#include "utils/pool.h"
#include "utils/string.h"

__attribute__((warn_unused_result)) result_t json_parse(pool_t* pool, json_value_t** dst, const string_t* path);

__attribute__((warn_unused_result)) result_t json_stringify(pool_t* pool, string_t** dst, const json_value_t* src);

__attribute__((warn_unused_result)) result_t json_create_null(pool_t* pool, json_value_t** dst);

__attribute__((warn_unused_result)) result_t json_create_object(pool_t* pool, json_value_t** dst);

__attribute__((warn_unused_result)) result_t json_create_array(pool_t* pool, json_value_t** dst);

__attribute__((warn_unused_result)) result_t json_create_bool(pool_t* pool, json_value_t** dst, int b);

__attribute__((warn_unused_result)) result_t json_create_number(pool_t* pool, json_value_t** dst, double num);

__attribute__((warn_unused_result)) result_t json_create_string(pool_t* pool, json_value_t** dst, const string_t* src);

__attribute__((warn_unused_result)) result_t json_object_set(pool_t* pool, json_value_t* object, const string_t* path, json_value_t* value);

__attribute__((warn_unused_result)) result_t json_object_get(json_value_t** dst, const json_value_t* object, const string_t* path);

__attribute__((warn_unused_result)) result_t json_array_append(pool_t* pool, json_value_t* array, json_value_t* value);

__attribute__((warn_unused_result)) result_t json_array_get(json_value_t** dst, const json_value_t* array, uint64_t index);

__attribute__((warn_unused_result)) result_t json_array_length(uint64_t* dst, const json_value_t* array);

__attribute__((warn_unused_result)) result_t json_object_length(uint64_t* dst, const json_value_t* object);

__attribute__((warn_unused_result)) result_t json_delete(pool_t* pool, json_value_t* value);

__attribute__((warn_unused_result)) result_t json_object_remove(pool_t* pool, json_value_t* object, const string_t* path);

__attribute__((warn_unused_result)) result_t json_array_remove(pool_t* pool, json_value_t* array, uint64_t index);

__attribute__((warn_unused_result)) result_t json_deep_copy(pool_t* pool, json_value_t** dst, const json_value_t* src);

#endif
