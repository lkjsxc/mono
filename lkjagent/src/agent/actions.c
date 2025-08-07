#include "agent/actions.h"
#include "utils/file.h"

// Helper function to execute working_memory_add action
result_t agent_actions_execute_working_memory_add(pool_t* pool, agent_t* agent, object_t* action) {
    object_t* tags_obj;
    object_t* value_obj;
    object_t* working_memory;
    string_t* key_string;

    // Get working memory object
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory from agent");
    }

    // Extract tags and value from action
    if (object_provide_str(pool, &tags_obj, action, "tags") != RESULT_OK) {
        RETURN_ERR("Failed to get tags from working_memory_add action");
    }
    if (object_provide_str(pool, &value_obj, action, "value") != RESULT_OK) {
        RETURN_ERR("Failed to get value from working_memory_add action");
    }

    // Use tags as the key
    if (string_create_string(pool, &key_string, tags_obj->string) != RESULT_OK) {
        RETURN_ERR("Failed to create key string from tags");
    }

    // Replace spaces with underscores in the key to make it a valid identifier
    for (uint64_t i = 0; i < key_string->size; i++) {
        if (key_string->data[i] == ' ') {
            key_string->data[i] = '_';
        }
    }

    // Add the key-value pair to working memory
    if (object_set_string(pool, working_memory, key_string, value_obj->string) != RESULT_OK) {
        if (string_destroy(pool, key_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy key string after set failure");
        }
        RETURN_ERR("Failed to add value to working memory");
    }

    if (string_destroy(pool, key_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy key string");
    }

    printf("Added to working memory: %.*s = %.*s\n",
           (int)tags_obj->string->size, tags_obj->string->data,
           (int)value_obj->string->size, value_obj->string->data);

    return RESULT_OK;
}

// Helper function to execute working_memory_remove action
result_t agent_actions_execute_working_memory_remove(pool_t* pool, agent_t* agent, object_t* action) {
    object_t* tags_obj;
    object_t* working_memory;
    object_t* target_obj;
    string_t* key_string;

    // Get working memory object
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory from agent");
    }

    // Extract tags from action
    if (object_provide_str(pool, &tags_obj, action, "tags") != RESULT_OK) {
        RETURN_ERR("Failed to get tags from working_memory_remove action");
    }

    // Use tags as the key
    if (string_create_string(pool, &key_string, tags_obj->string) != RESULT_OK) {
        RETURN_ERR("Failed to create key string from tags");
    }

    // Replace spaces with underscores in the key
    for (uint64_t i = 0; i < key_string->size; i++) {
        if (key_string->data[i] == ' ') {
            key_string->data[i] = '_';
        }
    }

    // Check if the key exists before trying to remove it
    if (object_get(&target_obj, working_memory, key_string) == RESULT_OK && target_obj) {
        // For simplicity, we'll set the value to an empty object (effectively removing it)
        object_t* empty_obj;
        if (object_create(pool, &empty_obj) != RESULT_OK) {
            if (string_destroy(pool, key_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy key string after empty object creation failure");
            }
            RETURN_ERR("Failed to create empty object for removal");
        }

        if (object_set(pool, working_memory, key_string, empty_obj) != RESULT_OK) {
            if (string_destroy(pool, key_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy key string after set failure");
            }
            RETURN_ERR("Failed to remove value from working memory");
        }

        printf("Removed from working memory: %.*s\n",
               (int)tags_obj->string->size, tags_obj->string->data);
    } else {
        printf("Key not found in working memory: %.*s\n",
               (int)tags_obj->string->size, tags_obj->string->data);
    }

    if (string_destroy(pool, key_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy key string");
    }

    return RESULT_OK;
}

// Helper function to execute storage_load action
result_t agent_actions_execute_storage_load(pool_t* pool, agent_t* agent, object_t* action) {
    object_t* tags_obj;
    object_t* working_memory;
    object_t* storage;
    object_t* stored_value;
    string_t* key_string;

    // Get working memory and storage objects
    if (object_provide_str(pool, &working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory from agent");
    }
    if (object_provide_str(pool, &storage, agent->data, "storage") != RESULT_OK) {
        RETURN_ERR("Failed to get storage from agent");
    }

    // Extract tags from action
    if (object_provide_str(pool, &tags_obj, action, "tags") != RESULT_OK) {
        RETURN_ERR("Failed to get tags from storage_load action");
    }

    // Use tags as the key
    if (string_create_string(pool, &key_string, tags_obj->string) != RESULT_OK) {
        RETURN_ERR("Failed to create key string from tags");
    }

    // Replace spaces with underscores in the key
    for (uint64_t i = 0; i < key_string->size; i++) {
        if (key_string->data[i] == ' ') {
            key_string->data[i] = '_';
        }
    }

    // Try to find the value in storage
    if (object_get(&stored_value, storage, key_string) == RESULT_OK && stored_value && stored_value->string) {
        // Load the value from storage into working memory
        if (object_set_string(pool, working_memory, key_string, stored_value->string) != RESULT_OK) {
            if (string_destroy(pool, key_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy key string after set failure");
            }
            RETURN_ERR("Failed to load value from storage to working memory");
        }

        printf("Loaded from storage to working memory: %.*s = %.*s\n",
               (int)tags_obj->string->size, tags_obj->string->data,
               (int)stored_value->string->size, stored_value->string->data);
    } else {
        printf("Key not found in storage: %.*s\n",
               (int)tags_obj->string->size, tags_obj->string->data);
    }

    if (string_destroy(pool, key_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy key string");
    }

    return RESULT_OK;
}

// Helper function to execute storage_save action
result_t agent_actions_execute_storage_save(pool_t* pool, agent_t* agent, object_t* action) {
    object_t* tags_obj;
    object_t* value_obj;
    object_t* storage;
    string_t* key_string;

    // Get storage object
    if (object_provide_str(pool, &storage, agent->data, "storage") != RESULT_OK) {
        RETURN_ERR("Failed to get storage from agent");
    }

    // Extract tags and value from action
    if (object_provide_str(pool, &tags_obj, action, "tags") != RESULT_OK) {
        RETURN_ERR("Failed to get tags from storage_save action");
    }
    if (object_provide_str(pool, &value_obj, action, "value") != RESULT_OK) {
        RETURN_ERR("Failed to get value from storage_save action");
    }

    // Use tags as the key
    if (string_create_string(pool, &key_string, tags_obj->string) != RESULT_OK) {
        RETURN_ERR("Failed to create key string from tags");
    }

    // Replace spaces with underscores in the key
    for (uint64_t i = 0; i < key_string->size; i++) {
        if (key_string->data[i] == ' ') {
            key_string->data[i] = '_';
        }
    }

    // Save the key-value pair to storage
    if (object_set_string(pool, storage, key_string, value_obj->string) != RESULT_OK) {
        if (string_destroy(pool, key_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy key string after set failure");
        }
        RETURN_ERR("Failed to save value to storage");
    }

    if (string_destroy(pool, key_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy key string");
    }

    printf("Saved to storage: %.*s = %.*s\n",
           (int)tags_obj->string->size, tags_obj->string->data,
           (int)value_obj->string->size, value_obj->string->data);

    return RESULT_OK;
}

// Helper function to parse LLM response XML
result_t agent_actions_parse_response(pool_t* pool, const string_t* recv, object_t** response_obj) {
    if (object_parse_xml(pool, response_obj, recv) != RESULT_OK) {
        RETURN_ERR("Failed to parse LLM response as XML");
    }
    return RESULT_OK;
}

// Helper function to dispatch actions based on type
result_t agent_actions_dispatch(pool_t* pool, agent_t* agent, object_t* action_obj) {
    object_t* action_type_obj;

    // Extract action type
    if (object_provide_str(pool, &action_type_obj, action_obj, "type") != RESULT_OK) {
        RETURN_ERR("Failed to get action type from agent response");
    }

    // Execute the appropriate action based on type
    if (strncmp(action_type_obj->string->data, "working_memory_add", action_type_obj->string->size) == 0) {
        if (agent_actions_execute_working_memory_add(pool, agent, action_obj) != RESULT_OK) {
            RETURN_ERR("Failed to execute working_memory_add action");
        }
    } else if (strncmp(action_type_obj->string->data, "working_memory_remove", action_type_obj->string->size) == 0) {
        if (agent_actions_execute_working_memory_remove(pool, agent, action_obj) != RESULT_OK) {
            RETURN_ERR("Failed to execute working_memory_remove action");
        }
    } else if (strncmp(action_type_obj->string->data, "storage_load", action_type_obj->string->size) == 0) {
        if (agent_actions_execute_storage_load(pool, agent, action_obj) != RESULT_OK) {
            RETURN_ERR("Failed to execute storage_load action");
        }
    } else if (strncmp(action_type_obj->string->data, "storage_save", action_type_obj->string->size) == 0) {
        if (agent_actions_execute_storage_save(pool, agent, action_obj) != RESULT_OK) {
            RETURN_ERR("Failed to execute storage_save action");
        }
    } else {
        printf("Unknown action type: %.*s\n",
               (int)action_type_obj->string->size, action_type_obj->string->data);
    }

    return RESULT_OK;
}

// Helper function to save agent memory to file
result_t agent_actions_save_memory(pool_t* pool, agent_t* agent) {
    string_t* json_output;

    // Convert agent data to JSON string
    if (object_tostring_json(pool, &json_output, agent->data) != RESULT_OK) {
        printf("Warning: Failed to convert agent data to JSON, skipping save (agent still functional)\n");
        return RESULT_OK;  // Continue execution despite save failure
    }

    // Write to memory file
    if (file_write(MEMORY_PATH, json_output) != RESULT_OK) {
        if (string_destroy(pool, json_output) != RESULT_OK) {
            RETURN_ERR("Failed to destroy JSON output after file write failure");
        }
        RETURN_ERR("Failed to write agent memory to file");
    }

    if (string_destroy(pool, json_output) != RESULT_OK) {
        RETURN_ERR("Failed to destroy JSON output string");
    }

    return RESULT_OK;
}
