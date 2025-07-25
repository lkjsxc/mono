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

__attribute__((warn_unused_result)) result_t string_clean(pool_t* pool, string_t* string);

__attribute__((warn_unused_result)) result_t string_copy_string(pool_t* pool, string_t** string1, const string_t* string2);

__attribute__((warn_unused_result)) result_t string_copy_str(pool_t* pool, string_t** string, const char* str);

__attribute__((warn_unused_result)) result_t string_append_string(pool_t* pool, string_t** string1, const string_t* string2);

__attribute__((warn_unused_result)) result_t string_append_str(pool_t* pool, string_t** string, const char* str);

__attribute__((warn_unused_result)) result_t string_append_char(pool_t* pool, string_t** string, char c);

uint64_t string_equal_string(const string_t* string1, const string_t* string2);

uint64_t string_equal_str(const string_t* string, const char* str);

int64_t string_find_string(const string_t* string1, const string_t* string2, uint64_t index);

int64_t string_find_str(const string_t* string, const char* str, uint64_t index);

int64_t string_find_char(const string_t* string, char c, uint64_t index);

__attribute__((warn_unused_result)) result_t string_destroy(pool_t* pool, string_t* string);

#endif
