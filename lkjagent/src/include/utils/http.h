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
    string_t* scheme; // "http"
} url_t;

typedef struct {
    string_t* method;   // "GET", "POST", etc.
    string_t* url;      // Full URL
    string_t* headers;  // HTTP headers
    string_t* body;     // Request body
} http_request_t;

// URL parsing
__attribute__((warn_unused_result)) result_t url_init(url_t* url, pool_t* pool, const string_t* url_string);
result_t url_destroy(pool_t* pool, url_t* url);

// HTTP request management
__attribute__((warn_unused_result)) result_t http_request_init(http_request_t* request);
__attribute__((warn_unused_result)) result_t http_request_destroy(pool_t* pool, http_request_t* request);

// HTTP client functions
__attribute__((warn_unused_result)) result_t http_send_request(pool_t* pool, const http_request_t* request, string_t** response);
__attribute__((warn_unused_result)) result_t http_get(pool_t* pool, const string_t* url, string_t** response);
__attribute__((warn_unused_result)) result_t http_post(pool_t* pool, const string_t* url, const string_t* content_type, const string_t* body, string_t** response);

#endif
