/**
 * @file tagged_memory.c
 * @brief Tagged memory system implementation for LKJAgent
 * 
 * This module implements the core tagged memory system with unified storage,
 * context key management, and LLM-directed paging capabilities. It provides
 * the foundation for intelligent memory management in autonomous agents.
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
#include <sys/time.h>

/**
 * @defgroup Tagged_Memory_Internal Internal Tagged Memory Functions
 * @{
 */

/**
 * @brief Get current timestamp in microseconds
 * 
 * @return Current timestamp in microseconds since epoch
 */
static uint64_t get_timestamp_microseconds(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

/**
 * @brief Find context key index in array
 * 
 * @param memory Pointer to tagged memory system
 * @param key_name Name of context key to find
 * @return Index of key in array, or SIZE_MAX if not found
 */
static size_t find_context_key_index(const tagged_memory_t* memory, const char* key_name) {
    if (!memory || !key_name) {
        return SIZE_MAX;
    }
    
    for (size_t i = 0; i < memory->context_key_count; i++) {
        if (strncmp(memory->context_keys[i].key, key_name, MAX_TAG_SIZE - 1) == 0) {
            return i;
        }
    }
    
    return SIZE_MAX;
}

/**
 * @brief Validate memory layer value
 * 
 * @param layer Memory layer to validate
 * @return true if valid, false otherwise
 */
static bool is_valid_layer(memory_layer_t layer) {
    return layer >= LAYER_WORKING && layer <= LAYER_ARCHIVED;
}

/**
 * @brief Validate importance score
 * 
 * @param importance Importance score to validate
 * @return true if valid, false otherwise
 */
static bool is_valid_importance(size_t importance) {
    return importance <= 100;
}

/**
 * @brief Update memory access statistics
 * 
 * @param memory Pointer to tagged memory system
 * @param operation_time_us Operation time in microseconds
 */
static void update_access_stats(tagged_memory_t* memory, uint64_t operation_time_us) {
    if (!memory) return;
    
    memory->access_count++;
    
    /* Update rolling average of access time */
    if (memory->access_count == 1) {
        /* First access, use directly */
        memory->avg_access_time = operation_time_us;
    } else {
        /* Exponential moving average with alpha = 0.1 */
        memory->avg_access_time = (9 * memory->avg_access_time + operation_time_us) / 10;
    }
}

/**
 * @brief Calculate total memory size across all layers
 * 
 * @param memory Pointer to tagged memory system
 * @return Total memory size in bytes
 */
size_t calculate_total_memory_size(const tagged_memory_t* memory) {
    if (!memory) return 0;
    
    return memory->working_memory.size + memory->disk_memory.size;
}

/** @} */

result_t tagged_memory_init(tagged_memory_t* memory, const char* memory_file,
                           const char* context_keys_file, size_t max_working_size,
                           size_t max_disk_size) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in tagged_memory_init");
        return RESULT_ERR;
    }
    
    if (!memory_file || !context_keys_file) {
        RETURN_ERR("Null file path parameter in tagged_memory_init");
        return RESULT_ERR;
    }
    
    if (max_working_size == 0 || max_disk_size == 0) {
        RETURN_ERR("Invalid memory size parameters in tagged_memory_init");
        return RESULT_ERR;
    }
    
    /* Initialize memory structure */
    memset(memory, 0, sizeof(tagged_memory_t));
    
    /* Set configuration parameters */
    memory->max_working_size = max_working_size;
    memory->max_disk_size = max_disk_size;
    strncpy(memory->memory_file, memory_file, MAX_FILENAME_SIZE - 1);
    strncpy(memory->context_keys_file, context_keys_file, MAX_FILENAME_SIZE - 1);
    
    /* Initialize working memory */
    if (data_init(&memory->working_memory, max_working_size) != RESULT_OK) {
        RETURN_ERR("Failed to initialize working memory");
        return RESULT_ERR;
    }
    
    /* Initialize disk memory */
    if (data_init(&memory->disk_memory, max_disk_size) != RESULT_OK) {
        data_destroy(&memory->working_memory);
        RETURN_ERR("Failed to initialize disk memory");
        return RESULT_ERR;
    }
    
    /* Initialize statistics */
    memory->last_modified = time(NULL);
    memory->access_count = 0;
    memory->store_count = 0;
    memory->delete_count = 0;
    memory->avg_access_time = 0;
    memory->total_size = 0;
    memory->working_size = 0;
    memory->disk_size = 0;
    memory->archived_size = 0;
    memory->context_key_count = 0;
    
    /* Load existing memory data */
    result_t load_result = persist_memory_load(memory_file, &memory->working_memory, &memory->disk_memory);
    if (load_result != RESULT_OK) {
        /* If loading fails, try to initialize empty files */
        if (persist_memory_initialize(memory_file, context_keys_file) != RESULT_OK) {
            data_destroy(&memory->working_memory);
            data_destroy(&memory->disk_memory);
            RETURN_ERR("Failed to initialize memory files");
            return RESULT_ERR;
        }
    }
    
    /* Load context keys */
    size_t loaded_count;
    result_t keys_result = persist_context_keys_load(context_keys_file, memory->context_keys,
                                                    MAX_CONTEXT_KEYS, &loaded_count);
    if (keys_result == RESULT_OK) {
        memory->context_key_count = loaded_count;
    } else {
        /* If loading fails, start with empty context keys */
        memory->context_key_count = 0;
    }
    
    /* Update total size */
    memory->total_size = calculate_total_memory_size(memory);
    
    return RESULT_OK;
}

