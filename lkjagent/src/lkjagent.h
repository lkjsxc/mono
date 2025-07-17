#ifndef LKJAGENT_H
#define LKJAGENT_H

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define RETURN_ERR3(n) #n
#define RETURN_ERR2(n) RETURN_ERR3(n)
#define RETURN_ERR(error_message)                                                   \
    do {                                                                            \
        _Pragma("GCC diagnostic push")                                              \
        _Pragma("GCC diagnostic ignored \"-Wunused-result\"")                      \
        write(STDERR_FILENO, "{Error: { file: \"", 18);                             \
        write(STDERR_FILENO, __FILE__, sizeof(__FILE__));                           \
        write(STDERR_FILENO, "\", func: \"", 11);                                   \
        write(STDERR_FILENO, __func__, sizeof(__func__));                           \
        write(STDERR_FILENO, "\", line: ", 10);                                     \
        write(STDERR_FILENO, RETURN_ERR2(__LINE__), sizeof(RETURN_ERR2(__LINE__))); \
        write(STDERR_FILENO, "\", message: \"", 13);                                \
        write(STDERR_FILENO, error_message, strlen(error_message));                 \
        write(STDERR_FILENO, "\"}}\n", 5);                                          \
        _Pragma("GCC diagnostic pop")                                               \
    } while(0)

// ============================================================================
// Core Types
// ============================================================================

/**
 * @brief Result type for all functions requiring error handling
 */
typedef enum {
    RESULT_OK, /**< Operation completed successfully */
    RESULT_ERR /**< Operation failed */
} result_t;

/**
 * @brief Safe string token with bounded buffer
 */
typedef struct {
    char* data;      /**< Pointer to character buffer */
    size_t capacity; /**< Maximum buffer size including null terminator */
    size_t size;     /**< Current string length (excluding null terminator) */
} token_t;

/**
 * @brief Main agent structure
 */
typedef struct {
    token_t config_path; /**< Path to configuration file */
    // Add more fields as needed
} lkjagent_t;

// ============================================================================
// Error Handling API
// ============================================================================

/**
 * @brief Set the last error message
 * @param error Error message string (can be NULL to clear error)
 */
void lkj_set_error(const char* error);

/**
 * @brief Get the last error message
 * @return Pointer to error message string (never NULL, may be empty)
 */
const char* lkj_get_last_error(void);

/**
 * @brief Clear the last error message
 */
void lkj_clear_last_error(void);

/**
 * @brief Check if there is a current error
 * @return 1 if there is an error, 0 if no error
 */
int lkj_has_error(void);

/**
 * @brief Print last error to stderr
 */
void lkj_print_error(void);

// ============================================================================
// Token Management API
// ============================================================================

/**
 * @brief Initialize a token with a static buffer
 */
__attribute__((warn_unused_result))
result_t
token_init(token_t* token, char* buffer, size_t capacity);

/**
 * @brief Set token content from a C string
 */
__attribute__((warn_unused_result))
result_t
token_set(token_t* token, const char* str);

/**
 * @brief Set token content from buffer with specified length
 */
__attribute__((warn_unused_result))
result_t
token_set_length(token_t* token, const char* buffer, size_t length);

/**
 * @brief Append string to existing token content
 */
__attribute__((warn_unused_result))
result_t
token_append(token_t* token, const char* str);

/**
 * @brief Copy content from source token to destination token
 */
__attribute__((warn_unused_result))
result_t
token_copy(token_t* dest, const token_t* src);

/**
 * @brief Compare two tokens for equality
 */
int token_equals(const token_t* token1, const token_t* token2);

/**
 * @brief Compare token with C string for equality
 */
int token_equals_str(const token_t* token, const char* str);

/**
 * @brief Find substring in token content
 */
__attribute__((warn_unused_result))
result_t
token_find(const token_t* token, const char* needle, size_t* position);

/**
 * @brief Extract substring from token
 */
