/**
 * @file file_io.h
 * @brief File I/O operations interface for LKJAgent
 * 
 * This header provides safe, atomic file operations with comprehensive
 * error handling and backup mechanisms. All operations are designed to
 * prevent data corruption and handle concurrent access scenarios.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#ifndef LKJAGENT_FILE_IO_H
#define LKJAGENT_FILE_IO_H

#include "types.h"
#include "data.h"
#include <sys/stat.h>

/**
 * @defgroup File_IO File I/O Operations
 * @{
 */

/**
 * @brief Read entire file content into a data buffer
 * 
 * Safely reads the complete contents of a file with size limits and
 * comprehensive error handling. The buffer is always null-terminated.
 * 
 * @param filename Path to file to read
 * @param output Data buffer to store file contents
 * @param max_size Maximum file size to read (0 = no limit, but respects MAX_DATA_SIZE)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note If the file is larger than max_size, reading will fail with error
 * @note The output buffer will be cleared before reading
 * @note File content is treated as text and null-terminated
 * @note Empty files are successfully read as empty content
 * 
 * @warning filename parameter must not be NULL
 * @warning output parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t file_content;
 * data_init(&file_content, 1024);
 * if (file_read_all("/path/to/file.txt", &file_content, 0) == RESULT_OK) {
 *     printf("File content: %s\n", file_content.data);
 * }
 * data_destroy(&file_content);
 * @endcode
 */
result_t file_read_all(const char* filename, data_t* output, size_t max_size) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Write data to file with atomic operation and backup
 * 
 * Safely writes data to a file using atomic operations to prevent corruption.
 * Creates a backup of the existing file before writing if it exists.
 * 
 * @param filename Path to file to write
 * @param data Data buffer containing content to write
 * @param create_backup Whether to create backup of existing file
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Write operation is atomic - either completely succeeds or fails
 * @note If create_backup is true, existing file is backed up with .backup extension
 * @note Parent directories are created if they don't exist
 * @note File permissions are preserved if file already exists
 * 
 * @warning filename parameter must not be NULL
 * @warning data parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t content;
 * data_init(&content, 256);
 * data_set(&content, "New file content", 0);
 * if (file_write_atomic("/path/to/file.txt", &content, true) == RESULT_OK) {
 *     printf("File written successfully\n");
 * }
 * data_destroy(&content);
 * @endcode
 */
result_t file_write_atomic(const char* filename, const data_t* data, bool create_backup) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Check if file exists and is accessible
 * 
 * Checks whether a file exists and is readable. Does not follow symlinks
 * for security reasons.
 * 
 * @param filename Path to file to check
 * @return RESULT_OK if file exists and is accessible, RESULT_ERR otherwise
 * 
 * @note This function only checks for read accessibility
 * @note Returns RESULT_ERR for directories, symlinks, and special files
 * @note Does not check write permissions
 * 
 * @warning filename parameter must not be NULL
 * 
 * Example usage:
 * @code
 * if (file_exists("/path/to/config.json") == RESULT_OK) {
 *     // File exists and can be read
 *     printf("Configuration file found\n");
 * } else {
 *     printf("Configuration file not found or not accessible\n");
 * }
 * @endcode
 */
result_t file_exists(const char* filename) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Get file size in bytes
 * 
 * Retrieves the size of a file in bytes. Handles large files correctly
 * and validates that the file is a regular file.
 * 
 * @param filename Path to file to check
 * @param size Pointer to store file size
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Returns size of 0 for empty files
 * @note Fails for directories, symlinks, and special files
 * @note Size is returned as size_t, which may limit maximum file size on some systems
 * 
 * @warning filename parameter must not be NULL
 * @warning size parameter must not be NULL
 * 
 * Example usage:
 * @code
 * size_t file_size;
 * if (file_size("/path/to/data.json", &file_size) == RESULT_OK) {
 *     printf("File size: %zu bytes\n", file_size);
 * }
 * @endcode
 */
result_t file_size(const char* filename, size_t* size) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Create backup copy of a file
 * 
 * Creates a backup copy of the specified file with a backup extension.
 * The backup operation is atomic to prevent corruption.
 * 
 * @param filename Path to file to backup
 * @param backup_suffix Suffix to append to backup filename (NULL for default ".backup")
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note If backup_suffix is NULL, uses default ".backup" extension
 * @note Existing backup files are overwritten
 * @note File permissions and timestamps are preserved in backup
 * @note Operation is atomic - backup either succeeds completely or fails
 * 
 * @warning filename parameter must not be NULL
 * 
 * Example usage:
 * @code
 * if (file_backup("/path/to/important.json", NULL) == RESULT_OK) {
 *     printf("Backup created as important.json.backup\n");
 * }
 * 
 * if (file_backup("/path/to/config.txt", ".old") == RESULT_OK) {
 *     printf("Backup created as config.txt.old\n");
 * }
 * @endcode
 */