result_t context_key_create(tagged_memory_t* memory, const char* key_name, 
                           memory_layer_t layer, size_t importance, size_t data_size) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in context_key_create");
        return RESULT_ERR;
    }
    
    if (!key_name || key_name[0] == '\0') {
        RETURN_ERR("Invalid key_name parameter in context_key_create");
        return RESULT_ERR;
    }
    
    if (!is_valid_layer(layer)) {
        RETURN_ERR("Invalid layer parameter in context_key_create");
        return RESULT_ERR;
    }
    
    if (!is_valid_importance(importance)) {
        RETURN_ERR("Invalid importance parameter in context_key_create");
        return RESULT_ERR;
    }
    
    /* Check if key already exists */
    if (find_context_key_index(memory, key_name) != SIZE_MAX) {
        RETURN_ERR("Context key already exists in context_key_create");
        return RESULT_ERR;
    }
    
    /* Check if we have space for new key */
    if (memory->context_key_count >= MAX_CONTEXT_KEYS) {
        RETURN_ERR("Maximum context keys reached in context_key_create");
        return RESULT_ERR;
    }
    
    /* Create new context key */
    context_key_t* new_key = &memory->context_keys[memory->context_key_count];
    
    /* Copy key name with bounds checking */
    strncpy(new_key->key, key_name, MAX_TAG_SIZE - 1);
    new_key->key[MAX_TAG_SIZE - 1] = '\0';
    
    /* Set metadata */
    new_key->layer = layer;
    new_key->importance_score = importance;
    new_key->last_accessed = time(NULL);
    new_key->data_size = data_size;
    
    /* Increment count */
    memory->context_key_count++;
    memory->last_modified = time(NULL);
    
    return RESULT_OK;
}

result_t context_key_find(tagged_memory_t* memory, const char* key_name, context_key_t* key) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in context_key_find");
        return RESULT_ERR;
    }
    
    if (!key_name || key_name[0] == '\0') {
        RETURN_ERR("Invalid key_name parameter in context_key_find");
        return RESULT_ERR;
    }
    
    if (!key) {
        RETURN_ERR("Null key parameter in context_key_find");
        return RESULT_ERR;
    }
    
    uint64_t start_time = get_timestamp_microseconds();
    
    /* Find key index */
    size_t index = find_context_key_index(memory, key_name);
    if (index == SIZE_MAX) {
        return RESULT_ERR; /* Key not found */
    }
    
    /* Copy key data */
    *key = memory->context_keys[index];
    
    /* Update access timestamp */
    memory->context_keys[index].last_accessed = time(NULL);
    
    /* Update statistics */
    uint64_t operation_time = get_timestamp_microseconds() - start_time;
    update_access_stats(memory, operation_time);
    
    return RESULT_OK;
}

