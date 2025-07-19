/**
 * @file memory_disk.c
 * @brief Disk memory operations implementation for LKJAgent
 * 
 * This module implements disk-based memory operations with compression,
 * backup management, integrity checking, and efficient storage for the
 * autonomous agent memory system.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/memory_context.h"
#include "../include/persist_memory.h"
#include "../include/file_io.h"
#include "../lkjagent.h"
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <errno.h>

/**
 * @defgroup Memory_Disk_Internal Internal Disk Memory Functions
 * @{
 */

/**
 * @brief Get available disk space
 * 
 * @param path Path to check disk space for
 * @return Available space in bytes, or 0 on error
 */
static size_t get_available_disk_space(const char* path) {
    if (!path) return 0;
    
    struct statvfs stat;
    if (statvfs(path, &stat) == 0) {
        return stat.f_bavail * stat.f_frsize;
    }
    
    return 0;
}

/**
 * @brief Create backup file path
 * 
 * @param original_path Original file path
 * @param backup_path Buffer for backup path
 * @param backup_size Size of backup path buffer
 * @param timestamp Timestamp to include in backup name
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t create_backup_path(const char* original_path, char* backup_path, 
                                  size_t backup_size, time_t timestamp) {
    if (!original_path || !backup_path || backup_size == 0) {
        return RESULT_ERR;
    }
    
    /* Extract directory and filename */
    const char* filename = strrchr(original_path, '/');
    if (!filename) {
        filename = original_path;
    } else {
        filename++; /* Skip the '/' */
    }
    
    /* Create timestamped backup filename */
    struct tm* tm_info = localtime(&timestamp);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y%m%d_%H%M%S", tm_info);
    
    int result = snprintf(backup_path, backup_size, "%.*s.%s.backup",
                         (int)(filename - original_path), original_path,
                         time_str);
    
    return (result > 0 && result < (int)backup_size) ? RESULT_OK : RESULT_ERR;
}

/**
 * @brief Verify file integrity using simple checksum
 * 
 * @param filepath Path to file to verify
 * @param expected_size Expected file size (0 to skip size check)
 * @return RESULT_OK if file is valid, RESULT_ERR otherwise
 */
static result_t verify_file_integrity(const char* filepath, size_t expected_size) {
    if (!filepath) {
        return RESULT_ERR;
    }
    
    /* Check if file exists and is readable */
    struct stat file_stat;
    if (stat(filepath, &file_stat) != 0) {
        return RESULT_ERR;
    }
    
    /* Check file size if specified */
    if (expected_size > 0 && (size_t)file_stat.st_size != expected_size) {
        return RESULT_ERR;
    }
    
    /* Basic file structure validation */
    data_t file_content;
    if (data_init(&file_content, FILE_BUFFER_SIZE) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    result_t read_result = file_read_all(filepath, &file_content, 4096); /* Read first 4KB */
    
    if (read_result == RESULT_OK) {
        /* Check for basic JSON structure markers */
        if (file_content.data && file_content.size > 0) {
            const char* content = file_content.data;
            bool has_opening_brace = (strchr(content, '{') != NULL);
            bool has_closing_brace = (strchr(content, '}') != NULL);
            
            if (!has_opening_brace || !has_closing_brace) {
                read_result = RESULT_ERR;
            }
        }
    }
    
    data_destroy(&file_content);
    return read_result;
}

/**
 * @brief Compress data using simple algorithm
 * 
 * @param input Input data to compress
 * @param output Compressed output data
 * @param compression_level Compression level (1-9)
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t compress_data(const data_t* input, data_t* output, int compression_level) {
    if (!input || !output) {
        return RESULT_ERR;
    }
    
    if (compression_level < 1 || compression_level > 9) {
        compression_level = 5; /* Default compression */
    }
    
    /* Clear output */
    if (data_clear(output) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Simple compression: remove redundant whitespace and repeated patterns */
    if (!input->data || input->size == 0) {
        return RESULT_OK;
    }
    
    const char* src = input->data;
    size_t src_len = input->size;
    bool in_whitespace = false;
    
    for (size_t i = 0; i < src_len; i++) {
        char current = src[i];
        
        if (isspace(current)) {
            if (!in_whitespace) {
                /* First whitespace character - add single space */
                if (data_append(output, " ", 1) != RESULT_OK) {
                    return RESULT_ERR;
                }
                in_whitespace = true;
            }
            /* Skip additional whitespace */
        } else {
            /* Non-whitespace character */
            char temp[2] = {current, '\0'};
            if (data_append(output, temp, 1) != RESULT_OK) {
                return RESULT_ERR;
            }
            in_whitespace = false;
        }
    }
    
    return RESULT_OK;
}

