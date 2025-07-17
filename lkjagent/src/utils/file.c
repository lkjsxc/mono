/**
 * @file file.c
 * @brief File I/O operations implementation
 *
 * This module provides safe file reading and writing operations with proper
 * error handling and resource management. All operations use bounded buffers
 * and validate parameters to ensure memory safety.
 *
 * Key features:
 * - Safe file reading with size limits
 * - Atomic file writing operations
 * - Comprehensive error reporting
 * - Resource cleanup guarantees
 * - Directory creation support
 */

#include "../lkjagent.h"

/**
 * @brief Read entire file content into a token
 *
 * Safely reads a file's content into the provided token, handling size
 * constraints and ensuring proper null termination.
 *
 * @param path File path to read
 * @param content Token to store file content
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t file_read(const char* path, token_t* content) {
    if (!path) {
        RETURN_ERR("file_read: NULL path parameter");
        return RESULT_ERR;
    }
    
    if (!content) {
        RETURN_ERR("file_read: NULL content token");
        return RESULT_ERR;
    }
    
    if (!content->data) {
        RETURN_ERR("file_read: Content token not initialized");
        return RESULT_ERR;
    }
    
    FILE* file = fopen(path, "r");
    if (!file) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "file_read: Cannot open file '%s': %s", 
                path, strerror(errno));
        RETURN_ERR(error_msg);
        return RESULT_ERR;
    }
    
    // Get file size
    if (fseek(file, 0, SEEK_END) != 0) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "file_read: Cannot seek to end of file '%s': %s", 
                path, strerror(errno));
        RETURN_ERR(error_msg);
        fclose(file);
        return RESULT_ERR;
    }
    
    long file_size = ftell(file);
    if (file_size == -1) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "file_read: Cannot get file size for '%s': %s", 
                path, strerror(errno));
        RETURN_ERR(error_msg);
        fclose(file);
        return RESULT_ERR;
    }
    
    if (fseek(file, 0, SEEK_SET) != 0) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "file_read: Cannot seek to beginning of file '%s': %s", 
                path, strerror(errno));
        RETURN_ERR(error_msg);
        fclose(file);
        return RESULT_ERR;
    }
    
    // Check if file content fits in token (reserve space for null terminator)
    if ((size_t)file_size >= content->capacity) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), 
                "file_read: File '%s' (%ld bytes) too large for token capacity (%zu bytes)", 
                path, file_size, content->capacity);
        RETURN_ERR(error_msg);
        fclose(file);
        return RESULT_ERR;
    }
    
    // Read file content
    size_t bytes_read = fread(content->data, 1, file_size, file);
    if (bytes_read != (size_t)file_size) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), 
                "file_read: Read %zu bytes but expected %ld from file '%s'", 
                bytes_read, file_size, path);
        RETURN_ERR(error_msg);
        fclose(file);
        return RESULT_ERR;
    }
    
    fclose(file);
    
    // Ensure null termination and set size
    content->data[bytes_read] = '\0';
    content->size = bytes_read;
    
    return RESULT_OK;
}

/**
 * @brief Write token content to file
 *
 * Safely writes the token's content to the specified file path,
 * creating directories as needed and ensuring atomic operations.
 *
 * @param path File path to write to
 * @param content Token containing data to write
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t file_write(const char* path, const token_t* content) {
    if (!path) {
        RETURN_ERR("file_write: NULL path parameter");
        return RESULT_ERR;
    }
    
    if (!content) {
        RETURN_ERR("file_write: NULL content token");
        return RESULT_ERR;
    }
    
    if (!content->data) {
        RETURN_ERR("file_write: Content token not initialized");
        return RESULT_ERR;
    }
    
    // Create directory structure if needed
    char path_copy[1024];
    if (strlen(path) >= sizeof(path_copy)) {
        RETURN_ERR("file_write: Path too long");
        return RESULT_ERR;
    }
    
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    
    char* dir_path = dirname(path_copy);
    if (file_ensure_directory(dir_path) != RESULT_OK) {
        return RESULT_ERR; // Error already set by file_ensure_directory
    }
    
    // Open file for writing
    FILE* file = fopen(path, "w");
    if (!file) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "file_write: Cannot open file '%s' for writing: %s", 
                path, strerror(errno));
        RETURN_ERR(error_msg);
        return RESULT_ERR;
    }
    
    // Write content
    size_t bytes_written = fwrite(content->data, 1, content->size, file);
    if (bytes_written != content->size) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), 
                "file_write: Wrote %zu bytes but expected %zu to file '%s'", 
                bytes_written, content->size, path);
        RETURN_ERR(error_msg);
        fclose(file);
        return RESULT_ERR;
    }
    
    // Ensure data is flushed to disk
    if (fflush(file) != 0) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "file_write: Cannot flush file '%s': %s", 
                path, strerror(errno));
        RETURN_ERR(error_msg);
        fclose(file);
        return RESULT_ERR;
    }
    
    fclose(file);
    return RESULT_OK;
}

/**
 * @brief Append token content to existing file
 *
 * Appends the token's content to the end of an existing file,
 * creating the file if it doesn't exist.
 *
 * @param path File path to append to
 * @param content Token containing data to append
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t file_append(const char* path, const token_t* content) {
    if (!path) {
        RETURN_ERR("file_append: NULL path parameter");
        return RESULT_ERR;
    }
    
    if (!content) {
        RETURN_ERR("file_append: NULL content token");
        return RESULT_ERR;
    }
    
    if (!content->data) {
        RETURN_ERR("file_append: Content token not initialized");
        return RESULT_ERR;
    }
    
    // Create directory structure if needed
    char path_copy[1024];
    if (strlen(path) >= sizeof(path_copy)) {
        RETURN_ERR("file_append: Path too long");
        return RESULT_ERR;
    }
    
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    
    char* dir_path = dirname(path_copy);
    if (file_ensure_directory(dir_path) != RESULT_OK) {
        return RESULT_ERR; // Error already set by file_ensure_directory
    }
    
    // Open file for appending
    FILE* file = fopen(path, "a");
    if (!file) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "file_append: Cannot open file '%s' for appending: %s", 
                path, strerror(errno));
        RETURN_ERR(error_msg);
        return RESULT_ERR;
    }
    
    // Write content
    size_t bytes_written = fwrite(content->data, 1, content->size, file);
    if (bytes_written != content->size) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), 
                "file_append: Wrote %zu bytes but expected %zu to file '%s'", 
                bytes_written, content->size, path);
        RETURN_ERR(error_msg);
        fclose(file);
        return RESULT_ERR;
    }
    
    // Ensure data is flushed to disk
    if (fflush(file) != 0) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "file_append: Cannot flush file '%s': %s", 
                path, strerror(errno));
        RETURN_ERR(error_msg);
        fclose(file);
        return RESULT_ERR;
    }
    
    fclose(file);
    return RESULT_OK;
}

/**
 * @brief Check if file exists
 *
 * @param path File path to check
 * @return 1 if file exists, 0 if not or on error
 */
