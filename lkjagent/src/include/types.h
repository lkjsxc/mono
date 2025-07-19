/**
 * @file types.h
 * @brief Core type definitions for LKJAgent system
 * 
 * This header defines all fundamental types, enums, and structures used
 * throughout the LKJAgent autonomous AI agent system. All type definitions
 * are designed for robustness, type safety, and clear semantics.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#ifndef LKJAGENT_TYPES_H
#define LKJAGENT_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/**
 * @defgroup Core_Types Core Type Definitions
 * @{
 */

/**
 * @brief Result type for all function operations
 * 
 * This enum provides a consistent return type for all functions that can fail.
 * Functions returning this type should use the WARN_UNUSED_RESULT attribute
 * to ensure proper error handling.
 */
typedef enum {
    /** Operation completed successfully */
    RESULT_OK = 0,
    /** Operation failed with error (details in RETURN_ERR) */
    RESULT_ERR = 1
} result_t;

/**
 * @brief Agent execution states
 * 
 * Defines the four primary states of the LKJAgent autonomous operation cycle.
 * Each state has associated system prompts and specific behavioral patterns.
 */
typedef enum {
    /** Agent is analyzing situation and planning actions */
    STATE_THINKING = 0,
    /** Agent is executing planned actions */
    STATE_EXECUTING = 1,
    /** Agent is evaluating results and outcomes */
    STATE_EVALUATING = 2,
    /** Agent is managing memory context and paging */
    STATE_PAGING = 3
} agent_state_t;

/**
 * @brief Memory layer types for unified storage
 * 
 * Defines the different layers of the memory hierarchy, each with
 * different access patterns and persistence characteristics.
 */
typedef enum {
    /** Active working memory - fastest access, limited size */
    LAYER_WORKING = 0,
    /** Disk-based memory - slower access, larger capacity */
    LAYER_DISK = 1,
    /** Archived memory - slowest access, unlimited capacity */
    LAYER_ARCHIVED = 2
} memory_layer_t;

/**
 * @brief Dynamic string/data buffer structure
 * 
 * Provides safe, bounds-checked string and binary data management.
 * All operations on this structure maintain null termination and
 * prevent buffer overflows.
 */
typedef struct {
    /** Pointer to data buffer (always null-terminated for strings) */
    char* data;
    /** Current used size of data (excluding null terminator) */
    size_t size;
    /** Total allocated capacity of buffer */
    size_t capacity;
} data_t;

/**
 * @brief Context key structure for memory management
 * 
 * Represents a context key used by the LLM to direct memory paging
 * and context management operations.
 */
typedef struct {
    /** Context key name (null-terminated string) */
    char key[64];  /* MAX_TAG_SIZE from main header */
    /** Memory layer where this context resides */
    memory_layer_t layer;
    /** Importance score for paging decisions (0-100) */
    size_t importance_score;
    /** Timestamp of last access */
    time_t last_accessed;
    /** Size of data associated with this key */
    size_t data_size;
} context_key_t;

/** @} */

/**
 * @defgroup Function_Pointer_Types Function Pointer Types
 * @{
 */

/**
 * @brief State handler function pointer type
 * 
 * Functions of this type handle state-specific processing for each
 * agent state (thinking, executing, evaluating, paging).
 * 
 * @param agent Pointer to the main agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
typedef result_t (*state_handler_fn)(void* agent);

/**
 * @brief Memory callback function pointer type
 * 
 * Functions of this type are called when memory operations complete,
 * allowing for asynchronous memory management notifications.
 * 
 * @param context User-provided context pointer
 * @param result Result of the memory operation
 */
typedef void (*memory_callback_fn)(void* context, result_t result);

/**
 * @brief Progress callback function pointer type
 * 
 * Functions of this type are called to report progress during
 * long-running operations like LLM requests or file operations.
 * 
 * @param context User-provided context pointer
 * @param current Current progress value
 * @param total Total progress value (current/total = percentage)
 */
typedef void (*progress_callback_fn)(void* context, size_t current, size_t total);

/** @} */

/**
 * @defgroup Type_Validation Type Validation Macros
 * @{
 */

/**
 * @brief Check if result indicates success
 */
#define RESULT_IS_OK(r) ((r) == RESULT_OK)

/**
 * @brief Check if result indicates error
 */
#define RESULT_IS_ERR(r) ((r) == RESULT_ERR)

/**
 * @brief Check if agent state is valid
 */
#define STATE_IS_VALID(s) ((s) >= STATE_THINKING && (s) <= STATE_PAGING)

/**
 * @brief Check if memory layer is valid
 */
#define LAYER_IS_VALID(l) ((l) >= LAYER_WORKING && (l) <= LAYER_ARCHIVED)

/**
 * @brief Check if data_t structure is valid
 */
#define DATA_IS_VALID(d) \
    ((d) != NULL && (d)->data != NULL && (d)->size <= (d)->capacity && \
     ((d)->size == 0 || (d)->data[(d)->size] == '\0'))

/**
 * @brief Check if context key is valid
 */
#define CONTEXT_KEY_IS_VALID(k) \
    ((k) != NULL && (k)->key[0] != '\0' && LAYER_IS_VALID((k)->layer) && \
     (k)->importance_score <= 100)

/** @} */

/**
 * @defgroup Forward_Declarations Forward Declarations
 * @{
 */

/* Forward declarations for main structures defined in lkjagent.h */
struct lkjagent;
struct tagged_memory;
struct config;

/* Type aliases for forward-declared structures */
typedef struct lkjagent lkjagent_t;
typedef struct tagged_memory tagged_memory_t;
typedef struct config config_t;

/** @} */

#endif /* LKJAGENT_TYPES_H */