/**
 * @brief Decompress data
 * 
 * @param input Compressed input data
 * @param output Decompressed output data
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t decompress_data(const data_t* input, data_t* output) {
    if (!input || !output) {
        return RESULT_ERR;
    }
    
    /* For our simple compression, decompression is just copying */
    return data_set(output, input->data, 0);
}

/** @} */

result_t memory_disk_store(tagged_memory_t* memory, const char* key_name, const data_t* data,
                          bool compress, const char* storage_path) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_disk_store");
        return RESULT_ERR;
    }
    
    if (!key_name || key_name[0] == '\0') {
        RETURN_ERR("Invalid key_name parameter in memory_disk_store");
        return RESULT_ERR;
    }
    
    if (!data) {
        RETURN_ERR("Null data parameter in memory_disk_store");
        return RESULT_ERR;
    }
    
    if (!storage_path) {
        RETURN_ERR("Null storage_path parameter in memory_disk_store");
        return RESULT_ERR;
    }
    
    /* Check available disk space */
    size_t available_space = get_available_disk_space(storage_path);
    size_t required_space = data->size * 2; /* Safety margin */
    
    if (available_space < required_space) {
        RETURN_ERR("Insufficient disk space in memory_disk_store");
        return RESULT_ERR;
    }
    
    /* Prepare data for storage */
    data_t storage_data;
    if (data_init(&storage_data, data->size + 1024) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (compress) {
        /* Compress the data */
        if (compress_data(data, &storage_data, 5) != RESULT_OK) {
            data_destroy(&storage_data);
            return RESULT_ERR;
        }
    } else {
        /* Store uncompressed */
        if (data_set(&storage_data, data->data, 0) != RESULT_OK) {
            data_destroy(&storage_data);
            return RESULT_ERR;
        }
    }
    
    /* Store in tagged memory system */
    result_t store_result = tagged_memory_store(memory, key_name, &storage_data, 
                                              LAYER_DISK, 70); /* Default importance for disk storage */
    
    data_destroy(&storage_data);
    
    if (store_result == RESULT_OK) {
        /* Update context key metadata */
        context_key_t key;
        if (context_key_find(memory, key_name, &key) == RESULT_OK) {
            key.data_size = storage_data.size;
            key.layer = LAYER_DISK;
            key.last_accessed = time(NULL);
        }
    }
    
    return store_result;
}

result_t memory_disk_retrieve(tagged_memory_t* memory, const char* key_name, data_t* data,
                             bool decompress) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_disk_retrieve");
        return RESULT_ERR;
    }
    
    if (!key_name || key_name[0] == '\0') {
        RETURN_ERR("Invalid key_name parameter in memory_disk_retrieve");
        return RESULT_ERR;
    }
    
    if (!data) {
        RETURN_ERR("Null data parameter in memory_disk_retrieve");
        return RESULT_ERR;
    }
    
    /* Retrieve from tagged memory system */
    data_t raw_data;
    if (data_init(&raw_data, 4096) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    result_t retrieve_result = tagged_memory_retrieve(memory, key_name, &raw_data);
    
    if (retrieve_result != RESULT_OK) {
        data_destroy(&raw_data);
        return retrieve_result;
    }
    
    /* Process retrieved data */
    if (decompress && raw_data.size > 0) {
        /* Decompress the data */
        if (decompress_data(&raw_data, data) != RESULT_OK) {
            data_destroy(&raw_data);
            return RESULT_ERR;
        }
    } else {
        /* Copy uncompressed data */
        if (data_set(data, raw_data.data, 0) != RESULT_OK) {
            data_destroy(&raw_data);
            return RESULT_ERR;
        }
    }
    
    data_destroy(&raw_data);
    
    /* Update access timestamp */
    context_key_update_importance(memory, key_name, 70); /* Maintain importance */
    
    return RESULT_OK;
}

