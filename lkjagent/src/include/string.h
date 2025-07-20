#ifndef LKJAGENT_STRING_H
#define LKJAGENT_STRING_H

#include "std.h"
#include "types.h"

__attribute__((warn_unused_result)) __attribute__((nonnull)) result_t string_init(string_t* string, char* data);
__attribute__((warn_unused_result)) __attribute__((nonnull)) result_t string_assign(string_t* string, const char* str);
__attribute__((warn_unused_result)) __attribute__((nonnull)) result_t string_copy(string_t* dest, const string_t* src);
__attribute__((nonnull)) void string_clear(string_t* string);

__attribute__((warn_unused_result)) __attribute__((nonnull)) result_t string_append(string_t* string, const char* str);
__attribute__((warn_unused_result)) __attribute__((nonnull)) result_t string_append_data(string_t* string, const char* data, uint64_t size);
__attribute__((warn_unused_result)) __attribute__((nonnull)) result_t string_append_char(string_t* string, char c);

__attribute__((nonnull)) int string_equal(const string_t* string1, const string_t* string2);
__attribute__((nonnull)) int string_equal_str(const string_t* string, const char* str);

__attribute__((nonnull)) int64_t string_find(const string_t* string, const char* substr);
__attribute__((nonnull)) int64_t string_find_char(const string_t* string, char c);
__attribute__((nonnull)) int64_t string_find_from(const string_t* string, const char* substr, uint64_t pos);
__attribute__((nonnull)) int64_t string_find_char_from(const string_t* string, char c, uint64_t pos);

#endif
