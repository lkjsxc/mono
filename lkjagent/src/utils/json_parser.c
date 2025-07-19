/**
 * @file json_parser.c
 * @brief JSON parsing implementation for LKJAgent
 * 
 * This module provides a lightweight JSON parser implementation designed
 * for configuration files and memory storage formats with robust error
 * handling for malformed input.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/json_parser.h"
#include "../lkjagent.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

/**
 * @defgroup JSON_Parser_Internal Internal JSON Parser Functions
 * @{
 */

/**
 * @brief Skip whitespace characters in JSON string
 * 
 * @param json Pointer to current position in JSON string
 * @return Pointer to first non-whitespace character
 */
static const char* skip_whitespace(const char* json) {
    if (!json) return NULL;
    
    while (isspace(*json)) {
        json++;
    }
    return json;
}

/**
 * @brief Parse JSON string literal and handle escaping
 * 
 * @param json Pointer to start of string (at opening quote)
 * @param output Buffer to store unescaped string
 * @param end_ptr Pointer to store position after closing quote
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t parse_string_literal(const char* json, data_t* output, const char** end_ptr) {
    if (!json || !output || !end_ptr) {
        RETURN_ERR("Null pointer in parse_string_literal");
        return RESULT_ERR;
    }
    
    if (*json != '"') {
        RETURN_ERR("String must start with quote");
        return RESULT_ERR;
    }
    
    if (data_clear(output) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    json++; /* Skip opening quote */
    
    while (*json != '\0' && *json != '"') {
        if (*json == '\\') {
            /* Handle escape sequences */
            json++;
            if (*json == '\0') {
                RETURN_ERR("Unterminated escape sequence");
                return RESULT_ERR;
            }
            
            char escaped_char;
            switch (*json) {
                case '"':  escaped_char = '"'; break;
                case '\\': escaped_char = '\\'; break;
                case '/':  escaped_char = '/'; break;
                case 'b':  escaped_char = '\b'; break;
                case 'f':  escaped_char = '\f'; break;
                case 'n':  escaped_char = '\n'; break;
                case 'r':  escaped_char = '\r'; break;
                case 't':  escaped_char = '\t'; break;
                case 'u':
                    /* Unicode escape - simplified implementation */
                    RETURN_ERR("Unicode escape sequences not fully supported");
                    return RESULT_ERR;
                default:
                    RETURN_ERR("Invalid escape sequence");
                    return RESULT_ERR;
            }
            
            char temp_str[2] = {escaped_char, '\0'};
            if (data_append(output, temp_str, 0) != RESULT_OK) {
                return RESULT_ERR;
            }
        } else {
            /* Regular character */
            char temp_str[2] = {*json, '\0'};
            if (data_append(output, temp_str, 0) != RESULT_OK) {
                return RESULT_ERR;
            }
        }
        json++;
    }
    
    if (*json != '"') {
        RETURN_ERR("Unterminated string literal");
        return RESULT_ERR;
    }
    
    *end_ptr = json + 1; /* Skip closing quote */
    return RESULT_OK;
}

/**
 * @brief Find the end of a JSON value starting at given position
 * 
 * @param json Pointer to start of JSON value
 * @return Pointer to position after the value, or NULL on error
 */
