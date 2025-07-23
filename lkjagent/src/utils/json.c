#include "utils/lkjjson.h"

// Helper function prototypes
static result_t json_parse_value(pool_t* pool, const char** cursor, json_value_t** value);
static result_t json_parse_object(pool_t* pool, const char** cursor, json_value_t** value);
static result_t json_parse_array(pool_t* pool, const char** cursor, json_value_t** value);
static result_t json_parse_string(pool_t* pool, const char** cursor, json_value_t** value);
static result_t json_parse_number(pool_t* pool, const char** cursor, json_value_t** value);
static result_t json_parse_literal(pool_t* pool, const char** cursor, json_value_t** value);
static void json_skip_whitespace(const char** cursor);
static result_t json_stringify_value(pool_t* pool, const json_value_t* value, string_t* output);
static result_t json_escape_string(pool_t* pool, const char* str, string_t* output);

result_t json_parse(pool_t* pool, const string_t* json_string, json_value_t** value) {
    if (!pool || !json_string || !value) {
        RETURN_ERR("Invalid arguments to json_parse");
    }

    const char* cursor = json_string->data;
    json_skip_whitespace(&cursor);

    return json_parse_value(pool, &cursor, value);
}

static result_t json_parse_value(pool_t* pool, const char** cursor, json_value_t** value) {
    json_skip_whitespace(cursor);

    if (**cursor == '{') {
        return json_parse_object(pool, cursor, value);
    } else if (**cursor == '[') {
        return json_parse_array(pool, cursor, value);
    } else if (**cursor == '"') {
        return json_parse_string(pool, cursor, value);
    } else if (**cursor == '-' || (**cursor >= '0' && **cursor <= '9')) {
        return json_parse_number(pool, cursor, value);
    } else if (**cursor == 't' || **cursor == 'f' || **cursor == 'n') {
        return json_parse_literal(pool, cursor, value);
    }

    return RESULT_ERR;
}

static result_t json_parse_object(pool_t* pool, const char** cursor, json_value_t** value) {
    if (pool_json_value_alloc(pool, value) != RESULT_OK) {
        return RESULT_ERR;
    }

    json_object_t* object;
    if (pool_json_object_alloc(pool, &object) != RESULT_OK) {
        if (pool_json_value_free(pool, *value) != RESULT_OK) {
            RETURN_ERR("Failed to free JSON value after object allocation failure");
        }
        return RESULT_ERR;
    }

    (*value)->type = JSON_TYPE_OBJECT;
    (*value)->u.object_value = object;

    (*cursor)++;  // Skip '{'
    json_skip_whitespace(cursor);

    if (**cursor == '}') {
        (*cursor)++;  // Skip '}'
        return RESULT_OK;
    }

    while (**cursor != '\0') {
        // Parse key
        json_value_t* key_value;
        if (json_parse_string(pool, cursor, &key_value) != RESULT_OK) {
            return RESULT_ERR;
        }

        json_skip_whitespace(cursor);
        if (**cursor != ':') {
            return RESULT_ERR;
        }
        (*cursor)++;  // Skip ':'

        // Parse value
        json_value_t* element_value;
        if (json_parse_value(pool, cursor, &element_value) != RESULT_OK) {
            return RESULT_ERR;
        }

        // Add to object
        if (json_object_set(pool, *value, key_value->u.string_value->data, element_value) != RESULT_OK) {
            return RESULT_ERR;
        }

        json_skip_whitespace(cursor);
        if (**cursor == '}') {
            (*cursor)++;  // Skip '}'
            break;
        } else if (**cursor == ',') {
            (*cursor)++;  // Skip ','
            json_skip_whitespace(cursor);
        } else {
            return RESULT_ERR;
        }
    }

    return RESULT_OK;
}

