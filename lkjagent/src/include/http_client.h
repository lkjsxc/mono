/**
 * @file http_client.h
 * @brief HTTP client interface for LKJAgent LLM communication
 * 
 * This header provides a robust HTTP client implementation designed for
 * reliable communication with LMStudio and other LLM services. It includes
 * comprehensive error handling, retry mechanisms, and connection management.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#ifndef LKJAGENT_HTTP_CLIENT_H
#define LKJAGENT_HTTP_CLIENT_H

#include "types.h"
#include "data.h"

/**
 * @defgroup HTTP_Client HTTP Client Operations
 * @{
 */

/**
 * @brief HTTP request methods
 */
typedef enum {
    HTTP_METHOD_GET = 0,
    HTTP_METHOD_POST = 1,
    HTTP_METHOD_PUT = 2,
    HTTP_METHOD_DELETE = 3
} http_method_t;

/**
 * @brief HTTP response status codes
 */
typedef enum {
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_UNAUTHORIZED = 401,
    HTTP_STATUS_FORBIDDEN = 403,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
    HTTP_STATUS_BAD_GATEWAY = 502,
    HTTP_STATUS_SERVICE_UNAVAILABLE = 503,
    HTTP_STATUS_GATEWAY_TIMEOUT = 504
} http_status_t;

/**
 * @brief HTTP client configuration structure
 */
typedef struct {
    /** Connection timeout in seconds */
    uint32_t connect_timeout;
    /** Request timeout in seconds */
    uint32_t request_timeout;
    /** Maximum number of retry attempts */
    uint32_t max_retries;
    /** Retry delay in milliseconds */
    uint32_t retry_delay;
    /** Maximum response size in bytes */
    size_t max_response_size;
    /** User agent string */
    char user_agent[256];
    /** Enable keepalive connections */
    bool enable_keepalive;
} http_client_config_t;

/**
 * @brief HTTP client instance structure
 */
typedef struct {
    /** Client configuration */
    http_client_config_t config;
    /** Current connection file descriptor */
    int connection_fd;
    /** Connection state */
    bool is_connected;
    /** Current host */
    char current_host[256];
    /** Current port */
    uint16_t current_port;
    /** Custom headers data */
    data_t custom_headers;
} http_client_t;

/**
 * @brief HTTP response structure
 */
typedef struct {
    /** HTTP status code */
    http_status_t status_code;
    /** Response headers */
    data_t headers;
    /** Response body */
    data_t body;
    /** Response time in milliseconds */
    uint64_t response_time;
} http_response_t;

/**
 * @brief Initialize HTTP client with configuration
 * 
 * Creates and initializes an HTTP client instance with the specified
 * configuration. Sets up internal structures and prepares for connections.
 * 
 * @param client Pointer to HTTP client structure to initialize
 * @param config Configuration parameters for the client
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note The client structure is fully initialized and ready for use
 * @note Default configuration values are applied if config is NULL
 * @note Internal buffers are allocated and initialized
 * 
 * @warning client parameter must not be NULL
 * 
 * Example usage:
 * @code
 * http_client_t client;
 * http_client_config_t config = {
 *     .connect_timeout = 10,
 *     .request_timeout = 30,
 *     .max_retries = 3,
 *     .retry_delay = 1000,
 *     .max_response_size = 1024 * 1024
 * };
 * strcpy(config.user_agent, "LKJAgent/1.0");
 * 
 * if (http_client_init(&client, &config) == RESULT_OK) {
 *     // Client ready for use
 *     http_client_cleanup(&client);
 * }
 * @endcode
 */
result_t http_client_init(http_client_t* client, const http_client_config_t* config) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Send HTTP POST request with JSON payload
 * 
 * Sends a POST request to the specified URL with JSON content type and
 * handles the complete request/response cycle with robust error handling.
 * 
 * @param client HTTP client instance
 * @param url Target URL for the request
 * @param json_payload JSON payload to send in request body
 * @param response Response structure to store the result
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Automatically sets Content-Type to application/json
 * @note Handles connection establishment, request sending, and response parsing
 * @note Implements retry logic according to client configuration
 * @note Response body is stored in response->body data buffer
 * 
 * @warning client parameter must not be NULL and must be initialized
 * @warning url parameter must not be NULL and must be valid
 * @warning json_payload parameter must not be NULL
 * @warning response parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * http_response_t response;
 * data_init(&response.body, 1024);
 * data_init(&response.headers, 512);
 * 
 * const char* payload = "{\"prompt\": \"Hello\", \"max_tokens\": 100}";
 * if (http_client_post(&client, "http://localhost:1234/v1/completions", 
 *                      payload, &response) == RESULT_OK) {
 *     printf("Response: %s\n", response.body.data);
 * }
 * 
 * data_destroy(&response.body);
 * data_destroy(&response.headers);
 * @endcode
 */
result_t http_client_post(http_client_t* client, const char* url, const char* json_payload, http_response_t* response) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3, 4)));

