#ifndef LKJAGENT_OBJECT_H
#define LKJAGENT_OBJECT_H

#include "global/macro.h"
#include "global/std.h"
#include "global/types.h"
#include "utils/pool.h"
#include "utils/string.h"

__attribute__((warn_unused_result)) result_t object_create(pool_t* pool, object_t** dst);

__attribute__((warn_unused_result)) result_t object_destroy(pool_t* pool, object_t* object);

__attribute__((warn_unused_result)) result_t object_parse_json(pool_t* pool, object_t** dst, const string_t* src);

__attribute__((warn_unused_result)) result_t object_parse_xml(pool_t* pool, object_t** dst, const string_t* src);

__attribute__((warn_unused_result)) result_t object_tostring_json(pool_t* pool, string_t** dst, const object_t* src);

__attribute__((warn_unused_result)) result_t object_tostring_xml(pool_t* pool, string_t** dst, const object_t* src);

__attribute__((warn_unused_result)) result_t object_set(pool_t* pool, object_t* object, const string_t* path, object_t* value);

__attribute__((warn_unused_result)) result_t object_set_string(pool_t* pool, object_t* object, const string_t* path, const string_t* value);

__attribute__((warn_unused_result)) result_t object_get(object_t** dst, const object_t* object, const string_t* path);

__attribute__((warn_unused_result)) result_t object_provide_string(object_t** dst, const object_t* object, const string_t* path);

__attribute__((warn_unused_result)) result_t object_provide_str(pool_t* pool, object_t** dst, const object_t* object, const char* path);

#endif
