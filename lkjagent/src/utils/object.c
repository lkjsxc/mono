#include "utils/object.h"
#include <ctype.h>

// Helper function to recursively destroy an object tree
static result_t object_destroy_recursive(pool_t* pool, object_t* object) {
    if (!object) {
        return RESULT_OK;
    }
    
    // Destroy children first
    if (object->child) {
        if (object_destroy_recursive(pool, object->child) != RESULT_OK) {
            RETURN_ERR("Failed to destroy object children");
        }
    }
    
    // Destroy siblings
    if (object->next) {
        if (object_destroy_recursive(pool, object->next) != RESULT_OK) {
            RETURN_ERR("Failed to destroy object siblings");
        }
    }
    
    // Destroy the string if it exists
    if (object->string) {
        if (string_destroy(pool, object->string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy object string");
        }
    }
    
    // Free the object itself
    if (pool_object_free(pool, object) != RESULT_OK) {
        RETURN_ERR("Failed to free object");
    }
    
    return RESULT_OK;
}

// Helper function to skip whitespace in JSON parsing
static const char* skip_whitespace(const char* json, const char* end) {
    while (json < end && (*json == ' ' || *json == '\t' || *json == '\n' || *json == '\r')) {
        json++;
    }
    return json;
}

// Helper function to parse a JSON string value
static result_t parse_json_string(pool_t* pool, const char** json, const char* end, string_t** result) {
    const char* start = *json;
    
    if (start >= end || *start != '"') {
        RETURN_ERR("Expected opening quote for JSON string");
    }
    
    start++; // Skip opening quote
    const char* current = start;
    
    // Find the closing quote (handle escapes later)
    while (current < end && *current != '"') {
        if (*current == '\\' && current + 1 < end) {
            current += 2; // Skip escaped character
        } else {
            current++;
        }
    }
    
    if (current >= end) {
        RETURN_ERR("Unterminated JSON string");
    }
    
    // Create string with the content between quotes
    size_t length = current - start;
    if (pool_string_alloc(pool, result, length + 1) != RESULT_OK) {
        RETURN_ERR("Failed to allocate string for JSON value");
    }
    
    (*result)->size = length;
    memcpy((*result)->data, start, length);
    
    *json = current + 1; // Skip closing quote
    return RESULT_OK;
}

// Forward declaration for recursive parsing
static result_t parse_json_value(pool_t* pool, const char** json, const char* end, object_t** result);

// Helper function to parse a JSON object
static result_t parse_json_object(pool_t* pool, const char** json, const char* end, object_t** result) {
    const char* current = *json;
    
    if (current >= end || *current != '{') {
        RETURN_ERR("Expected opening brace for JSON object");
    }
    
    current++; // Skip opening brace
    current = skip_whitespace(current, end);
    
    if (pool_object_alloc(pool, result) != RESULT_OK) {
        RETURN_ERR("Failed to allocate object");
    }
    
    (*result)->string = NULL;
    (*result)->child = NULL;
    (*result)->next = NULL;
    
    // Handle empty object
    if (current < end && *current == '}') {
        *json = current + 1;
        return RESULT_OK;
    }
    
    object_t* first_child = NULL;
    object_t* last_child = NULL;
    
    while (current < end) {
        current = skip_whitespace(current, end);
        
        // Check if we've reached the end of the object
        if (current < end && *current == '}') {
            break;
        }
        
        // Parse key
        string_t* key;
        if (parse_json_string(pool, &current, end, &key) != RESULT_OK) {
            RETURN_ERR("Failed to parse JSON object key");
        }
        
        current = skip_whitespace(current, end);
        
        // Expect colon
        if (current >= end || *current != ':') {
            RETURN_ERR("Expected colon after JSON object key");
        }
        current++;
        current = skip_whitespace(current, end);
        
        // Parse value
        object_t* value;
        if (parse_json_value(pool, &current, end, &value) != RESULT_OK) {
            RETURN_ERR("Failed to parse JSON object value");
        }
        
        // Create key-value pair object
        object_t* pair;
        if (pool_object_alloc(pool, &pair) != RESULT_OK) {
            RETURN_ERR("Failed to allocate object for key-value pair");
        }
        
        pair->string = key;
        pair->child = value;
        pair->next = NULL;
        
        // Add to children list
        if (!first_child) {
            first_child = pair;
            last_child = pair;
        } else {
            last_child->next = pair;
            last_child = pair;
        }
        
        current = skip_whitespace(current, end);
        
        // Check for comma or end
        if (current < end && *current == ',') {
            current++;
            current = skip_whitespace(current, end);
            // Continue to next key-value pair
        } else if (current < end && *current == '}') {
            // End of object
            break;
        } else {
            RETURN_ERR("Expected comma or closing brace in JSON object");
        }
    }
    
    if (current >= end || *current != '}') {
        RETURN_ERR("Expected closing brace for JSON object");
    }
    
    (*result)->child = first_child;
    *json = current + 1;
    return RESULT_OK;
}

// Helper function to parse a JSON array
static result_t parse_json_array(pool_t* pool, const char** json, const char* end, object_t** result) {
    const char* current = *json;
    
    if (current >= end || *current != '[') {
        RETURN_ERR("Expected opening bracket for JSON array");
    }
    
    current++; // Skip opening bracket
    current = skip_whitespace(current, end);
    
    if (pool_object_alloc(pool, result) != RESULT_OK) {
        RETURN_ERR("Failed to allocate object for array");
    }
    
    (*result)->string = NULL;
    (*result)->child = NULL;
    (*result)->next = NULL;
    
    // Handle empty array
    if (current < end && *current == ']') {
        *json = current + 1;
        return RESULT_OK;
    }
    
    object_t* first_child = NULL;
    object_t* last_child = NULL;
    
    while (current < end) {
        current = skip_whitespace(current, end);
        
        // Check if we've reached the end of the array
        if (current < end && *current == ']') {
            break;
        }
        
        // Parse array element
        object_t* element;
        if (parse_json_value(pool, &current, end, &element) != RESULT_OK) {
            RETURN_ERR("Failed to parse JSON array element");
        }
        
        // Add to children list
        if (!first_child) {
            first_child = element;
            last_child = element;
        } else {
            last_child->next = element;
            last_child = element;
        }
        
        current = skip_whitespace(current, end);
        
        // Check for comma or end
        if (current < end && *current == ',') {
            current++;
            current = skip_whitespace(current, end);
            // Continue to next element
        } else if (current < end && *current == ']') {
            // End of array
            break;
        } else {
            RETURN_ERR("Expected comma or closing bracket in JSON array");
        }
    }
    
    if (current >= end || *current != ']') {
        RETURN_ERR("Expected closing bracket for JSON array");
    }
    
    (*result)->child = first_child;
    *json = current + 1;
    return RESULT_OK;
}

// Helper function to parse a JSON value (string, number, boolean, null, object, or array)
static result_t parse_json_value(pool_t* pool, const char** json, const char* end, object_t** result) {
    const char* current = skip_whitespace(*json, end);
    
    if (current >= end) {
        RETURN_ERR("Unexpected end of JSON input");
    }
    
    if (*current == '"') {
        // String value
        if (pool_object_alloc(pool, result) != RESULT_OK) {
            RETURN_ERR("Failed to allocate object for string value");
        }
        
        *json = current;
        if (parse_json_string(pool, json, end, &((*result)->string)) != RESULT_OK) {
            RETURN_ERR("Failed to parse JSON string value");
        }
        
        (*result)->child = NULL;
        (*result)->next = NULL;
        return RESULT_OK;
    } else if (*current == '{') {
        // Object value
        *json = current;
        return parse_json_object(pool, json, end, result);
    } else if (*current == '[') {
        // Array value
        *json = current;
        return parse_json_array(pool, json, end, result);
    } else {
        // Number, boolean, or null
        const char* start = current;
        
        // Find the end of the value
        while (current < end && *current != ',' && *current != '}' && *current != ']' && 
               *current != ' ' && *current != '\t' && *current != '\n' && *current != '\r') {
            current++;
        }
        
        if (current == start) {
            RETURN_ERR("Invalid JSON value");
        }
        
        // Create object for the value
        if (pool_object_alloc(pool, result) != RESULT_OK) {
            RETURN_ERR("Failed to allocate object for primitive value");
        }
        
        size_t length = current - start;
        if (pool_string_alloc(pool, &((*result)->string), length + 1) != RESULT_OK) {
            RETURN_ERR("Failed to allocate string for primitive value");
        }
        
        (*result)->string->size = length;
        memcpy((*result)->string->data, start, length);
        (*result)->child = NULL;
        (*result)->next = NULL;
        
        *json = current;
        return RESULT_OK;
    }
}

result_t object_create(pool_t* pool, object_t** dst) {
    if (pool_object_alloc(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to allocate object");
    }
    
    (*dst)->string = NULL;
    (*dst)->child = NULL;
    (*dst)->next = NULL;
    
    return RESULT_OK;
}

result_t object_destroy(pool_t* pool, object_t* object) {
    if (!object) {
        return RESULT_OK;
    }
    
    return object_destroy_recursive(pool, object);
}

result_t object_parse_json(pool_t* pool, object_t** dst, const string_t* src) {
    if (!src || src->size == 0) {
        RETURN_ERR("Empty JSON string");
    }
    
    const char* json = src->data;
    const char* end = src->data + src->size;
    const char* current = skip_whitespace(json, end);
    
    if (parse_json_value(pool, &current, end, dst) != RESULT_OK) {
        RETURN_ERR("Failed to parse JSON");
    }
    
    return RESULT_OK;
}

result_t object_parse_xml(pool_t* pool, object_t** dst, const string_t* src) {
    // XML parsing is complex and not implemented yet
    RETURN_ERR("XML parsing not implemented");
}

// Helper function to convert object to JSON string recursively
static result_t object_to_json_recursive(pool_t* pool, string_t** dst, const object_t* src, int depth) {
    if (!src) {
        return string_append_str(pool, dst, "null");
    }
    
    if (src->string && !src->child) {
        // Leaf node - just a value (check if it's a number/boolean or string)
        const char* data = src->string->data;
        size_t size = src->string->size;
        
        // Check if it looks like a number, boolean, or null
        if ((size >= 4 && strncmp(data, "true", 4) == 0) ||
            (size >= 5 && strncmp(data, "false", 5) == 0) ||
            (size >= 4 && strncmp(data, "null", 4) == 0) ||
            (size > 0 && (isdigit(data[0]) || data[0] == '-'))) {
            // Number, boolean, or null - don't quote
            return string_append_string(pool, dst, src->string);
        } else {
            // String value - add quotes
            if (string_append_char(pool, dst, '"') != RESULT_OK) {
                RETURN_ERR("Failed to append quote");
            }
            if (string_append_string(pool, dst, src->string) != RESULT_OK) {
                RETURN_ERR("Failed to append string value");
            }
            if (string_append_char(pool, dst, '"') != RESULT_OK) {
                RETURN_ERR("Failed to append quote");
            }
            return RESULT_OK;
        }
    } else if (src->child) {
        // Check if this is an object (has key-value pairs) or array
        object_t* first_child = src->child;
        
        // If the first child has a string (key), it's an object
        if (first_child && first_child->string) {
            // Object with key-value pairs
            if (string_append_char(pool, dst, '{') != RESULT_OK) {
                RETURN_ERR("Failed to append opening brace");
            }
            
            object_t* child = first_child;
            int first = 1;
            while (child) {
                if (!first) {
                    if (string_append_char(pool, dst, ',') != RESULT_OK) {
                        RETURN_ERR("Failed to append comma");
                    }
                }
                first = 0;
                
                // Key
                if (string_append_char(pool, dst, '"') != RESULT_OK) {
                    RETURN_ERR("Failed to append quote");
                }
                if (string_append_string(pool, dst, child->string) != RESULT_OK) {
                    RETURN_ERR("Failed to append key");
                }
                if (string_append_str(pool, dst, "\":") != RESULT_OK) {
                    RETURN_ERR("Failed to append colon");
                }
                
                // Value
                if (object_to_json_recursive(pool, dst, child->child, depth + 1) != RESULT_OK) {
                    RETURN_ERR("Failed to convert child to JSON");
                }
                
                child = child->next;
            }
            
            if (string_append_char(pool, dst, '}') != RESULT_OK) {
                RETURN_ERR("Failed to append closing brace");
            }
        } else {
            // Array
            if (string_append_char(pool, dst, '[') != RESULT_OK) {
                RETURN_ERR("Failed to append opening bracket");
            }
            
            object_t* child = first_child;
            int first = 1;
            while (child) {
                if (!first) {
                    if (string_append_char(pool, dst, ',') != RESULT_OK) {
                        RETURN_ERR("Failed to append comma");
                    }
                }
                first = 0;
                
                if (object_to_json_recursive(pool, dst, child, depth + 1) != RESULT_OK) {
                    RETURN_ERR("Failed to convert array element to JSON");
                }
                
                child = child->next;
            }
            
            if (string_append_char(pool, dst, ']') != RESULT_OK) {
                RETURN_ERR("Failed to append closing bracket");
            }
        }
        return RESULT_OK;
    } else {
        // Empty object or null
        return string_append_str(pool, dst, "null");
    }
}

result_t object_tostring_json(pool_t* pool, string_t** dst, const object_t* src) {
    if (string_create(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to create destination string");
    }
    
    return object_to_json_recursive(pool, dst, src, 0);
}

result_t object_tostring_xml(pool_t* pool, string_t** dst, const object_t* src) {
    // XML serialization is complex and not implemented yet
    RETURN_ERR("XML serialization not implemented");
}

// Helper function to find an object by path
static object_t* find_object_by_path(const object_t* object, const string_t* path) {
    if (!object || !path || path->size == 0) {
        return NULL;
    }
    
    // Find the first dot or use the entire path
    int64_t dot_pos = string_find_char(path, '.', 0);
    
    if (dot_pos == -1) {
        // No more dots, look for exact match in children
        object_t* child = object->child;
        while (child) {
            if (child->string && string_equal_string(child->string, path)) {
                return child->child; // Return the value, not the key-value pair
            }
            child = child->next;
        }
        return NULL;
    } else {
        // Extract the first part of the path
        string_t first_part;
        first_part.data = path->data;
        first_part.size = dot_pos;
        first_part.capacity = path->capacity;
        
        // Find matching child
        object_t* child = object->child;
        while (child) {
            if (child->string && string_equal_string(child->string, &first_part)) {
                // Create remaining path
                string_t remaining_path;
                remaining_path.data = path->data + dot_pos + 1;
                remaining_path.size = path->size - dot_pos - 1;
                remaining_path.capacity = path->capacity;
                
                return find_object_by_path(child->child, &remaining_path);
            }
            child = child->next;
        }
        return NULL;
    }
}

result_t object_set(pool_t* pool, object_t* object, const string_t* path, object_t* value) {
    // Setting objects by path is complex and requires path parsing and tree manipulation
    RETURN_ERR("Object set by path not implemented yet");
}

result_t object_set_string(pool_t* pool, object_t* object, const string_t* path, const string_t* value) {
    // Create a string object from the value and call object_set
    object_t* value_object;
    if (object_create(pool, &value_object) != RESULT_OK) {
        RETURN_ERR("Failed to create value object");
    }
    
    if (string_create_string(pool, &(value_object->string), value) != RESULT_OK) {
        RETURN_ERR("Failed to create value string");
    }
    
    return object_set(pool, object, path, value_object);
}

result_t object_get(object_t** dst, const object_t* object, const string_t* path) {
    *dst = find_object_by_path(object, path);
    if (*dst == NULL) {
        RETURN_ERR("Object not found at specified path");
    }
    return RESULT_OK;
}
