#include "utils/json.h"

// Helper function to skip whitespace
static void skip_whitespace(const char** str, uint64_t* remaining) {
    while (*remaining > 0 && (**str == ' ' || **str == '\t' || **str == '\n' || **str == '\r')) {
        (*str)++;
        (*remaining)--;
    }
}

// Helper function to parse a JSON string value
static result_t parse_string(pool_t* pool, const char** str, uint64_t* remaining, string_t** result) {
    if (*remaining == 0 || **str != '"') {
        RETURN_ERR("Expected '\"' at start of string");
    }
    
    (*str)++; // Skip opening quote
    (*remaining)--;
    
    const char* start = *str;
    uint64_t length = 0;
    
    // Find the end quote and calculate length
    while (*remaining > 0 && **str != '"') {
        if (**str == '\\') {
            // Skip escape sequence
            (*str)++;
            (*remaining)--;
            if (*remaining == 0) {
                RETURN_ERR("Unterminated escape sequence in string");
            }
        }
        (*str)++;
        (*remaining)--;
        length++;
    }
    
    if (*remaining == 0) {
        RETURN_ERR("Unterminated string");
    }
    
    // Allocate string and copy content
    if (pool_string_alloc(pool, result, length + 1) != RESULT_OK) {
        RETURN_ERR("Failed to allocate string for JSON string value");
    }
    
    memcpy((*result)->data, start, length);
    (*result)->size = length;
    
    (*str)++; // Skip closing quote
    (*remaining)--;
    
    return RESULT_OK;
}

// Helper function to parse a JSON number
static result_t parse_number(pool_t* pool, const char** str, uint64_t* remaining, double* result) {
    const char* start = *str;
    uint64_t length = 0;
    
    // Parse number characters
    while (*remaining > 0 && ((**str >= '0' && **str <= '9') || **str == '.' || **str == '-' || **str == '+' || **str == 'e' || **str == 'E')) {
        (*str)++;
        (*remaining)--;
        length++;
    }
    
    if (length == 0) {
        RETURN_ERR("Invalid number format");
    }
    
    // Create a null-terminated string for parsing
    char temp[64];
    if (length >= sizeof(temp)) {
        RETURN_ERR("Number too long");
    }
    
    memcpy(temp, start, length);
    temp[length] = '\0';
    
    *result = strtod(temp, NULL);
    return RESULT_OK;
}

// Forward declaration for recursive parsing
static result_t parse_value(pool_t* pool, const char** str, uint64_t* remaining, json_value_t** result);

// Helper function to parse a JSON object
static result_t parse_object(pool_t* pool, const char** str, uint64_t* remaining, json_value_t** result) {
    if (*remaining == 0 || **str != '{') {
        RETURN_ERR("Expected '{' at start of object");
    }
    
    (*str)++; // Skip opening brace
    (*remaining)--;
    
    if (pool_json_value_alloc(pool, result) != RESULT_OK) {
        RETURN_ERR("Failed to allocate JSON value for object");
    }
    
    (*result)->type = JSON_OBJECT;
    (*result)->object.elements = NULL;
    (*result)->object.count = 0;
    
    skip_whitespace(str, remaining);
    
    // Handle empty object
    if (*remaining > 0 && **str == '}') {
        (*str)++;
        (*remaining)--;
        return RESULT_OK;
    }
    
    json_object_element_t* last_element = NULL;
    
    while (*remaining > 0) {
        skip_whitespace(str, remaining);
        
        // Parse key
        string_t* key;
        if (parse_string(pool, str, remaining, &key) != RESULT_OK) {
            RETURN_ERR("Failed to parse object key");
        }
        
        skip_whitespace(str, remaining);
        
        // Expect colon
        if (*remaining == 0 || **str != ':') {
            RETURN_ERR("Expected ':' after object key");
        }
        (*str)++;
        (*remaining)--;
        
        skip_whitespace(str, remaining);
        
        // Parse value
        json_value_t* value;
        if (parse_value(pool, str, remaining, &value) != RESULT_OK) {
            RETURN_ERR("Failed to parse object value");
        }
        
        // Create object element
        json_object_element_t* element;
        if (pool_json_object_element_alloc(pool, &element) != RESULT_OK) {
            RETURN_ERR("Failed to allocate object element");
        }
        
        element->key = key;
        element->value = value;
        element->next = NULL;
        
        // Add to linked list
        if (last_element == NULL) {
            (*result)->object.elements = element;
        } else {
            last_element->next = element;
        }
        last_element = element;
        (*result)->object.count++;
        
        skip_whitespace(str, remaining);
        
        if (*remaining == 0) {
            RETURN_ERR("Unterminated object");
        }
        
        if (**str == '}') {
            (*str)++;
            (*remaining)--;
            break;
        } else if (**str == ',') {
            (*str)++;
            (*remaining)--;
        } else {
            RETURN_ERR("Expected ',' or '}' in object");
        }
    }
    
    return RESULT_OK;
}

