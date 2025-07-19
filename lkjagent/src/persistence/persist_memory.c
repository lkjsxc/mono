/**
 * @file persist_memory.c
 * @brief Memory persistence implementation for LKJAgent
 * 
 * This module provides memory persistence capabilities for the unified
 * memory storage system with comprehensive error handling, backup
 * mechanisms, and integrity checking.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/persist_memory.h"
#include "../include/file_io.h"
#include "../include/json_parser.h"
#include "../include/json_builder.h"
#include "../lkjagent.h"
#include <string.h>
#include <time.h>

/**
 * @defgroup Persist_Memory_Internal Internal Memory Persistence Functions
 * @{
 */

/**
 * @brief Check if memory file exists and is accessible
 * 
 * @param filename Path to memory file
 * @return true if file exists and is accessible, false otherwise
 */
static bool memory_file_exists(const char* filename) {
    return (file_exists(filename) == RESULT_OK);
}

/**
 * @brief Validate memory.json file structure
 * 
 * @param filename Path to memory.json file
 * @return RESULT_OK if valid, RESULT_ERR if invalid
 */
static result_t validate_memory_file(const char* filename) {
    if (!filename) {
        RETURN_ERR("Null filename in validate_memory_file");
        return RESULT_ERR;
    }
    
    if (!memory_file_exists(filename)) {
        /* Missing file is considered valid (will be created) */
        return RESULT_OK;
    }
    
    /* Read file content */
    data_t file_content;
    if (data_init(&file_content, FILE_BUFFER_SIZE) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (file_read_all(filename, &file_content, MAX_DATA_SIZE) != RESULT_OK) {
        data_destroy(&file_content);
        return RESULT_ERR;
    }
    
    /* Validate JSON structure */
    result_t result = json_validate_structure(file_content.data);
    
    if (result == RESULT_OK) {
        /* Check for required fields */
        data_t working_memory, disk_memory;
        if (data_init(&working_memory, 512) != RESULT_OK ||
            data_init(&disk_memory, 512) != RESULT_OK) {
            data_destroy(&file_content);
            return RESULT_ERR;
        }
        
        /* Try to parse memory format */
        if (json_parse_memory_format(file_content.data, &working_memory, &disk_memory) != RESULT_OK) {
            result = RESULT_ERR;
        }
        
        data_destroy(&working_memory);
        data_destroy(&disk_memory);
    }
    
    data_destroy(&file_content);
    return result;
}

/**
 * @brief Validate context_keys.json file structure
 * 
 * @param filename Path to context_keys.json file
 * @return RESULT_OK if valid, RESULT_ERR if invalid
 */
static result_t validate_context_keys_file(const char* filename) {
    if (!filename) {
        RETURN_ERR("Null filename in validate_context_keys_file");
        return RESULT_ERR;
    }
    
    if (!memory_file_exists(filename)) {
        /* Missing file is considered valid (will be created) */
        return RESULT_OK;
    }
    
    /* Read file content */
    data_t file_content;
    if (data_init(&file_content, FILE_BUFFER_SIZE) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (file_read_all(filename, &file_content, MAX_DATA_SIZE) != RESULT_OK) {
        data_destroy(&file_content);
        return RESULT_ERR;
    }
    
    /* Validate JSON structure */
    result_t result = json_validate_structure(file_content.data);
    
    if (result == RESULT_OK) {
        /* Try to parse context keys format */
        context_key_t keys[10]; /* Small test array */
        size_t count;
        if (json_parse_context_keys_format(file_content.data, keys, 10, &count) != RESULT_OK) {
            result = RESULT_ERR;
        }
    }
    
    data_destroy(&file_content);
    return result;
}

/** @} */

result_t persist_memory_load(const char* filename, data_t* working_memory, data_t* disk_memory) {
    if (!filename) {
        RETURN_ERR("Null filename in persist_memory_load");
        return RESULT_ERR;
    }
    
    if (!working_memory || !disk_memory) {
        RETURN_ERR("Null memory buffer in persist_memory_load");
        return RESULT_ERR;
    }
    
    /* Clear output buffers */
    if (data_clear(working_memory) != RESULT_OK || data_clear(disk_memory) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Check if file exists */
    if (!memory_file_exists(filename)) {
        /* File doesn't exist, return empty memory */
        return RESULT_OK;
    }
    
    /* Read file content */
    data_t file_content;
    if (data_init(&file_content, FILE_BUFFER_SIZE) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (file_read_all(filename, &file_content, MAX_DATA_SIZE) != RESULT_OK) {
        data_destroy(&file_content);
        return RESULT_ERR;
    }
    
    /* Parse memory format */
    result_t result = json_parse_memory_format(file_content.data, working_memory, disk_memory);
    data_destroy(&file_content);
    
    return result;
}

result_t persist_memory_save(const char* filename, const data_t* working_memory, const data_t* disk_memory) {
    if (!filename) {
        RETURN_ERR("Null filename in persist_memory_save");
        return RESULT_ERR;
    }
    
    if (!working_memory || !disk_memory) {
        RETURN_ERR("Null memory buffer in persist_memory_save");
        return RESULT_ERR;
    }
    
    /* Validate memory buffers */
    if (data_validate(working_memory) != RESULT_OK || data_validate(disk_memory) != RESULT_OK) {
        RETURN_ERR("Invalid memory buffer in persist_memory_save");
        return RESULT_ERR;
    }
    
    /* Build memory JSON */
    data_t memory_json;
    if (data_init(&memory_json, working_memory->size + disk_memory->size + 1024) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Use empty strings for null data */
    const char* working_str = working_memory->data ? working_memory->data : "";
    const char* disk_str = disk_memory->data ? disk_memory->data : "";
    
    if (json_build_memory(working_str, disk_str, &memory_json) != RESULT_OK) {
        data_destroy(&memory_json);
        return RESULT_ERR;
    }
    
    /* Write to file atomically */
    result_t result = file_write_atomic(filename, &memory_json, true);
    data_destroy(&memory_json);
    
    return result;
}

result_t persist_context_keys_load(const char* filename, context_key_t* context_keys, size_t max_keys, size_t* loaded_count) {
    if (!filename) {
        RETURN_ERR("Null filename in persist_context_keys_load");
        return RESULT_ERR;
    }
    
    if (!loaded_count) {
        RETURN_ERR("Null loaded_count in persist_context_keys_load");
        return RESULT_ERR;
    }
    
    if (max_keys > 0 && !context_keys) {
        RETURN_ERR("Null context_keys with non-zero max_keys");
        return RESULT_ERR;
    }
    
    *loaded_count = 0;
    
    /* Check if file exists */
    if (!memory_file_exists(filename)) {
        /* File doesn't exist, return empty set */
        return RESULT_OK;
    }
    
    /* Read file content */
    data_t file_content;
    if (data_init(&file_content, FILE_BUFFER_SIZE) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (file_read_all(filename, &file_content, MAX_DATA_SIZE) != RESULT_OK) {
        data_destroy(&file_content);
        return RESULT_ERR;
    }
    
    /* Parse context keys format */
    result_t result = json_parse_context_keys_format(file_content.data, context_keys, max_keys, loaded_count);
    data_destroy(&file_content);
    
    return result;
}

result_t persist_context_keys_save(const char* filename, const context_key_t* context_keys, size_t key_count) {
    if (!filename) {
        RETURN_ERR("Null filename in persist_context_keys_save");
        return RESULT_ERR;
    }
    
    if (key_count > 0 && !context_keys) {
        RETURN_ERR("Null context_keys with non-zero count");
        return RESULT_ERR;
    }
    
    /* Validate all context keys */
    for (size_t i = 0; i < key_count; i++) {
        if (!CONTEXT_KEY_IS_VALID(&context_keys[i])) {
            RETURN_ERR("Invalid context key in array");
            return RESULT_ERR;
        }
    }
    
    /* Build context keys JSON */
    data_t keys_json;
    if (data_init(&keys_json, key_count * 256 + 1024) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (json_build_context_keys(context_keys, key_count, &keys_json) != RESULT_OK) {
        data_destroy(&keys_json);
        return RESULT_ERR;
    }
    
    /* Write to file atomically */
    result_t result = file_write_atomic(filename, &keys_json, true);
    data_destroy(&keys_json);
    
    return result;
}

result_t persist_memory_backup(const char* memory_filename, const char* context_keys_filename) {
    if (!memory_filename) {
        RETURN_ERR("Null memory_filename in persist_memory_backup");
        return RESULT_ERR;
    }
    
    if (!context_keys_filename) {
        RETURN_ERR("Null context_keys_filename in persist_memory_backup");
        return RESULT_ERR;
    }
    
    bool memory_backed_up = false;
    bool keys_backed_up = false;
    
    /* Backup memory file if it exists */
    if (memory_file_exists(memory_filename)) {
        if (file_backup(memory_filename, NULL) == RESULT_OK) {
            memory_backed_up = true;
        }
    } else {
        /* No file to backup is considered success */
        memory_backed_up = true;
    }
    
    /* Backup context keys file if it exists */
    if (memory_file_exists(context_keys_filename)) {
        if (file_backup(context_keys_filename, NULL) == RESULT_OK) {
            keys_backed_up = true;
        }
    } else {
        /* No file to backup is considered success */
        keys_backed_up = true;
    }
    
    if (memory_backed_up && keys_backed_up) {
        return RESULT_OK;
    } else {
        RETURN_ERR("Failed to backup one or more memory files");
        return RESULT_ERR;
    }
}

result_t persist_memory_recover(const char* memory_filename, const char* context_keys_filename) {
    if (!memory_filename) {
        RETURN_ERR("Null memory_filename in persist_memory_recover");
        return RESULT_ERR;
    }
    
    if (!context_keys_filename) {
        RETURN_ERR("Null context_keys_filename in persist_memory_recover");
        return RESULT_ERR;
    }
    
    bool memory_recovered = false;
    bool keys_recovered = false;
    
    /* Try to recover memory file */
    char memory_backup[MAX_FILENAME_SIZE];
    snprintf(memory_backup, sizeof(memory_backup), "%s%s", memory_filename, BACKUP_EXTENSION);
    
    if (memory_file_exists(memory_backup)) {
        if (validate_memory_file(memory_backup) == RESULT_OK) {
            /* Read backup and write to original location */
            data_t backup_content;
            if (data_init(&backup_content, FILE_BUFFER_SIZE) == RESULT_OK) {
                if (file_read_all(memory_backup, &backup_content, MAX_DATA_SIZE) == RESULT_OK) {
                    if (file_write_atomic(memory_filename, &backup_content, false) == RESULT_OK) {
                        memory_recovered = true;
                    }
                }
                data_destroy(&backup_content);
            }
        }
    } else {
        /* No backup to recover is not an error */
        memory_recovered = true;
    }
    
    /* Try to recover context keys file */
    char keys_backup[MAX_FILENAME_SIZE];
    snprintf(keys_backup, sizeof(keys_backup), "%s%s", context_keys_filename, BACKUP_EXTENSION);
    
    if (memory_file_exists(keys_backup)) {
        if (validate_context_keys_file(keys_backup) == RESULT_OK) {
            /* Read backup and write to original location */
            data_t backup_content;
            if (data_init(&backup_content, FILE_BUFFER_SIZE) == RESULT_OK) {
                if (file_read_all(keys_backup, &backup_content, MAX_DATA_SIZE) == RESULT_OK) {
                    if (file_write_atomic(context_keys_filename, &backup_content, false) == RESULT_OK) {
                        keys_recovered = true;
                    }
                }
                data_destroy(&backup_content);
            }
        }
    } else {
        /* No backup to recover is not an error */
        keys_recovered = true;
    }
    
    if (memory_recovered && keys_recovered) {
        return RESULT_OK;
    } else {
        RETURN_ERR("Failed to recover one or more memory files from backup");
        return RESULT_ERR;
    }
}

result_t persist_memory_validate(const char* memory_filename, const char* context_keys_filename, 
                                 bool* memory_valid, bool* context_keys_valid) {
    if (!memory_filename) {
        RETURN_ERR("Null memory_filename in persist_memory_validate");
        return RESULT_ERR;
    }
    
    if (!context_keys_filename) {
        RETURN_ERR("Null context_keys_filename in persist_memory_validate");
        return RESULT_ERR;
    }
    
    if (!memory_valid || !context_keys_valid) {
        RETURN_ERR("Null validation result pointer in persist_memory_validate");
        return RESULT_ERR;
    }
    
    /* Validate memory file */
    *memory_valid = (validate_memory_file(memory_filename) == RESULT_OK);
    
    /* Validate context keys file */
    *context_keys_valid = (validate_context_keys_file(context_keys_filename) == RESULT_OK);
    
    return RESULT_OK;
}

result_t persist_memory_initialize(const char* memory_filename, const char* context_keys_filename) {
    if (!memory_filename) {
        RETURN_ERR("Null memory_filename in persist_memory_initialize");
        return RESULT_ERR;
    }
    
    if (!context_keys_filename) {
        RETURN_ERR("Null context_keys_filename in persist_memory_initialize");
        return RESULT_ERR;
    }
    
    /* Create empty memory file */
    data_t empty_working, empty_disk;
    if (data_init(&empty_working, 1) != RESULT_OK || data_init(&empty_disk, 1) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    result_t memory_result = persist_memory_save(memory_filename, &empty_working, &empty_disk);
    data_destroy(&empty_working);
    data_destroy(&empty_disk);
    
    if (memory_result != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Create empty context keys file */
    result_t keys_result = persist_context_keys_save(context_keys_filename, NULL, 0);
    
    return keys_result;
}

result_t persist_memory_compact(const char* memory_filename, const char* context_keys_filename, size_t cleanup_threshold) {
    if (!memory_filename) {
        RETURN_ERR("Null memory_filename in persist_memory_compact");
        return RESULT_ERR;
    }
    
    if (!context_keys_filename) {
        RETURN_ERR("Null context_keys_filename in persist_memory_compact");
        return RESULT_ERR;
    }
    
    if (cleanup_threshold > 100) {
        RETURN_ERR("Cleanup threshold must be between 0 and 100");
        return RESULT_ERR;
    }
    
    /* Create backup before compacting */
    if (persist_memory_backup(memory_filename, context_keys_filename) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Load context keys to check for cleanup opportunities */
    context_key_t keys[MAX_CONTEXT_KEYS];
    size_t key_count;
    
    if (persist_context_keys_load(context_keys_filename, keys, MAX_CONTEXT_KEYS, &key_count) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Simple compaction: remove old low-importance keys */
    time_t now = time(NULL);
    time_t cutoff_time = now - (24 * 3600); /* 24 hours ago */
    size_t compacted_count = 0;
    
    for (size_t i = 0; i < key_count; i++) {
        /* Keep key if it's recent, high importance, or in working memory */
        if (keys[i].last_accessed > cutoff_time || 
            keys[i].importance_score >= cleanup_threshold ||
            keys[i].layer == LAYER_WORKING) {
            if (compacted_count != i) {
                keys[compacted_count] = keys[i];
            }
            compacted_count++;
        }
    }
    
    /* Save compacted context keys if any were removed */
    if (compacted_count < key_count) {
        if (persist_context_keys_save(context_keys_filename, keys, compacted_count) != RESULT_OK) {
            return RESULT_ERR;
        }
    }
    
    /* For memory compaction, we would need more sophisticated logic
     * to analyze and compact the actual memory content. For now,
     * we just ensure the files are valid. */
    
    return RESULT_OK;
}