static result_t json_parse_array(pool_t* pool, const char** cursor, json_value_t** value) {
    if (pool_json_value_alloc(pool, value) != RESULT_OK) {
        return RESULT_ERR;
    }

    json_array_t* array;
    if (pool_json_array_alloc(pool, &array) != RESULT_OK) {
        if (pool_json_value_free(pool, *value) != RESULT_OK) {
            RETURN_ERR("Failed to free JSON value after array allocation failure");
        }
        return RESULT_ERR;
    }

    (*value)->type = JSON_TYPE_ARRAY;
    (*value)->u.array_value = array;

    (*cursor)++;  // Skip '['
    json_skip_whitespace(cursor);

    if (**cursor == ']') {
        (*cursor)++;  // Skip ']'
        return RESULT_OK;
    }

    while (**cursor != '\0') {
        json_value_t* element_value;
        if (json_parse_value(pool, cursor, &element_value) != RESULT_OK) {
            return RESULT_ERR;
        }

        if (json_array_append(pool, *value, element_value) != RESULT_OK) {
            return RESULT_ERR;
        }

        json_skip_whitespace(cursor);
        if (**cursor == ']') {
            (*cursor)++;  // Skip ']'
            break;
        } else if (**cursor == ',') {
            (*cursor)++;  // Skip ','
            json_skip_whitespace(cursor);
        } else {
            return RESULT_ERR;
        }
    }

    return RESULT_OK;
}

static result_t json_parse_string(pool_t* pool, const char** cursor, json_value_t** value) {
    if (pool_json_value_alloc(pool, value) != RESULT_OK) {
        return RESULT_ERR;
    }

    string_t* str;
    if (pool_string_alloc(pool, &str, 256) != RESULT_OK) {
        if (pool_json_value_free(pool, *value) != RESULT_OK) {
            RETURN_ERR("Failed to free JSON value after string allocation failure");
        }
        return RESULT_ERR;
    }

    (*value)->type = JSON_TYPE_STRING;
    (*value)->u.string_value = str;

    (*cursor)++;  // Skip opening '"'

    while (**cursor != '\0' && **cursor != '"') {
        if (**cursor == '\\') {
            (*cursor)++;  // Skip backslash
            char escaped;
            switch (**cursor) {
                case '"':
                    escaped = '"';
                    break;
                case '\\':
                    escaped = '\\';
                    break;
                case '/':
                    escaped = '/';
                    break;
                case 'b':
                    escaped = '\b';
                    break;
                case 'f':
                    escaped = '\f';
                    break;
                case 'n':
                    escaped = '\n';
                    break;
                case 'r':
                    escaped = '\r';
                    break;
                case 't':
                    escaped = '\t';
                    break;
                default:
                    return RESULT_ERR;
            }
            if (string_append_char(pool, &str, escaped) != RESULT_OK) {
                return RESULT_ERR;
            }
        } else {
            if (string_append_char(pool, &str, **cursor) != RESULT_OK) {
                return RESULT_ERR;
            }
        }
        (*cursor)++;
    }

    if (**cursor != '"') {
        return RESULT_ERR;
    }
    (*cursor)++;  // Skip closing '"'

    return RESULT_OK;
}

static result_t json_parse_number(pool_t* pool, const char** cursor, json_value_t** value) {
    if (pool_json_value_alloc(pool, value) != RESULT_OK) {
        return RESULT_ERR;
    }

    (*value)->type = JSON_TYPE_NUMBER;

    char* endptr;
    (*value)->u.number_value = strtod(*cursor, &endptr);

    if (endptr == *cursor) {
        if (pool_json_value_free(pool, *value) != RESULT_OK) {
            RETURN_ERR("Failed to free JSON value after number parsing failure");
        }
        return RESULT_ERR;
    }

    *cursor = endptr;
    return RESULT_OK;
}

