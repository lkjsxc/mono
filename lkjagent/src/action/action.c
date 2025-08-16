#include "action.h"

static action_type_t parse_action_type(const data_t* type_data) {
    if (!type_data || !type_data->data) {
        return ACTION_UNKNOWN;
    }
    
    if (data_equal_str(type_data, "working_memory_add")) {
        return ACTION_WORKING_MEMORY_ADD;
    } else if (data_equal_str(type_data, "working_memory_remove")) {
        return ACTION_WORKING_MEMORY_REMOVE;
    } else if (data_equal_str(type_data, "storage_load")) {
        return ACTION_STORAGE_LOAD;
    } else if (data_equal_str(type_data, "storage_save")) {
        return ACTION_STORAGE_SAVE;
    } else if (data_equal_str(type_data, "storage_search")) {
        return ACTION_STORAGE_SEARCH;
    }
    
    return ACTION_UNKNOWN;
}

result_t action_parse_xml(pool_t* pool, action_t* action, const object_t* xml_obj) {
    object_t* type_obj = NULL;
    object_t* tags_obj = NULL;
    object_t* value_obj = NULL;
    
    if (!pool || !action || !xml_obj) {
        RETURN_ERR("Invalid arguments for action XML parsing");
    }
    
    // Initialize action structure
    action->type = ACTION_UNKNOWN;
    action->tags = NULL;
    action->value = NULL;
    
    // Extract type
    if (object_provide_str(&type_obj, xml_obj, "type") != RESULT_OK) {
        RETURN_ERR("Failed to extract action type from XML");
    }
    action->type = parse_action_type(type_obj->data);
    if (action->type == ACTION_UNKNOWN) {
        RETURN_ERR("Unknown action type in XML");
    }
    
    // Extract tags
    if (object_provide_str(&tags_obj, xml_obj, "tags") != RESULT_OK) {
        RETURN_ERR("Failed to extract action tags from XML");
    }
    if (data_create_data(pool, &action->tags, tags_obj->data) != RESULT_OK) {
        RETURN_ERR("Failed to copy action tags");
    }
    
    // Extract value (optional for some action types)
    if (object_provide_str(&value_obj, xml_obj, "value") == RESULT_OK) {
        if (data_create_data(pool, &action->value, value_obj->data) != RESULT_OK) {
            // Cleanup tags on failure
            result_t cleanup = data_destroy(pool, action->tags);
            if (cleanup != RESULT_OK) {
                PRINT_ERR("Failed to cleanup action tags after value copy error");
            }
            action->tags = NULL;
            RETURN_ERR("Failed to copy action value");
        }
    }
    
    // Validate that required fields are present based on action type
    switch (action->type) {
        case ACTION_WORKING_MEMORY_ADD:
        case ACTION_STORAGE_SAVE:
            if (!action->value) {
                if (action_cleanup(pool, action) != RESULT_OK) {
                    PRINT_ERR("Failed to cleanup action after validation error");
                }
                RETURN_ERR("Action type requires value but none provided");
            }
            break;
        case ACTION_WORKING_MEMORY_REMOVE:
        case ACTION_STORAGE_LOAD:
        case ACTION_STORAGE_SEARCH:
            // Value is not required for these actions
            break;
        case ACTION_UNKNOWN:
            if (action_cleanup(pool, action) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup action after validation error");
            }
            RETURN_ERR("Unknown action type cannot be validated");
    }
    
    return RESULT_OK;
}

result_t action_execute(pool_t* pool, action_t* action, object_t* agent_memory) {
    if (!pool || !action || !agent_memory) {
        RETURN_ERR("Invalid arguments for action execution");
    }
    
    switch (action->type) {
        case ACTION_WORKING_MEMORY_ADD:
            return action_working_memory_add(pool, agent_memory, action->tags, action->value);
        case ACTION_WORKING_MEMORY_REMOVE:
            return action_working_memory_remove(pool, agent_memory, action->tags);
        case ACTION_STORAGE_LOAD:
            return action_storage_load(pool, agent_memory, action->tags);
        case ACTION_STORAGE_SAVE:
            return action_storage_save(pool, agent_memory, action->tags, action->value);
        case ACTION_STORAGE_SEARCH:
            return action_storage_search(pool, agent_memory, action->tags);
        case ACTION_UNKNOWN:
            RETURN_ERR("Cannot execute unknown action type");
    }
    
    RETURN_ERR("Unhandled action type in execution");
}

result_t action_cleanup(pool_t* pool, action_t* action) {
    if (!pool || !action) {
        return RESULT_OK;  // Nothing to cleanup
    }
    
    result_t result = RESULT_OK;
    
    if (action->tags) {
        if (data_destroy(pool, action->tags) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup action tags");
            result = RESULT_ERR;
        }
        action->tags = NULL;
    }
    
    if (action->value) {
        if (data_destroy(pool, action->value) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup action value");
            result = RESULT_ERR;
        }
        action->value = NULL;
    }
    
    action->type = ACTION_UNKNOWN;
    
    return result;
}
