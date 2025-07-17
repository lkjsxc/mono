/**
 * @file json.c
 * @brief JSON parsing and generation implementation
 *
 * This module provides JSON parsing and generation capabilities with zero
 * external dependencies. It implements a simple but robust JSON processor
 * that handles validation, value extraction, and object creation.
 *
 * Key features:
 * - Zero external dependencies (manual parsing)
 * - Safe memory operations with bounded buffers
 * - Support for strings, numbers, booleans, and objects
 * - Comprehensive error handling
 * - Input validation for all operations
 */

#include "../lkjagent.h"

// JSON parsing state
typedef enum {
    JSON_STATE_NONE,
    JSON_STATE_OBJECT,
    JSON_STATE_ARRAY,
    JSON_STATE_KEY,
    JSON_STATE_VALUE,
    JSON_STATE_STRING,
    JSON_STATE_NUMBER,
    JSON_STATE_BOOLEAN,
    JSON_STATE_NULL
} json_state_t;

/**
 * @brief Skip whitespace characters in JSON
 *
 * @param json JSON string pointer
 * @return Pointer to next non-whitespace character
 */
static const char* json_skip_whitespace(const char* json) {
    if (!json) return NULL;
    
    while (*json && (*json == ' ' || *json == '\t' || *json == '\n' || *json == '\r')) {
        json++;
    }
    return json;
}

/**
 * @brief Parse JSON string value
 *
 * @param json JSON string starting at opening quote
 * @param result Token to store parsed string
 * @param end_ptr Pointer to store position after closing quote
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
static result_t json_parse_string(const char* json, token_t* result, const char** end_ptr) {
    if (!json || !result || !end_ptr) {
        RETURN_ERR("json_parse_string: NULL parameter");
        return RESULT_ERR;
    }
    
    if (*json != '"') {
        RETURN_ERR("json_parse_string: Expected opening quote");
        return RESULT_ERR;
    }
    
    json++; // Skip opening quote
    const char* start = json;
    size_t length = 0;
    
    // Find closing quote and calculate length
    while (*json && *json != '"') {
        if (*json == '\\') {
            // Handle escape sequences
            json++;
            if (!*json) {
                RETURN_ERR("json_parse_string: Unterminated escape sequence");
                return RESULT_ERR;
            }
        }
        json++;
        length++;
    }
    
    if (*json != '"') {
        RETURN_ERR("json_parse_string: Missing closing quote");
        return RESULT_ERR;
    }
    
    // Check if string fits in result token
    if (length >= result->capacity) {
        RETURN_ERR("json_parse_string: String too long for result token");
        return RESULT_ERR;
    }
    
    // Copy string content (simplified - no escape processing)
    strncpy(result->data, start, length);
    result->data[length] = '\0';
    result->size = length;
    
    *end_ptr = json + 1; // Position after closing quote
    return RESULT_OK;
}

/**
 * @brief Find value for key in JSON object
 *
 * @param json JSON object string
 * @param key Key to search for
 * @param value_start Pointer to store start of value
 * @param value_end Pointer to store end of value
 * @return RESULT_OK if found, RESULT_ERR if not found
 */
