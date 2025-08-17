#include "lkjagent.h"

result_t lkjagent_action_storage_load(pool_t* pool, lkjagent_t* lkjagent, data_t* tags, uint64_t iteration) {
    
    object_t* storage = NULL;
    object_t* current = NULL;
    
    // Get storage object
    if (object_provide_str(&storage, lkjagent->memory, "storage") != RESULT_OK) {
        // No storage exists, nothing to load
        return RESULT_OK;
    }
    
    // Search storage for entries matching the tags
    current = storage->child;
    
    while (current != NULL) {
        bool match = false;
        
        // Check if current entry matches tags
        if (current->data != NULL) {
            // Entry format is "tags:value:iteration"
            // Check if tags match at the beginning
            if (data_find_data(current->data, tags, 0) == 0) {
                // Tags found at beginning, check if followed by ':'
                char separator[2] = ":";
                if (data_find_str(current->data, separator, tags->size) == (int64_t)tags->size) {
                    match = true;
                }
            }
            
            // Also check if tags appear anywhere in the entry (more flexible matching)
            if (!match && data_find_data(current->data, tags, 0) >= 0) {
                match = true;
            }
        }
        
        if (match) {
            // Found matching entry, extract value and add to working memory
            data_t* entry_tags = NULL;
            data_t* entry_value = NULL;
            
            // Parse the entry: "tags:value:iteration"
            int64_t first_colon = data_find_char(current->data, ':', 0);
            int64_t second_colon = data_find_char(current->data, ':', first_colon + 1);
            
            if (first_colon > 0 && second_colon > first_colon) {
                // Extract tags part
                if (data_create(pool, &entry_tags) != RESULT_OK) {
                    RETURN_ERR("Failed to create entry tags");
                }
                
                // Copy tags (from start to first colon)
                for (int64_t i = 0; i < first_colon; i++) {
                    if (data_append_char(pool, &entry_tags, current->data->data[i]) != RESULT_OK) {
                        if (data_destroy(pool, entry_tags) != RESULT_OK) {
                            PRINT_ERR("Failed to cleanup entry_tags after copy tags error");
                        }
                        RETURN_ERR("Failed to copy tags");
                    }
                }
                
                // Extract value part
                if (data_create(pool, &entry_value) != RESULT_OK) {
                    if (data_destroy(pool, entry_tags) != RESULT_OK) {
                        PRINT_ERR("Failed to cleanup entry_tags after entry_value creation error");
                    }
                    RETURN_ERR("Failed to create entry value");
                }
                
                // Copy value (from first colon+1 to second colon)
                for (int64_t i = first_colon + 1; i < second_colon; i++) {
                    if (data_append_char(pool, &entry_value, current->data->data[i]) != RESULT_OK) {
                        if (data_destroy(pool, entry_tags) != RESULT_OK) {
                            PRINT_ERR("Failed to cleanup entry_tags after copy value error");
                        }
                        if (data_destroy(pool, entry_value) != RESULT_OK) {
                            PRINT_ERR("Failed to cleanup entry_value after copy value error");
                        }
                        RETURN_ERR("Failed to copy value");
                    }
                }
                
                // Add to working memory
                if (lkjagent_action_working_memory_add(pool, lkjagent, entry_tags, entry_value, iteration) != RESULT_OK) {
                    if (data_destroy(pool, entry_tags) != RESULT_OK) {
                        PRINT_ERR("Failed to cleanup entry_tags after working_memory_add error");
                    }
                    if (data_destroy(pool, entry_value) != RESULT_OK) {
                        PRINT_ERR("Failed to cleanup entry_value after working_memory_add error");
                    }
                    RETURN_ERR("Failed to add loaded entry to working memory");
                }
                
                if (data_destroy(pool, entry_tags) != RESULT_OK) {
                    PRINT_ERR("Failed to cleanup entry_tags");
                }
                if (data_destroy(pool, entry_value) != RESULT_OK) {
                    PRINT_ERR("Failed to cleanup entry_value");
                }
            }
        }
        
        current = current->next;
    }
    
    return RESULT_OK;
}