result_t context_key_update_importance(tagged_memory_t* memory, const char* key_name, size_t new_importance) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in context_key_update_importance");
        return RESULT_ERR;
    }
    
    if (!key_name || key_name[0] == '\0') {
        RETURN_ERR("Invalid key_name parameter in context_key_update_importance");
        return RESULT_ERR;
    }
    
    if (!is_valid_importance(new_importance)) {
        RETURN_ERR("Invalid importance parameter in context_key_update_importance");
        return RESULT_ERR;
    }
    
    /* Find key index */
    size_t index = find_context_key_index(memory, key_name);
    if (index == SIZE_MAX) {
        RETURN_ERR("Context key not found in context_key_update_importance");
        return RESULT_ERR;
    }
    
    /* Update importance score */
    memory->context_keys[index].importance_score = new_importance;
    memory->context_keys[index].last_accessed = time(NULL);
    memory->last_modified = time(NULL);
    
    return RESULT_OK;
}

result_t context_key_move_layer(tagged_memory_t* memory, const char* key_name, memory_layer_t target_layer) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in context_key_move_layer");
        return RESULT_ERR;
    }
    
    if (!key_name || key_name[0] == '\0') {
        RETURN_ERR("Invalid key_name parameter in context_key_move_layer");
        return RESULT_ERR;
    }
    
    if (!is_valid_layer(target_layer)) {
        RETURN_ERR("Invalid target_layer parameter in context_key_move_layer");
        return RESULT_ERR;
    }
    
    /* Find key index */
    size_t index = find_context_key_index(memory, key_name);
    if (index == SIZE_MAX) {
        RETURN_ERR("Context key not found in context_key_move_layer");
        return RESULT_ERR;
    }
    
    /* Check if already in target layer */
    if (memory->context_keys[index].layer == target_layer) {
        return RESULT_OK; /* Already in target layer */
    }
    
    /* Update layer */
    memory->context_keys[index].layer = target_layer;
    memory->context_keys[index].last_accessed = time(NULL);
    memory->last_modified = time(NULL);
    
    /* Note: Actual data movement would be handled by higher-level functions
     * that manage the data storage. This function only updates the metadata. */
    
    return RESULT_OK;
}

result_t context_key_archive(tagged_memory_t* memory, const char* key_name) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in context_key_archive");
        return RESULT_ERR;
    }
    
    if (!key_name || key_name[0] == '\0') {
        RETURN_ERR("Invalid key_name parameter in context_key_archive");
        return RESULT_ERR;
    }
    
    /* Find key index */
    size_t index = find_context_key_index(memory, key_name);
    if (index == SIZE_MAX) {
        RETURN_ERR("Context key not found in context_key_archive");
        return RESULT_ERR;
    }
    
    /* Move to archived layer */
    memory->context_keys[index].layer = LAYER_ARCHIVED;
    memory->context_keys[index].last_accessed = time(NULL);
    memory->last_modified = time(NULL);
    
    /* Note: Actual data archiving (compression, storage) would be handled
     * by specialized archiving functions. This updates the metadata. */
    
    return RESULT_OK;
}