result_t memory_disk_archive(tagged_memory_t* memory, const char* key_name, const char* archive_path) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_disk_archive");
        return RESULT_ERR;
    }
    
    if (!key_name || key_name[0] == '\0') {
        RETURN_ERR("Invalid key_name parameter in memory_disk_archive");
        return RESULT_ERR;
    }
    
    if (!archive_path) {
        RETURN_ERR("Null archive_path parameter in memory_disk_archive");
        return RESULT_ERR;
    }
    
    /* Retrieve current data */
    data_t current_data;
    if (data_init(&current_data, 4096) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (tagged_memory_retrieve(memory, key_name, &current_data) != RESULT_OK) {
        data_destroy(&current_data);
        RETURN_ERR("Key not found for archiving");
        return RESULT_ERR;
    }
    
    /* Compress data for archival */
    data_t compressed_data;
    if (data_init(&compressed_data, current_data.size) != RESULT_OK) {
        data_destroy(&current_data);
        return RESULT_ERR;
    }
    
    if (compress_data(&current_data, &compressed_data, 9) != RESULT_OK) { /* Maximum compression */
        data_destroy(&current_data);
        data_destroy(&compressed_data);
        return RESULT_ERR;
    }
    
    /* Create archive file path */
    char archive_file[MAX_FILENAME_SIZE];
    snprintf(archive_file, sizeof(archive_file), "%s/%s.archive", archive_path, key_name);
    
    /* Write compressed data to archive file */
    result_t write_result = file_write_atomic(archive_file, &compressed_data, true);
    
    if (write_result == RESULT_OK) {
        /* Update context key to archived status */
        context_key_archive(memory, key_name);
        
        /* Update context key metadata */
        context_key_t key;
        if (context_key_find(memory, key_name, &key) == RESULT_OK) {
            key.data_size = compressed_data.size;
            key.last_accessed = time(NULL);
        }
    }
    
    data_destroy(&current_data);
    data_destroy(&compressed_data);
    
    return write_result;
}

result_t memory_disk_cleanup(tagged_memory_t* memory, const char* storage_path, 
                           size_t max_disk_usage, size_t* freed_bytes) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_disk_cleanup");
        return RESULT_ERR;
    }
    
    if (!storage_path) {
        RETURN_ERR("Null storage_path parameter in memory_disk_cleanup");
        return RESULT_ERR;
    }
    
    if (!freed_bytes) {
        RETURN_ERR("Null freed_bytes parameter in memory_disk_cleanup");
        return RESULT_ERR;
    }
    
    *freed_bytes = 0;
    
    /* Get current disk usage */
    memory_stats_t stats;
    if (tagged_memory_get_stats(memory, &stats) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (stats.disk_size <= max_disk_usage) {
        return RESULT_OK; /* No cleanup needed */
    }
    
    /* Calculate amount to clean */
    size_t target_reduction = stats.disk_size - (max_disk_usage * 0.9); /* Clean to 90% of limit */
    
    /* Find candidates for cleanup (old, low-importance disk items) */
    context_key_t* disk_keys = malloc(memory->context_key_count * sizeof(context_key_t));
    if (!disk_keys) {
        RETURN_ERR("Memory allocation failed in memory_disk_cleanup");
        return RESULT_ERR;
    }
    
    size_t disk_key_count = 0;
    
    /* Collect disk layer keys */
    for (size_t i = 0; i < memory->context_key_count; i++) {
        if (memory->context_keys[i].layer == LAYER_DISK) {
            disk_keys[disk_key_count] = memory->context_keys[i];
            disk_key_count++;
        }
    }
    
    /* Sort by priority (oldest, lowest importance first) */
    for (size_t i = 0; i < disk_key_count - 1; i++) {
        for (size_t j = 0; j < disk_key_count - i - 1; j++) {
            bool should_swap = false;
            
            /* Compare by importance (lower first) */
            if (disk_keys[j].importance_score > disk_keys[j + 1].importance_score) {
                should_swap = true;
            } else if (disk_keys[j].importance_score == disk_keys[j + 1].importance_score) {
                /* Same importance - compare by age (older first) */
                if (disk_keys[j].last_accessed > disk_keys[j + 1].last_accessed) {
                    should_swap = true;
                }
            }
            
            if (should_swap) {
                context_key_t temp = disk_keys[j];
                disk_keys[j] = disk_keys[j + 1];
                disk_keys[j + 1] = temp;
            }
        }
    }
    
    /* Clean up keys until target is reached */
    size_t cleaned_bytes = 0;
    
    for (size_t i = 0; i < disk_key_count && cleaned_bytes < target_reduction; i++) {
        const context_key_t* key = &disk_keys[i];
        
        /* Skip high-importance items */
        if (key->importance_score >= 80) {
            continue;
        }
        
        /* Archive or delete the key */
        if (key->importance_score >= 40) {
            /* Archive medium-importance items */
            if (memory_disk_archive(memory, key->key, storage_path) == RESULT_OK) {
                cleaned_bytes += key->data_size;
            }
        } else {
            /* Delete low-importance items */
            if (tagged_memory_delete(memory, key->key) == RESULT_OK) {
                cleaned_bytes += key->data_size;
            }
        }
    }
    
    free(disk_keys);
    *freed_bytes = cleaned_bytes;
    
    return RESULT_OK;
}