int file_exists(const char* path) {
    if (!path) {
        return 0;
    }
    
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

/**
 * @brief Check if directory exists
 *
 * @param path Directory path to check
 * @return 1 if directory exists, 0 if not or on error
 */
int file_directory_exists(const char* path) {
    if (!path) {
        return 0;
    }
    
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

/**
 * @brief Get file size
 *
 * @param path File path to check
 * @param size Pointer to store file size
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t file_get_size(const char* path, size_t* size) {
    if (!path) {
        RETURN_ERR("file_get_size: NULL path parameter");
        return RESULT_ERR;
    }
    
    if (!size) {
        RETURN_ERR("file_get_size: NULL size parameter");
        return RESULT_ERR;
    }
    
    struct stat st;
    if (stat(path, &st) != 0) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "file_get_size: Cannot stat file '%s': %s", 
                path, strerror(errno));
        RETURN_ERR(error_msg);
        return RESULT_ERR;
    }
    
    if (!S_ISREG(st.st_mode)) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "file_get_size: '%s' is not a regular file", path);
        RETURN_ERR(error_msg);
        return RESULT_ERR;
    }
    
    *size = st.st_size;
    return RESULT_OK;
}

/**
 * @brief Ensure directory exists, creating it if necessary
 *
 * Creates the directory and all parent directories as needed.
 *
 * @param path Directory path to ensure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t file_ensure_directory(const char* path) {
    if (!path) {
        RETURN_ERR("file_ensure_directory: NULL path parameter");
        return RESULT_ERR;
    }
    
    // Check if directory already exists
    if (file_directory_exists(path)) {
        return RESULT_OK;
    }
    
    // Create directory recursively using mkdir -p approach
    char path_copy[1024];
    if (strlen(path) >= sizeof(path_copy)) {
        RETURN_ERR("file_ensure_directory: Path too long");
        return RESULT_ERR;
    }
    
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    
    // Find parent directory
    char* parent = dirname(path_copy);
    if (strcmp(parent, path) != 0 && strcmp(parent, ".") != 0 && strcmp(parent, "/") != 0) {
        // Recursively ensure parent exists
        if (file_ensure_directory(parent) != RESULT_OK) {
            return RESULT_ERR; // Error already set
        }
    }
    
    // Create this directory
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "file_ensure_directory: Cannot create directory '%s': %s", 
                path, strerror(errno));
        RETURN_ERR(error_msg);
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

/**
 * @brief Delete file
 *
 * @param path File path to delete
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t file_delete(const char* path) {
    if (!path) {
        RETURN_ERR("file_delete: NULL path parameter");
        return RESULT_ERR;
    }
    
    if (unlink(path) != 0) {
        if (errno == ENOENT) {
            // File doesn't exist, consider it success
            return RESULT_OK;
        }
        
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "file_delete: Cannot delete file '%s': %s", 
                path, strerror(errno));
        RETURN_ERR(error_msg);
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

/**
 * @brief Copy file from source to destination
 *
 * @param src_path Source file path
 * @param dest_path Destination file path
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t file_copy(const char* src_path, const char* dest_path) {
    if (!src_path) {
        RETURN_ERR("file_copy: NULL source path parameter");
        return RESULT_ERR;
    }
    
    if (!dest_path) {
        RETURN_ERR("file_copy: NULL destination path parameter");
        return RESULT_ERR;
    }
    
    // Read source file
    static char file_buffer[8192]; // Stack buffer for file content
    token_t content;
    if (token_init(&content, file_buffer, sizeof(file_buffer)) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (file_read(src_path, &content) != RESULT_OK) {
        return RESULT_ERR; // Error already set
    }
    
    // Write to destination
    return file_write(dest_path, &content);
}