static result_t json_parse_literal(pool_t* pool, const char** cursor, json_value_t** value) {
    if (pool_json_value_alloc(pool, value) != RESULT_OK) {
        return RESULT_ERR;
    }

    if (strncmp(*cursor, "true", 4) == 0) {
        (*value)->type = JSON_TYPE_BOOL;
        (*value)->u.bool_value = 1;
        *cursor += 4;
    } else if (strncmp(*cursor, "false", 5) == 0) {
        (*value)->type = JSON_TYPE_BOOL;
        (*value)->u.bool_value = 0;
        *cursor += 5;
    } else if (strncmp(*cursor, "null", 4) == 0) {
        (*value)->type = JSON_TYPE_NULL;
        *cursor += 4;
    } else {
        if (pool_json_value_free(pool, *value) != RESULT_OK) {
            RETURN_ERR("Failed to free JSON value after literal parsing failure");
        }
        return RESULT_ERR;
    }

    return RESULT_OK;
}

static void json_skip_whitespace(const char** cursor) {
    while (**cursor == ' ' || **cursor == '\t' || **cursor == '\n' || **cursor == '\r') {
        (*cursor)++;
    }
}

result_t json_stringify(pool_t* pool, const json_value_t* value, string_t* output) {
    if (!pool || !value || !output) {
        return RESULT_ERR;
    }

    string_clear(output);
    return json_stringify_value(pool, value, output);
}

static result_t json_stringify_value(pool_t* pool, const json_value_t* value, string_t* output) {
    switch (value->type) {
        case JSON_TYPE_NULL:
            return string_append_str(pool, &output, "null");

        case JSON_TYPE_BOOL:
            return string_append_str(pool, &output, value->u.bool_value ? "true" : "false");

        case JSON_TYPE_NUMBER: {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%.15g", value->u.number_value);
            return string_append_str(pool, &output, buffer);
        }

        case JSON_TYPE_STRING:
            if (string_append_char(pool, &output, '"') != RESULT_OK)
                return RESULT_ERR;
            if (json_escape_string(pool, value->u.string_value->data, output) != RESULT_OK)
                return RESULT_ERR;
            return string_append_char(pool, &output, '"');

        case JSON_TYPE_ARRAY: {
            if (string_append_char(pool, &output, '[') != RESULT_OK)
                return RESULT_ERR;

            json_array_element_t* element = value->u.array_value->head;
            int first = 1;
            while (element) {
                if (!first) {
                    if (string_append_char(pool, &output, ',') != RESULT_OK)
                        return RESULT_ERR;
                }
                if (json_stringify_value(pool, element->value, output) != RESULT_OK)
                    return RESULT_ERR;
                element = element->next;
                first = 0;
            }

            return string_append_char(pool, &output, ']');
        }

        case JSON_TYPE_OBJECT: {
            if (string_append_char(pool, &output, '{') != RESULT_OK)
                return RESULT_ERR;

            json_object_element_t* element = value->u.object_value->head;
            int first = 1;
            while (element) {
                if (!first) {
                    if (string_append_char(pool, &output, ',') != RESULT_OK)
                        return RESULT_ERR;
                }

                if (string_append_char(pool, &output, '"') != RESULT_OK)
                    return RESULT_ERR;
                if (json_escape_string(pool, element->key->data, output) != RESULT_OK)
                    return RESULT_ERR;
                if (string_append_str(pool, &output, "\":") != RESULT_OK)
                    return RESULT_ERR;
                if (json_stringify_value(pool, element->value, output) != RESULT_OK)
                    return RESULT_ERR;

                element = element->next;
                first = 0;
            }

            return string_append_char(pool, &output, '}');
        }
    }

    return RESULT_ERR;
}

static result_t json_escape_string(pool_t* pool, const char* str, string_t* output) {
    while (*str) {
        switch (*str) {
            case '"':
                if (string_append_str(pool, &output, "\\\"") != RESULT_OK)
                    return RESULT_ERR;
                break;
            case '\\':
                if (string_append_str(pool, &output, "\\\\") != RESULT_OK)
                    return RESULT_ERR;
                break;
            case '\b':
                if (string_append_str(pool, &output, "\\b") != RESULT_OK)
                    return RESULT_ERR;
                break;
            case '\f':
                if (string_append_str(pool, &output, "\\f") != RESULT_OK)
                    return RESULT_ERR;
                break;
            case '\n':
                if (string_append_str(pool, &output, "\\n") != RESULT_OK)
                    return RESULT_ERR;
                break;
            case '\r':
                if (string_append_str(pool, &output, "\\r") != RESULT_OK)
                    return RESULT_ERR;
                break;
            case '\t':
                if (string_append_str(pool, &output, "\\t") != RESULT_OK)
                    return RESULT_ERR;
                break;
            default:
                if (string_append_char(pool, &output, *str) != RESULT_OK)
                    return RESULT_ERR;
                break;
        }
        str++;
    }

    return RESULT_OK;
}

