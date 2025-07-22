#ifndef LKJAGENT_LKJHTTP_H
#define LKJAGENT_LKJHTTP_H

#include "macro.h"
#include "std.h"
#include "types.h"
#include "utils/lkjstring.h"
#include "utils/lkjpool.h"

#define HTTP_DEFAULT_PORT 80
#define HTTP_DEFAULT_PORT_STR "80"
#define HTTPS_DEFAULT_PORT 443
#define HTTPS_DEFAULT_PORT_STR "443"
#define HTTP_MAX_HEADER_SIZE 8192
#define HTTP_MAX_RESPONSE_SIZE (1024 * 1024)  // 1MB max response
#define HTTP_BUFFER_SIZE 4096

typedef enum {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_PATCH
} http_method_t;

typedef struct {
    string_t* name;
    string_t* value;
} http_header_t;

typedef struct {
    string_t* url;
    string_t* host;
    string_t* port;
    string_t* path;
    string_t* query;
    int is_https;
} http_url_t;

typedef struct {
    http_method_t method;
    http_url_t url;
    http_header_t* headers;
    uint64_t header_count;
    string_t* body;
    uint64_t timeout_seconds;
} http_request_t;

typedef struct {
    int status_code;
    string_t* status_message;
    http_header_t* headers;
    uint64_t header_count;
    string_t* body;
} http_response_t;

/**
 * Parse a URL string into components
 * @param pool Memory pool for allocations
 * @param url_str The URL string to parse
 * @param url Output URL structure
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t http_parse_url(pool_t* pool, const char* url_str, http_url_t* url);

/**
 * Initialize an HTTP request structure
 * @param pool Memory pool for allocations
 * @param request The request structure to initialize
 * @param method HTTP method
 * @param url_str URL string
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t http_request_init(pool_t* pool, http_request_t* request, http_method_t method, const char* url_str);

/**
 * Add a header to an HTTP request
 * @param pool Memory pool for allocations
 * @param request The request to add header to
 * @param name Header name
 * @param value Header value
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t http_request_add_header(pool_t* pool, http_request_t* request, const char* name, const char* value);

/**
 * Set the body of an HTTP request
 * @param pool Memory pool for allocations
 * @param request The request to set body for
 * @param body Body content
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t http_request_set_body(pool_t* pool, http_request_t* request, const char* body);

/**
 * Initialize an HTTP response structure
 * @param pool Memory pool for allocations
 * @param response The response structure to initialize
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t http_response_init(pool_t* pool, http_response_t* response);

/**
 * Send an HTTP request and receive response
 * @param pool Memory pool for allocations
 * @param request The HTTP request to send
 * @param response Output response structure
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t http_send_request(pool_t* pool, const http_request_t* request, http_response_t* response);

/**
 * Perform a simple GET request
 * @param pool Memory pool for allocations
 * @param url URL to request
 * @param response Output response structure
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t http_get(pool_t* pool, const char* url, http_response_t* response);

/**
 * Perform a simple POST request with JSON body
 * @param pool Memory pool for allocations
 * @param url URL to request
 * @param json_body JSON body content
 * @param response Output response structure
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t http_post_json(pool_t* pool, const char* url, const char* json_body, http_response_t* response);

#endif
