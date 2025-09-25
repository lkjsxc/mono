#include "lkjagent.h"

// **UNIFIED STORAGE FORMAT**: key="tags", value="data" (consistent across all operations)
// This fixes the data format inconsistency that was breaking load/search operations

result_t lkjagent_action_storage_save(pool_t* pool, lkjagent_t* lkjagent, data_t* tags, data_t* value) {
    // Input validation
    if (!pool || !lkjagent || !tags || !value) {
        RETURN_ERR("Invalid parameters: null pool, lkjagent, tags, or value");
    }
    
    // Get or create storage object (efficient path)
    object_t* storage = NULL;
    if (object_provide_str(&storage, lkjagent->memory, "storage") != RESULT_OK) {
        // Storage doesn't exist - create it
        if (object_create(pool, &storage) != RESULT_OK) {
            RETURN_ERR("Failed to create storage object");
        }
        
        // Set in memory using efficient direct approach
        data_t* storage_key = NULL;
        if (data_create_str(pool, &storage_key, "storage") != RESULT_OK) {
            if (object_destroy(pool, storage) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup storage after storage_key creation error");
            }
            RETURN_ERR("Failed to create storage key");
        }
        
        if (object_set_data(pool, lkjagent->memory, storage_key, NULL) != RESULT_OK) {
            if (data_destroy(pool, storage_key) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup storage_key after set_data error");
            }
            if (object_destroy(pool, storage) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup storage after set_data error");
            }
            RETURN_ERR("Failed to set storage in memory");
        }
        
        if (data_destroy(pool, storage_key) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup storage_key");
        }
        
        // Re-obtain reference
        if (object_provide_str(&storage, lkjagent->memory, "storage") != RESULT_OK) {
            RETURN_ERR("Failed to re-obtain storage reference");
        }
    }
    
    // **EFFICIENT STORAGE SAVE**: Direct key-value storage (no complex parsing needed)
    // Format: storage object contains direct key-value pairs where key=tags, value=data
    // This is compatible with load and search operations
    if (object_set_data(pool, storage, tags, value) != RESULT_OK) {
        RETURN_ERR("Failed to save data to storage");
    }
    
    return RESULT_OK;
}

