#ifndef LKJAGENT_UTILS_HTTP_H
#define LKJAGENT_UTILS_HTTP_H

#include "global/const.h"
#include "global/macro.h"
#include "global/std.h"
#include "global/types.h"
#include "utils/pool.h"
#include "utils/string.h"
#include "utils/object.h"

typedef struct {
    string_t* host;
    uint16_t port;
    string_t* path;
    string_t* scheme; // "http" or "https"
} url_t;

typedef struct {
    string_t* method;   // "GET", "POST", etc.
    string_t* url;      // Full URL
    string_t* headers;  // HTTP headers
    string_t* body;     // Request body
} http_request_t;

typedef struct {
    uint16_t status_code;
    string_t* headers;
    string_t* body;
} http_response_t;

// URL parsing
__attribute__((warn_unused_result)) result_t url_parse(pool_t* pool, url_t** dst, const string_t* url_string);
__attribute__((warn_unused_result)) result_t url_destroy(pool_t* pool, url_t* url);

// HTTP request/response management
__attribute__((warn_unused_result)) result_t http_request_create(pool_t* pool, http_request_t** dst);
__attribute__((warn_unused_result)) result_t http_request_destroy(pool_t* pool, http_request_t* request);
__attribute__((warn_unused_result)) result_t http_response_create(pool_t* pool, http_response_t** dst);
__attribute__((warn_unused_result)) result_t http_response_destroy(pool_t* pool, http_response_t* response);

// HTTP client functions
__attribute__((warn_unused_result)) result_t http_send_request(pool_t* pool, const http_request_t* request, http_response_t** response);
__attribute__((warn_unused_result)) result_t http_get(pool_t* pool, const string_t* url, http_response_t** response);
__attribute__((warn_unused_result)) result_t http_post(pool_t* pool, const string_t* url, const string_t* content_type, const string_t* body, http_response_t** response);

// LM Studio specific functions
__attribute__((warn_unused_result)) result_t lmstudio_chat_completion(pool_t* pool, const string_t* endpoint, const object_t* request_data, object_t** response_data);
__attribute__((warn_unused_result)) result_t lmstudio_create_chat_request(pool_t* pool, const string_t* model, const string_t* message, double temperature, object_t** request_data);

#endif