result_t context_key_validate(const context_key_t* key) {
    if (!key) {
        RETURN_ERR("Null key parameter in context_key_validate");
        return RESULT_ERR;
    }
    
    /* Validate key name */
    if (key->key[0] == '\0') {
        RETURN_ERR("Empty key name in context_key_validate");
        return RESULT_ERR;
    }
    
    /* Ensure null termination */
    bool null_terminated = false;
    for (size_t i = 0; i < MAX_TAG_SIZE; i++) {
        if (key->key[i] == '\0') {
            null_terminated = true;
            break;
        }
    }
    
    if (!null_terminated) {
        RETURN_ERR("Key name not null-terminated in context_key_validate");
        return RESULT_ERR;
    }
    
    /* Validate layer */
    if (!is_valid_layer(key->layer)) {
        RETURN_ERR("Invalid layer in context_key_validate");
        return RESULT_ERR;
    }
    
    /* Validate importance score */
    if (!is_valid_importance(key->importance_score)) {
        RETURN_ERR("Invalid importance score in context_key_validate");
        return RESULT_ERR;
    }
    
    /* Validate timestamp (must be reasonable) */
    time_t now = time(NULL);
    if (key->last_accessed > now + 3600) { /* Allow 1 hour clock skew */
        RETURN_ERR("Invalid timestamp in context_key_validate");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t context_key_list_by_layer(tagged_memory_t* memory, memory_layer_t layer,
                                  context_key_t* keys, size_t max_keys, size_t* key_count,
                                  bool sort_by_importance) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in context_key_list_by_layer");
        return RESULT_ERR;
    }
    
    if (!is_valid_layer(layer)) {
        RETURN_ERR("Invalid layer parameter in context_key_list_by_layer");
        return RESULT_ERR;
    }
    
    if (!key_count) {
        RETURN_ERR("Null key_count parameter in context_key_list_by_layer");
        return RESULT_ERR;
    }
    
    if (max_keys > 0 && !keys) {
        RETURN_ERR("Null keys parameter with non-zero max_keys");
        return RESULT_ERR;
    }
    
    *key_count = 0;
    
    /* First pass: count matching keys */
    size_t matching_count = 0;
    for (size_t i = 0; i < memory->context_key_count; i++) {
        if (memory->context_keys[i].layer == layer) {
            matching_count++;
        }
    }
    
    if (matching_count == 0) {
        return RESULT_OK; /* No keys in this layer */
    }
    
    /* Second pass: collect keys */
    size_t collected = 0;
    for (size_t i = 0; i < memory->context_key_count && collected < max_keys; i++) {
        if (memory->context_keys[i].layer == layer) {
            keys[collected] = memory->context_keys[i];
            collected++;
        }
    }
    
    *key_count = collected;
    
    /* Sort if requested */
    if (sort_by_importance && collected > 1) {
        /* Simple bubble sort - sufficient for expected key counts */
        for (size_t i = 0; i < collected - 1; i++) {
            for (size_t j = 0; j < collected - i - 1; j++) {
                bool should_swap = false;
                
                if (sort_by_importance) {
                    should_swap = keys[j].importance_score < keys[j + 1].importance_score;
                } else {
                    should_swap = keys[j].last_accessed < keys[j + 1].last_accessed;
                }
                
                if (should_swap) {
                    context_key_t temp = keys[j];
                    keys[j] = keys[j + 1];
                    keys[j + 1] = temp;
                }
            }
        }
    }
    
    return RESULT_OK;
}

result_t context_key_cleanup_expired(tagged_memory_t* memory, time_t expiry_threshold,
                                    bool archive_instead_of_delete, size_t* cleaned_count) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in context_key_cleanup_expired");
        return RESULT_ERR;
    }
    
    if (expiry_threshold <= 0) {
        RETURN_ERR("Invalid expiry_threshold parameter in context_key_cleanup_expired");
        return RESULT_ERR;
    }
    
    if (!cleaned_count) {
        RETURN_ERR("Null cleaned_count parameter in context_key_cleanup_expired");
        return RESULT_ERR;
    }
    
    *cleaned_count = 0;
    time_t current_time = time(NULL);
    time_t cutoff_time = current_time - expiry_threshold;
    
    /* Scan for expired keys */
    size_t write_index = 0;
    for (size_t read_index = 0; read_index < memory->context_key_count; read_index++) {
        context_key_t* key = &memory->context_keys[read_index];
        
        /* Check if key is expired */
        bool is_expired = (key->last_accessed < cutoff_time);
        
        /* Exempt high-importance keys from automatic cleanup */
        bool is_high_importance = (key->importance_score >= 80);
        
        if (is_expired && !is_high_importance) {
            if (archive_instead_of_delete) {
                /* Archive the key */
                key->layer = LAYER_ARCHIVED;
                memory->context_keys[write_index] = *key;
                write_index++;
            } else {
                /* Delete the key (don't copy to write position) */
                (*cleaned_count)++;
            }
        } else {
            /* Keep the key */
            if (write_index != read_index) {
                memory->context_keys[write_index] = *key;
            }
            write_index++;
        }
    }
    
    /* Update key count */
    memory->context_key_count = write_index;
    memory->last_modified = current_time;
    
    if (archive_instead_of_delete) {
        /* Count archived keys */
        *cleaned_count = 0;
        for (size_t i = 0; i < memory->context_key_count; i++) {
            if (memory->context_keys[i].layer == LAYER_ARCHIVED &&
                memory->context_keys[i].last_accessed >= cutoff_time) {
                (*cleaned_count)++;
            }
        }
    }
    
    return RESULT_OK;
}

