#include "lkjagent.h"

// Helper function to create storage key in the format "tags,iteration_123"
static result_t create_storage_key(pool_t* pool, const data_t* tags, uint64_t iteration, data_t** storage_key) {
    if (!pool || !tags || !storage_key) {
        RETURN_ERR("Invalid parameters for create_storage_key");
    }
    
    // Create the storage key data structure
    if (data_create(pool, storage_key) != RESULT_OK) {
        RETURN_ERR("Failed to create storage key data");
    }
    
    // Append the tags
    if (data_append_data(pool, storage_key, tags) != RESULT_OK) {
        if (data_destroy(pool, *storage_key) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup storage_key after tags append error");
        }
        RETURN_ERR("Failed to append tags to storage key");
    }
    
    // Append ",iteration_"
    if (data_append_str(pool, storage_key, ",iteration_") != RESULT_OK) {
        if (data_destroy(pool, *storage_key) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup storage_key after iteration prefix append error");
        }
        RETURN_ERR("Failed to append iteration prefix to storage key");
    }
    
    // Convert iteration to string and append
    char iteration_buffer[32];
    snprintf(iteration_buffer, sizeof(iteration_buffer), "%lu", iteration);
    
    if (data_append_str(pool, storage_key, iteration_buffer) != RESULT_OK) {
        if (data_destroy(pool, *storage_key) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup storage_key after iteration append error");
        }
        RETURN_ERR("Failed to append iteration number to storage key");
    }
    
    return RESULT_OK;
}

// Action handler for storage_save - saves data to persistent storage
result_t lkjagent_action_storage_save(pool_t* pool, lkjagent_t* lkjagent, data_t* tags, data_t* value, uint64_t iteration) {
    if (pool == NULL || lkjagent == NULL || tags == NULL || value == NULL) {
        RETURN_ERR("Invalid parameters (null pool, lkjagent, tags, or value)");
    }
    
    object_t* storage = NULL;
    data_t* storage_path = NULL;
    data_t* storage_key = NULL;
    
    // Create path to storage in memory
    if (data_create_str(pool, &storage_path, "storage") != RESULT_OK) {
        RETURN_ERR("Failed to create storage path data");
    }
    
    // Get or create storage object
    if (object_provide_str(&storage, lkjagent->memory, "storage") != RESULT_OK) {
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
    
    // Create storage key in format "tags,iteration_123"
    if (create_storage_key(pool, tags, iteration, &storage_key) != RESULT_OK) {
        if (data_destroy(pool, storage_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup storage_path after storage_key creation error");
        }
        RETURN_ERR("Failed to create storage key");
    }
    
    // Use object_set_data to store the value directly in storage using the key as path
    if (object_set_data(pool, storage, storage_key, value) != RESULT_OK) {
        if (data_destroy(pool, storage_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup storage_path after storage set_data error");
        }
        if (data_destroy(pool, storage_key) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup storage_key after storage set_data error");
        }
        RETURN_ERR("Failed to set data in storage");
    }
    
    // Cleanup temporary data
    if (data_destroy(pool, storage_key) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup storage_key");
    }
    if (data_destroy(pool, storage_path) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup storage_path");
    }
    
    return RESULT_OK;
}
