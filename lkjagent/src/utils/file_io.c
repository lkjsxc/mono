/**
 * @file file_io.c
 * @brief File I/O operations implementation for LKJAgent
 * 
 * This module provides safe, atomic file operations with comprehensive
 * error handling and backup mechanisms. All operations prevent data
 * corruption and handle concurrent access scenarios.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/file_io.h"
#include "../lkjagent.h"
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>

/**
 * @defgroup File_IO_Internal Internal File I/O Functions
 * @{
 */

/**
 * @brief Create all parent directories for a given path
 * 
 * @param path Path containing directories to create
 * @param mode Permission mode for created directories
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t create_parent_directories(const char* path, mode_t mode) {
    if (!path) {
        RETURN_ERR("Null path in create_parent_directories");
        return RESULT_ERR;
    }
    
    /* Make a copy of the path for modification */
    char* path_copy = malloc(strlen(path) + 1);
    if (!path_copy) {
        RETURN_ERR("Memory allocation failed in create_parent_directories");
        return RESULT_ERR;
    }
    strcpy(path_copy, path);
    
    /* Get the directory part of the path */
    char* dir_path = dirname(path_copy);
    
    /* Check if directory already exists */
    struct stat st;
    if (stat(dir_path, &st) == 0) {
        free(path_copy);
        if (S_ISDIR(st.st_mode)) {
            return RESULT_OK;
        } else {
            RETURN_ERR("Path exists but is not a directory");
            return RESULT_ERR;
        }
    }
    
    /* Recursively create parent directories */
    if (create_parent_directories(dir_path, mode) != RESULT_OK) {
        free(path_copy);
        return RESULT_ERR;
    }
    
    /* Create this directory */
    if (mkdir(dir_path, mode) != 0 && errno != EEXIST) {
        free(path_copy);
        RETURN_ERR("Failed to create directory");
        return RESULT_ERR;
    }
    
    free(path_copy);
    return RESULT_OK;
}

/**
 * @brief Generate temporary filename for atomic operations
 * 
 * @param original_filename Original filename
 * @param temp_filename Buffer to store temporary filename
 * @param temp_size Size of temp_filename buffer
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t generate_temp_filename(const char* original_filename, char* temp_filename, size_t temp_size) {
    if (!original_filename || !temp_filename || temp_size == 0) {
        RETURN_ERR("Invalid parameters in generate_temp_filename");
        return RESULT_ERR;
    }
    
    int result = snprintf(temp_filename, temp_size, "%s.tmp.%d", original_filename, getpid());
    if (result < 0 || (size_t)result >= temp_size) {
        RETURN_ERR("Temporary filename too long");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

/** @} */

result_t file_read_all(const char* filename, data_t* output, size_t max_size) {
    if (!filename) {
        RETURN_ERR("Null filename in file_read_all");
        return RESULT_ERR;
    }
    
    if (!output) {
        RETURN_ERR("Null output buffer in file_read_all");
        return RESULT_ERR;
    }
    
    /* Clear output buffer */
    if (data_clear(output) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Open file for reading */
    FILE* file = fopen(filename, "rb");
    if (!file) {
        RETURN_ERR("Failed to open file for reading");
        return RESULT_ERR;
    }
    
    /* Get file size */
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        RETURN_ERR("Failed to seek to end of file");
        return RESULT_ERR;
    }
    
    long file_size_long = ftell(file);
    if (file_size_long < 0) {
        fclose(file);
        RETURN_ERR("Failed to get file size");
        return RESULT_ERR;
    }
    
    size_t file_size = (size_t)file_size_long;
    
    /* Check size limits */
    if (max_size > 0 && file_size > max_size) {
        fclose(file);
        RETURN_ERR("File size exceeds maximum allowed size");
        return RESULT_ERR;
    }
    
    if (file_size > MAX_DATA_SIZE) {
        fclose(file);
        RETURN_ERR("File size exceeds system maximum data size");
        return RESULT_ERR;
    }
    
    /* Seek back to beginning */
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        RETURN_ERR("Failed to seek to beginning of file");
        return RESULT_ERR;
    }
    
    /* Handle empty file */
    if (file_size == 0) {
        fclose(file);
        return RESULT_OK;
    }
    
    /* Ensure output buffer has sufficient capacity by setting with temporary string */
    char* temp_buffer = malloc(file_size + 1);
    if (!temp_buffer) {
        fclose(file);
        RETURN_ERR("Memory allocation failed for file content buffer");
        return RESULT_ERR;
    }
    
    /* Read file content into temporary buffer first */
    size_t bytes_read = fread(temp_buffer, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != file_size) {
        free(temp_buffer);
        RETURN_ERR("Failed to read complete file content");
        return RESULT_ERR;
    }
    
    /* Null-terminate the temporary buffer */
    temp_buffer[file_size] = '\0';
    
    /* Set the content using data_set which handles capacity management */
    result_t set_result = data_set(output, temp_buffer, 0);
    free(temp_buffer);
    
    return set_result;
}

