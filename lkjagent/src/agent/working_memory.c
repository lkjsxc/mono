#include "agent/working_memory.h"

result_t agent_working_memory_add(pool_t* pool, agent_t* agent, json_value_t* working_memory_add) {
    if (!working_memory_add || working_memory_add->type != JSON_TYPE_OBJECT) {
        return RESULT_OK; // Nothing to process
    }

    json_object_t* add_obj = working_memory_add->u.object_value;
    json_object_element_t* element = add_obj->head;

    while (element) {
        // Add each key-value pair to working memory
        if (json_object_set(pool, agent->working_memory, element->key->data, element->value) != RESULT_OK) {
            RETURN_ERR("Failed to add item to working memory");
        }
        element = element->next;
    }

    return RESULT_OK;
}

result_t agent_working_memory_remove(pool_t* pool, agent_t* agent, json_value_t* working_memory_remove) {
    if (!working_memory_remove) {
        return RESULT_OK; // Nothing to process
    }

    if (working_memory_remove->type == JSON_TYPE_STRING) {
        // Remove single key
        json_value_t* working_memory_value = agent->working_memory;
        if (json_object_remove(pool, working_memory_value, working_memory_remove->u.string_value->data) != RESULT_OK) {
            RETURN_ERR("Failed to remove item from working memory");
        }
    } else if (working_memory_remove->type == JSON_TYPE_ARRAY) {
        // Remove multiple keys
        json_array_t* remove_array = working_memory_remove->u.array_value;
        json_array_element_t* element = remove_array->head;

        while (element) {
            if (element->value->type == JSON_TYPE_STRING) {
                json_value_t* working_memory_value = agent->working_memory;
                if (json_object_remove(pool, working_memory_value, element->value->u.string_value->data) != RESULT_OK) {
                    RETURN_ERR("Failed to remove item from working memory");
                }
            }
            element = element->next;
        }
    }

    return RESULT_OK;
}
