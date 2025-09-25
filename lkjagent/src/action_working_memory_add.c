#include "lkjagent.h"

// **STREAMLINED WORKING MEMORY ADD** - Unified format, efficient implementation
// Consistent with storage format for seamless interoperability
result_t lkjagent_action_working_memory_add(pool_t* pool, lkjagent_t* lkjagent, data_t* tags, data_t* value, uint64_t iteration) {
    // Input validation
    if (!pool || !lkjagent || !tags || !value) {
        RETURN_ERR("Invalid parameters: pool, lkjagent, tags, or value is NULL");
    }
    
    // Get or create working memory object (efficient path)
    object_t* working_memory = NULL;
    if (object_provide_str(&working_memory, lkjagent->memory, "working_memory") != RESULT_OK) {
        // Working memory doesn't exist - create it
        if (object_create(pool, &working_memory) != RESULT_OK) {
            RETURN_ERR("Failed to create working_memory object");
        }
        
        data_t* memory_key = NULL;
        if (data_create_str(pool, &memory_key, "working_memory") != RESULT_OK) {
            if (object_destroy(pool, working_memory) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup working_memory after memory_key creation error");
            }
            RETURN_ERR("Failed to create working_memory key");
        }
        
        if (object_set_data(pool, lkjagent->memory, memory_key, NULL) != RESULT_OK) {
            if (data_destroy(pool, memory_key) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup memory_key after set_data error");
            }
            if (object_destroy(pool, working_memory) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup working_memory after set_data error");
            }
            RETURN_ERR("Failed to set working_memory in memory");
        }
        
        if (data_destroy(pool, memory_key) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup memory_key");
        }
        
        // Re-obtain reference
        if (object_provide_str(&working_memory, lkjagent->memory, "working_memory") != RESULT_OK) {
            RETURN_ERR("Failed to re-obtain working_memory reference");
        }
    }
    
    // **UNIFIED FORMAT**: Create entry key with iteration info
    // Format: "tags,iteration_N" (consistent with expected format)
    data_t* entry_key = NULL;
    if (data_create_data(pool, &entry_key, tags) != RESULT_OK) {
        RETURN_ERR("Failed to create entry key");
    }
    
    // Append iteration info efficiently  
    char iter_suffix[64];
    snprintf(iter_suffix, sizeof(iter_suffix), ",iteration_%lu", iteration);
    
    if (data_append_str(pool, &entry_key, iter_suffix) != RESULT_OK) {
        if (data_destroy(pool, entry_key) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_key after append error");
        }
        RETURN_ERR("Failed to append iteration to entry key");
    }
    
    // Create value copy
    data_t* entry_value = NULL;
    if (data_create_data(pool, &entry_value, value) != RESULT_OK) {
        if (data_destroy(pool, entry_key) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_key after entry_value creation error");
        }
        RETURN_ERR("Failed to create entry value");
    }
    
    // **EFFICIENT OBJECT CREATION**: Create entry objects with minimal allocations
    object_t* new_entry = NULL;
    object_t* value_object = NULL;
    
    if (object_create(pool, &new_entry) != RESULT_OK ||
        object_create(pool, &value_object) != RESULT_OK) {
        // Clean up any successful allocations
        if (new_entry && object_destroy(pool, new_entry) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup new_entry after creation error");
        }
        if (value_object && object_destroy(pool, value_object) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup value_object after creation error");
        }
        if (data_destroy(pool, entry_key) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_key after object creation error");
        }
        if (data_destroy(pool, entry_value) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_value after object creation error");
        }
        RETURN_ERR("Failed to create entry objects");
    }
    
    // Set up entry structure: key -> value
    new_entry->data = entry_key;
    value_object->data = entry_value;
    new_entry->child = value_object;
    
    // **OPTIMIZED INSERTION**: Add to working memory efficiently
    if (working_memory->child == NULL) {
        working_memory->child = new_entry;
    } else {
        // Append to end of list (could be optimized with tail pointer if needed)
        object_t* current = working_memory->child;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_entry;
    }
    
    return RESULT_OK;
}

