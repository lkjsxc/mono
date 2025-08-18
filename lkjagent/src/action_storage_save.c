#include "lkjagent.h"

// Action handler for storage_save - saves data to persistent storage
result_t lkjagent_action_storage_save(pool_t* pool, lkjagent_t* lkjagent, data_t* tags, data_t* value) {
    if (pool == NULL || lkjagent == NULL || tags == NULL || value == NULL) {
        RETURN_ERR("Invalid parameters (null pool, lkjagent, tags, or value)");
    }
    
    object_t* storage = NULL;
    data_t* storage_path = NULL;
    
    // Create path to storage in memory
    if (data_create_str(pool, &storage_path, "storage") != RESULT_OK) {
        RETURN_ERR("Failed to create storage path data");
    }
    
    // Get or create storage object
    if (object_provide_str(&storage, lkjagent->memory, "storage") != RESULT_OK) {
        PRINT_ERR("Failed to get storage object from memory");
        // Storage doesn't exist, create new storage object
        if (object_create(pool, &storage) != RESULT_OK) {
            if (data_destroy(pool, storage_path) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup storage_path after storage creation error");
            }
            RETURN_ERR("Failed to create storage object");
        }
        
        // Set the new storage object in memory
        if (object_set_data(pool, lkjagent->memory, storage_path, NULL) != RESULT_OK) {
            if (data_destroy(pool, storage_path) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup storage_path after set_data error");
            }
            if (object_destroy(pool, storage) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup storage after set_data error");
            }
            RETURN_ERR("Failed to set storage object in memory");
        }
        
        // Re-obtain the storage reference after setting it
        if (object_provide_str(&storage, lkjagent->memory, "storage") != RESULT_OK) {
            if (data_destroy(pool, storage_path) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup storage_path after re-provide error");
            }
            RETURN_ERR("Failed to re-obtain storage object reference");
        }
    }
    
    // Use object_set_data to store the value directly in storage using the key as path
    if (object_set_data(pool, storage, tags, value) != RESULT_OK) {
        if (data_destroy(pool, storage_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup storage_path after storage set_data error");
        }
        if (data_destroy(pool, tags) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup tags after storage set_data error");
        }
        RETURN_ERR("Failed to set data in storage");
    }
    
    // Cleanup temporary data
    if (data_destroy(pool, tags) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup tags");
    }
    if (data_destroy(pool, storage_path) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup storage_path");
    }
    
    return RESULT_OK;
}