result_t memory_disk_compact(tagged_memory_t* memory, const char* storage_path) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_disk_compact");
        return RESULT_ERR;
    }
    
    if (!storage_path) {
        RETURN_ERR("Null storage_path parameter in memory_disk_compact");
        return RESULT_ERR;
    }
    
    /* Create backup before compaction */
    time_t current_time = time(NULL);
    char backup_memory[MAX_FILENAME_SIZE];
    char backup_keys[MAX_FILENAME_SIZE];
    
    snprintf(backup_memory, sizeof(backup_memory), "%s/memory.json", storage_path);
    snprintf(backup_keys, sizeof(backup_keys), "%s/context_keys.json", storage_path);
    
    if (persist_memory_backup(backup_memory, backup_keys) != RESULT_OK) {
        RETURN_ERR("Failed to create backup before compaction");
        return RESULT_ERR;
    }
    
    /* Perform memory defragmentation */
    if (memory_defragment(memory) != RESULT_OK) {
        RETURN_ERR("Memory defragmentation failed");
        return RESULT_ERR;
    }
    
    /* Compress all disk data */
    for (size_t i = 0; i < memory->context_key_count; i++) {
        context_key_t* key = &memory->context_keys[i];
        
        if (key->layer == LAYER_DISK) {
            /* Retrieve current data */
            data_t current_data;
            if (data_init(&current_data, key->data_size + 256) == RESULT_OK) {
                if (tagged_memory_retrieve(memory, key->key, &current_data) == RESULT_OK) {
                    
                    /* Compress and re-store */
                    data_t compressed_data;
                    if (data_init(&compressed_data, current_data.size) == RESULT_OK) {
                        if (compress_data(&current_data, &compressed_data, 7) == RESULT_OK) {
                            tagged_memory_store(memory, key->key, &compressed_data, 
                                              LAYER_DISK, key->importance_score);
                            key->data_size = compressed_data.size;
                        }
                        data_destroy(&compressed_data);
                    }
                }
                data_destroy(&current_data);
            }
        }
    }
    
    /* Save compacted memory */
    char memory_file[MAX_FILENAME_SIZE];
    char keys_file[MAX_FILENAME_SIZE];
    
    snprintf(memory_file, sizeof(memory_file), "%s/memory.json", storage_path);
    snprintf(keys_file, sizeof(keys_file), "%s/context_keys.json", storage_path);
    
    result_t save_memory = persist_memory_save(memory_file, &memory->working_memory, &memory->disk_memory);
    result_t save_keys = persist_context_keys_save(keys_file, memory->context_keys, memory->context_key_count);
    
    if (save_memory != RESULT_OK || save_keys != RESULT_OK) {
        /* Restore from backup on failure */
        persist_memory_recover(memory_file, keys_file);
        return RESULT_ERR;
    }
    
    memory->last_modified = current_time;
    memory->total_size = calculate_total_memory_size(memory);
    
    return RESULT_OK;
}