result_t file_write_atomic(const char* filename, const data_t* data, bool create_backup) {
    if (!filename) {
        RETURN_ERR("Null filename in file_write_atomic");
        return RESULT_ERR;
    }
    
    if (!data) {
        RETURN_ERR("Null data buffer in file_write_atomic");
        return RESULT_ERR;
    }
    
    if (data_validate(data) != RESULT_OK) {
        RETURN_ERR("Invalid data buffer in file_write_atomic");
        return RESULT_ERR;
    }
    
    /* Create backup if requested and file exists */
    if (create_backup && file_exists(filename) == RESULT_OK) {
        if (file_backup(filename, NULL) != RESULT_OK) {
            RETURN_ERR("Failed to create backup before writing");
            return RESULT_ERR;
        }
    }
    
    /* Ensure parent directory exists */
    if (create_parent_directories(filename, 0755) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Generate temporary filename */
    char temp_filename[MAX_FILENAME_SIZE];
    if (generate_temp_filename(filename, temp_filename, sizeof(temp_filename)) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Open temporary file for writing */
    FILE* temp_file = fopen(temp_filename, "wb");
    if (!temp_file) {
        RETURN_ERR("Failed to open temporary file for writing");
        return RESULT_ERR;
    }
    
    /* Write data to temporary file */
    if (data->size > 0) {
        size_t bytes_written = fwrite(data->data, 1, data->size, temp_file);
        if (bytes_written != data->size) {
            fclose(temp_file);
            unlink(temp_filename); /* Clean up temporary file */
            RETURN_ERR("Failed to write complete data to temporary file");
            return RESULT_ERR;
        }
    }
    
    /* Flush and sync to ensure data is written to disk */
    if (fflush(temp_file) != 0) {
        fclose(temp_file);
        unlink(temp_filename);
        RETURN_ERR("Failed to flush temporary file");
        return RESULT_ERR;
    }
    
    if (fsync(fileno(temp_file)) != 0) {
        fclose(temp_file);
        unlink(temp_filename);
        RETURN_ERR("Failed to sync temporary file to disk");
        return RESULT_ERR;
    }
    
    fclose(temp_file);
    
    /* Atomically move temporary file to final location */
    if (rename(temp_filename, filename) != 0) {
        unlink(temp_filename); /* Clean up temporary file */
        RETURN_ERR("Failed to rename temporary file to final filename");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t file_exists(const char* filename) {
    if (!filename) {
        RETURN_ERR("Null filename in file_exists");
        return RESULT_ERR;
    }
    
    struct stat st;
    if (stat(filename, &st) != 0) {
        return RESULT_ERR;
    }
    
    /* Check if it's a regular file */
    if (!S_ISREG(st.st_mode)) {
        return RESULT_ERR;
    }
    
    /* Check if file is readable */
    if (access(filename, R_OK) != 0) {
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t file_size(const char* filename, size_t* size) {
    if (!filename) {
        RETURN_ERR("Null filename in file_size");
        return RESULT_ERR;
    }
    
    if (!size) {
        RETURN_ERR("Null size pointer in file_size");
        return RESULT_ERR;
    }
    
    struct stat st;
    if (stat(filename, &st) != 0) {
        RETURN_ERR("Failed to get file statistics");
        return RESULT_ERR;
    }
    
    /* Check if it's a regular file */
    if (!S_ISREG(st.st_mode)) {
        RETURN_ERR("Path is not a regular file");
        return RESULT_ERR;
    }
    
    /* Check for size overflow */
    if (st.st_size < 0) {
        RETURN_ERR("Invalid file size");
        return RESULT_ERR;
    }
    
    *size = (size_t)st.st_size;
    return RESULT_OK;
}

result_t file_backup(const char* filename, const char* backup_suffix) {
    if (!filename) {
        RETURN_ERR("Null filename in file_backup");
        return RESULT_ERR;
    }
    
    /* Check if source file exists */
    if (file_exists(filename) != RESULT_OK) {
        RETURN_ERR("Source file does not exist or is not accessible");
        return RESULT_ERR;
    }
    
    /* Use default suffix if none provided */
    const char* suffix = backup_suffix ? backup_suffix : BACKUP_EXTENSION;
    
    /* Generate backup filename */
    char backup_filename[MAX_FILENAME_SIZE];
    int result = snprintf(backup_filename, sizeof(backup_filename), "%s%s", filename, suffix);
    if (result < 0 || result >= (int)sizeof(backup_filename)) {
        RETURN_ERR("Backup filename too long");
        return RESULT_ERR;
    }
    
    /* Read source file */
    data_t file_content;
    if (data_init(&file_content, FILE_BUFFER_SIZE) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (file_read_all(filename, &file_content, 0) != RESULT_OK) {
        data_destroy(&file_content);
        return RESULT_ERR;
    }
    
    /* Write backup file */
    result_t write_result = file_write_atomic(backup_filename, &file_content, false);
    data_destroy(&file_content);
    
    return write_result;
}

result_t file_ensure_directory(const char* directory_path, mode_t mode) {
    if (!directory_path) {
        RETURN_ERR("Null directory_path in file_ensure_directory");
        return RESULT_ERR;
    }
    
    struct stat st;
    if (stat(directory_path, &st) == 0) {
        /* Path exists */
        if (S_ISDIR(st.st_mode)) {
            return RESULT_OK;
        } else {
            RETURN_ERR("Path exists but is not a directory");
            return RESULT_ERR;
        }
    }
    
    /* Directory doesn't exist, create it and all parents */
    return create_parent_directories(directory_path, mode);
}

result_t file_lock(const char* filename, int* lock_fd) {
    if (!filename) {
        RETURN_ERR("Null filename in file_lock");
        return RESULT_ERR;
    }
    
    if (!lock_fd) {
        RETURN_ERR("Null lock_fd pointer in file_lock");
        return RESULT_ERR;
    }
    
    /* Open or create lock file */
    int fd = open(filename, O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        RETURN_ERR("Failed to open lock file");
        return RESULT_ERR;
    }
    
    /* Try to acquire exclusive lock (non-blocking) */
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        close(fd);
        if (errno == EWOULDBLOCK) {
            RETURN_ERR("Lock is already held by another process");
        } else {
            RETURN_ERR("Failed to acquire file lock");
        }
        return RESULT_ERR;
    }
    
    *lock_fd = fd;
    return RESULT_OK;
}

result_t file_unlock(int lock_fd) {
    if (lock_fd < 0) {
        return RESULT_OK; /* Already unlocked or invalid */
    }
    
    /* Release lock and close file descriptor */
    if (flock(lock_fd, LOCK_UN) != 0) {
        close(lock_fd);
        RETURN_ERR("Failed to release file lock");
        return RESULT_ERR;
    }
    
    if (close(lock_fd) != 0) {
        RETURN_ERR("Failed to close lock file descriptor");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t file_get_mtime(const char* filename, time_t* mtime) {
    if (!filename) {
        RETURN_ERR("Null filename in file_get_mtime");
        return RESULT_ERR;
    }
    
    if (!mtime) {
        RETURN_ERR("Null mtime pointer in file_get_mtime");
        return RESULT_ERR;
    }
    
    struct stat st;
    if (stat(filename, &st) != 0) {
        RETURN_ERR("Failed to get file statistics");
        return RESULT_ERR;
    }
    
    /* Check if it's a regular file */
    if (!S_ISREG(st.st_mode)) {
        RETURN_ERR("Path is not a regular file");
        return RESULT_ERR;
    }
    
    *mtime = st.st_mtime;
    return RESULT_OK;
}

result_t file_is_newer(const char* filename, time_t reference_time, bool* is_newer) {
    if (!filename) {
        RETURN_ERR("Null filename in file_is_newer");
        return RESULT_ERR;
    }
    
    if (!is_newer) {
        RETURN_ERR("Null is_newer pointer in file_is_newer");
        return RESULT_ERR;
    }
    
    time_t file_mtime;
    if (file_get_mtime(filename, &file_mtime) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    *is_newer = (file_mtime > reference_time);
    return RESULT_OK;
}