result_t tagged_memory_store(tagged_memory_t* memory, const char* key_name,
                            const data_t* data, memory_layer_t layer, size_t importance) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in tagged_memory_store");
        return RESULT_ERR;
    }
    
    if (!key_name || key_name[0] == '\0') {
        RETURN_ERR("Invalid key_name parameter in tagged_memory_store");
        return RESULT_ERR;
    }
    
    if (!data) {
        RETURN_ERR("Null data parameter in tagged_memory_store");
        return RESULT_ERR;
    }
    
    if (!is_valid_layer(layer)) {
        RETURN_ERR("Invalid layer parameter in tagged_memory_store");
        return RESULT_ERR;
    }
    
    if (!is_valid_importance(importance)) {
        RETURN_ERR("Invalid importance parameter in tagged_memory_store");
        return RESULT_ERR;
    }
    
    uint64_t start_time = get_timestamp_microseconds();
    
    /* Find existing key or create new one */
    size_t key_index = find_context_key_index(memory, key_name);
    if (key_index == SIZE_MAX) {
        /* Create new context key */
        result_t create_result = context_key_create(memory, key_name, layer, importance, data->size);
        if (create_result != RESULT_OK) {
            return create_result;
        }
        key_index = memory->context_key_count - 1;
    } else {
        /* Update existing key metadata */
        memory->context_keys[key_index].layer = layer;
        memory->context_keys[key_index].importance_score = importance;
        memory->context_keys[key_index].data_size = data->size;
        memory->context_keys[key_index].last_accessed = time(NULL);
    }
    
    /* Store data in appropriate layer */
    data_t* target_memory = NULL;
    switch (layer) {
        case LAYER_WORKING:
            target_memory = &memory->working_memory;
            break;
        case LAYER_DISK:
            target_memory = &memory->disk_memory;
            break;
        case LAYER_ARCHIVED:
            /* For now, store archived data in disk memory
             * In a full implementation, this would go to compressed storage */
            target_memory = &memory->disk_memory;
            break;
        default:
            RETURN_ERR("Invalid layer in tagged_memory_store");
            return RESULT_ERR;
    }
    
    /* Append data with key marker */
    char key_marker[MAX_TAG_SIZE + 32];
    snprintf(key_marker, sizeof(key_marker), "\n<key:%s>\n", key_name);
    
    if (data_append(target_memory, key_marker, 0) != RESULT_OK) {
        RETURN_ERR("Failed to append key marker in tagged_memory_store");
        return RESULT_ERR;
    }
    
    if (data_append(target_memory, data->data, 0) != RESULT_OK) {
        RETURN_ERR("Failed to append data in tagged_memory_store");
        return RESULT_ERR;
    }
    
    if (data_append(target_memory, "\n</key>\n", 0) != RESULT_OK) {
        RETURN_ERR("Failed to append end marker in tagged_memory_store");
        return RESULT_ERR;
    }
    
    /* Update statistics */
    memory->store_count++;
    memory->last_modified = time(NULL);
    memory->total_size = calculate_total_memory_size(memory);
    memory->working_size = memory->working_memory.size;
    memory->disk_size = memory->disk_memory.size;
    
    uint64_t operation_time = get_timestamp_microseconds() - start_time;
    update_access_stats(memory, operation_time);
    
    return RESULT_OK;
}