__attribute__((warn_unused_result))
static result_t json_find_key_value(const char* json, const char* key, 
                                   const char** value_start, const char** value_end) {
    if (!json || !key || !value_start || !value_end) {
        RETURN_ERR("json_find_key_value: NULL parameter");
        return RESULT_ERR;
    }
    
    json = json_skip_whitespace(json);
    if (*json != '{') {
        RETURN_ERR("json_find_key_value: Not a JSON object");
        return RESULT_ERR;
    }
    
    json++; // Skip opening brace
    json = json_skip_whitespace(json);
    
    while (*json && *json != '}') {
        // Parse key
        if (*json != '"') {
            RETURN_ERR("json_find_key_value: Expected key string");
            return RESULT_ERR;
        }
        
        json++; // Skip opening quote
        const char* key_start = json;
        
        // Find end of key
        while (*json && *json != '"') {
            if (*json == '\\') {
                json++; // Skip escape character
                if (*json) json++; // Skip escaped character
            } else {
                json++;
            }
        }
        
        if (*json != '"') {
            RETURN_ERR("json_find_key_value: Unterminated key string");
            return RESULT_ERR;
        }
        
        size_t key_length = json - key_start;
        json++; // Skip closing quote
        
        // Check if this is the key we're looking for
        if (strlen(key) == key_length && strncmp(key_start, key, key_length) == 0) {
            // Found the key, now find the value
            json = json_skip_whitespace(json);
            if (*json != ':') {
                RETURN_ERR("json_find_key_value: Expected colon after key");
                return RESULT_ERR;
            }
            json++; // Skip colon
            json = json_skip_whitespace(json);
            
            *value_start = json;
            
            // Find end of value
            int brace_count = 0;
            int bracket_count = 0;
            int in_string = 0;
            
            while (*json) {
                if (!in_string) {
                    if (*json == '{') brace_count++;
                    else if (*json == '}') {
                        if (brace_count == 0) break; // End of object
                        brace_count--;
                    }
                    else if (*json == '[') bracket_count++;
                    else if (*json == ']') bracket_count--;
                    else if (*json == ',' && brace_count == 0 && bracket_count == 0) break;
                    else if (*json == '"') in_string = 1;
                } else {
                    if (*json == '"' && (json == *value_start || *(json-1) != '\\')) {
                        in_string = 0;
                    }
                }
                json++;
            }
            
            *value_end = json;
            return RESULT_OK;
        }
        
        // Skip to value and past it
        json = json_skip_whitespace(json);
        if (*json != ':') {
            RETURN_ERR("json_find_key_value: Expected colon after key");
            return RESULT_ERR;
        }
        json++; // Skip colon
        json = json_skip_whitespace(json);
        
        // Skip value
        int brace_count = 0;
        int bracket_count = 0;
        int in_string = 0;
        
        while (*json) {
            if (!in_string) {
                if (*json == '{') brace_count++;
                else if (*json == '}') {
                    if (brace_count == 0) break;
                    brace_count--;
                }
                else if (*json == '[') bracket_count++;
                else if (*json == ']') bracket_count--;
                else if (*json == ',' && brace_count == 0 && bracket_count == 0) break;
                else if (*json == '"') in_string = 1;
            } else {
                if (*json == '"' && *(json-1) != '\\') {
                    in_string = 0;
                }
            }
            json++;
        }
        
        // Skip comma if present
        json = json_skip_whitespace(json);
        if (*json == ',') {
            json++;
            json = json_skip_whitespace(json);
        }
    }
    
    RETURN_ERR("json_find_key_value: Key not found");
    return RESULT_ERR;
}

/**
 * @brief Validate JSON structure and syntax
 *
 * @param json_token Token containing JSON string
 * @return RESULT_OK if valid, RESULT_ERR if invalid
 */
