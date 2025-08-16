#include "action.h"

result_t action_working_memory_add(pool_t* pool, object_t* agent_memory, const data_t* tags, const data_t* value) {
    object_t* working_memory_obj = NULL;
    object_t* new_entry = NULL;
    
    if (!pool || !agent_memory || !tags || !value) {
        RETURN_ERR("Invalid arguments for working_memory_add");
    }
    
    // Get working_memory object
    if (object_provide_str(&working_memory_obj, agent_memory, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working_memory from agent memory");
    }
    
    // Create new object for the value
    if (object_create(pool, &new_entry) != RESULT_OK) {
        RETURN_ERR("Failed to create new object for working memory entry");
    }
    
    // Set the data for the new entry
    if (data_create_data(pool, &new_entry->data, value) != RESULT_OK) {
        result_t cleanup = object_destroy(pool, new_entry);
        if (cleanup != RESULT_OK) {
            PRINT_ERR("Failed to cleanup new entry after data copy error");
        }
        RETURN_ERR("Failed to copy value data to new working memory entry");
    }
    
    // TODO: Implement proper object insertion with tags as key
    // For now, we'll add it as a child object
    // This is a simplified implementation that needs to be enhanced
    // to properly handle tag-based key insertion
    
    new_entry->next = working_memory_obj->child;
    working_memory_obj->child = new_entry;
    
    printf("Added to working memory: tags='%.*s', value='%.*s'\n",
           (int)tags->size, tags->data,
           (int)value->size, value->data);
    
    return RESULT_OK;
}

result_t action_working_memory_remove(pool_t* pool, object_t* agent_memory, const data_t* tags) {
    object_t* working_memory_obj = NULL;
    
    if (!pool || !agent_memory || !tags) {
        RETURN_ERR("Invalid arguments for working_memory_remove");
    }
    
    // Get working_memory object
    if (object_provide_str(&working_memory_obj, agent_memory, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working_memory from agent memory");
    }
    
    // TODO: Implement proper object removal based on tags
    // For now, we'll just log the operation
    printf("Removing from working memory: tags='%.*s'\n",
           (int)tags->size, tags->data);
    
    return RESULT_OK;
}