static const char* find_value_end(const char* json) {
    if (!json) return NULL;
    
    json = skip_whitespace(json);
    if (!json || *json == '\0') return NULL;
    
    switch (*json) {
        case '"': {
            /* String value */
            json++; /* Skip opening quote */
            while (*json != '\0' && *json != '"') {
                if (*json == '\\') {
                    json++; /* Skip escape character */
                    if (*json != '\0') json++; /* Skip escaped character */
                } else {
                    json++;
                }
            }
            if (*json == '"') json++; /* Skip closing quote */
            return json;
        }
        
        case '{': {
            /* Object value */
            int brace_count = 1;
            json++;
            while (*json != '\0' && brace_count > 0) {
                if (*json == '"') {
                    /* Skip string content */
                    json++;
                    while (*json != '\0' && *json != '"') {
                        if (*json == '\\') {
                            json++; /* Skip escape character */
                            if (*json != '\0') json++; /* Skip escaped character */
                        } else {
                            json++;
                        }
                    }
                    if (*json == '"') json++;
                } else if (*json == '{') {
                    brace_count++;
                    json++;
                } else if (*json == '}') {
                    brace_count--;
                    json++;
                } else {
                    json++;
                }
            }
            return json;
        }
        
        case '[': {
            /* Array value */
            int bracket_count = 1;
            json++;
            while (*json != '\0' && bracket_count > 0) {
                if (*json == '"') {
                    /* Skip string content */
                    json++;
                    while (*json != '\0' && *json != '"') {
                        if (*json == '\\') {
                            json++; /* Skip escape character */
                            if (*json != '\0') json++; /* Skip escaped character */
                        } else {
                            json++;
                        }
                    }
                    if (*json == '"') json++;
                } else if (*json == '[') {
                    bracket_count++;
                    json++;
                } else if (*json == ']') {
                    bracket_count--;
                    json++;
                } else {
                    json++;
                }
            }
            return json;
        }
        
        case 't': /* true */
            if (strncmp(json, "true", 4) == 0) {
                return json + 4;
            }
            return NULL;
            
        case 'f': /* false */
            if (strncmp(json, "false", 5) == 0) {
                return json + 5;
            }
            return NULL;
            
        case 'n': /* null */
            if (strncmp(json, "null", 4) == 0) {
                return json + 4;
            }
            return NULL;
            
        default:
            /* Number value */
            if (*json == '-' || isdigit(*json)) {
                if (*json == '-') json++;
                while (isdigit(*json)) json++;
                if (*json == '.') {
                    json++;
                    while (isdigit(*json)) json++;
                }
                if (*json == 'e' || *json == 'E') {
                    json++;
                    if (*json == '+' || *json == '-') json++;
                    while (isdigit(*json)) json++;
                }
                return json;
            }
            return NULL;
    }
}

/** @} */