__attribute__((warn_unused_result))
result_t json_validate(const token_t* json_token) {
    if (!json_token) {
        RETURN_ERR("json_validate: NULL token parameter");
        return RESULT_ERR;
    }
    
    if (!json_token->data) {
        RETURN_ERR("json_validate: Token not initialized");
        return RESULT_ERR;
    }
    
    const char* json = json_skip_whitespace(json_token->data);
    if (!*json) {
        RETURN_ERR("json_validate: Empty JSON");
        return RESULT_ERR;
    }
    
    // Simple validation - check if it starts with { or [
    if (*json != '{' && *json != '[' && *json != '"' && !isdigit(*json) && 
        strncmp(json, "true", 4) != 0 && strncmp(json, "false", 5) != 0 && 
        strncmp(json, "null", 4) != 0) {
        RETURN_ERR("json_validate: Invalid JSON start");
        return RESULT_ERR;
    }
    
    // Basic brace/bracket matching
    int brace_count = 0;
    int bracket_count = 0;
    int in_string = 0;
    
    for (const char* p = json; *p; p++) {
        if (!in_string) {
            if (*p == '{') brace_count++;
            else if (*p == '}') brace_count--;
            else if (*p == '[') bracket_count++;
            else if (*p == ']') bracket_count--;
            else if (*p == '"') in_string = 1;
            
            if (brace_count < 0 || bracket_count < 0) {
                RETURN_ERR("json_validate: Mismatched braces or brackets");
                return RESULT_ERR;
            }
        } else {
            if (*p == '"' && (p == json || *(p-1) != '\\')) {
                in_string = 0;
            }
        }
    }
    
    if (brace_count != 0 || bracket_count != 0) {
        RETURN_ERR("json_validate: Unmatched braces or brackets");
        return RESULT_ERR;
    }
    
    if (in_string) {
        RETURN_ERR("json_validate: Unterminated string");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

/**
 * @brief Extract string value from JSON using key path
 *
 * @param json_token JSON token
 * @param key_path Dot-separated path to value (e.g., "user.name")
 * @param result Token to store extracted string
 * @return RESULT_OK if found, RESULT_ERR if not found or not a string
 */
__attribute__((warn_unused_result))
result_t json_get_string(const token_t* json_token, const char* key_path, token_t* result) {
    if (!json_token || !key_path || !result) {
        RETURN_ERR("json_get_string: NULL parameter");
        return RESULT_ERR;
    }
    
    if (!json_token->data || !result->data) {
        RETURN_ERR("json_get_string: Uninitialized token");
        return RESULT_ERR;
    }
    
    // For now, support only simple keys (no dot notation)
    const char* value_start;
    const char* value_end;
    
    if (json_find_key_value(json_token->data, key_path, &value_start, &value_end) != RESULT_OK) {
        return RESULT_ERR; // Error already set
    }
    
    // Check if value is a string
    value_start = json_skip_whitespace(value_start);
    if (*value_start != '"') {
        RETURN_ERR("json_get_string: Value is not a string");
        return RESULT_ERR;
    }
    
    // Parse the string value
    const char* end_ptr;
    return json_parse_string(value_start, result, &end_ptr);
}

/**
 * @brief Extract numeric value from JSON
 *
 * @param json_token JSON token
 * @param key_path Dot-separated path to value
 * @param result Pointer to store numeric value
 * @return RESULT_OK if found, RESULT_ERR if not found or not a number
 */
__attribute__((warn_unused_result))
result_t json_get_number(const token_t* json_token, const char* key_path, double* result) {
    if (!json_token || !key_path || !result) {
        RETURN_ERR("json_get_number: NULL parameter");
        return RESULT_ERR;
    }
    
    if (!json_token->data) {
        RETURN_ERR("json_get_number: Token not initialized");
        return RESULT_ERR;
    }
    
    const char* value_start;
    const char* value_end;
    
    if (json_find_key_value(json_token->data, key_path, &value_start, &value_end) != RESULT_OK) {
        return RESULT_ERR; // Error already set
    }
    
    // Parse number
    value_start = json_skip_whitespace(value_start);
    char* endptr;
    *result = strtod(value_start, &endptr);
    
    if (endptr == value_start) {
        RETURN_ERR("json_get_number: Value is not a number");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

/**
 * @brief Extract boolean value from JSON
 *
 * @param json_token JSON token
 * @param key_path Dot-separated path to value
 * @param result Pointer to store boolean value
 * @return RESULT_OK if found, RESULT_ERR if not found or not a boolean
 */
__attribute__((warn_unused_result))
result_t json_get_boolean(const token_t* json_token, const char* key_path, int* result) {
    if (!json_token || !key_path || !result) {
        RETURN_ERR("json_get_boolean: NULL parameter");
        return RESULT_ERR;
    }
    
    if (!json_token->data) {
        RETURN_ERR("json_get_boolean: Token not initialized");
        return RESULT_ERR;
    }
    
    const char* value_start;
    const char* value_end;
    
    if (json_find_key_value(json_token->data, key_path, &value_start, &value_end) != RESULT_OK) {
        return RESULT_ERR; // Error already set
    }
    
    value_start = json_skip_whitespace(value_start);
    
    if (strncmp(value_start, "true", 4) == 0) {
        *result = 1;
        return RESULT_OK;
    } else if (strncmp(value_start, "false", 5) == 0) {
        *result = 0;
        return RESULT_OK;
    }
    
    RETURN_ERR("json_get_boolean: Value is not a boolean");
    return RESULT_ERR;
}

/**
 * @brief Create JSON object from key-value pairs
 *
 * @param result Token to store created JSON
 * @param keys Array of key strings
 * @param values Array of value strings
 * @param count Number of key-value pairs
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t json_create_object(token_t* result, const char* keys[], const char* values[], size_t count) {
    if (!result || !keys || !values) {
        RETURN_ERR("json_create_object: NULL parameter");
        return RESULT_ERR;
    }
    
    if (!result->data) {
        RETURN_ERR("json_create_object: Result token not initialized");
        return RESULT_ERR;
    }
    
    if (count == 0) {
        return token_set(result, "{}");
    }
    
    // Start with opening brace
    if (token_set(result, "{") != RESULT_OK) {
        return RESULT_ERR;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (!keys[i] || !values[i]) {
            RETURN_ERR("json_create_object: NULL key or value");
            return RESULT_ERR;
        }
        
        // Add comma for subsequent items
        if (i > 0) {
            if (token_append(result, ",") != RESULT_OK) {
                return RESULT_ERR;
            }
        }
        
        // Add key
        if (token_append(result, "\"") != RESULT_OK ||
            token_append(result, keys[i]) != RESULT_OK ||
            token_append(result, "\":") != RESULT_OK) {
            return RESULT_ERR;
        }
        
        // Add value (assume it's a string for now)
        if (token_append(result, "\"") != RESULT_OK ||
            token_append(result, values[i]) != RESULT_OK ||
            token_append(result, "\"") != RESULT_OK) {
            return RESULT_ERR;
        }
    }
    
    // Close object
    return token_append(result, "}");
}

/**
 * @brief Create JSON array from string values
 *
 * @param result Token to store created JSON array
 * @param values Array of value strings
 * @param count Number of values
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t json_create_array(token_t* result, const char* values[], size_t count) {
    if (!result || !values) {
        RETURN_ERR("json_create_array: NULL parameter");
        return RESULT_ERR;
    }
    
    if (!result->data) {
        RETURN_ERR("json_create_array: Result token not initialized");
        return RESULT_ERR;
    }
    
    if (count == 0) {
        return token_set(result, "[]");
    }
    
    // Start with opening bracket
    if (token_set(result, "[") != RESULT_OK) {
        return RESULT_ERR;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (!values[i]) {
            RETURN_ERR("json_create_array: NULL value");
            return RESULT_ERR;
        }
        
        // Add comma for subsequent items
        if (i > 0) {
            if (token_append(result, ",") != RESULT_OK) {
                return RESULT_ERR;
            }
        }
        
        // Add value as string
        if (token_append(result, "\"") != RESULT_OK ||
            token_append(result, values[i]) != RESULT_OK ||
            token_append(result, "\"") != RESULT_OK) {
            return RESULT_ERR;
        }
    }
    
    // Close array
    return token_append(result, "]");
}

/**
 * @brief Escape string for JSON
 *
 * @param input Input string to escape
 * @param result Token to store escaped string
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t json_escape_string(const char* input, token_t* result) {
    if (!input || !result) {
        RETURN_ERR("json_escape_string: NULL parameter");
        return RESULT_ERR;
    }
    
    if (!result->data) {
        RETURN_ERR("json_escape_string: Result token not initialized");
        return RESULT_ERR;
    }
    
    if (token_clear(result) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    for (const char* p = input; *p; p++) {
        switch (*p) {
            case '"':
                if (token_append(result, "\\\"") != RESULT_OK) return RESULT_ERR;
                break;
            case '\\':
                if (token_append(result, "\\\\") != RESULT_OK) return RESULT_ERR;
                break;
            case '\n':
                if (token_append(result, "\\n") != RESULT_OK) return RESULT_ERR;
                break;
            case '\r':
                if (token_append(result, "\\r") != RESULT_OK) return RESULT_ERR;
                break;
            case '\t':
                if (token_append(result, "\\t") != RESULT_OK) return RESULT_ERR;
                break;
            default:
                // Add single character
                char temp[2] = {*p, '\0'};
                if (token_append(result, temp) != RESULT_OK) return RESULT_ERR;
                break;
        }
    }
    
    return RESULT_OK;
}

/**
 * @brief Extract object value from JSON
 *
 * @param json_token JSON token
 * @param key Key to extract object for
 * @param result Token to store the object JSON
 * @return RESULT_OK if found, RESULT_ERR if not found or not an object
 */
__attribute__((warn_unused_result))
result_t json_get_object(const token_t* json_token, const char* key, token_t* result) {
    if (!json_token || !key || !result) {
        RETURN_ERR("json_get_object: NULL parameter");
        return RESULT_ERR;
    }
    
    if (!json_token->data || !result->data) {
        RETURN_ERR("json_get_object: Uninitialized token");
        return RESULT_ERR;
    }
    
    const char* value_start;
    const char* value_end;
    
    if (json_find_key_value(json_token->data, key, &value_start, &value_end) != RESULT_OK) {
        return RESULT_ERR; // Error already set
    }
    
    // Check if value is an object
    value_start = json_skip_whitespace(value_start);
    if (*value_start != '{') {
        RETURN_ERR("json_get_object: Value is not an object");
        return RESULT_ERR;
    }
    
    // Find the end of the object
    const char* object_start = value_start;
    const char* current = value_start + 1; // Skip opening brace
    int brace_count = 1;
    int in_string = 0;
    
    while (*current && brace_count > 0) {
        if (!in_string) {
            if (*current == '{') {
                brace_count++;
            } else if (*current == '}') {
                brace_count--;
            } else if (*current == '"') {
                in_string = 1;
            }
        } else {
            if (*current == '"' && (current == object_start + 1 || *(current-1) != '\\')) {
                in_string = 0;
            }
        }
        current++;
    }
    
    if (brace_count != 0) {
        RETURN_ERR("json_get_object: Malformed object");
        return RESULT_ERR;
    }
    
    // Copy the object to result token
    size_t object_length = current - object_start;
    if (object_length >= result->capacity) {
        RETURN_ERR("json_get_object: Object too large for result buffer");
        return RESULT_ERR;
    }
    
    if (token_clear(result) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Copy the object content
    strncpy(result->data, object_start, object_length);
    result->data[object_length] = '\0';
    result->size = object_length;
    
    return RESULT_OK;
}
