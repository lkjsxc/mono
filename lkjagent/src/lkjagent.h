/**
 * @file lkjagent.h
 * @brief Main header file for LKJAgent - Autonomous AI Agent System
 * 
 * This is the primary header file that includes all core definitions and
 * component headers for the LKJAgent system. It provides the complete
 * public API and type definitions for autonomous agent operations.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#ifndef LKJAGENT_H
#define LKJAGENT_H

/* Standard library includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Core type definitions */
#include "include/types.h"

/* Component headers */
#include "include/data.h"
#include "include/tag_parser.h"
#include "include/file_io.h"
#include "include/json_parser.h"
#include "include/json_builder.h"
#include "include/config_loader.h"
#include "include/persist_memory.h"

/**
 * @defgroup Constants Core System Constants
 * @{
 */

/** Maximum size for data buffers in bytes */
#define MAX_DATA_SIZE           (1024 * 1024)  /* 1MB */

/** Maximum filename length including path */
#define MAX_FILENAME_SIZE       256

/** Maximum tag name length for simple tag format */
#define MAX_TAG_SIZE            64

/** Maximum number of context keys to track */
#define MAX_CONTEXT_KEYS        1000

/** Maximum LLM response size */
#define MAX_LLM_RESPONSE_SIZE   (512 * 1024)   /* 512KB */

/** Maximum configuration value length */
#define MAX_CONFIG_VALUE_SIZE   512

/** Buffer size for file operations */
#define FILE_BUFFER_SIZE        8192

/** Backup file extension */
#define BACKUP_EXTENSION        ".backup"

/** @} */

/**
 * @defgroup Macros Core System Macros
 * @{
 */

#define RETURN_ERR(error_message)                                                   \
    do {                                                                            \
        _Pragma("GCC diagnostic push")                                              \
        _Pragma("GCC diagnostic ignored \"-Wunused-result\"")                       \
        write(STDERR_FILENO, "Error: { file: \"", 17);                              \
        write(STDERR_FILENO, __FILE__, strlen(__FILE__));                           \
        write(STDERR_FILENO, "\", func: \"", 11);                                   \
        write(STDERR_FILENO, __func__, strlen(__func__));                           \
        write(STDERR_FILENO, "\", line: ", 10);                                     \
        /* Convert line number to string */                                         \
        char line_str[32];                                                          \
        int line_len = snprintf(line_str, sizeof(line_str), "%d", __LINE__);       \
        write(STDERR_FILENO, line_str, line_len);                                   \
        write(STDERR_FILENO, "\", message: \"", 13);                                \
        write(STDERR_FILENO, error_message, strlen(error_message));                 \
        write(STDERR_FILENO, "\"}\n", 4);                                           \
        _Pragma("GCC diagnostic pop")                                               \
    } while(0)

/**
 * @brief Macro to check if a pointer is valid
 */
#define VALID_PTR(ptr) ((ptr) != NULL)

/**
 * @brief Macro to check if a string is valid and non-empty
 */
#define VALID_STR(str) (VALID_PTR(str) && (str)[0] != '\0')

/**
 * @brief Macro to get the minimum of two values
 */
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * @brief Macro to get the maximum of two values
 */
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * @brief Macro to safely calculate string length with bounds checking
 */
#define SAFE_STRLEN(str, max_len) \
    (VALID_PTR(str) ? strnlen(str, max_len) : 0)

/** @} */

/**
 * @defgroup Forward_Declarations Forward Type Declarations
 * @{
 */

/* Forward declarations for main structures */
typedef struct lkjagent lkjagent_t;
typedef struct tagged_memory tagged_memory_t;
typedef struct config config_t;

/** @} */

/**
 * @defgroup Core_Structures Core System Structures
 * @{
 */

/**
 * @brief Main LKJAgent structure
 * 
 * This structure contains the complete state of the autonomous agent,
 * including current execution state, memory system, configuration,
 * and operational metrics.
 */
struct lkjagent {
    /** Current agent execution state */
    agent_state_t state;
    
    /** Agent configuration */
    config_t* config;
    
    /** Tagged memory system */
    tagged_memory_t* memory;
    
    /** Current working directory for file operations */
    char working_dir[MAX_FILENAME_SIZE];
    
    /** Agent start time for operational metrics */
    time_t start_time;
    
    /** Total number of state transitions */
    uint64_t state_transitions;
    
    /** Last error message */
    char last_error[MAX_CONFIG_VALUE_SIZE];
    
    /** Flag indicating if agent should continue running */
    bool running;
};

/**
 * @brief Tagged memory system structure
 * 
 * This structure manages the unified memory storage with context keys
 * and LLM-directed paging capabilities.
 */
struct tagged_memory {
    /** Working memory layer data */
    data_t working_memory;
    
    /** Disk memory layer data */
    data_t disk_memory;
    
    /** Context keys array */
    context_key_t context_keys[MAX_CONTEXT_KEYS];
    
    /** Number of active context keys */
    size_t context_key_count;
    
    /** Memory modification timestamp */
    time_t last_modified;
    
    /** Memory access statistics */
    uint64_t access_count;
    
    /** Memory size statistics */
    size_t total_size;
};

/**
 * @brief Configuration structure
 * 
 * This structure contains all configuration parameters for the agent,
 * including LLM settings, memory configuration, and state-specific prompts.
 */
struct config {
    /** LLM API endpoint URL */
    char llm_endpoint[MAX_CONFIG_VALUE_SIZE];
    
    /** LLM model name */
    char llm_model[MAX_CONFIG_VALUE_SIZE];
    
    /** LLM API key (if required) */
    char llm_api_key[MAX_CONFIG_VALUE_SIZE];
    
    /** Maximum context window size for LLM */
    size_t llm_max_context;
    
    /** LLM request timeout in seconds */
    int llm_timeout;
    
    /** Memory management settings */
    size_t memory_max_working_size;
    size_t memory_max_disk_size;
    size_t memory_cleanup_threshold;
    
    /** State-specific system prompts */
    data_t thinking_prompt;
    data_t executing_prompt;
    data_t evaluating_prompt;
    data_t paging_prompt;
    
    /** Configuration file modification time */
    time_t config_mtime;
    
    /** Configuration validation flag */
    bool is_valid;
};

/** @} */

/**
 * @defgroup Function_Attributes Function Attributes
 * @{
 */

/**
 * @brief Attribute to warn when function return value is not used
 * 
 * This attribute should be applied to all functions that return result_t
 * to ensure proper error handling throughout the system.
 */
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))

/**
 * @brief Attribute for functions that should not return NULL
 */
#define RETURNS_NONNULL __attribute__((returns_nonnull))

/**
 * @brief Attribute for functions that require non-NULL parameters
 */
#define NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))

/** @} */

#endif /* LKJAGENT_H */