__attribute__((warn_unused_result))
result_t
token_substring(const token_t* token, size_t start, size_t length, token_t* dest);

/**
 * @brief Clear token content
 */
__attribute__((warn_unused_result))
result_t
token_clear(token_t* token);

/**
 * @brief Check if token is empty
 */
int token_is_empty(const token_t* token);

/**
 * @brief Get remaining capacity in token
 */
size_t token_remaining_capacity(const token_t* token);

// ============================================================================
// File Operations API
// ============================================================================

/**
 * @brief Read entire file content into a token
 */
__attribute__((warn_unused_result))
result_t
file_read(const char* path, token_t* content);

/**
 * @brief Write token content to file
 */
__attribute__((warn_unused_result))
result_t
file_write(const char* path, const token_t* content);

/**
 * @brief Append token content to existing file
 */
__attribute__((warn_unused_result))
result_t
file_append(const char* path, const token_t* content);

/**
 * @brief Check if file exists
 */
int file_exists(const char* path);

/**
 * @brief Check if directory exists
 */
int file_directory_exists(const char* path);

/**
 * @brief Get file size
 */
__attribute__((warn_unused_result))
result_t
file_get_size(const char* path, size_t* size);

/**
 * @brief Ensure directory exists, creating it if necessary
 */
__attribute__((warn_unused_result))
result_t
file_ensure_directory(const char* path);

/**
 * @brief Delete file
 */
__attribute__((warn_unused_result))
result_t
file_delete(const char* path);

/**
 * @brief Copy file from source to destination
 */
__attribute__((warn_unused_result))
result_t
file_copy(const char* src_path, const char* dest_path);

// ============================================================================
// HTTP Client API
// ============================================================================

/**
 * @brief Perform HTTP GET request
 */
__attribute__((warn_unused_result))
result_t
http_get(token_t* url, token_t* response);

/**
 * @brief Perform HTTP POST request with body
 */
__attribute__((warn_unused_result))
result_t
http_post(token_t* url, const token_t* body, token_t* response);

/**
 * @brief Generic HTTP request function (legacy API)
 */
__attribute__((warn_unused_result))
result_t
http_request(token_t* method, token_t* url, const token_t* body, token_t* response);

/**
 * @brief Generic HTTP request function with method string
 */
__attribute__((warn_unused_result))
result_t
http_request_method(const char* method, token_t* url, const token_t* body, token_t* response);

// ============================================================================
// JSON Processing API
// ============================================================================

/**
 * @brief Validate JSON structure and syntax
 */
__attribute__((warn_unused_result))
result_t
json_validate(const token_t* json_token);

/**
 * @brief Extract string value from JSON using key path
 */
__attribute__((warn_unused_result))
result_t
json_get_string(const token_t* json_token, const char* key_path, token_t* result);

/**
 * @brief Extract numeric value from JSON
 */
__attribute__((warn_unused_result))
result_t
json_get_number(const token_t* json_token, const char* key_path, double* result);

/**
 * @brief Extract boolean value from JSON
 */
__attribute__((warn_unused_result))
result_t
json_get_boolean(const token_t* json_token, const char* key_path, int* result);

/**
 * @brief Create JSON object from key-value pairs
 */
__attribute__((warn_unused_result))
result_t
json_create_object(token_t* result, const char* keys[], const char* values[], size_t count);

/**
 * @brief Create JSON array from string values
 */
__attribute__((warn_unused_result))
result_t
json_create_array(token_t* result, const char* values[], size_t count);

/**
 * @brief Escape string for JSON
 */
__attribute__((warn_unused_result))
result_t
json_escape_string(const char* input, token_t* result);

// ============================================================================
// Agent Management API
// ============================================================================

/**
 * @brief Initialize agent structure
 */
__attribute__((warn_unused_result))
result_t
lkjagent_init(lkjagent_t* agent);

/**
 * @brief Run agent main loop
 */
__attribute__((warn_unused_result))
result_t
lkjagent_run(lkjagent_t* agent);

#endif