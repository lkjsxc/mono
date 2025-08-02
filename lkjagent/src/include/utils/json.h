#ifndef LKJAGENT_JSON_H
#define LKJAGENT_JSON_H

#include "global/macro.h"
#include "global/std.h"
#include "global/types.h"
#include "utils/pool.h"
#include "utils/string.h"

__attribute__((warn_unused_result)) result_t json_parse(pool_t* pool, json_value_t** dst, const string_t* src);

__attribute__((warn_unused_result)) result_t json_parse_xml(pool_t* pool, json_value_t** dst, const string_t* src);

__attribute__((warn_unused_result)) result_t json_to_string(pool_t* pool, string_t** dst, const json_value_t* src);

__attribute__((warn_unused_result)) result_t json_to_string_xml(pool_t* pool, string_t** dst, const json_value_t* src);

__attribute__((warn_unused_result)) result_t json_object_set(pool_t* pool, json_value_t* object, const string_t* path, json_value_t* value);

__attribute__((warn_unused_result)) result_t json_object_set_string(pool_t* pool, json_value_t* object, const string_t* path, const string_t* value);

__attribute__((warn_unused_result)) result_t json_object_get(json_value_t** dst, const json_value_t* object, const string_t* path);

__attribute__((warn_unused_result)) result_t json_destroy(pool_t* pool, json_value_t* value);

#endif