// Parse any JSON value
static result_t parse_value(pool_t* pool, const char** str, uint64_t* remaining, json_value_t** result) {
    skip_whitespace(str, remaining);
    
    if (*remaining == 0) {
        RETURN_ERR("Unexpected end of input");
    }
    
    if (**str == 'n' && *remaining >= 4 && memcmp(*str, "null", 4) == 0) {
        // Parse null
        if (pool_json_value_alloc(pool, result) != RESULT_OK) {
            RETURN_ERR("Failed to allocate JSON value for null");
        }
        (*result)->type = JSON_NULL;
        *str += 4;
        *remaining -= 4;
        return RESULT_OK;
    } else if (**str == 't' && *remaining >= 4 && memcmp(*str, "true", 4) == 0) {
        // Parse true
        if (pool_json_value_alloc(pool, result) != RESULT_OK) {
            RETURN_ERR("Failed to allocate JSON value for true");
        }
        (*result)->type = JSON_BOOL;
        (*result)->bool_value = 1;
        *str += 4;
        *remaining -= 4;
        return RESULT_OK;
    } else if (**str == 'f' && *remaining >= 5 && memcmp(*str, "false", 5) == 0) {
        // Parse false
        if (pool_json_value_alloc(pool, result) != RESULT_OK) {
            RETURN_ERR("Failed to allocate JSON value for false");
        }
        (*result)->type = JSON_BOOL;
        (*result)->bool_value = 0;
        *str += 5;
        *remaining -= 5;
        return RESULT_OK;
    } else if (**str == '"') {
        // Parse string
        if (pool_json_value_alloc(pool, result) != RESULT_OK) {
            RETURN_ERR("Failed to allocate JSON value for string");
        }
        (*result)->type = JSON_STRING;
        if (parse_string(pool, str, remaining, &(*result)->string_value) != RESULT_OK) {
            RETURN_ERR("Failed to parse string value");
        }
        return RESULT_OK;
    } else if (**str == '{') {
        // Parse object
        return parse_object(pool, str, remaining, result);
    } else if ((**str >= '0' && **str <= '9') || **str == '-') {
        // Parse number
        if (pool_json_value_alloc(pool, result) != RESULT_OK) {
            RETURN_ERR("Failed to allocate JSON value for number");
        }
        (*result)->type = JSON_NUMBER;
        if (parse_number(pool, str, remaining, &(*result)->number_value) != RESULT_OK) {
            RETURN_ERR("Failed to parse number value");
        }
        return RESULT_OK;
    } else {
        RETURN_ERR("Unexpected character in JSON");
    }
}

result_t json_parse(pool_t* pool, json_value_t** dst, const string_t* src) {
    const char* str = src->data;
    uint64_t remaining = src->size;
    
    skip_whitespace(&str, &remaining);
    
    if (remaining == 0) {
        RETURN_ERR("Empty JSON input");
    }
    
    return parse_value(pool, &str, &remaining, dst);
}

