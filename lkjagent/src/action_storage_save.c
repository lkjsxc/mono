#include "lkjagent.h"

result_t lkjagent_action_storage_save(pool_t* pool, lkjagent_t* lkjagent, data_t* tags, data_t* value, uint64_t iteration) {
    object_t* storage = NULL;
    object_t* new_entry = NULL;
    data_t* storage_path = NULL;
    data_t* entry_data = NULL;
    
    // Create path to storage
    if (data_create_str(pool, &storage_path, "storage") != RESULT_OK) {
        RETURN_ERR("Failed to create storage path");
    }
    
    // Get or create storage object
    if (object_provide_str(&storage, lkjagent->memory, "storage") != RESULT_OK) {
        // Create new storage array if it doesn't exist
        if (object_create(pool, &storage) != RESULT_OK) {
            if (data_destroy(pool, storage_path) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup storage_path after object creation error");
            }
            RETURN_ERR("Failed to create storage object");
        }
        
        if (object_set_data(pool, lkjagent->memory, storage_path, NULL) != RESULT_OK) {
            if (data_destroy(pool, storage_path) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup storage_path after set_data error");
            }
            if (object_destroy(pool, storage) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup storage after set_data error");
            }
            RETURN_ERR("Failed to set storage object");
        }
    }
    
    // Check if entry with same tags already exists and remove it
    object_t* current = storage->child;
    object_t* prev = NULL;
    
    while (current != NULL) {
        object_t* next = current->next;
        bool should_remove = false;
        
        // Check if current entry matches tags
        if (current->data != NULL) {
            // Entry format is "tags:value:iteration"
            if (data_find_data(current->data, tags, 0) == 0) {
                // Tags found at beginning, check if followed by ':'
                char separator[2] = ":";
                if (data_find_str(current->data, separator, tags->size) == (int64_t)tags->size) {
                    should_remove = true;
                }
            }
        }
        
        if (should_remove) {
            // Remove existing entry with same tags
            if (prev == NULL) {
                storage->child = next;
            } else {
                prev->next = next;
            }
            
            if (object_destroy(pool, current) != RESULT_OK) {
                RETURN_ERR("Failed to destroy replaced storage entry");
            }
            break;
        } else {
            prev = current;
        }
        
        current = next;
    }
    
    // Create new storage entry as a simple data entry
    // Format: "tags:value:iteration"
    if (data_create(pool, &entry_data) != RESULT_OK) {
        if (data_destroy(pool, storage_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup storage_path after entry_data creation error");
        }
        RETURN_ERR("Failed to create entry data");
    }
    
    // Build entry string: "tags:value:iteration"
    if (data_append_data(pool, &entry_data, tags) != RESULT_OK) {
        if (data_destroy(pool, storage_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup storage_path after append tags error");
        }
        if (data_destroy(pool, entry_data) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_data after append tags error");
        }
        RETURN_ERR("Failed to append tags");
    }
    
    if (data_append_str(pool, &entry_data, ":") != RESULT_OK) {
        if (data_destroy(pool, storage_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup storage_path after append separator error");
        }
        if (data_destroy(pool, entry_data) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_data after append separator error");
        }
        RETURN_ERR("Failed to append separator");
    }
    
    if (data_append_data(pool, &entry_data, value) != RESULT_OK) {
        if (data_destroy(pool, storage_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup storage_path after append value error");
        }
        if (data_destroy(pool, entry_data) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_data after append value error");
        }
        RETURN_ERR("Failed to append value");
    }
    
    if (data_append_str(pool, &entry_data, ":") != RESULT_OK) {
        if (data_destroy(pool, storage_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup storage_path after append separator error");
        }
        if (data_destroy(pool, entry_data) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_data after append separator error");
        }
        RETURN_ERR("Failed to append separator");
    }
    
    // Convert iteration to string and append
    data_t* iteration_str = NULL;
    char iteration_buffer[32];
    snprintf(iteration_buffer, sizeof(iteration_buffer), "%lu", iteration);
    
    if (data_create_str(pool, &iteration_str, iteration_buffer) != RESULT_OK) {
        if (data_destroy(pool, storage_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup storage_path after iteration_str creation error");
        }
        if (data_destroy(pool, entry_data) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_data after iteration_str creation error");
        }
        RETURN_ERR("Failed to create iteration string");
    }
    
    if (data_append_data(pool, &entry_data, iteration_str) != RESULT_OK) {
        if (data_destroy(pool, storage_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup storage_path after append iteration error");
        }
        if (data_destroy(pool, entry_data) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_data after append iteration error");
        }
        if (data_destroy(pool, iteration_str) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup iteration_str after append iteration error");
        }
        RETURN_ERR("Failed to append iteration");
    }
    
    if (data_destroy(pool, iteration_str) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup iteration_str");
    }
    
    // Create object for the entry
    if (object_create(pool, &new_entry) != RESULT_OK) {
        if (data_destroy(pool, storage_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup storage_path after new_entry creation error");
        }
        if (data_destroy(pool, entry_data) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_data after new_entry creation error");
        }
        RETURN_ERR("Failed to create new entry object");
    }
    
    new_entry->data = entry_data;
    
    // Add to storage by linking as child
    if (storage->child == NULL) {
        storage->child = new_entry;
    } else {
        // Find the last child and append
        object_t* current = storage->child;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_entry;
    }
    
    // Cleanup
    if (data_destroy(pool, storage_path) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup storage_path");
    }
    
    return RESULT_OK;
}