/**
 * @file persist_memory.h
 * @brief Memory persistence interface for LKJAgent
 * 
 * This header provides memory persistence capabilities for the unified
 * memory storage system, including loading and saving memory layers
 * and context key directories with integrity checking and recovery.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#ifndef LKJAGENT_PERSIST_MEMORY_H
#define LKJAGENT_PERSIST_MEMORY_H

#include "types.h"
#include "data.h"

/**
 * @defgroup Memory_Persistence Memory Persistence Operations
 * @{
 */

/**
 * @brief Load unified memory storage from memory.json
 * 
 * Loads both working and disk memory layers from the unified memory.json
 * file format. Handles missing files gracefully by initializing empty memory.
 * 
 * @param filename Path to memory.json file
 * @param working_memory Data buffer to store working memory content
 * @param disk_memory Data buffer to store disk memory content
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note If file doesn't exist, both memory layers are initialized as empty
 * @note JSON structure is validated before parsing
 * @note Memory content is unescaped and ready for direct use
 * @note Both output buffers are cleared before loading
 * 
 * @warning filename parameter must not be NULL
 * @warning working_memory parameter must not be NULL and must be initialized
 * @warning disk_memory parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t working, disk;
 * data_init(&working, 1024);
 * data_init(&disk, 1024);
 * 
 * if (persist_memory_load("/path/to/memory.json", &working, &disk) == RESULT_OK) {
 *     printf("Working memory size: %zu\n", working.size);
 *     printf("Disk memory size: %zu\n", disk.size);
 * }
 * 
 * data_destroy(&working);
 * data_destroy(&disk);
 * @endcode
 */
result_t persist_memory_load(const char* filename, data_t* working_memory, data_t* disk_memory) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Save unified memory storage to memory.json
 * 
 * Saves both working and disk memory layers to the unified memory.json
 * file format using atomic operations to prevent corruption.
 * 
 * @param filename Path to memory.json file
 * @param working_memory Working memory content to save
 * @param disk_memory Disk memory content to save
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Write operation is atomic - either completely succeeds or fails
 * @note Creates backup of existing file before overwriting
 * @note Memory content is properly JSON-escaped
 * @note Parent directories are created if they don't exist
 * 
 * @warning filename parameter must not be NULL
 * @warning working_memory parameter must not be NULL (can be empty)
 * @warning disk_memory parameter must not be NULL (can be empty)
 * 
 * Example usage:
 * @code
 * data_t working, disk;
 * data_init(&working, 1024);
 * data_init(&disk, 1024);
 * 
 * data_set(&working, "Current session data", 0);
 * data_set(&disk, "Historical context", 0);
 * 
 * if (persist_memory_save("/path/to/memory.json", &working, &disk) == RESULT_OK) {
 *     printf("Memory saved successfully\n");
 * }
 * 
 * data_destroy(&working);
 * data_destroy(&disk);
 * @endcode
 */
result_t persist_memory_save(const char* filename, const data_t* working_memory, const data_t* disk_memory) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Load context key directory from context_keys.json
 * 
 * Loads context key metadata from the context_keys.json file, populating
 * an array of context_key_t structures with all metadata.
 * 
 * @param filename Path to context_keys.json file
 * @param context_keys Array to store loaded context keys
 * @param max_keys Maximum number of context keys to load
 * @param loaded_count Pointer to store number of keys actually loaded
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note If file doesn't exist, loaded_count is set to 0
 * @note Invalid or incomplete entries are skipped with warnings
 * @note All metadata fields are validated during loading
 * @note The context_keys array is filled with valid context_key_t structures
 * 
 * @warning filename parameter must not be NULL
 * @warning context_keys parameter must not be NULL if max_keys > 0
 * @warning loaded_count parameter must not be NULL
 * @warning max_keys must be greater than 0
 * 
 * Example usage:
 * @code
 * context_key_t keys[100];
 * size_t count;
 * 
 * if (persist_context_keys_load("/path/to/context_keys.json", keys, 100, &count) == RESULT_OK) {
 *     printf("Loaded %zu context keys\n", count);
 *     for (size_t i = 0; i < count; i++) {
 *         printf("Key: %s, Layer: %d, Score: %zu\n", 
 *                keys[i].key, keys[i].layer, keys[i].importance_score);
 *     }
 * }
 * @endcode
 */
result_t persist_context_keys_load(const char* filename, context_key_t* context_keys, size_t max_keys, size_t* loaded_count) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 4)));

/**
 * @brief Save context key directory to context_keys.json
 * 
 * Saves context key metadata to the context_keys.json file using atomic
 * operations and proper JSON formatting.
 * 
 * @param filename Path to context_keys.json file
 * @param context_keys Array of context keys to save
 * @param key_count Number of context keys in the array
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Write operation is atomic - either completely succeeds or fails
 * @note Creates backup of existing file before overwriting
 * @note All context keys are validated before saving
 * @note Parent directories are created if they don't exist
 * 
 * @warning filename parameter must not be NULL
 * @warning context_keys parameter must not be NULL if key_count > 0
 * 
 * Example usage:
 * @code
 * context_key_t keys[2];
 * strcpy(keys[0].key, "user_session");
 * keys[0].layer = LAYER_WORKING;
 * keys[0].importance_score = 85;
 * keys[0].last_accessed = time(NULL);
 * keys[0].data_size = 1024;
 * 
 * strcpy(keys[1].key, "historical_data");
 * keys[1].layer = LAYER_DISK;
 * keys[1].importance_score = 60;
 * keys[1].last_accessed = time(NULL) - 3600;
 * keys[1].data_size = 4096;
 * 
 * if (persist_context_keys_save("/path/to/context_keys.json", keys, 2) == RESULT_OK) {
 *     printf("Context keys saved successfully\n");
 * }
 * @endcode
 */
