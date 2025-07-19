/**
 * @file data.h
 * @brief Data buffer management interface for LKJAgent
 * 
 * This header provides the interface for safe, bounds-checked data buffer
 * operations. All functions maintain null termination and prevent buffer
 * overflows through comprehensive validation and error handling.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#ifndef LKJAGENT_DATA_H
#define LKJAGENT_DATA_H

#include "types.h"

/**
 * @defgroup Data_Management Data Buffer Management
 * @{
 */

/**
 * @brief Initialize a data_t structure with initial capacity
 * 
 * Allocates memory for the data buffer and initializes all fields.
 * The buffer is always null-terminated and ready for string operations.
 * 
 * @param data Pointer to data_t structure to initialize
 * @param initial_capacity Initial capacity for the buffer (minimum 1)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note The actual allocated capacity may be larger than requested for alignment
 * @note After successful initialization, data->size will be 0 and data->data[0] will be '\0'
 * 
 * @warning data parameter must not be NULL
 * @warning initial_capacity must be greater than 0
 * 
 * Example usage:
 * @code
 * data_t buffer;
 * if (data_init(&buffer, 1024) != RESULT_OK) {
 *     // Handle initialization failure
 *     return RESULT_ERR;
 * }
 * // buffer is now ready for use
 * data_clear(&buffer); // Clean up when done
 * @endcode
 */
result_t data_init(data_t* data, size_t initial_capacity) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Set data buffer content from a string with bounds checking
 * 
 * Safely copies the source string into the data buffer, reallocating
 * if necessary. The buffer is always null-terminated after this operation.
 * 
 * @param data Pointer to data_t structure to modify
 * @param source Source string to copy (null-terminated)
 * @param max_size Maximum size limit for the operation (0 = no limit)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note If source is longer than max_size, it will be truncated
 * @note The buffer will be reallocated if necessary to fit the source
 * @note After successful operation, data->size equals strlen(source) (or max_size if truncated)
 * 
 * @warning data parameter must not be NULL and must be initialized
 * @warning source parameter must not be NULL
 * 
 * Example usage:
 * @code
 * data_t buffer;
 * data_init(&buffer, 64);
 * if (data_set(&buffer, "Hello, World!", 0) != RESULT_OK) {
 *     // Handle set failure
 * }
 * printf("Buffer contains: %s\n", buffer.data);
 * data_clear(&buffer);
 * @endcode
 */
result_t data_set(data_t* data, const char* source, size_t max_size) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Append string to existing data buffer with bounds checking
 * 
 * Safely appends the source string to the existing buffer content,
 * reallocating if necessary. The buffer remains null-terminated.
 * 
 * @param data Pointer to data_t structure to modify
 * @param source Source string to append (null-terminated)
 * @param max_total_size Maximum total size after append (0 = no limit)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note If the result would exceed max_total_size, source will be truncated
 * @note The buffer will be reallocated if necessary to fit the appended data
 * @note After successful operation, data->size equals old_size + strlen(source) (or less if truncated)
 * 
 * @warning data parameter must not be NULL and must be initialized
 * @warning source parameter must not be NULL
 * 
 * Example usage:
 * @code
 * data_t buffer;
 * data_init(&buffer, 64);
 * data_set(&buffer, "Hello", 0);
 * if (data_append(&buffer, ", World!", 0) != RESULT_OK) {
 *     // Handle append failure
 * }
 * printf("Buffer contains: %s\n", buffer.data); // "Hello, World!"
 * data_clear(&buffer);
 * @endcode
 */
result_t data_append(data_t* data, const char* source, size_t max_total_size) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Remove characters from the front of the buffer
 * 
 * Removes the specified number of characters from the beginning of the buffer,
 * shifting remaining content to the front. Used for context window management.
 * 
 * @param data Pointer to data_t structure to modify
 * @param chars_to_remove Number of characters to remove from front
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note If chars_to_remove >= data->size, the buffer will be emptied
 * @note The buffer remains null-terminated after this operation
 * @note Memory is not reallocated; capacity remains unchanged
 * 
 * @warning data parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t buffer;
 * data_init(&buffer, 64);
 * data_set(&buffer, "Hello, World!", 0);
 * data_trim_front(&buffer, 7); // Remove "Hello, "
 * printf("Buffer contains: %s\n", buffer.data); // "World!"
 * data_clear(&buffer);
 * @endcode
 */
