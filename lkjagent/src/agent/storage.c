#include "agent/storage.h"

result_t agent_storage_add(pool_t* pool, agent_t* agent, json_value_t* storage_add) {
    if (!storage_add || storage_add->type != JSON_TYPE_OBJECT) {
        return RESULT_OK; // Nothing to process
    }

    json_object_t* add_obj = storage_add->u.object_value;
    json_object_element_t* element = add_obj->head;

    while (element) {
        // Add each key-value pair to storage
        if (json_object_set(pool, agent->storage, element->key->data, element->value) != RESULT_OK) {
            RETURN_ERR("Failed to add item to storage");
        }
        element = element->next;
    }

    return RESULT_OK;
}

result_t agent_storage_remove(pool_t* pool, agent_t* agent, json_value_t* storage_remove) {
    if (!storage_remove) {
        return RESULT_OK; // Nothing to process
    }

    if (storage_remove->type == JSON_TYPE_STRING) {
        // Remove single key - ignore if key doesn't exist
        json_value_t* storage_value = agent->storage;
        result_t result = json_object_remove(pool, storage_value, storage_remove->u.string_value->data);
        // Ignore key not found errors, but report other errors
        if (result != RESULT_OK) {
            // Key not found is not an error in this context, just ignore it
        }
    } else if (storage_remove->type == JSON_TYPE_ARRAY) {
        // Remove multiple keys
        json_array_t* remove_array = storage_remove->u.array_value;
        json_array_element_t* element = remove_array->head;

        while (element) {
            if (element->value->type == JSON_TYPE_STRING) {
                json_value_t* storage_value = agent->storage;
                result_t result = json_object_remove(pool, storage_value, element->value->u.string_value->data);
                // Ignore key not found errors, but report other errors
                if (result != RESULT_OK) {
                    // Key not found is not an error in this context, just ignore it
                }
            }
            element = element->next;
        }
    }

    return RESULT_OK;
}
