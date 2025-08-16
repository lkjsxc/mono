#include "action.h"

result_t action_storage_load(pool_t* pool, object_t* agent_memory, const data_t* tags) {
    object_t* storage_obj = NULL;
    
    if (!pool || !agent_memory || !tags) {
        RETURN_ERR("Invalid arguments for storage_load");
    }
    
    // Get storage object
    if (object_provide_str(&storage_obj, agent_memory, "storage") != RESULT_OK) {
        RETURN_ERR("Failed to get storage from agent memory");
    }
    
    // TODO: Implement storage loading based on tags
    // For now, we'll just log the operation
    printf("Loading from storage: tags='%.*s'\n",
           (int)tags->size, tags->data);
    
    return RESULT_OK;
}

result_t action_storage_save(pool_t* pool, object_t* agent_memory, const data_t* tags, const data_t* value) {
    object_t* storage_obj = NULL;
    object_t* new_entry = NULL;
    
    if (!pool || !agent_memory || !tags || !value) {
        RETURN_ERR("Invalid arguments for storage_save");
    }
    
    // Get storage object
    if (object_provide_str(&storage_obj, agent_memory, "storage") != RESULT_OK) {
        RETURN_ERR("Failed to get storage from agent memory");
    }
    
    // Create new object for the value
    if (object_create(pool, &new_entry) != RESULT_OK) {
        RETURN_ERR("Failed to create new object for storage entry");
    }
    
    // Set the data for the new entry
    if (data_create_data(pool, &new_entry->data, value) != RESULT_OK) {
        result_t cleanup = object_destroy(pool, new_entry);
        if (cleanup != RESULT_OK) {
            PRINT_ERR("Failed to cleanup new entry after data copy error");
        }
        RETURN_ERR("Failed to copy value data to new storage entry");
    }
    
    // TODO: Implement proper object insertion with tags as key
    // For now, we'll add it as a child object
    // This is a simplified implementation that needs to be enhanced
    // to properly handle tag-based key insertion
    
    new_entry->next = storage_obj->child;
    storage_obj->child = new_entry;
    
    printf("Saved to storage: tags='%.*s', value='%.*s'\n",
           (int)tags->size, tags->data,
           (int)value->size, value->data);
    
    return RESULT_OK;
}

result_t action_storage_search(pool_t* pool, object_t* agent_memory, const data_t* tags) {
    object_t* storage_obj = NULL;
    
    if (!pool || !agent_memory || !tags) {
        RETURN_ERR("Invalid arguments for storage_search");
    }
    
    // Get storage object
    if (object_provide_str(&storage_obj, agent_memory, "storage") != RESULT_OK) {
        RETURN_ERR("Failed to get storage from agent memory");
    }
    
    // TODO: Implement storage searching based on tags
    // For now, we'll just log the operation
    printf("Searching storage: tags='%.*s'\n",
           (int)tags->size, tags->data);
    
    return RESULT_OK;
}
