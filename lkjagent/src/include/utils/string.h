#ifndef LKJAGENT_UTILS_STRING_H
#define LKJAGENT_UTILS_STRING_H

#include "global/const.h"
#include "global/macro.h"
#include "global/std.h"
#include "global/types.h"
#include "utils/pool.h"

__attribute__((warn_unused_result)) result_t string_create(pool_t* pool, string_t** string);

__attribute__((warn_unused_result)) result_t string_create_string(pool_t* pool, string_t** string1, const string_t* string2);

__attribute__((warn_unused_result)) result_t string_create_str(pool_t* pool, string_t** string, const char* str);

__attribute__((warn_unused_result)) result_t string_copy_string(pool_t* pool, string_t** string1, const string_t* string2);

__attribute__((warn_unused_result)) result_t string_copy_str(pool_t* pool, string_t** string, const char* str);

__attribute__((warn_unused_result)) result_t string_destroy(pool_t* pool, string_t* string);

#endif