result_t json_create_null(pool_t* pool, json_value_t** value) {
    if (pool_json_value_alloc(pool, value) != RESULT_OK) {
        return RESULT_ERR;
    }

    (*value)->type = JSON_TYPE_NULL;
    return RESULT_OK;
}

result_t json_create_bool(pool_t* pool, int bool_val, json_value_t** value) {
    if (pool_json_value_alloc(pool, value) != RESULT_OK) {
        return RESULT_ERR;
    }

    (*value)->type = JSON_TYPE_BOOL;
    (*value)->u.bool_value = bool_val;
    return RESULT_OK;
}

result_t json_create_number(pool_t* pool, double number_val, json_value_t** value) {
    if (pool_json_value_alloc(pool, value) != RESULT_OK) {
        return RESULT_ERR;
    }

    (*value)->type = JSON_TYPE_NUMBER;
    (*value)->u.number_value = number_val;
    return RESULT_OK;
}

result_t json_create_string(pool_t* pool, const char* string_val, json_value_t** value) {
    if (pool_json_value_alloc(pool, value) != RESULT_OK) {
        return RESULT_ERR;
    }

    string_t* str;
    if (pool_string_alloc(pool, &str, 256) != RESULT_OK) {
        if (pool_json_value_free(pool, *value) != RESULT_OK) {
            RETURN_ERR("Failed to free JSON value after string allocation failure");
        }
        return RESULT_ERR;
    }

    if (string_assign(pool, &str, string_val) != RESULT_OK) {
        if (pool_string_free(pool, str) != RESULT_OK) {
            RETURN_ERR("Failed to free string after string assignment failure");
        }
        if (pool_json_value_free(pool, *value) != RESULT_OK) {
            RETURN_ERR("Failed to free JSON value after string assignment failure");
        }
        return RESULT_ERR;
    }

    (*value)->type = JSON_TYPE_STRING;
    (*value)->u.string_value = str;
    return RESULT_OK;
}

result_t json_create_object(pool_t* pool, json_value_t** value) {
    if (pool_json_value_alloc(pool, value) != RESULT_OK) {
        return RESULT_ERR;
    }

    json_object_t* object;
    if (pool_json_object_alloc(pool, &object) != RESULT_OK) {
        if (pool_json_value_free(pool, *value) != RESULT_OK) {
            RETURN_ERR("Failed to free JSON value after object allocation failure");
        }
        return RESULT_ERR;
    }

    (*value)->type = JSON_TYPE_OBJECT;
    (*value)->u.object_value = object;
    return RESULT_OK;
}

result_t json_create_array(pool_t* pool, json_value_t** value) {
    if (pool_json_value_alloc(pool, value) != RESULT_OK) {
        return RESULT_ERR;
    }

    json_array_t* array;
    if (pool_json_array_alloc(pool, &array) != RESULT_OK) {
        if (pool_json_value_free(pool, *value) != RESULT_OK) {
            RETURN_ERR("Failed to free JSON value after array allocation failure");
        }
        return RESULT_ERR;
    }

    (*value)->type = JSON_TYPE_ARRAY;
    (*value)->u.array_value = array;
    return RESULT_OK;
}

