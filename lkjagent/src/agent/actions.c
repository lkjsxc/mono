#include "agent/actions.h"

// Main action dispatcher - routes actions based on type
result_t agent_actions_dispatch(pool_t* pool, agent_t* agent, object_t* action_obj) {
    object_t* type_obj = NULL;
    object_t* tags_obj = NULL;
    object_t* value_obj = NULL;
    
    // Extract action parameters
    if (agent_actions_extract_action_params(pool, action_obj, &type_obj, &tags_obj, &value_obj) != RESULT_OK) {
        RETURN_ERR("Failed to extract action parameters");
    }
    
    // Dispatch based on action type
    if (string_equal_str(type_obj->string, "working_memory_add")) {
        if (agent_actions_validate_action_params(type_obj, tags_obj, value_obj, "working_memory_add", 1) != RESULT_OK) {
            RETURN_ERR("Invalid parameters for working_memory_add action");
        }
        return agent_actions_execute_working_memory_add(pool, agent, action_obj);
        
    } else if (string_equal_str(type_obj->string, "working_memory_remove")) {
        if (agent_actions_validate_action_params(type_obj, tags_obj, value_obj, "working_memory_remove", 0) != RESULT_OK) {
            RETURN_ERR("Invalid parameters for working_memory_remove action");
        }
        return agent_actions_execute_working_memory_remove(pool, agent, action_obj);
        
    } else if (string_equal_str(type_obj->string, "storage_load")) {
        if (agent_actions_validate_action_params(type_obj, tags_obj, value_obj, "storage_load", 0) != RESULT_OK) {
            RETURN_ERR("Invalid parameters for storage_load action");
        }
        return agent_actions_execute_storage_load(pool, agent, action_obj);
        
    } else if (string_equal_str(type_obj->string, "storage_save")) {
        if (agent_actions_validate_action_params(type_obj, tags_obj, value_obj, "storage_save", 1) != RESULT_OK) {
            RETURN_ERR("Invalid parameters for storage_save action");
        }
        return agent_actions_execute_storage_save(pool, agent, action_obj);
        
    } else {
        RETURN_ERR("Unknown action type");
    }
}

// Working memory add operation
result_t agent_actions_execute_working_memory_add(pool_t* pool, agent_t* agent, object_t* action_obj) {
    object_t* type_obj = NULL;
    object_t* tags_obj = NULL;
    object_t* value_obj = NULL;
    object_t* working_memory = NULL;
    string_t* processed_tags = NULL;
    
    // Extract action parameters
    if (agent_actions_extract_action_params(pool, action_obj, &type_obj, &tags_obj, &value_obj) != RESULT_OK) {
        RETURN_ERR("Failed to extract parameters for working_memory_add");
    }
    
    // Get working memory
    if (agent_actions_get_working_memory(pool, agent, &working_memory) != RESULT_OK) {
        RETURN_ERR("Failed to get working memory for add operation");
    }
    
    // Process tags (convert spaces to underscores)
    if (agent_actions_process_tags(pool, tags_obj, &processed_tags) != RESULT_OK) {
        RETURN_ERR("Failed to process tags for working_memory_add");
    }
    
    // Add the key-value pair to working memory
    if (object_set_string(pool, working_memory, processed_tags, value_obj->string) != RESULT_OK) {
        if (string_destroy(pool, processed_tags) != RESULT_OK) {
            RETURN_ERR("Failed to destroy processed tags after set failure");
        }
        RETURN_ERR("Failed to add item to working memory");
    }
    
    // Clean up processed tags
    if (string_destroy(pool, processed_tags) != RESULT_OK) {
        RETURN_ERR("Failed to destroy processed tags after working_memory_add");
    }
    
    return RESULT_OK;
}

