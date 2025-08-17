#include "lkjagent.h"

result_t lkjagent_action_working_memory_add(pool_t* pool, lkjagent_t* lkjagent, data_t* tags, data_t* value, uint64_t iteration) {
    object_t* working_memory = NULL;
    object_t* new_entry = NULL;
    data_t* working_memory_path = NULL;
    data_t* entry_data = NULL;
    
    // Create path to working memory
    if (data_create_str(pool, &working_memory_path, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to create working_memory path");
    }
    
    // Get or create working_memory object
    if (object_provide_str(&working_memory, lkjagent->memory, "working_memory") != RESULT_OK) {
        // Create new working_memory array if it doesn't exist
        if (object_create(pool, &working_memory) != RESULT_OK) {
            if (data_destroy(pool, working_memory_path) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup working_memory_path after object creation error");
            }
            RETURN_ERR("Failed to create working_memory object");
        }
        
        if (object_set_data(pool, lkjagent->memory, working_memory_path, NULL) != RESULT_OK) {
            if (data_destroy(pool, working_memory_path) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup working_memory_path after set_data error");
            }
            if (object_destroy(pool, working_memory) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup working_memory after set_data error");
            }
            RETURN_ERR("Failed to set working_memory object");
        }
    }
    
    // Create new memory entry using key-value structure
    // Key format: "sorted_tags,iteration_123"
    // Value: the actual data value
    data_t* entry_key = NULL;
    if (data_create(pool, &entry_key) != RESULT_OK) {
        if (data_destroy(pool, working_memory_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup working_memory_path after entry_key creation error");
        }
        RETURN_ERR("Failed to create entry key");
    }
    
    // Build entry key: "sorted_tags,iteration_123"
    if (data_append_data(pool, &entry_key, tags) != RESULT_OK) {
        if (data_destroy(pool, working_memory_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup working_memory_path after append tags error");
        }
        if (data_destroy(pool, entry_key) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_key after append tags error");
        }
        RETURN_ERR("Failed to append tags to entry key");
    }
    
    if (data_append_str(pool, &entry_key, ",iteration_") != RESULT_OK) {
        if (data_destroy(pool, working_memory_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup working_memory_path after append iteration prefix error");
        }
        if (data_destroy(pool, entry_key) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_key after append iteration prefix error");
        }
        RETURN_ERR("Failed to append iteration prefix to entry key");
    }
    
    // Convert iteration to string and append
    data_t* iteration_str = NULL;
    char iteration_buffer[32];
    snprintf(iteration_buffer, sizeof(iteration_buffer), "%lu", iteration);
    
    if (data_create_str(pool, &iteration_str, iteration_buffer) != RESULT_OK) {
        if (data_destroy(pool, working_memory_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup working_memory_path after iteration_str creation error");
        }
        if (data_destroy(pool, entry_key) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_key after iteration_str creation error");
        }
        RETURN_ERR("Failed to create iteration string");
    }
    
    if (data_append_data(pool, &entry_key, iteration_str) != RESULT_OK) {
        if (data_destroy(pool, working_memory_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup working_memory_path after append iteration error");
        }
        if (data_destroy(pool, entry_key) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_key after append iteration error");
        }
        if (data_destroy(pool, iteration_str) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup iteration_str after append iteration error");
        }
        RETURN_ERR("Failed to append iteration to entry key");
    }
    
    if (data_destroy(pool, iteration_str) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup iteration_str");
    }
    
    // Create the value data (copy of the input value)
    if (data_create_data(pool, &entry_data, value) != RESULT_OK) {
        if (data_destroy(pool, working_memory_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup working_memory_path after entry_data creation error");
        }
        if (data_destroy(pool, entry_key) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_key after entry_data creation error");
        }
        RETURN_ERR("Failed to create entry data");
    }
    
    // Create object for the entry (key-value pair)
    if (object_create(pool, &new_entry) != RESULT_OK) {
        if (data_destroy(pool, working_memory_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup working_memory_path after new_entry creation error");
        }
        if (data_destroy(pool, entry_key) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_key after new_entry creation error");
        }
        if (data_destroy(pool, entry_data) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_data after new_entry creation error");
        }
        RETURN_ERR("Failed to create new entry object");
    }
    
    // Set up the key-value structure
    new_entry->data = entry_key;  // The key
    
    // Create child object to hold the value
    object_t* value_object = NULL;
    if (object_create(pool, &value_object) != RESULT_OK) {
        if (data_destroy(pool, working_memory_path) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup working_memory_path after value_object creation error");
        }
        if (data_destroy(pool, entry_key) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_key after value_object creation error");
        }
        if (data_destroy(pool, entry_data) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup entry_data after value_object creation error");
        }
        if (object_destroy(pool, new_entry) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup new_entry after value_object creation error");
        }
        RETURN_ERR("Failed to create value object");
    }
    
    value_object->data = entry_data;  // The value
    new_entry->child = value_object;  // Link value as child of key
    
    // Add to working memory by linking as child
    if (working_memory->child == NULL) {
        working_memory->child = new_entry;
    } else {
        // Find the last child and append
        object_t* current = working_memory->child;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_entry;
    }
    
    // Cleanup
    if (data_destroy(pool, working_memory_path) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup working_memory_path");
    }
    
    return RESULT_OK;
}