/**
 * @brief Send HTTP GET request
 * 
 * Sends a GET request to the specified URL and handles the complete
 * request/response cycle with comprehensive error handling.
 * 
 * @param client HTTP client instance
 * @param url Target URL for the request
 * @param response Response structure to store the result
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Handles connection establishment and response parsing
 * @note Implements retry logic according to client configuration
 * @note Response is stored in the response structure
 * 
 * @warning client parameter must not be NULL and must be initialized
 * @warning url parameter must not be NULL and must be valid
 * @warning response parameter must not be NULL and must be initialized
 */
result_t http_client_get(http_client_t* client, const char* url, http_response_t* response) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Set custom HTTP headers
 * 
 * Configures custom headers that will be included in all subsequent requests.
 * Headers are stored in the client instance and automatically added.
 * 
 * @param client HTTP client instance
 * @param headers Header string in "Name: Value\r\n" format
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Headers are appended to existing custom headers
 * @note Headers must be in proper HTTP format
 * @note Headers persist until client cleanup or explicit reset
 * 
 * @warning client parameter must not be NULL and must be initialized
 * @warning headers parameter must not be NULL
 */
result_t http_client_set_headers(http_client_t* client, const char* headers) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Set request timeout configuration
 * 
 * Updates timeout settings for the HTTP client. Changes take effect
 * immediately for subsequent requests.
 * 
 * @param client HTTP client instance
 * @param connect_timeout Connection timeout in seconds
 * @param request_timeout Request timeout in seconds
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Timeout values must be greater than 0
 * @note Changes affect all subsequent requests
 * @note Current active connections are not affected
 * 
 * @warning client parameter must not be NULL and must be initialized
 */
result_t http_client_set_timeout(http_client_t* client, uint32_t connect_timeout, uint32_t request_timeout) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Handle and classify HTTP errors
 * 
 * Analyzes HTTP response status codes and network errors to determine
 * the appropriate error handling strategy and retry behavior.
 * 
 * @param status_code HTTP status code from response
 * @param should_retry Pointer to store whether operation should be retried
 * @param error_message Buffer to store human-readable error message
 * @param message_size Size of error message buffer
 * @return RESULT_OK if error was handled, RESULT_ERR if unrecoverable
 * 
 * @note Sets should_retry to true for transient errors (5xx, timeouts)
 * @note Sets should_retry to false for client errors (4xx)
 * @note Error message provides detailed information for logging
 * 
 * @warning should_retry parameter must not be NULL
 * @warning error_message parameter must not be NULL
 */
result_t http_client_handle_errors(http_status_t status_code, bool* should_retry, char* error_message, size_t message_size) __attribute__((warn_unused_result)) __attribute__((nonnull(2, 3)));

/**
 * @brief Test connectivity to specified host
 * 
 * Performs a connectivity test to verify that the specified host and port
 * are reachable and responsive. Used for health checks and diagnostics.
 * 
 * @param client HTTP client instance
 * @param host Hostname or IP address to test
 * @param port Port number to test
 * @param response_time Pointer to store response time in milliseconds
 * @return RESULT_OK if host is reachable, RESULT_ERR if not
 * 
 * @note Performs actual connection attempt to verify reachability
 * @note Response time is measured for performance monitoring
 * @note Uses client timeout configuration for test duration
 * 
 * @warning client parameter must not be NULL and must be initialized
 * @warning host parameter must not be NULL
 * @warning response_time parameter must not be NULL
 */
result_t http_client_test_connectivity(http_client_t* client, const char* host, uint16_t port, uint64_t* response_time) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 4)));

/**
 * @brief Clean up HTTP client resources
 * 
 * Closes any open connections and frees all resources associated with
 * the HTTP client instance. Must be called before program termination.
 * 
 * @param client HTTP client instance to clean up
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Closes any active connections gracefully
 * @note Frees all internal buffers and data structures
 * @note Client instance becomes invalid after cleanup
 * 
 * @warning client parameter must not be NULL
 * 
 * Example usage:
 * @code
 * http_client_t client;
 * // ... use client ...
 * http_client_cleanup(&client);
 * // client is now invalid and cannot be used
 * @endcode
 */
result_t http_client_cleanup(http_client_t* client) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Initialize HTTP response structure
 * 
 * Initializes all data buffers in an HTTP response structure with
 * appropriate initial capacities for headers and body content.
 * 
 * @param response HTTP response structure to initialize
 * @param body_capacity Initial capacity for response body buffer
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Initializes both headers and body data buffers
 * @note Sets status code to 0 and response time to 0
 * @note Response is ready for use after successful initialization
 * 
 * @warning response parameter must not be NULL
 */
result_t http_response_init(http_response_t* response, size_t body_capacity) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Clean up HTTP response structure
 * 
 * Frees all data buffers associated with an HTTP response structure.
 * Must be called to prevent memory leaks.
 * 
 * @param response HTTP response structure to clean up
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Frees headers and body data buffers
 * @note Response structure becomes invalid after cleanup
 * 
 * @warning response parameter must not be NULL
 */
result_t http_response_cleanup(http_response_t* response) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/** @} */

#endif /* LKJAGENT_HTTP_CLIENT_H */