result_t json_parse_object(const char* json_string, data_t* parsed_object) {
    if (!json_string) {
        RETURN_ERR("Null JSON string in json_parse_object");
        return RESULT_ERR;
    }
    
    if (!parsed_object) {
        RETURN_ERR("Null parsed_object in json_parse_object");
        return RESULT_ERR;
    }
    
    if (data_clear(parsed_object) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    const char* json = skip_whitespace(json_string);
    if (!json || *json != '{') {
        RETURN_ERR("JSON object must start with '{'");
        return RESULT_ERR;
    }
    
    /* For now, store the entire JSON string as-is after validation */
    /* A full implementation would parse into a structured representation */
    if (json_validate_structure(json_string) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    return data_set(parsed_object, json_string, 0);
}

result_t json_parse_array(const char* json_string, data_t* parsed_array) {
    if (!json_string) {
        RETURN_ERR("Null JSON string in json_parse_array");
        return RESULT_ERR;
    }
    
    if (!parsed_array) {
        RETURN_ERR("Null parsed_array in json_parse_array");
        return RESULT_ERR;
    }
    
    if (data_clear(parsed_array) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    const char* json = skip_whitespace(json_string);
    if (!json || *json != '[') {
        RETURN_ERR("JSON array must start with '['");
        return RESULT_ERR;
    }
    
    /* For now, store the entire JSON string as-is after validation */
    if (json_validate_structure(json_string) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    return data_set(parsed_array, json_string, 0);
}

result_t json_parse_string(const char* json_string, data_t* output) {
    if (!json_string) {
        RETURN_ERR("Null JSON string in json_parse_string");
        return RESULT_ERR;
    }
    
    if (!output) {
        RETURN_ERR("Null output in json_parse_string");
        return RESULT_ERR;
    }
    
    const char* json = skip_whitespace(json_string);
    if (!json || *json != '"') {
        RETURN_ERR("JSON string must start with quote");
        return RESULT_ERR;
    }
    
    const char* end_ptr;
    return parse_string_literal(json, output, &end_ptr);
}

result_t json_parse_number(const char* json_string, double* output) {
    if (!json_string) {
        RETURN_ERR("Null JSON string in json_parse_number");
        return RESULT_ERR;
    }
    
    if (!output) {
        RETURN_ERR("Null output pointer in json_parse_number");
        return RESULT_ERR;
    }
    
    const char* json = skip_whitespace(json_string);
    if (!json) {
        RETURN_ERR("Empty JSON string for number parsing");
        return RESULT_ERR;
    }
    
    char* end_ptr;
    double value = strtod(json, &end_ptr);
    
    /* Check for parsing errors */
    if (end_ptr == json) {
        RETURN_ERR("No valid number found in JSON string");
        return RESULT_ERR;
    }
    
    /* Check for overflow/underflow */
    if (value == HUGE_VAL || value == -HUGE_VAL) {
        RETURN_ERR("Number value overflow in JSON parsing");
        return RESULT_ERR;
    }
    
    /* Skip trailing whitespace and check for extra characters */
    const char* remaining = skip_whitespace(end_ptr);
    if (remaining && *remaining != '\0') {
        RETURN_ERR("Extra characters after number in JSON string");
        return RESULT_ERR;
    }
    
    *output = value;
    return RESULT_OK;
}

result_t json_parse_boolean(const char* json_string, bool* output) {
    if (!json_string) {
        RETURN_ERR("Null JSON string in json_parse_boolean");
        return RESULT_ERR;
    }
    
    if (!output) {
        RETURN_ERR("Null output pointer in json_parse_boolean");
        return RESULT_ERR;
    }
    
    const char* json = skip_whitespace(json_string);
    if (!json) {
        RETURN_ERR("Empty JSON string for boolean parsing");
        return RESULT_ERR;
    }
    
    if (strncmp(json, "true", 4) == 0) {
        const char* remaining = skip_whitespace(json + 4);
        if (remaining && *remaining != '\0') {
            RETURN_ERR("Extra characters after 'true' in JSON string");
            return RESULT_ERR;
        }
        *output = true;
        return RESULT_OK;
    } else if (strncmp(json, "false", 5) == 0) {
        const char* remaining = skip_whitespace(json + 5);
        if (remaining && *remaining != '\0') {
            RETURN_ERR("Extra characters after 'false' in JSON string");
            return RESULT_ERR;
        }
        *output = false;
        return RESULT_OK;
    } else {
        RETURN_ERR("Invalid boolean value in JSON string");
        return RESULT_ERR;
    }
}

result_t json_find_key(const char* json_object, const char* key, data_t* value) {
    if (!json_object) {
        RETURN_ERR("Null JSON object in json_find_key");
        return RESULT_ERR;
    }
    
    if (!key) {
        RETURN_ERR("Null key in json_find_key");
        return RESULT_ERR;
    }
    
    if (!value) {
        RETURN_ERR("Null value buffer in json_find_key");
        return RESULT_ERR;
    }
    
    if (data_clear(value) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    const char* json = skip_whitespace(json_object);
    if (!json || *json != '{') {
        RETURN_ERR("JSON object must start with '{'");
        return RESULT_ERR;
    }
    
    json++; /* Skip opening brace */
    json = skip_whitespace(json);
    
    /* Handle empty object */
    if (json && *json == '}') {
        RETURN_ERR("Key not found in empty object");
        return RESULT_ERR;
    }
    
    while (json && *json != '}' && *json != '\0') {
        /* Parse key */
        if (*json != '"') {
            RETURN_ERR("Object key must be a string");
            return RESULT_ERR;
        }
        
        data_t current_key;
        if (data_init(&current_key, 64) != RESULT_OK) {
            return RESULT_ERR;
        }
        
        const char* after_key;
        if (parse_string_literal(json, &current_key, &after_key) != RESULT_OK) {
            data_destroy(&current_key);
            return RESULT_ERR;
        }
        
        json = skip_whitespace(after_key);
        if (!json || *json != ':') {
            data_destroy(&current_key);
            RETURN_ERR("Expected ':' after object key");
            return RESULT_ERR;
        }
        
        json++; /* Skip colon */
        json = skip_whitespace(json);
        
        /* Check if this is the key we're looking for */
        if (strcmp(current_key.data, key) == 0) {
            /* Found the key, extract the value */
            const char* value_start = json;
            const char* value_end = find_value_end(json);
            
            if (!value_end) {
                data_destroy(&current_key);
                RETURN_ERR("Invalid JSON value for found key");
                return RESULT_ERR;
            }
            
            /* Extract the value substring */
            size_t value_len = value_end - value_start;
            char* temp_value = malloc(value_len + 1);
            if (!temp_value) {
                data_destroy(&current_key);
                RETURN_ERR("Memory allocation failed for value extraction");
                return RESULT_ERR;
            }
            
            memcpy(temp_value, value_start, value_len);
            temp_value[value_len] = '\0';
            
            result_t set_result = data_set(value, temp_value, 0);
            free(temp_value);
            data_destroy(&current_key);
            
            return set_result;
        }
        
        data_destroy(&current_key);
        
        /* Skip the value */
        json = find_value_end(json);
        if (!json) {
            RETURN_ERR("Invalid JSON value in object");
            return RESULT_ERR;
        }
        
        json = skip_whitespace(json);
        if (json && *json == ',') {
            json++; /* Skip comma */
            json = skip_whitespace(json);
        } else if (json && *json != '}') {
            RETURN_ERR("Expected ',' or '}' in JSON object");
            return RESULT_ERR;
        }
    }
    
    RETURN_ERR("Key not found in JSON object");
    return RESULT_ERR;
}

result_t json_validate_structure(const char* json_string) {
    if (!json_string) {
        RETURN_ERR("Null JSON string in json_validate_structure");
        return RESULT_ERR;
    }
    
    const char* json = skip_whitespace(json_string);
    if (!json || *json == '\0') {
        RETURN_ERR("Empty JSON string");
        return RESULT_ERR;
    }
    
    /* Simple validation - check that we can find the end of the value */
    const char* end = find_value_end(json);
    if (!end) {
        RETURN_ERR("Invalid JSON structure");
        return RESULT_ERR;
    }
    
    /* Check that there's nothing significant after the value */
    const char* remaining = skip_whitespace(end);
    if (remaining && *remaining != '\0') {
        RETURN_ERR("Extra content after JSON value");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t json_parse_memory_format(const char* json_content, data_t* working_memory, data_t* disk_memory) {
    if (!json_content) {
        RETURN_ERR("Null JSON content in json_parse_memory_format");
        return RESULT_ERR;
    }
    
    if (!working_memory || !disk_memory) {
        RETURN_ERR("Null memory buffer in json_parse_memory_format");
        return RESULT_ERR;
    }
    
    /* Clear output buffers */
    if (data_clear(working_memory) != RESULT_OK || data_clear(disk_memory) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Parse working_memory field */
    data_t working_value;
    if (data_init(&working_value, 512) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (json_find_key(json_content, "working_memory", &working_value) == RESULT_OK) {
        /* Parse the string value */
        if (json_parse_string(working_value.data, working_memory) != RESULT_OK) {
            /* If it's not a string, use the raw value */
            if (data_set(working_memory, working_value.data, 0) != RESULT_OK) {
                data_destroy(&working_value);
                return RESULT_ERR;
            }
        }
    }
    
    data_destroy(&working_value);
    
    /* Parse disk_memory field */
    data_t disk_value;
    if (data_init(&disk_value, 512) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (json_find_key(json_content, "disk_memory", &disk_value) == RESULT_OK) {
        /* Parse the string value */
        if (json_parse_string(disk_value.data, disk_memory) != RESULT_OK) {
            /* If it's not a string, use the raw value */
            if (data_set(disk_memory, disk_value.data, 0) != RESULT_OK) {
                data_destroy(&disk_value);
                return RESULT_ERR;
            }
        }
    }
    
    data_destroy(&disk_value);
    return RESULT_OK;
}

result_t json_parse_context_keys_format(const char* json_content, context_key_t* context_keys, size_t max_keys, size_t* parsed_count) {
    if (!json_content) {
        RETURN_ERR("Null JSON content in json_parse_context_keys_format");
        return RESULT_ERR;
    }
    
    if (!context_keys) {
        RETURN_ERR("Null context_keys array in json_parse_context_keys_format");
        return RESULT_ERR;
    }
    
    if (!parsed_count) {
        RETURN_ERR("Null parsed_count in json_parse_context_keys_format");
        return RESULT_ERR;
    }
    
    if (max_keys == 0) {
        RETURN_ERR("max_keys must be greater than zero");
        return RESULT_ERR;
    }
    
    *parsed_count = 0;
    
    /* This is a simplified implementation that would need to be expanded
     * for full array parsing. For now, we'll just validate the JSON structure. */
    if (json_validate_structure(json_content) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* In a full implementation, this would:
     * 1. Parse the JSON array
     * 2. Iterate through each object in the array
     * 3. Extract key, layer, importance_score, last_accessed fields
     * 4. Populate the context_key_t structures
     * 5. Handle validation and error cases for each field
     */
    
    return RESULT_OK;
}