result_t json_object_set(pool_t* pool, json_value_t* object, const char* key, json_value_t* value) {
    if (!object || object->type != JSON_TYPE_OBJECT || !key || !value) {
        return RESULT_ERR;
    }

    json_object_element_t* element;
    if (pool_json_object_element_alloc(pool, &element) != RESULT_OK) {
        return RESULT_ERR;
    }

    string_t* key_string;
    if (pool_string_alloc(pool, &key_string, 256) != RESULT_OK) {
        if (pool_json_object_element_free(pool, element) != RESULT_OK) {
            RETURN_ERR("Failed to free object element after key string allocation failure");
        }
        return RESULT_ERR;
    }

    if (string_assign(pool, &key_string, key) != RESULT_OK) {
        if (pool_string_free(pool, key_string) != RESULT_OK) {
            RETURN_ERR("Failed to free key string after assignment failure");
        }
        if (pool_json_object_element_free(pool, element) != RESULT_OK) {
            RETURN_ERR("Failed to free object element after key assignment failure");
        }
        return RESULT_ERR;
    }

    element->key = key_string;
    element->value = value;
    element->next = object->u.object_value->head;
    object->u.object_value->head = element;
    object->u.object_value->length++;

    return RESULT_OK;
}

json_value_t* json_object_get(const json_value_t* object, const char* key) {
    if (!object || object->type != JSON_TYPE_OBJECT || !key) {
        return NULL;
    }

    json_object_element_t* element = object->u.object_value->head;
    while (element) {
        if (string_equal_str(element->key, key)) {
            return element->value;
        }
        element = element->next;
    }

    return NULL;
}

result_t json_array_append(pool_t* pool, json_value_t* array, json_value_t* value) {
    if (!array || array->type != JSON_TYPE_ARRAY || !value) {
        return RESULT_ERR;
    }

    json_array_element_t* element;
    if (pool_json_array_element_alloc(pool, &element) != RESULT_OK) {
        return RESULT_ERR;
    }

    element->value = value;
    element->next = NULL;

    if (array->u.array_value->head == NULL) {
        array->u.array_value->head = element;
    } else {
        json_array_element_t* current = array->u.array_value->head;
        while (current->next) {
            current = current->next;
        }
        current->next = element;
    }

    array->u.array_value->length++;
    return RESULT_OK;
}

json_value_t* json_array_get(const json_value_t* array, uint64_t index) {
    if (!array || array->type != JSON_TYPE_ARRAY) {
        return NULL;
    }

    if (index >= array->u.array_value->length) {
        return NULL;
    }

    json_array_element_t* element = array->u.array_value->head;
    for (uint64_t i = 0; i < index && element; i++) {
        element = element->next;
    }

    return element ? element->value : NULL;
}

uint64_t json_array_length(const json_value_t* array) {
    if (!array || array->type != JSON_TYPE_ARRAY) {
        return 0;
    }

    return array->u.array_value->length;
}

uint64_t json_object_length(const json_value_t* object) {
    if (!object || object->type != JSON_TYPE_OBJECT) {
        return 0;
    }

    return object->u.object_value->length;
}