result_t tagged_memory_retrieve(tagged_memory_t* memory, const char* key_name, data_t* data) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in tagged_memory_retrieve");
        return RESULT_ERR;
    }
    
    if (!key_name || key_name[0] == '\0') {
        RETURN_ERR("Invalid key_name parameter in tagged_memory_retrieve");
        return RESULT_ERR;
    }
    
    if (!data) {
        RETURN_ERR("Null data parameter in tagged_memory_retrieve");
        return RESULT_ERR;
    }
    
    uint64_t start_time = get_timestamp_microseconds();
    
    /* Find context key */
    context_key_t key;
    if (context_key_find(memory, key_name, &key) != RESULT_OK) {
        return RESULT_ERR; /* Key not found */
    }
    
    /* Clear output data */
    if (data_clear(data) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Select source memory based on layer */
    const data_t* source_memory = NULL;
    switch (key.layer) {
        case LAYER_WORKING:
            source_memory = &memory->working_memory;
            break;
        case LAYER_DISK:
        case LAYER_ARCHIVED:
            source_memory = &memory->disk_memory;
            break;
        default:
            RETURN_ERR("Invalid layer in tagged_memory_retrieve");
            return RESULT_ERR;
    }
    
    /* Search for key marker in memory */
    char key_marker[MAX_TAG_SIZE + 32];
    snprintf(key_marker, sizeof(key_marker), "<key:%s>", key_name);
    
    const char* start_pos = strstr(source_memory->data, key_marker);
    if (!start_pos) {
        /* Key marker not found in memory */
        return RESULT_ERR;
    }
    
    /* Find start of data (after marker and newline) */
    const char* data_start = start_pos + strlen(key_marker);
    if (*data_start == '\n') {
        data_start++;
    }
    
    /* Find end marker */
    const char* end_pos = strstr(data_start, "</key>");
    if (!end_pos) {
        RETURN_ERR("End marker not found in tagged_memory_retrieve");
        return RESULT_ERR;
    }
    
    /* Extract data between markers */
    size_t data_length = end_pos - data_start;
    
    /* Handle trailing newline before end marker */
    if (data_length > 0 && data_start[data_length - 1] == '\n') {
        data_length--;
    }
    
    /* Copy extracted data */
    if (data_length > 0) {
        char* temp_buffer = malloc(data_length + 1);
        if (!temp_buffer) {
            RETURN_ERR("Memory allocation failed in tagged_memory_retrieve");
            return RESULT_ERR;
        }
        
        memcpy(temp_buffer, data_start, data_length);
        temp_buffer[data_length] = '\0';
        
        result_t set_result = data_set(data, temp_buffer, 0);
        free(temp_buffer);
        
        if (set_result != RESULT_OK) {
            return set_result;
        }
    }
    
    /* Update statistics */
    uint64_t operation_time = get_timestamp_microseconds() - start_time;
    update_access_stats(memory, operation_time);
    
    return RESULT_OK;
}

result_t tagged_memory_delete(tagged_memory_t* memory, const char* key_name) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in tagged_memory_delete");
        return RESULT_ERR;
    }
    
    if (!key_name || strlen(key_name) == 0) {
        RETURN_ERR("Invalid key_name parameter in tagged_memory_delete");
        return RESULT_ERR;
    }
    
    uint64_t start_time = get_timestamp_microseconds();
    
    /* Find the context key */
    size_t key_index = find_context_key_index(memory, key_name);
    if (key_index == SIZE_MAX) {
        /* Key not found */
        return RESULT_ERR;
    }
    
    context_key_t* key = &memory->context_keys[key_index];
    
    /* Remove data from the appropriate memory layer */
    data_t* target_memory = NULL;
    
    switch (key->layer) {
        case LAYER_WORKING:
            target_memory = &memory->working_memory;
            break;
        case LAYER_DISK:
            target_memory = &memory->disk_memory;
            break;
        default:
            RETURN_ERR("Invalid layer in tagged_memory_delete");
            return RESULT_ERR;
    }
    
    /* Find and remove data from memory */
    char key_marker[MAX_TAG_SIZE + 32];
    snprintf(key_marker, sizeof(key_marker), "<key:%s>", key_name);
    
    char* start_pos = strstr(target_memory->data, key_marker);
    if (start_pos) {
        /* Find end marker */
        char* end_pos = strstr(start_pos, "</key>");
        if (end_pos) {
            /* Move data after end marker to fill the gap */
            end_pos += 6; /* Length of "</key>" */
            if (*end_pos == '\n') {
                end_pos++;
            }
            
            size_t data_to_remove = end_pos - start_pos;
            size_t remaining_data = target_memory->size - (end_pos - target_memory->data);
            
            /* Move remaining data */
            memmove(start_pos, end_pos, remaining_data);
            
            /* Update memory size */
            target_memory->size -= data_to_remove;
            
            /* Null-terminate */
            target_memory->data[target_memory->size] = '\0';
        }
    }
    
    /* Remove context key from array */
    if (key_index < memory->context_key_count - 1) {
        /* Move keys after this one down */
        memmove(&memory->context_keys[key_index],
                &memory->context_keys[key_index + 1],
                (memory->context_key_count - key_index - 1) * sizeof(context_key_t));
    }
    
    /* Clear the last key entry */
    memset(&memory->context_keys[memory->context_key_count - 1], 0, sizeof(context_key_t));
    memory->context_key_count--;
    
    /* Update memory statistics */
    memory->delete_count++;
    memory->total_size = calculate_total_memory_size(memory);
    memory->last_modified = time(NULL);
    
    /* Update working and disk sizes */
    memory->working_size = memory->working_memory.size;
    memory->disk_size = memory->disk_memory.size;
    
    /* Update statistics */
    uint64_t operation_time = get_timestamp_microseconds() - start_time;
    update_access_stats(memory, operation_time);
    
    return RESULT_OK;
}

