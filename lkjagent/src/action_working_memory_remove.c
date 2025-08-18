#include "lkjagent.h"

result_t lkjagent_action_working_memory_remove(pool_t* pool, lkjagent_t* lkjagent, data_t* tags) {
    
    object_t* working_memory = NULL;
    object_t* current = NULL;
    object_t* prev = NULL;
    
    // Get working memory object
    if (object_provide_str(&working_memory, lkjagent->memory, "working_memory") != RESULT_OK) {
        PRINT_ERR("Failed to get working memory object from memory");
        return RESULT_OK;
    }
    
    // Iterate through working memory entries and remove matching ones
    current = working_memory->child;
    
    while (current != NULL) {
        object_t* next = current->next;
        bool should_remove = false;
        
        // Check if current entry matches tags
        if (current->data != NULL) {
            // Entry key format is now "sorted_tags,iteration_123"
            // Check if tags match by looking for the tags at the beginning
            if (data_find_data(current->data, tags, 0) == 0) {
                // Tags found at beginning, check if followed by ',' or we're at the end
                // This handles cases where the tags match exactly at the start of the key
                if (current->data->size == tags->size || 
                    (current->data->size > tags->size && current->data->data[tags->size] == ',')) {
                    should_remove = true;
                }
            }
        }
        
        if (should_remove) {
            // Remove current entry from the list
            if (prev == NULL) {
                // Removing first child
                working_memory->child = next;
            } else {
                // Removing from middle or end
                prev->next = next;
            }
            
            // Destroy the removed entry
            if (object_destroy(pool, current) != RESULT_OK) {
                RETURN_ERR("Failed to destroy removed working memory entry");
            }
        } else {
            prev = current;
        }
        
        current = next;
    }
    
    return RESULT_OK;
}