result_t json_parse_xml(pool_t* pool, json_value_t** dst, const string_t* src) {
    // XML parsing not implemented - placeholder
    RETURN_ERR("XML parsing not implemented");
}

// Helper function to write string to output with escaping
static result_t write_string_escaped(pool_t* pool, string_t** dst, const string_t* src) {
    // Calculate required size (worst case: all characters need escaping)
    uint64_t required_size = src->size * 2 + 2; // +2 for quotes
    
    if ((*dst)->capacity < (*dst)->size + required_size) {
        if (pool_string_realloc(pool, dst, (*dst)->size + required_size) != RESULT_OK) {
            RETURN_ERR("Failed to reallocate string for JSON output");
        }
    }
    
    // Add opening quote
    (*dst)->data[(*dst)->size++] = '"';
    
    // Copy string with escaping
    for (uint64_t i = 0; i < src->size; i++) {
        char c = src->data[i];
        if (c == '"' || c == '\\') {
            (*dst)->data[(*dst)->size++] = '\\';
            (*dst)->data[(*dst)->size++] = c;
        } else if (c == '\n') {
            (*dst)->data[(*dst)->size++] = '\\';
            (*dst)->data[(*dst)->size++] = 'n';
        } else if (c == '\r') {
            (*dst)->data[(*dst)->size++] = '\\';
            (*dst)->data[(*dst)->size++] = 'r';
        } else if (c == '\t') {
            (*dst)->data[(*dst)->size++] = '\\';
            (*dst)->data[(*dst)->size++] = 't';
        } else {
            (*dst)->data[(*dst)->size++] = c;
        }
    }
    
    // Add closing quote
    (*dst)->data[(*dst)->size++] = '"';
    
    return RESULT_OK;
}

// Helper function to append string literal to output
static result_t append_str(pool_t* pool, string_t** dst, const char* str) {
    uint64_t len = strlen(str);
    
    if ((*dst)->capacity < (*dst)->size + len) {
        if (pool_string_realloc(pool, dst, (*dst)->size + len + 1024) != RESULT_OK) {
            RETURN_ERR("Failed to reallocate string for JSON output");
        }
    }
    
    memcpy((*dst)->data + (*dst)->size, str, len);
    (*dst)->size += len;
    
    return RESULT_OK;
}

// Forward declaration for recursive serialization
static result_t serialize_value(pool_t* pool, string_t** dst, const json_value_t* src);

// Helper function to serialize JSON object
static result_t serialize_object(pool_t* pool, string_t** dst, const json_value_t* src) {
    if (append_str(pool, dst, "{") != RESULT_OK) {
        RETURN_ERR("Failed to append opening brace");
    }
    
    json_object_element_t* element = src->object.elements;
    uint64_t count = 0;
    
    while (element != NULL) {
        if (count > 0) {
            if (append_str(pool, dst, ",") != RESULT_OK) {
                RETURN_ERR("Failed to append comma");
            }
        }
        
        // Serialize key
        if (write_string_escaped(pool, dst, element->key) != RESULT_OK) {
            RETURN_ERR("Failed to serialize object key");
        }
        
        if (append_str(pool, dst, ":") != RESULT_OK) {
            RETURN_ERR("Failed to append colon");
        }
        
        // Serialize value
        if (serialize_value(pool, dst, element->value) != RESULT_OK) {
            RETURN_ERR("Failed to serialize object value");
        }
        
        element = element->next;
        count++;
    }
    
    if (append_str(pool, dst, "}") != RESULT_OK) {
        RETURN_ERR("Failed to append closing brace");
    }
    
    return RESULT_OK;
}