result_t file_backup(const char* filename, const char* backup_suffix) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Ensure directory exists, creating it if necessary
 * 
 * Creates the specified directory and all parent directories as needed.
 * Similar to "mkdir -p" but with proper error handling.
 * 
 * @param directory_path Path to directory to create
 * @param mode Permissions for created directories (e.g., 0755)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Creates parent directories recursively if they don't exist
 * @note If directory already exists, succeeds without error
 * @note All created directories receive the specified mode permissions
 * @note Uses umask for actual permissions (mode & ~umask)
 * 
 * @warning directory_path parameter must not be NULL
 * 
 * Example usage:
 * @code
 * if (file_ensure_directory("/path/to/deep/directory", 0755) == RESULT_OK) {
 *     printf("Directory structure created successfully\n");
 * }
 * @endcode
 */
result_t file_ensure_directory(const char* directory_path, mode_t mode) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Acquire exclusive lock on a file
 * 
 * Acquires an exclusive lock on the specified file to prevent concurrent
 * access. Uses file locking mechanisms appropriate for the platform.
 * 
 * @param filename Path to file to lock
 * @param lock_fd Pointer to store file descriptor for the lock
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note The lock is exclusive - only one process can hold it at a time
 * @note The lock is automatically released when the file descriptor is closed
 * @note Locks are advisory on most systems - cooperation is required
 * @note Non-blocking operation - fails immediately if lock cannot be acquired
 * 
 * @warning filename parameter must not be NULL
 * @warning lock_fd parameter must not be NULL
 * 
 * Example usage:
 * @code
 * int lock_fd;
 * if (file_lock("/path/to/lockfile", &lock_fd) == RESULT_OK) {
 *     // Perform protected operations
 *     file_unlock(lock_fd);
 * }
 * @endcode
 */
result_t file_lock(const char* filename, int* lock_fd) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Release exclusive lock on a file
 * 
 * Releases the exclusive lock acquired by file_lock and closes the
 * associated file descriptor.
 * 
 * @param lock_fd File descriptor for the lock to release
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Automatically closes the file descriptor
 * @note Safe to call multiple times on the same file descriptor
 * @note After this call, lock_fd becomes invalid
 * 
 * Example usage:
 * @code
 * int lock_fd;
 * if (file_lock("/path/to/lockfile", &lock_fd) == RESULT_OK) {
 *     // Perform protected operations
 *     file_unlock(lock_fd); // lock_fd is now invalid
 * }
 * @endcode
 */
result_t file_unlock(int lock_fd) __attribute__((warn_unused_result));

/**
 * @brief Get file modification time
 * 
 * Retrieves the last modification time of the specified file.
 * 
 * @param filename Path to file to check
 * @param mtime Pointer to store modification time
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Returns modification time as time_t (seconds since epoch)
 * @note Fails for non-existent files, directories, and special files
 * @note Time is in UTC
 * 
 * @warning filename parameter must not be NULL
 * @warning mtime parameter must not be NULL
 * 
 * Example usage:
 * @code
 * time_t mod_time;
 * if (file_get_mtime("/path/to/file.txt", &mod_time) == RESULT_OK) {
 *     printf("File modified: %s", ctime(&mod_time));
 * }
 * @endcode
 */
result_t file_get_mtime(const char* filename, time_t* mtime) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Check if file has been modified since specified time
 * 
 * Checks whether a file has been modified more recently than the
 * specified timestamp. Useful for change detection.
 * 
 * @param filename Path to file to check
 * @param reference_time Reference timestamp to compare against
 * @param is_newer Pointer to store result (true if file is newer)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Sets is_newer to true if file modification time > reference_time
 * @note Sets is_newer to false if file modification time <= reference_time
 * @note Fails if file doesn't exist or cannot be accessed
 * 
 * @warning filename parameter must not be NULL
 * @warning is_newer parameter must not be NULL
 * 
 * Example usage:
 * @code
 * time_t last_check = time(NULL) - 3600; // 1 hour ago
 * bool file_changed;
 * if (file_is_newer("/path/to/config.json", last_check, &file_changed) == RESULT_OK) {
 *     if (file_changed) {
 *         printf("Configuration file has been updated\n");
 *     }
 * }
 * @endcode
 */
result_t file_is_newer(const char* filename, time_t reference_time, bool* is_newer) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 3)));

/** @} */

#endif /* LKJAGENT_FILE_IO_H */