result_t persist_context_keys_save(const char* filename, const context_key_t* context_keys, size_t key_count) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Create backup of memory files
 * 
 * Creates backup copies of both memory.json and context_keys.json files
 * with timestamp-based naming for recovery purposes.
 * 
 * @param memory_filename Path to memory.json file
 * @param context_keys_filename Path to context_keys.json file
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Backup files are named with .backup extension by default
 * @note Only existing files are backed up (missing files are not errors)
 * @note Backup operations are atomic to prevent partial backups
 * @note Both backups must succeed for the operation to succeed
 * 
 * @warning memory_filename parameter must not be NULL
 * @warning context_keys_filename parameter must not be NULL
 * 
 * Example usage:
 * @code
 * if (persist_memory_backup("/path/to/memory.json", "/path/to/context_keys.json") == RESULT_OK) {
 *     printf("Memory files backed up successfully\n");
 * }
 * @endcode
 */
result_t persist_memory_backup(const char* memory_filename, const char* context_keys_filename) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Recover memory files from backup
 * 
 * Attempts to recover memory files from backup copies when corruption
 * is detected or original files are missing.
 * 
 * @param memory_filename Path to memory.json file
 * @param context_keys_filename Path to context_keys.json file
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Only recovers if backup files exist and are valid
 * @note Validates backup files before restoring
 * @note Original files are replaced only if backup validation succeeds
 * @note Partial recovery is possible (one file may succeed while other fails)
 * 
 * @warning memory_filename parameter must not be NULL
 * @warning context_keys_filename parameter must not be NULL
 * 
 * Example usage:
 * @code
 * if (persist_memory_recover("/path/to/memory.json", "/path/to/context_keys.json") == RESULT_OK) {
 *     printf("Memory files recovered from backup\n");
 * } else {
 *     printf("Recovery failed or no valid backups found\n");
 * }
 * @endcode
 */
result_t persist_memory_recover(const char* memory_filename, const char* context_keys_filename) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Validate memory file integrity
 * 
 * Performs integrity checking on memory files to detect corruption
 * or structural problems that could cause loading failures.
 * 
 * @param memory_filename Path to memory.json file
 * @param context_keys_filename Path to context_keys.json file
 * @param memory_valid Pointer to store memory file validation result
 * @param context_keys_valid Pointer to store context keys file validation result
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Validates JSON structure and required fields
 * @note Checks for reasonable data sizes and formats
 * @note Missing files are considered valid (will be created as needed)
 * @note Each file is validated independently
 * 
 * @warning memory_filename parameter must not be NULL
 * @warning context_keys_filename parameter must not be NULL
 * @warning memory_valid parameter must not be NULL
 * @warning context_keys_valid parameter must not be NULL
 * 
 * Example usage:
 * @code
 * bool memory_ok, keys_ok;
 * if (persist_memory_validate("/path/to/memory.json", "/path/to/context_keys.json", 
 *                            &memory_ok, &keys_ok) == RESULT_OK) {
 *     if (!memory_ok) {
 *         printf("Memory file is corrupted\n");
 *         // Attempt recovery or reinitialize
 *     }
 *     if (!keys_ok) {
 *         printf("Context keys file is corrupted\n");
 *         // Attempt recovery or reinitialize
 *     }
 * }
 * @endcode
 */
result_t persist_memory_validate(const char* memory_filename, const char* context_keys_filename, bool* memory_valid, bool* context_keys_valid) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3, 4)));

/**
 * @brief Initialize empty memory files
 * 
 * Creates new empty memory files with proper structure when they don't
 * exist or need to be reinitialized after corruption.
 * 
 * @param memory_filename Path to memory.json file
 * @param context_keys_filename Path to context_keys.json file
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Creates files with empty but valid JSON structure
 * @note Existing files are backed up before being replaced
 * @note Parent directories are created if they don't exist
 * @note Both files are created atomically
 * 
 * @warning memory_filename parameter must not be NULL
 * @warning context_keys_filename parameter must not be NULL
 * 
 * Example usage:
 * @code
 * if (persist_memory_initialize("/path/to/memory.json", "/path/to/context_keys.json") == RESULT_OK) {
 *     printf("Empty memory files initialized\n");
 * }
 * @endcode
 */
result_t persist_memory_initialize(const char* memory_filename, const char* context_keys_filename) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Compact and optimize memory files
 * 
 * Performs maintenance on memory files by removing unused entries,
 * optimizing structure, and defragmenting content.
 * 
 * @param memory_filename Path to memory.json file
 * @param context_keys_filename Path to context_keys.json file
 * @param cleanup_threshold Percentage threshold for cleanup (0-100)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Removes expired or low-importance context keys
 * @note Compacts memory content by removing redundant data
 * @note Creates backup before performing optimization
 * @note Only optimizes if cleanup threshold is exceeded
 * 
 * @warning memory_filename parameter must not be NULL
 * @warning context_keys_filename parameter must not be NULL
 * @warning cleanup_threshold must be between 0 and 100
 * 
 * Example usage:
 * @code
 * if (persist_memory_compact("/path/to/memory.json", "/path/to/context_keys.json", 80) == RESULT_OK) {
 *     printf("Memory files optimized successfully\n");
 * }
 * @endcode
 */
result_t persist_memory_compact(const char* memory_filename, const char* context_keys_filename, size_t cleanup_threshold) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/** @} */

#endif /* LKJAGENT_PERSIST_MEMORY_H */
