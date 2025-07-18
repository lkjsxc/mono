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
 * @brief LMStudio configuration
 */
typedef struct {
    token_t base_url;       /**< LMStudio API base URL */
    token_t model;          /**< Model name */
    double temperature;     /**< Temperature parameter */
    int max_tokens;         /**< Maximum tokens */
    int timeout_ms;         /**< Timeout in milliseconds */
} lmstudio_config_t;

/**
 * @brief Tagged memory configuration
 */
typedef struct {
    int max_entries;                /**< Maximum memory entries */
    int max_tags_per_entry;         /**< Maximum tags per entry */
    double auto_cleanup_threshold;  /**< Auto cleanup threshold */
    double tag_similarity_threshold; /**< Tag similarity threshold */
} tagged_memory_config_t;

/**
 * @brief LLM decisions configuration
 */
typedef struct {
    double confidence_threshold; /**< Confidence threshold */
    int decision_timeout_ms;     /**< Decision timeout in milliseconds */
    int fallback_enabled;        /**< Fallback enabled flag */
    int context_window_size;     /**< Context window size */
} llm_decisions_config_t;

/**
 * @brief Enhanced tools configuration
 */
typedef struct {
    int tool_chaining_enabled;    /**< Tool chaining enabled flag */
    int max_tool_chain_length;    /**< Maximum tool chain length */
    int parallel_tool_execution;  /**< Parallel tool execution flag */
} enhanced_tools_config_t;

/**
 * @brief Agent configuration
 */
typedef struct {
    int max_iterations;                     /**< Maximum iterations */
    int self_directed;                      /**< Self-directed flag */
    token_t system_prompt;                  /**< System prompt */
    tagged_memory_config_t tagged_memory;   /**< Tagged memory configuration */
    llm_decisions_config_t llm_decisions;   /**< LLM decisions configuration */
    enhanced_tools_config_t enhanced_tools; /**< Enhanced tools configuration */
} agent_config_t;

/**
 * @brief HTTP configuration
 */
typedef struct {
    int timeout_seconds; /**< Timeout in seconds */
    int max_redirects;   /**< Maximum redirects */
    token_t user_agent;  /**< User agent string */
} http_config_t;

/**
 * @brief Complete application configuration
 */
typedef struct {
    lmstudio_config_t lmstudio; /**< LMStudio configuration */
    agent_config_t agent;        /**< Agent configuration */
    http_config_t http;          /**< HTTP configuration */
} config_t;

/**
 * @brief Agent working memory (RAM-based)
 */
typedef struct {
    token_t system_prompt;       /**< Fixed behavioral guidelines */
    token_t current_state;       /**< Current operational state */
    token_t task_goal;           /**< Final objective to achieve */
    token_t plan;                /**< Step-by-step execution strategy */
    token_t scratchpad;          /**< Temporary notes and results */
    token_t recent_history;      /**< Log of recent activities */
    token_t retrieved_from_disk; /**< Knowledge fetched from persistent storage */
} agent_memory_t;

/**
 * @brief Persistent memory metadata
 */
typedef struct {
    token_t version;        /**< Memory format version */
    token_t created;        /**< Creation timestamp */
    token_t last_modified;  /**< Last modification timestamp */
} memory_metadata_t;

/**
 * @brief Main agent structure
 */
typedef struct {
    token_t config_path;      /**< Path to configuration file */
    token_t memory_path;      /**< Path to memory.json file */
    config_t config;          /**< Application configuration */
    agent_memory_t memory;    /**< Working memory (RAM) */
    memory_metadata_t metadata; /**< Memory metadata */
    // Add more fields as needed
} lkjagent_t;

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
 * @brief Extract object value from JSON
 */
__attribute__((warn_unused_result))
result_t
json_get_object(const token_t* json_token, const char* key, token_t* result);

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
// Configuration Management API
// ============================================================================

/**
 * @brief Initialize configuration with default values
 */
__attribute__((warn_unused_result))
result_t
config_init(config_t* config);

/**
 * @brief Load configuration from JSON file
 */
__attribute__((warn_unused_result))
result_t
config_load_from_file(config_t* config, const char* file_path);

/**
 * @brief Load configuration from JSON token
 */
__attribute__((warn_unused_result))
result_t
config_load_from_json(config_t* config, const token_t* json_token);

/**
 * @brief Save configuration to JSON file
 */
__attribute__((warn_unused_result))
result_t
config_save_to_file(const config_t* config, const char* file_path);

/**
 * @brief Convert configuration to JSON
 */
__attribute__((warn_unused_result))
result_t
config_to_json(const config_t* config, token_t* json_token);

/**
 * @brief Free configuration resources
 */
void config_cleanup(config_t* config);

/**
 * @brief Validate configuration values
 */
__attribute__((warn_unused_result))
result_t
config_validate(const config_t* config);

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

// ============================================================================
// Memory Management API
// ============================================================================

/**
 * @brief Initialize agent memory with static buffers
 */
__attribute__((warn_unused_result))
result_t
agent_memory_init(agent_memory_t* memory, char buffers[][2048], size_t num_buffers);

/**
 * @brief Load persistent memory from memory.json file
 */
__attribute__((warn_unused_result))
result_t
agent_memory_load_from_file(lkjagent_t* agent, const char* file_path);

/**
 * @brief Save current agent memory to persistent storage
 */
__attribute__((warn_unused_result))
result_t
agent_memory_save_to_file(const lkjagent_t* agent, const char* file_path);

/**
 * @brief Clear working memory but preserve metadata
 */
__attribute__((warn_unused_result))
result_t
agent_memory_clear_working(agent_memory_t* memory);

/**
 * @brief Update memory metadata with current timestamp
 */
__attribute__((warn_unused_result))
result_t
agent_memory_update_metadata(memory_metadata_t* metadata);

/**
 * @brief Validate memory structure and content
 */
__attribute__((warn_unused_result))
result_t
agent_memory_validate(const agent_memory_t* memory);

/**
 * @brief Add a log entry to memory with timestamp
 */
__attribute__((warn_unused_result))
result_t
agent_memory_add_log_entry(agent_memory_t* memory, const char* state, 
                           const char* action, const char* details);

/**
 * @brief Update task goal in memory
 */
__attribute__((warn_unused_result))
result_t
agent_memory_update_task_goal(agent_memory_t* memory, const char* new_goal);

/**
 * @brief Update current state in memory
 */
__attribute__((warn_unused_result))
result_t
agent_memory_update_state(agent_memory_t* memory, const char* new_state);

/**
 * @brief Append to scratchpad with automatic formatting
 */
__attribute__((warn_unused_result))
result_t
agent_memory_append_scratchpad(agent_memory_t* memory, const char* content, const char* prefix);

/**
 * @brief Get memory usage statistics
 */
__attribute__((warn_unused_result))
result_t
agent_memory_get_stats(const agent_memory_t* memory, size_t* total_used, 
                       size_t* total_capacity, double* utilization_percent);

#endif