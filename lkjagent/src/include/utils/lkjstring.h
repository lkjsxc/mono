#ifndef LKJAGENT_LKJSTRING_H
#define LKJAGENT_LKJSTRING_H

#include "macro.h"
#include "std.h"
#include "types.h"

__attribute__((warn_unused_result)) result_t string_init(string_t* string, char* data, uint64_t capacity);

__attribute__((warn_unused_result)) result_t string_copy(pool_t* pool, string_t** dst, const string_t* src);
__attribute__((warn_unused_result)) result_t string_assign(pool_t* pool, string_t** string, const char* str);
void string_clear(string_t* string);

__attribute__((warn_unused_result)) result_t string_append(pool_t* pool, string_t** string, const string_t* src);
__attribute__((warn_unused_result)) result_t string_append_str(pool_t* pool, string_t** string, const char* str);
__attribute__((warn_unused_result)) result_t string_append_data(pool_t* pool, string_t** string, const char* data, uint64_t size);
__attribute__((warn_unused_result)) result_t string_append_char(pool_t* pool, string_t** string, char c);

int string_equal(const string_t* string1, const string_t* string2);
int string_equal_str(const string_t* string, const char* str);

int64_t string_find(const string_t* string, const char* substr);
int64_t string_find_char(const string_t* string, char c);
int64_t string_find_from(const string_t* string, const char* substr, uint64_t pos);
int64_t string_find_char_from(const string_t* string, char c, uint64_t pos);

#endif