result_t data_trim_front(data_t* data, size_t chars_to_remove) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Trim buffer to fit within LLM context size limits
 * 
 * Intelligently trims the buffer content to fit within the specified
 * context size, attempting to preserve important content and maintain
 * structural integrity where possible.
 * 
 * @param data Pointer to data_t structure to modify
 * @param max_context_size Maximum size for LLM context window
 * @param preserve_suffix_size Number of characters to preserve at end (0 = smart trimming)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note If preserve_suffix_size > 0, that many characters from the end are guaranteed to be preserved
 * @note If preserve_suffix_size == 0, the function uses smart trimming to preserve structure
 * @note The buffer is trimmed from the middle, keeping beginning and end portions when possible
 * 
 * @warning data parameter must not be NULL and must be initialized
 * @warning max_context_size must be greater than preserve_suffix_size
 * 
 * Example usage:
 * @code
 * data_t buffer;
 * data_init(&buffer, 1024);
 * data_set(&buffer, very_long_string, 0);
 * if (data_trim_context(&buffer, 512, 100) != RESULT_OK) {
 *     // Handle trimming failure
 * }
 * // Buffer is now at most 512 characters with last 100 preserved
 * data_clear(&buffer);
 * @endcode
 */
result_t data_trim_context(data_t* data, size_t max_context_size, size_t preserve_suffix_size) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Clear buffer content and reset to empty state
 * 
 * Clears the buffer content and resets size to 0 while preserving
 * the allocated capacity. The buffer remains valid and ready for reuse.
 * 
 * @param data Pointer to data_t structure to clear
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note After successful operation, data->size will be 0 and data->data[0] will be '\0'
 * @note Capacity and allocated memory are preserved for efficient reuse
 * @note This operation cannot fail under normal circumstances
 * 
 * @warning data parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t buffer;
 * data_init(&buffer, 64);
 * data_set(&buffer, "Some content", 0);
 * data_clear(&buffer); // Buffer is now empty but ready for reuse
 * data_set(&buffer, "New content", 0); // Efficient reuse
 * @endcode
 */
result_t data_clear(data_t* data) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Create a deep copy of a data buffer
 * 
 * Creates an independent copy of the source data buffer, including
 * allocating new memory and copying all content.
 * 
 * @param dest Pointer to destination data_t structure (will be initialized)
 * @param source Pointer to source data_t structure to copy
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note dest will be initialized by this function; do not initialize beforehand
 * @note The destination capacity will match the source size (optimized allocation)
 * @note Both buffers are independent after this operation
 * 
 * @warning dest parameter must not be NULL and must not be initialized
 * @warning source parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t original, copy;
 * data_init(&original, 64);
 * data_set(&original, "Original content", 0);
 * if (data_copy(&copy, &original) != RESULT_OK) {
 *     // Handle copy failure
 * }
 * // copy now contains independent copy of original
 * data_clear(&original);
 * data_clear(&copy);
 * @endcode
 */
result_t data_copy(data_t* dest, const data_t* source) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Validate data buffer integrity and consistency
 * 
 * Performs comprehensive validation of the data buffer structure,
 * checking for memory corruption, invalid pointers, and inconsistent state.
 * 
 * @param data Pointer to data_t structure to validate
 * @return RESULT_OK if valid, RESULT_ERR if invalid or corrupted
 * 
 * @note This function checks: null pointers, size <= capacity, null termination, memory accessibility
 * @note Should be used in debug builds and when corruption is suspected
 * @note Has minimal performance impact and can be called frequently
 * 
 * @warning data parameter must not be NULL
 * 
 * Example usage:
 * @code
 * data_t buffer;
 * data_init(&buffer, 64);
 * assert(data_validate(&buffer) == RESULT_OK); // Should always pass after init
 * // ... perform operations ...
 * if (data_validate(&buffer) != RESULT_OK) {
 *     // Buffer corruption detected
 *     RETURN_ERR("Data buffer validation failed");
 *     return RESULT_ERR;
 * }
 * @endcode
 */
result_t data_validate(const data_t* data) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Free all memory associated with data buffer
 * 
 * Releases all allocated memory and resets the structure to a safe state.
 * After this operation, the data_t structure must be reinitialized before use.
 * 
 * @param data Pointer to data_t structure to destroy
 * 
 * @note After this operation, all fields will be set to safe values (NULL/0)
 * @note This function is safe to call multiple times on the same structure
 * @note The data_t structure itself is not freed, only its internal memory
 * 
 * @warning data parameter must not be NULL
 * @warning The structure must not be used after this call without reinitialization
 * 
 * Example usage:
 * @code
 * data_t buffer;
 * data_init(&buffer, 64);
 * data_set(&buffer, "Some content", 0);
 * data_destroy(&buffer); // All memory freed, buffer now invalid
 * // buffer must be reinitialized before next use
 * data_init(&buffer, 32); // Now valid again
 * @endcode
 */
void data_destroy(data_t* data) __attribute__((nonnull(1)));

/** @} */

#endif /* LKJAGENT_DATA_H */