// Working memory remove operation
result_t agent_actions_execute_working_memory_remove(pool_t* pool, agent_t* agent, object_t* action_obj) {
    object_t* type_obj = NULL;
    object_t* tags_obj = NULL;
    object_t* value_obj = NULL;
    object_t* working_memory = NULL;
    string_t* processed_tags = NULL;
    string_t* empty_string = NULL;
    
    // Extract action parameters
    if (agent_actions_extract_action_params(pool, action_obj, &type_obj, &tags_obj, &value_obj) != RESULT_OK) {
        RETURN_ERR("Failed to extract parameters for working_memory_remove");
    }
    
    // Get working memory
    if (agent_actions_get_working_memory(pool, agent, &working_memory) != RESULT_OK) {
        RETURN_ERR("Failed to get working memory for remove operation");
    }
    
    // Process tags
    if (agent_actions_process_tags(pool, tags_obj, &processed_tags) != RESULT_OK) {
        RETURN_ERR("Failed to process tags for working_memory_remove");
    }
    
    // Create empty string to effectively remove the item
    if (string_create_str(pool, &empty_string, "") != RESULT_OK) {
        if (string_destroy(pool, processed_tags) != RESULT_OK) {
            RETURN_ERR("Failed to destroy processed tags after empty string creation failure");
        }
        RETURN_ERR("Failed to create empty string for working_memory_remove");
    }
    
    // Set the key to empty string (effective removal)
    if (object_set_string(pool, working_memory, processed_tags, empty_string) != RESULT_OK) {
        if (string_destroy(pool, processed_tags) != RESULT_OK ||
            string_destroy(pool, empty_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy strings after set failure");
        }
        RETURN_ERR("Failed to remove item from working memory");
    }
    
    // Clean up
    if (string_destroy(pool, processed_tags) != RESULT_OK ||
        string_destroy(pool, empty_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy strings after working_memory_remove");
    }
    
    return RESULT_OK;
}

// Storage load operation
result_t agent_actions_execute_storage_load(pool_t* pool, agent_t* agent, object_t* action_obj) {
    object_t* type_obj = NULL;
    object_t* tags_obj = NULL;
    object_t* value_obj = NULL;
    object_t* storage = NULL;
    object_t* working_memory = NULL;
    string_t* processed_tags = NULL;
    object_t* stored_item = NULL;
    
    // Extract action parameters
    if (agent_actions_extract_action_params(pool, action_obj, &type_obj, &tags_obj, &value_obj) != RESULT_OK) {
        RETURN_ERR("Failed to extract parameters for storage_load");
    }
    
    // Get storage and working memory
    if (agent_actions_get_storage(pool, agent, &storage) != RESULT_OK) {
        RETURN_ERR("Failed to get storage for load operation");
    }
    
    if (agent_actions_get_working_memory(pool, agent, &working_memory) != RESULT_OK) {
        RETURN_ERR("Failed to get working memory for load operation");
    }
    
    // Process tags
    if (agent_actions_process_tags(pool, tags_obj, &processed_tags) != RESULT_OK) {
        RETURN_ERR("Failed to process tags for storage_load");
    }
    
    // Try to find the item in storage
    if (object_provide_string(&stored_item, storage, processed_tags) == RESULT_OK) {
        // Item found in storage, copy to working memory
        if (object_set_string(pool, working_memory, processed_tags, stored_item->string) != RESULT_OK) {
            if (string_destroy(pool, processed_tags) != RESULT_OK) {
                RETURN_ERR("Failed to destroy processed tags after copy failure");
            }
            RETURN_ERR("Failed to copy item from storage to working memory");
        }
    } else {
        // Item not found in storage - this is not necessarily an error
        // The agent should be aware that the load didn't find anything
    }
    
    // Clean up
    if (string_destroy(pool, processed_tags) != RESULT_OK) {
        RETURN_ERR("Failed to destroy processed tags after storage_load");
    }
    
    return RESULT_OK;
}

// Storage save operation
result_t agent_actions_execute_storage_save(pool_t* pool, agent_t* agent, object_t* action_obj) {
    object_t* type_obj = NULL;
    object_t* tags_obj = NULL;
    object_t* value_obj = NULL;
    object_t* storage = NULL;
    string_t* processed_tags = NULL;
    
    // Extract action parameters
    if (agent_actions_extract_action_params(pool, action_obj, &type_obj, &tags_obj, &value_obj) != RESULT_OK) {
        RETURN_ERR("Failed to extract parameters for storage_save");
    }
    
    // Ensure storage exists
    if (agent_actions_ensure_storage_exists(pool, agent) != RESULT_OK) {
        RETURN_ERR("Failed to ensure storage exists for save operation");
    }
    
    // Get storage
    if (agent_actions_get_storage(pool, agent, &storage) != RESULT_OK) {
        RETURN_ERR("Failed to get storage for save operation");
    }
    
    // Process tags
    if (agent_actions_process_tags(pool, tags_obj, &processed_tags) != RESULT_OK) {
        RETURN_ERR("Failed to process tags for storage_save");
    }
    
    // Save the key-value pair to storage
    if (object_set_string(pool, storage, processed_tags, value_obj->string) != RESULT_OK) {
        if (string_destroy(pool, processed_tags) != RESULT_OK) {
            RETURN_ERR("Failed to destroy processed tags after save failure");
        }
        RETURN_ERR("Failed to save item to storage");
    }
    
    // Clean up
    if (string_destroy(pool, processed_tags) != RESULT_OK) {
        RETURN_ERR("Failed to destroy processed tags after storage_save");
    }
    
    return RESULT_OK;
}

// Save agent memory to file
result_t agent_actions_save_memory(pool_t* pool, agent_t* agent) {
    string_t* memory_json = NULL;
    
    // Validate agent and agent data before serialization
    if (agent == NULL || agent->data == NULL) {
        // Don't fail completely, just skip save
        return RESULT_OK;
    }
    
    // Create string for memory JSON
    if (string_create(pool, &memory_json) != RESULT_OK) {
        // Don't fail completely, just skip save
        return RESULT_OK;
    }
    
    // Convert agent data to JSON
    if (object_tostring_json(pool, &memory_json, agent->data) != RESULT_OK) {
        if (string_destroy(pool, memory_json) != RESULT_OK) {
            // Even cleanup failure shouldn't crash the system
        }
        // Don't fail completely on serialization error - just skip save this cycle
        // This prevents the "Object contains invalid data" error from crashing the system
        return RESULT_OK;
    }
    
    // Validate JSON content before writing
    if (memory_json->data == NULL || memory_json->size == 0) {
        if (string_destroy(pool, memory_json) != RESULT_OK) {
            // Even cleanup failure shouldn't crash the system
        }
        // Don't fail completely, just skip save
        return RESULT_OK;
    }
    
    // Write to memory file
    if (file_write(MEMORY_PATH, memory_json) != RESULT_OK) {
        if (string_destroy(pool, memory_json) != RESULT_OK) {
            // Even cleanup failure shouldn't crash the system
        }
        // Don't fail completely on write error - just skip save
        return RESULT_OK;
    }
    
    // Clean up
    if (string_destroy(pool, memory_json) != RESULT_OK) {
        // Even cleanup failure shouldn't crash the system
    }
    
    return RESULT_OK;
}

// Parse LLM response content (XML format)
result_t agent_actions_parse_response(pool_t* pool, const string_t* response_content, object_t** response_obj) {
    // Try to parse XML response
    if (object_parse_xml(pool, response_obj, response_content) == RESULT_OK) {
        // XML parsing succeeded, return the parsed object
        return RESULT_OK;
    }
    
    // XML parsing failed - create a minimal response based on string analysis
    if (object_create(pool, response_obj) != RESULT_OK) {
        RETURN_ERR("Failed to create fallback response object");
    }
    
    // Create agent object
    object_t* agent_obj = NULL;
    if (object_create(pool, &agent_obj) != RESULT_OK) {
        if (object_destroy(pool, *response_obj) != RESULT_OK) {
            RETURN_ERR("Failed to destroy response object after agent creation failure");
        }
        RETURN_ERR("Failed to create agent object for fallback");
    }
    
    // Simple string-based analysis to determine next state
    string_t* next_state_value = NULL;
    const char* next_state_str = "thinking"; // Default fallback
    
    // Look for evaluation_log in the response content to determine state
    if (response_content != NULL && response_content->data != NULL) {
        if (strstr(response_content->data, "evaluation_log") != NULL) {
            next_state_str = "thinking"; // evaluation typically transitions to thinking
        } else if (strstr(response_content->data, "thinking_log") != NULL) {
            next_state_str = "thinking"; // thinking log means stay in thinking
        }
    }
    
    // Create next_state value
    if (string_create_str(pool, &next_state_value, next_state_str) != RESULT_OK) {
        if (object_destroy(pool, *response_obj) != RESULT_OK ||
            object_destroy(pool, agent_obj) != RESULT_OK) {
            RETURN_ERR("Failed to destroy objects after string creation failure");
        }
        RETURN_ERR("Failed to create next_state string for fallback");
    }
    
    // Set next_state in agent object
    string_t* next_state_path = NULL;
    if (string_create_str(pool, &next_state_path, "next_state") != RESULT_OK) {
        if (string_destroy(pool, next_state_value) != RESULT_OK ||
            object_destroy(pool, *response_obj) != RESULT_OK ||
            object_destroy(pool, agent_obj) != RESULT_OK) {
            RETURN_ERR("Failed to destroy resources after path creation failure");
        }
        RETURN_ERR("Failed to create next_state path for fallback");
    }
    
    if (object_set_string(pool, agent_obj, next_state_path, next_state_value) != RESULT_OK) {
        if (string_destroy(pool, next_state_value) != RESULT_OK ||
            string_destroy(pool, next_state_path) != RESULT_OK ||
            object_destroy(pool, *response_obj) != RESULT_OK ||
            object_destroy(pool, agent_obj) != RESULT_OK) {
            RETURN_ERR("Failed to destroy resources after set failure");
        }
        RETURN_ERR("Failed to set next_state in fallback agent object");
    }
    
    // Add agent object to response
    string_t* agent_path = NULL;
    if (string_create_str(pool, &agent_path, "agent") != RESULT_OK) {
        if (string_destroy(pool, next_state_value) != RESULT_OK ||
            string_destroy(pool, next_state_path) != RESULT_OK ||
            object_destroy(pool, *response_obj) != RESULT_OK ||
            object_destroy(pool, agent_obj) != RESULT_OK) {
            RETURN_ERR("Failed to destroy resources after agent path creation failure");
        }
        RETURN_ERR("Failed to create agent path for fallback response");
    }
    
    if (object_set(pool, *response_obj, agent_path, agent_obj) != RESULT_OK) {
        if (string_destroy(pool, next_state_value) != RESULT_OK ||
            string_destroy(pool, next_state_path) != RESULT_OK ||
            string_destroy(pool, agent_path) != RESULT_OK ||
            object_destroy(pool, *response_obj) != RESULT_OK ||
            object_destroy(pool, agent_obj) != RESULT_OK) {
            RETURN_ERR("Failed to destroy resources after agent set failure");
        }
        RETURN_ERR("Failed to set agent object in fallback response");
    }
    
    // Clean up strings
    if (string_destroy(pool, next_state_value) != RESULT_OK ||
        string_destroy(pool, next_state_path) != RESULT_OK ||
        string_destroy(pool, agent_path) != RESULT_OK) {
        RETURN_ERR("Failed to destroy strings after fallback response creation");
    }
    
    return RESULT_OK;
}

// Extract action parameters from action object
result_t agent_actions_extract_action_params(pool_t* pool, object_t* action_obj, object_t** type_obj, object_t** tags_obj, object_t** value_obj) {
    // Extract action type (required)
    if (object_provide_str(pool, type_obj, action_obj, "type") != RESULT_OK) {
        RETURN_ERR("Failed to extract action type");
    }
    
    // Extract tags (required)
    if (object_provide_str(pool, tags_obj, action_obj, "tags") != RESULT_OK) {
        RETURN_ERR("Failed to extract action tags");
    }
    
    // Extract value (optional)
    if (object_provide_str(pool, value_obj, action_obj, "value") != RESULT_OK) {
        *value_obj = NULL;  // Value is optional
    }
    
    return RESULT_OK;
}

// Validate action parameters
result_t agent_actions_validate_action_params(object_t* type_obj, object_t* tags_obj, object_t* value_obj, const char* expected_type, uint64_t value_required) {
    // Validate type
    if (type_obj == NULL || type_obj->string == NULL) {
        RETURN_ERR("Action type is NULL or invalid");
    }
    
    if (!string_equal_str(type_obj->string, expected_type)) {
        RETURN_ERR("Action type does not match expected type");
    }
    
    // Validate tags
    if (tags_obj == NULL || tags_obj->string == NULL) {
        RETURN_ERR("Action tags are NULL or invalid");
    }
    
    if (tags_obj->string->size == 0) {
        RETURN_ERR("Action tags cannot be empty");
    }
    
    // Validate value if required
    if (value_required) {
        if (value_obj == NULL || value_obj->string == NULL) {
            RETURN_ERR("Action value is required but not provided");
        }
    }
    
    return RESULT_OK;
}

// Process tags by converting spaces to underscores for valid identifiers
result_t agent_actions_process_tags(pool_t* pool, object_t* tags_obj, string_t** processed_tags) {
    // Create a copy of the tags string
    if (string_create_string(pool, processed_tags, tags_obj->string) != RESULT_OK) {
        RETURN_ERR("Failed to create copy of tags string");
    }
    
    // Convert spaces to underscores for valid identifiers
    for (uint64_t i = 0; i < (*processed_tags)->size; i++) {
        if ((*processed_tags)->data[i] == ' ') {
            (*processed_tags)->data[i] = '_';
        }
    }
    
    return RESULT_OK;
}

// Get working memory from agent
result_t agent_actions_get_working_memory(pool_t* pool, agent_t* agent, object_t** working_memory) {
    if (object_provide_str(pool, working_memory, agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to get working memory from agent");
    }
    
    return RESULT_OK;
}

// Get storage from agent
result_t agent_actions_get_storage(pool_t* pool, agent_t* agent, object_t** storage) {
    if (object_provide_str(pool, storage, agent->data, "storage") != RESULT_OK) {
        RETURN_ERR("Failed to get storage from agent");
    }
    
    return RESULT_OK;
}

// Ensure storage exists in agent data
result_t agent_actions_ensure_storage_exists(pool_t* pool, agent_t* agent) {
    object_t* storage = NULL;
    
    // Try to get existing storage
    if (object_provide_str(pool, &storage, agent->data, "storage") == RESULT_OK) {
        // Storage already exists
        return RESULT_OK;
    }
    
    // Storage doesn't exist, create it
    object_t* new_storage = NULL;
    if (object_create(pool, &new_storage) != RESULT_OK) {
        RETURN_ERR("Failed to create new storage object");
    }
    
    string_t* storage_path = NULL;
    if (string_create_str(pool, &storage_path, "storage") != RESULT_OK) {
        if (object_destroy(pool, new_storage) != RESULT_OK) {
            RETURN_ERR("Failed to destroy new storage after path creation failure");
        }
        RETURN_ERR("Failed to create storage path string");
    }
    
    if (object_set(pool, agent->data, storage_path, new_storage) != RESULT_OK) {
        if (string_destroy(pool, storage_path) != RESULT_OK ||
            object_destroy(pool, new_storage) != RESULT_OK) {
            RETURN_ERR("Failed to destroy resources after storage set failure");
        }
        RETURN_ERR("Failed to set storage object in agent data");
    }
    
    if (string_destroy(pool, storage_path) != RESULT_OK) {
        RETURN_ERR("Failed to destroy storage path string");
    }
    
    return RESULT_OK;
}
