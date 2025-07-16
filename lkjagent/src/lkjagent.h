#ifndef _LKJAGENT_H
#define _LKJAGENT_H

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

typedef enum {
    RESULT_OK = 0,
    RESULT_ERR = 1,
} result_t;

typedef struct {
    char* data;
    size_t size;
    size_t capacity;
} token_t;

typedef struct {
    token_t base_url;
    token_t model;
} config_t;


// Token management functions
__attribute__((warn_unused_result)) result_t token_init(token_t* token, char* buffer, size_t capacity);
__attribute__((warn_unused_result)) result_t token_clear(token_t* token);
__attribute__((warn_unused_result)) result_t token_set(token_t* token, const char* str);
__attribute__((warn_unused_result)) result_t token_set_length(token_t* token, const char* buffer, size_t length);
__attribute__((warn_unused_result)) result_t token_append(token_t* token, const char* str);
__attribute__((warn_unused_result)) result_t token_append_length(token_t* token, const char* buffer, size_t length);
__attribute__((warn_unused_result)) result_t token_copy(token_t* dest, const token_t* src);
__attribute__((warn_unused_result)) result_t token_validate(const token_t* token);
int token_equals(const token_t* token1, const token_t* token2);
int token_equals_str(const token_t* token, const char* str);
int token_is_empty(const token_t* token);
int token_available_space(const token_t* token);

// Additional utility functions
__attribute__((warn_unused_result)) result_t token_find(const token_t* token, const char* needle, size_t* position);
__attribute__((warn_unused_result)) result_t token_substring(const token_t* token, size_t start, size_t length, token_t* dest);
__attribute__((warn_unused_result)) result_t token_trim(token_t* token);

// HTTP utility functions
__attribute__((warn_unused_result)) result_t http_request(token_t* method, token_t* url, token_t* body, token_t* response);
__attribute__((warn_unused_result)) result_t http_get(token_t* url, token_t* response);
__attribute__((warn_unused_result)) result_t http_post(token_t* url, token_t* body, token_t* response);

#endif