// Serialize any JSON value
static result_t serialize_value(pool_t* pool, string_t** dst, const json_value_t* src) {
    switch (src->type) {
        case JSON_NULL:
            return append_str(pool, dst, "null");
            
        case JSON_BOOL:
            return append_str(pool, dst, src->bool_value ? "true" : "false");
            
        case JSON_NUMBER: {
            char number_buf[64];
            snprintf(number_buf, sizeof(number_buf), "%.17g", src->number_value);
            return append_str(pool, dst, number_buf);
        }
        
        case JSON_STRING:
            return write_string_escaped(pool, dst, src->string_value);
            
        case JSON_OBJECT:
            return serialize_object(pool, dst, src);
            
        default:
            RETURN_ERR("Unknown JSON type");
    }
}

result_t json_to_string(pool_t* pool, string_t** dst, const json_value_t* src) {
    if (string_create(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to create output string");
    }
    
    return serialize_value(pool, dst, src);
}

result_t json_to_string_xml(pool_t* pool, string_t** dst, const json_value_t* src) {
    // XML serialization not implemented - placeholder
    RETURN_ERR("XML serialization not implemented");
}

result_t json_object_set(pool_t* pool, json_value_t* object, const string_t* path, json_value_t* value) {
    if (object->type != JSON_OBJECT) {
        RETURN_ERR("Cannot set property on non-object");
    }
    
    // Find existing element or create new one
    json_object_element_t* element = object->object.elements;
    json_object_element_t* last = NULL;
    
    while (element != NULL) {
        if (string_equal_string(element->key, path)) {
            // Replace existing value
            element->value = value;
            return RESULT_OK;
        }
        last = element;
        element = element->next;
    }
    
    // Create new element
    if (pool_json_object_element_alloc(pool, &element) != RESULT_OK) {
        RETURN_ERR("Failed to allocate object element");
    }
    
    // Create key copy
    if (string_create_string(pool, &element->key, path) != RESULT_OK) {
        RETURN_ERR("Failed to create key string");
    }
    
    element->value = value;
    element->next = NULL;
    
    // Add to list
    if (last == NULL) {
        object->object.elements = element;
    } else {
        last->next = element;
    }
    object->object.count++;
    
    return RESULT_OK;
}

result_t json_object_set_string(pool_t* pool, json_value_t* object, const string_t* path, const string_t* value) {
    json_value_t* json_value;
    if (pool_json_value_alloc(pool, &json_value) != RESULT_OK) {
        RETURN_ERR("Failed to allocate JSON value for string");
    }
    
    json_value->type = JSON_STRING;
    if (string_create_string(pool, &json_value->string_value, value) != RESULT_OK) {
        RETURN_ERR("Failed to create string value");
    }
    
    return json_object_set(pool, object, path, json_value);
}

result_t json_object_get(json_value_t** dst, const json_value_t* object, const string_t* path) {
    if (object->type != JSON_OBJECT) {
        RETURN_ERR("Cannot get property from non-object");
    }
    
    json_object_element_t* element = object->object.elements;
    
    while (element != NULL) {
        if (string_equal_string(element->key, path)) {
            *dst = element->value;
            return RESULT_OK;
        }
        element = element->next;
    }
    
    RETURN_ERR("Property not found in object");
}

result_t json_destroy(pool_t* pool, json_value_t* value) {
    if (value == NULL) {
        return RESULT_OK;
    }
    
    switch (value->type) {
        case JSON_STRING:
            if (string_destroy(pool, value->string_value) != RESULT_OK) {
                RETURN_ERR("Failed to destroy string value");
            }
            break;
            
        case JSON_OBJECT: {
            json_object_element_t* element = value->object.elements;
            while (element != NULL) {
                json_object_element_t* next = element->next;
                
                if (string_destroy(pool, element->key) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy object key");
                }
                
                if (json_destroy(pool, element->value) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy object value");
                }
                
                if (pool_json_object_element_free(pool, element) != RESULT_OK) {
                    RETURN_ERR("Failed to free object element");
                }
                
                element = next;
            }
            break;
        }
        
        case JSON_NULL:
        case JSON_BOOL:
        case JSON_NUMBER:
            // No additional cleanup needed
            break;
            
        default:
            RETURN_ERR("Unknown JSON type during destruction");
    }
    
    return pool_json_value_free(pool, value);
}
