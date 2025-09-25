#include "lkjagent.h"

// Forward declaration
static bool working_memory_tags_match(const data_t* search_tags, const data_t* entry_key);

// **OPTIMIZED WORKING MEMORY REMOVE** - Efficient tag-based removal with proper matching
result_t lkjagent_action_working_memory_remove(pool_t* pool, lkjagent_t* lkjagent, data_t* tags) {
    // Input validation
    if (!pool || !lkjagent || !tags) {
        RETURN_ERR("Invalid parameters: pool, lkjagent, or tags is NULL");
    }
    
    // Get working memory object
    object_t* working_memory = NULL;
    if (object_provide_str(&working_memory, lkjagent->memory, "working_memory") != RESULT_OK) {
        // Working memory doesn't exist - that's OK, nothing to remove
        return RESULT_OK;
    }
    
    if (!working_memory->child) {
        // Working memory is empty - that's OK, nothing to remove  
        return RESULT_OK;
    }
    
    // **EFFICIENT REMOVAL** - Single pass with proper tag matching
    object_t* current = working_memory->child;
    object_t* prev = NULL;
    uint32_t removed_count = 0;
    
    while (current != NULL) {
        object_t* next = current->next;
        bool should_remove = false;
        
        if (current->data && current->data->data) {
            // Check if entry tags match search tags
            // Format: "tags,iteration_N" - match the tags part exactly
            should_remove = working_memory_tags_match(tags, current->data);
        }
        
        if (should_remove) {
            // Remove current entry from linked list
            if (prev == NULL) {
                working_memory->child = next;
            } else {
                prev->next = next;
            }
            
            // Destroy the entry and its contents
            if (object_destroy(pool, current) != RESULT_OK) {
                PRINT_ERR("Warning: Failed to destroy removed working memory entry");
            } else {
                removed_count++;
            }
        } else {
            prev = current;
        }
        
        current = next;
    }
    
    return RESULT_OK;
}

// **PRECISE TAG MATCHING** - Match tags exactly at the beginning of the key
static bool working_memory_tags_match(const data_t* search_tags, const data_t* entry_key) {
    if (!search_tags || !entry_key) {
        return false;
    }
    
    if (search_tags->size == 0) {
        return true; // Empty search matches everything
    }
    
    if (entry_key->size < search_tags->size) {
        return false; // Entry key too short
    }
    
    // Check if search tags match at the start of entry key
    if (memcmp(search_tags->data, entry_key->data, search_tags->size) != 0) {
        return false; // Tags don't match
    }
    
    // Verify proper boundary: tags must be followed by comma or be the entire key
    if (entry_key->size == search_tags->size) {
        return true; // Exact match
    }
    
    if (entry_key->data[search_tags->size] == ',') {
        return true; // Tags followed by comma (iteration info)
    }
    
    return false; // Tags are substring but not properly bounded
}
