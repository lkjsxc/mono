#include "agent/process.h"

// Forward declarations for static helper functions
static result_t agent_process_action(pool_t* pool, agent_t* agent, json_value_t* action);
static result_t agent_process_tagged_operation(pool_t* pool, agent_t* agent, const char* operation, json_value_t* tags, json_value_t* value);
static result_t agent_process_legacy_operations(pool_t* pool, agent_t* agent, json_value_t* response_json);

// Helper function to process action-based format
static result_t agent_process_action(pool_t* pool, agent_t* agent, json_value_t* action) {
    json_value_t* type = json_object_get(action, "type");
    if (!type || type->type != JSON_TYPE_STRING) {
        RETURN_ERR("Action missing or invalid type field");
    }

    const char* action_type = type->u.string_value->data;
    json_value_t* tags = json_object_get(action, "tags");
    json_value_t* value = json_object_get(action, "value");

    if (strcmp(action_type, "working_memory_add") == 0) {
        return agent_process_tagged_operation(pool, agent, "working_memory_add", tags, value);
    } else if (strcmp(action_type, "working_memory_remove") == 0) {
        return agent_process_tagged_operation(pool, agent, "working_memory_remove", tags, value);
    } else if (strcmp(action_type, "storage_add") == 0) {
        return agent_process_tagged_operation(pool, agent, "storage_add", tags, value);
    } else if (strcmp(action_type, "storage_remove") == 0) {
        return agent_process_tagged_operation(pool, agent, "storage_remove", tags, value);
    } else {
        RETURN_ERR("Unknown action type");
    }
}

// Helper function to process tagged operations
static result_t agent_process_tagged_operation(pool_t* pool, agent_t* agent, const char* operation, json_value_t* tags, json_value_t* value) {
    if (!tags || tags->type != JSON_TYPE_ARRAY) {
        RETURN_ERR("Tags field missing or not an array");
    }

    // Create a key from tags by joining them with underscores
    string_t* key;
    if (pool_string_alloc(pool, &key, 1024) != RESULT_OK) {
        RETURN_ERR("Failed to allocate key string");
    }

    json_array_t* tags_array = tags->u.array_value;
    json_array_element_t* element = tags_array->head;
    int first = 1;

    while (element) {
        if (element->value->type == JSON_TYPE_STRING) {
            if (!first) {
                if (string_append_str(pool, &key, "_") != RESULT_OK) {
                    if (pool_string_free(pool, key) != RESULT_OK) {
                        RETURN_ERR("Failed to free key string and append underscore");
                    }
                    RETURN_ERR("Failed to append underscore to key");
                }
            }
            if (string_append_str(pool, &key, element->value->u.string_value->data) != RESULT_OK) {
                if (pool_string_free(pool, key) != RESULT_OK) {
                    RETURN_ERR("Failed to free key string and append tag");
                }
                RETURN_ERR("Failed to append tag to key");
            }
            first = 0;
        }
        element = element->next;
    }

    result_t result = RESULT_OK;

    if (strcmp(operation, "working_memory_add") == 0) {
        if (json_object_set(pool, agent->working_memory, key->data, value) != RESULT_OK) {
            result = RESULT_ERR;
        }
    } else if (strcmp(operation, "working_memory_remove") == 0) {
        // Ignore result for remove operations (key might not exist)
        result_t remove_result = json_object_remove(pool, agent->working_memory, key->data);
        (void)remove_result; // Explicitly ignore the result
    } else if (strcmp(operation, "storage_add") == 0) {
        if (json_object_set(pool, agent->storage, key->data, value) != RESULT_OK) {
            result = RESULT_ERR;
        }
    } else if (strcmp(operation, "storage_remove") == 0) {
        // Ignore result for remove operations (key might not exist)
        result_t remove_result = json_object_remove(pool, agent->storage, key->data);
        (void)remove_result; // Explicitly ignore the result
    }

    if (pool_string_free(pool, key) != RESULT_OK) {
        RETURN_ERR("Failed to free key string");
    }

    if (result != RESULT_OK) {
        RETURN_ERR("Failed to process tagged operation");
    }

    return RESULT_OK;
}

// Helper function to process legacy direct operations format
static result_t agent_process_legacy_operations(pool_t* pool, agent_t* agent, json_value_t* response_json) {
    // Process working_memory_add operations
    json_value_t* working_memory_add = json_object_get(response_json, "working_memory_add");
    if (agent_working_memory_add(pool, agent, working_memory_add) != RESULT_OK) {
        RETURN_ERR("Failed to process working memory add operations");
    }

    // Process working_memory_remove operations
    json_value_t* working_memory_remove = json_object_get(response_json, "working_memory_remove");
    if (agent_working_memory_remove(pool, agent, working_memory_remove) != RESULT_OK) {
        RETURN_ERR("Failed to process working memory remove operations");
    }

    // Process storage_add operations
    json_value_t* storage_add = json_object_get(response_json, "storage_add");
    if (agent_storage_add(pool, agent, storage_add) != RESULT_OK) {
        RETURN_ERR("Failed to process storage add operations");
    }

    // Process storage_remove operations
    json_value_t* storage_remove = json_object_get(response_json, "storage_remove");
    if (agent_storage_remove(pool, agent, storage_remove) != RESULT_OK) {
        RETURN_ERR("Failed to process storage remove operations");
    }

    // Process status_change operations
    json_value_t* status_change = json_object_get(response_json, "status_change");
    if (agent_status_change(agent, status_change) != RESULT_OK) {
        RETURN_ERR("Failed to process status change operations");
    }

    return RESULT_OK;
}

result_t agent_process(pool_t* pool, __attribute__((unused)) config_t* config, agent_t* agent, const string_t* response_text) {
    // Parse response and extract JSON payload
    json_value_t* response_json;
    if (agent_parse_response(pool, response_text, &response_json) != RESULT_OK) {
        RETURN_ERR("Failed to parse agent response");
    }

    // Check if this is the new format with "action" or "next_state"
    json_value_t* action = json_object_get(response_json, "action");
    json_value_t* next_state = json_object_get(response_json, "next_state");

    if (action && action->type == JSON_TYPE_OBJECT) {
        // Handle new action-based format
        if (agent_process_action(pool, agent, action) != RESULT_OK) {
            if (pool_json_value_free(pool, response_json) != RESULT_OK) {
                RETURN_ERR("Failed to free response JSON value and process action");
            }
            RETURN_ERR("Failed to process action operations");
        }
    } else {
        // Handle legacy direct operations format
        if (agent_process_legacy_operations(pool, agent, response_json) != RESULT_OK) {
            if (pool_json_value_free(pool, response_json) != RESULT_OK) {
                RETURN_ERR("Failed to free response JSON value and process legacy operations");
            }
            RETURN_ERR("Failed to process legacy operations");
        }
    }

    // Handle next_state (status change)
    if (next_state && next_state->type == JSON_TYPE_STRING) {
        if (agent_status_change(agent, next_state) != RESULT_OK) {
            if (pool_json_value_free(pool, response_json) != RESULT_OK) {
                RETURN_ERR("Failed to free response JSON value and process status change");
            }
            RETURN_ERR("Failed to process status change operations");
        }
    }

    if (pool_json_value_free(pool, response_json) != RESULT_OK) {
        RETURN_ERR("Failed to free response JSON value");
    }

    return RESULT_OK;
}