result_t json_delete(pool_t* pool, json_value_t* value) {
    if (!pool || !value) {
        RETURN_ERR("Invalid arguments to json_delete");
    }

    switch (value->type) {
        case JSON_TYPE_STRING:
            if (value->u.string_value) {
                if (pool_string_free(pool, value->u.string_value) != RESULT_OK) {
                    RETURN_ERR("Failed to free string value");
                }
            }
            break;

        case JSON_TYPE_OBJECT:
            if (value->u.object_value) {
                // Free all object elements and their contents
                json_object_element_t* element = value->u.object_value->head;
                while (element) {
                    json_object_element_t* next = element->next;
                    
                    // Free the key string
                    if (element->key && pool_string_free(pool, element->key) != RESULT_OK) {
                        RETURN_ERR("Failed to free object key");
                    }
                    
                    // Recursively delete the value
                    if (element->value && json_delete(pool, element->value) != RESULT_OK) {
                        RETURN_ERR("Failed to delete object element value");
                    }
                    
                    // Free the element itself
                    if (pool_json_object_element_free(pool, element) != RESULT_OK) {
                        RETURN_ERR("Failed to free object element");
                    }
                    
                    element = next;
                }
                
                // Free the object structure
                if (pool_json_object_free(pool, value->u.object_value) != RESULT_OK) {
                    RETURN_ERR("Failed to free object");
                }
            }
            break;

        case JSON_TYPE_ARRAY:
            if (value->u.array_value) {
                // Free all array elements and their contents
                json_array_element_t* element = value->u.array_value->head;
                while (element) {
                    json_array_element_t* next = element->next;
                    
                    // Recursively delete the value
                    if (element->value && json_delete(pool, element->value) != RESULT_OK) {
                        RETURN_ERR("Failed to delete array element value");
                    }
                    
                    // Free the element itself
                    if (pool_json_array_element_free(pool, element) != RESULT_OK) {
                        RETURN_ERR("Failed to free array element");
                    }
                    
                    element = next;
                }
                
                // Free the array structure
                if (pool_json_array_free(pool, value->u.array_value) != RESULT_OK) {
                    RETURN_ERR("Failed to free array");
                }
            }
            break;

        case JSON_TYPE_NULL:
        case JSON_TYPE_BOOL:
        case JSON_TYPE_NUMBER:
            // These types don't have additional memory to free
            break;
    }

    // Free the value itself
    if (pool_json_value_free(pool, value) != RESULT_OK) {
        RETURN_ERR("Failed to free JSON value");
    }

    return RESULT_OK;
}

result_t json_object_remove(pool_t* pool, json_value_t* object, const char* key) {
    if (!pool || !object || !key || object->type != JSON_TYPE_OBJECT) {
        RETURN_ERR("Invalid arguments to json_object_remove");
    }

    json_object_t* obj = object->u.object_value;
    if (!obj) {
        RETURN_ERR("Object value is NULL");
    }

    json_object_element_t* prev = NULL;
    json_object_element_t* current = obj->head;

    // Search for the element with the matching key
    while (current) {
        if (current->key && strcmp(current->key->data, key) == 0) {
            // Found the element to remove
            
            // Update the linked list
            if (prev) {
                prev->next = current->next;
            } else {
                obj->head = current->next;
            }
            
            // Decrease object length
            obj->length--;
            
            // Free the key string
            if (current->key && pool_string_free(pool, current->key) != RESULT_OK) {
                RETURN_ERR("Failed to free object key");
            }
            
            // Recursively delete the value
            if (current->value && json_delete(pool, current->value) != RESULT_OK) {
                RETURN_ERR("Failed to delete object element value");
            }
            
            // Free the element itself
            if (pool_json_object_element_free(pool, current) != RESULT_OK) {
                RETURN_ERR("Failed to free object element");
            }
            
            return RESULT_OK;
        }
        
        prev = current;
        current = current->next;
    }

    RETURN_ERR("Key not found in object");
}

result_t json_array_remove(pool_t* pool, json_value_t* array, uint64_t index) {
    if (!pool || !array || array->type != JSON_TYPE_ARRAY) {
        RETURN_ERR("Invalid arguments to json_array_remove");
    }

    json_array_t* arr = array->u.array_value;
    if (!arr) {
        RETURN_ERR("Array value is NULL");
    }

    if (index >= arr->length) {
        RETURN_ERR("Index out of bounds");
    }

    json_array_element_t* prev = NULL;
    json_array_element_t* current = arr->head;

    // Navigate to the element at the specified index
    for (uint64_t i = 0; i < index && current; i++) {
        prev = current;
        current = current->next;
    }

    if (!current) {
        RETURN_ERR("Element not found at index");
    }

    // Update the linked list
    if (prev) {
        prev->next = current->next;
    } else {
        arr->head = current->next;
    }

    // Decrease array length
    arr->length--;

    // Recursively delete the value
    if (current->value && json_delete(pool, current->value) != RESULT_OK) {
        RETURN_ERR("Failed to delete array element value");
    }

    // Free the element itself
    if (pool_json_array_element_free(pool, current) != RESULT_OK) {
        RETURN_ERR("Failed to free array element");
    }

    return RESULT_OK;
}