result_t tagged_memory_get_stats(tagged_memory_t* memory, memory_stats_t* stats) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in tagged_memory_get_stats");
        return RESULT_ERR;
    }
    
    if (!stats) {
        RETURN_ERR("Null stats parameter in tagged_memory_get_stats");
        return RESULT_ERR;
    }
    
    /* Clear stats structure */
    memset(stats, 0, sizeof(memory_stats_t));
    
    /* Basic memory statistics */
    stats->total_size = memory->total_size;
    stats->working_size = memory->working_size;
    stats->disk_size = memory->disk_size;
    stats->archived_size = memory->archived_size;
    stats->context_key_count = memory->context_key_count;
    
    /* Operation statistics */
    stats->access_count = memory->access_count;
    stats->store_count = memory->store_count;
    stats->delete_count = memory->delete_count;
    
    /* Timestamps */
    stats->last_modified = memory->last_modified;
    
    /* Performance statistics */
    stats->avg_access_time = memory->avg_access_time;
    
    return RESULT_OK;
}

result_t tagged_memory_compact(tagged_memory_t* memory, bool aggressive) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in tagged_memory_compact");
        return RESULT_ERR;
    }
    
    /* For basic compaction, we'll clean up expired keys */
    size_t cleaned_count;
    time_t expiry_threshold = aggressive ? 7 * 24 * 3600 : 30 * 24 * 3600; /* 7 or 30 days */
    
    result_t cleanup_result = context_key_cleanup_expired(memory, expiry_threshold, true, &cleaned_count);
    if (cleanup_result != RESULT_OK) {
        return cleanup_result;
    }
    
    /* Update total size after compaction */
    memory->total_size = calculate_total_memory_size(memory);
    memory->last_modified = time(NULL);
    
    /* In a full implementation, this would also:
     * - Defragment memory buffers
     * - Compress archived data
     * - Optimize key lookup structures
     * - Rebuild indices
     */
    
    return RESULT_OK;
}

result_t tagged_memory_destroy(tagged_memory_t* memory) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in tagged_memory_destroy");
        return RESULT_ERR;
    }
    
    /* Clean up data structures */
    data_destroy(&memory->working_memory);
    data_destroy(&memory->disk_memory);
    
    /* Clear context keys array */
    memset(memory->context_keys, 0, sizeof(memory->context_keys));
    memory->context_key_count = 0;
    
    /* Clear statistics */
    memory->last_modified = 0;
    memory->access_count = 0;
    memory->store_count = 0;
    memory->delete_count = 0;
    memory->avg_access_time = 0;
    memory->total_size = 0;
    memory->working_size = 0;
    memory->disk_size = 0;
    memory->archived_size = 0;
    memory->max_working_size = 0;
    memory->max_disk_size = 0;
    
    /* Clear file paths */
    memset(memory->memory_file, 0, sizeof(memory->memory_file));
    memset(memory->context_keys_file, 0, sizeof(memory->context_keys_file));
    
    return RESULT_OK;
}