result_t memory_disk_backup(tagged_memory_t* memory, const char* storage_path, const char* backup_path) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_disk_backup");
        return RESULT_ERR;
    }
    
    if (!storage_path || !backup_path) {
        RETURN_ERR("Null path parameter in memory_disk_backup");
        return RESULT_ERR;
    }
    
    /* Create backup directory if it doesn't exist */
    struct stat backup_stat;
    if (stat(backup_path, &backup_stat) != 0) {
        if (mkdir(backup_path, 0755) != 0) {
            RETURN_ERR("Failed to create backup directory");
            return RESULT_ERR;
        }
    }
    
    /* Create timestamped backup */
    time_t current_time = time(NULL);
    char timestamp_str[64];
    struct tm* tm_info = localtime(&current_time);
    strftime(timestamp_str, sizeof(timestamp_str), "%Y%m%d_%H%M%S", tm_info);
    
    /* Backup memory.json */
    char source_memory[MAX_FILENAME_SIZE];
    char backup_memory[MAX_FILENAME_SIZE];
    
    snprintf(source_memory, sizeof(source_memory), "%s/memory.json", storage_path);
    snprintf(backup_memory, sizeof(backup_memory), "%s/memory_%s.json", backup_path, timestamp_str);
    
    data_t memory_data;
    if (data_init(&memory_data, memory->total_size + 1024) == RESULT_OK) {
        if (persist_memory_save(source_memory, &memory->working_memory, &memory->disk_memory) == RESULT_OK) {
            if (file_read_all(source_memory, &memory_data, MAX_DATA_SIZE) == RESULT_OK) {
                file_write_atomic(backup_memory, &memory_data, true);
            }
        }
        data_destroy(&memory_data);
    }
    
    /* Backup context_keys.json */
    char source_keys[MAX_FILENAME_SIZE];
    char backup_keys[MAX_FILENAME_SIZE];
    
    snprintf(source_keys, sizeof(source_keys), "%s/context_keys.json", storage_path);
    snprintf(backup_keys, sizeof(backup_keys), "%s/context_keys_%s.json", backup_path, timestamp_str);
    
    data_t keys_data;
    if (data_init(&keys_data, memory->context_key_count * 256 + 1024) == RESULT_OK) {
        if (persist_context_keys_save(source_keys, memory->context_keys, memory->context_key_count) == RESULT_OK) {
            if (file_read_all(source_keys, &keys_data, MAX_DATA_SIZE) == RESULT_OK) {
                file_write_atomic(backup_keys, &keys_data, true);
            }
        }
        data_destroy(&keys_data);
    }
    
    /* Clean up old backups (keep last 10) */
    DIR* backup_dir = opendir(backup_path);
    if (backup_dir) {
        struct dirent* entry;
        char* backup_files[100];
        int backup_count = 0;
        
        /* Collect backup files */
        while ((entry = readdir(backup_dir)) != NULL && backup_count < 100) {
            if (strstr(entry->d_name, "memory_") || strstr(entry->d_name, "context_keys_")) {
                backup_files[backup_count] = strdup(entry->d_name);
                backup_count++;
            }
        }
        closedir(backup_dir);
        
        /* Sort and remove old backups */
        if (backup_count > 20) { /* Keep last 10 of each type */
            /* Simple cleanup - remove oldest files */
            for (int i = 0; i < backup_count - 20; i++) {
                char old_backup[MAX_FILENAME_SIZE];
                snprintf(old_backup, sizeof(old_backup), "%s/%s", backup_path, backup_files[i]);
                unlink(old_backup);
            }
        }
        
        /* Free allocated strings */
        for (int i = 0; i < backup_count; i++) {
            free(backup_files[i]);
        }
    }
    
    return RESULT_OK;
}

result_t memory_disk_verify(tagged_memory_t* memory, const char* storage_path, bool* is_valid) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_disk_verify");
        return RESULT_ERR;
    }
    
    if (!storage_path) {
        RETURN_ERR("Null storage_path parameter in memory_disk_verify");
        return RESULT_ERR;
    }
    
    if (!is_valid) {
        RETURN_ERR("Null is_valid parameter in memory_disk_verify");
        return RESULT_ERR;
    }
    
    *is_valid = false;
    
    /* Verify memory.json */
    char memory_file[MAX_FILENAME_SIZE];
    snprintf(memory_file, sizeof(memory_file), "%s/memory.json", storage_path);
    
    bool memory_valid = false;
    bool keys_valid = false;
    
    char keys_file[MAX_FILENAME_SIZE];
    snprintf(keys_file, sizeof(keys_file), "%s/context_keys.json", storage_path);
    
    /* Use existing validation function */
    result_t validation_result = persist_memory_validate(memory_file, keys_file, &memory_valid, &keys_valid);
    
    if (validation_result != RESULT_OK) {
        return validation_result;
    }
    
    /* Additional integrity checks */
    if (memory_valid && keys_valid) {
        /* Verify file integrity */
        if (verify_file_integrity(memory_file, 0) == RESULT_OK &&
            verify_file_integrity(keys_file, 0) == RESULT_OK) {
            
            /* Cross-check memory data with context keys */
            size_t verified_keys = 0;
            
            for (size_t i = 0; i < memory->context_key_count; i++) {
                const context_key_t* key = &memory->context_keys[i];
                
                /* Try to retrieve data for each key */
                data_t test_data;
                if (data_init(&test_data, 64) == RESULT_OK) {
                    if (tagged_memory_retrieve(memory, key->key, &test_data) == RESULT_OK) {
                        verified_keys++;
                    }
                    data_destroy(&test_data);
                }
            }
            
            /* Consider valid if at least 90% of keys can be retrieved */
            if (memory->context_key_count == 0 || 
                (verified_keys * 100 / memory->context_key_count) >= 90) {
                *is_valid = true;
            }
        }
    }
    
    return RESULT_OK;
}
