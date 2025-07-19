/**
 * @file json_builder.c
 * @brief JSON building implementation for LKJAgent
 * 
 * This module provides JSON construction capabilities for generating
 * configuration files and memory storage formats with proper escaping
 * and formatting.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/json_builder.h"
#include "../lkjagent.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/**
 * @defgroup JSON_Builder_Internal Internal JSON Builder Functions
 * @{
 */

/**
 * @brief Check if JSON object needs a comma separator before adding new field
 * 
 * @param json_object Current JSON object content
 * @return true if comma is needed, false otherwise
 */
static bool needs_comma(const data_t* json_object) {
    if (!json_object || !json_object->data || json_object->size < 2) {
        return false;
    }
    
    /* Check if object already has fields (more than just "{}") */
    const char* content = json_object->data;
    size_t len = json_object->size;
    
    /* Skip whitespace from the end */
    while (len > 0 && (content[len-1] == ' ' || content[len-1] == '\t' || 
                       content[len-1] == '\n' || content[len-1] == '\r')) {
        len--;
    }
    
    if (len < 2) return false;
    
    /* If we have more than just "{}", we need a comma */
    /* Look for content between braces - anything other than just "{}" */
    for (size_t i = 1; i < len - 1; i++) {
        if (content[i] != ' ' && content[i] != '\t' && 
            content[i] != '\n' && content[i] != '\r') {
            return true; /* Found non-whitespace content, need comma */
        }
    }
    
    return false; /* Only whitespace between braces, no comma needed */
}

/**
 * @brief Add field separator and key to JSON object
 * 
 * @param json_object JSON object to modify
 * @param key Field key to add
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t add_key_prefix(data_t* json_object, const char* key) {
    if (!json_object || !key) {
        RETURN_ERR("Null pointer in add_key_prefix");
        return RESULT_ERR;
    }
    
    /* Remove closing brace temporarily */
    if (json_object->size > 0 && json_object->data[json_object->size - 1] == '}') {
        json_object->data[json_object->size - 1] = '\0';
        json_object->size--;
    }
    
    /* Add comma if needed */
    if (needs_comma(json_object)) {
        if (data_append(json_object, ", ", 0) != RESULT_OK) {
            return RESULT_ERR;
        }
    }
    
    /* Escape the key */
    data_t escaped_key;
    if (data_init(&escaped_key, strlen(key) * 2 + 10) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (json_escape_string(key, &escaped_key) != RESULT_OK) {
        data_destroy(&escaped_key);
        return RESULT_ERR;
    }
    
    /* Add quoted key and colon */
    if (data_append(json_object, "\"", 0) != RESULT_OK ||
        data_append(json_object, escaped_key.data, 0) != RESULT_OK ||
        data_append(json_object, "\": ", 0) != RESULT_OK) {
        data_destroy(&escaped_key);
        return RESULT_ERR;
    }
    
    data_destroy(&escaped_key);
    return RESULT_OK;
}

/**
 * @brief Finalize JSON object by adding closing brace
 * 
 * @param json_object JSON object to finalize
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t finalize_object(data_t* json_object) {
    if (!json_object) {
        RETURN_ERR("Null json_object in finalize_object");
        return RESULT_ERR;
    }
    
    return data_append(json_object, "}", 0);
}

/** @} */

result_t json_build_object(data_t* output) {
    if (!output) {
        RETURN_ERR("Null output in json_build_object");
        return RESULT_ERR;
    }
    
    return data_set(output, "{}", 0);
}

result_t json_build_array(data_t* output) {
    if (!output) {
        RETURN_ERR("Null output in json_build_array");
        return RESULT_ERR;
    }
    
    return data_set(output, "[]", 0);
}

result_t json_add_string(data_t* json_object, const char* key, const char* value) {
    if (!json_object) {
        RETURN_ERR("Null json_object in json_add_string");
        return RESULT_ERR;
    }
    
    if (!key || key[0] == '\0') {
        RETURN_ERR("Null or empty key in json_add_string");
        return RESULT_ERR;
    }
    
    if (!value) {
        RETURN_ERR("Null value in json_add_string");
        return RESULT_ERR;
    }
    
    /* Add key prefix */
    if (add_key_prefix(json_object, key) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Escape the value */
    data_t escaped_value;
    if (data_init(&escaped_value, strlen(value) * 2 + 10) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (json_escape_string(value, &escaped_value) != RESULT_OK) {
        data_destroy(&escaped_value);
        return RESULT_ERR;
    }
    
    /* Add quoted value */
    result_t result = RESULT_OK;
    if (data_append(json_object, "\"", 0) != RESULT_OK ||
        data_append(json_object, escaped_value.data, 0) != RESULT_OK ||
        data_append(json_object, "\"", 0) != RESULT_OK) {
        result = RESULT_ERR;
    }
    
    data_destroy(&escaped_value);
    
    if (result != RESULT_OK) {
        return result;
    }
    
    return finalize_object(json_object);
}

result_t json_add_number(data_t* json_object, const char* key, double value) {
    if (!json_object) {
        RETURN_ERR("Null json_object in json_add_number");
        return RESULT_ERR;
    }
    
    if (!key || key[0] == '\0') {
        RETURN_ERR("Null or empty key in json_add_number");
        return RESULT_ERR;
    }
    
    /* Check for special values */
    if (isnan(value)) {
        RETURN_ERR("NaN values are not valid in JSON");
        return RESULT_ERR;
    }
    
    if (isinf(value)) {
        RETURN_ERR("Infinite values are not valid in JSON");
        return RESULT_ERR;
    }
    
    /* Add key prefix */
    if (add_key_prefix(json_object, key) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Format the number */
    char number_str[64];
    int result;
    
    /* Check if it's effectively an integer */
    if (value == floor(value) && value >= -9007199254740991.0 && value <= 9007199254740991.0) {
        /* Format as integer */
        result = snprintf(number_str, sizeof(number_str), "%.0f", value);
    } else {
        /* Format as floating point */
        result = snprintf(number_str, sizeof(number_str), "%g", value);
    }
    
    if (result < 0 || result >= (int)sizeof(number_str)) {
        RETURN_ERR("Number formatting failed");
        return RESULT_ERR;
    }
    
    /* Add number value */
    if (data_append(json_object, number_str, 0) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    return finalize_object(json_object);
}

result_t json_add_boolean(data_t* json_object, const char* key, bool value) {
    if (!json_object) {
        RETURN_ERR("Null json_object in json_add_boolean");
        return RESULT_ERR;
    }
    
    if (!key || key[0] == '\0') {
        RETURN_ERR("Null or empty key in json_add_boolean");
        return RESULT_ERR;
    }
    
    /* Add key prefix */
    if (add_key_prefix(json_object, key) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Add boolean value */
    const char* bool_str = value ? "true" : "false";
    if (data_append(json_object, bool_str, 0) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    return finalize_object(json_object);
}

result_t json_add_null(data_t* json_object, const char* key) {
    if (!json_object) {
        RETURN_ERR("Null json_object in json_add_null");
        return RESULT_ERR;
    }
    
    if (!key || key[0] == '\0') {
        RETURN_ERR("Null or empty key in json_add_null");
        return RESULT_ERR;
    }
    
    /* Add key prefix */
    if (add_key_prefix(json_object, key) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Add null value */
    if (data_append(json_object, "null", 0) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    return finalize_object(json_object);
}

result_t json_build_memory(const char* working_memory, const char* disk_memory, data_t* output) {
    if (!working_memory) {
        RETURN_ERR("Null working_memory in json_build_memory");
        return RESULT_ERR;
    }
    
    if (!disk_memory) {
        RETURN_ERR("Null disk_memory in json_build_memory");
        return RESULT_ERR;
    }
    
    if (!output) {
        RETURN_ERR("Null output in json_build_memory");
        return RESULT_ERR;
    }
    
    /* Start building the object */
    if (json_build_object(output) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Add working memory field */
    if (json_add_string(output, "working_memory", working_memory) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Add disk memory field */
    if (json_add_string(output, "disk_memory", disk_memory) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t json_build_context_keys(const context_key_t* context_keys, size_t key_count, data_t* output) {
    if (!output) {
        RETURN_ERR("Null output in json_build_context_keys");
        return RESULT_ERR;
    }
    
    if (key_count > 0 && !context_keys) {
        RETURN_ERR("Null context_keys with non-zero count");
        return RESULT_ERR;
    }
    
    /* Start with empty array */
    if (data_set(output, "[", 0) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    for (size_t i = 0; i < key_count; i++) {
        const context_key_t* key = &context_keys[i];
        
        /* Validate context key */
        if (!CONTEXT_KEY_IS_VALID(key)) {
            RETURN_ERR("Invalid context key in array");
            return RESULT_ERR;
        }
        
        /* Add comma if not first element */
        if (i > 0) {
            if (data_append(output, ", ", 0) != RESULT_OK) {
                return RESULT_ERR;
            }
        }
        
        /* Build object for this key */
        data_t key_object;
        if (data_init(&key_object, 256) != RESULT_OK) {
            return RESULT_ERR;
        }
        
        if (json_build_object(&key_object) != RESULT_OK ||
            json_add_string(&key_object, "key", key->key) != RESULT_OK ||
            json_add_number(&key_object, "layer", (double)key->layer) != RESULT_OK ||
            json_add_number(&key_object, "importance_score", (double)key->importance_score) != RESULT_OK ||
            json_add_number(&key_object, "last_accessed", (double)key->last_accessed) != RESULT_OK ||
            json_add_number(&key_object, "data_size", (double)key->data_size) != RESULT_OK) {
            data_destroy(&key_object);
            return RESULT_ERR;
        }
        
        /* Append the key object to the array */
        if (data_append(output, key_object.data, 0) != RESULT_OK) {
            data_destroy(&key_object);
            return RESULT_ERR;
        }
        
        data_destroy(&key_object);
    }
    
    /* Close array */
    if (data_append(output, "]", 0) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t json_build_config(const config_t* config, data_t* output) {
    if (!config) {
        RETURN_ERR("Null config in json_build_config");
        return RESULT_ERR;
    }
    
    if (!output) {
        RETURN_ERR("Null output in json_build_config");
        return RESULT_ERR;
    }
    
    /* Start building the object */
    if (json_build_object(output) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Add LLM settings */
    if (json_add_string(output, "llm_endpoint", config->llm_endpoint) != RESULT_OK ||
        json_add_string(output, "llm_model", config->llm_model) != RESULT_OK ||
        json_add_number(output, "llm_max_context", (double)config->llm_max_context) != RESULT_OK ||
        json_add_number(output, "llm_timeout", (double)config->llm_timeout) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Add memory settings */
    if (json_add_number(output, "memory_max_working_size", (double)config->memory_max_working_size) != RESULT_OK ||
        json_add_number(output, "memory_max_disk_size", (double)config->memory_max_disk_size) != RESULT_OK ||
        json_add_number(output, "memory_cleanup_threshold", (double)config->memory_cleanup_threshold) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Add validation flag */
    if (json_add_boolean(output, "is_valid", config->is_valid) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Note: API key and prompts are not included in serialized config for security/size reasons */
    
    return RESULT_OK;
}

result_t json_escape_string(const char* input, data_t* output) {
    if (!input) {
        RETURN_ERR("Null input in json_escape_string");
        return RESULT_ERR;
    }
    
    if (!output) {
        RETURN_ERR("Null output in json_escape_string");
        return RESULT_ERR;
    }
    
    if (data_clear(output) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    const char* ptr = input;
    while (*ptr != '\0') {
        char escape_seq[3] = {0}; /* For two-character escape sequences */
        const char* to_append = NULL;
        
        switch (*ptr) {
            case '"':
                strcpy(escape_seq, "\\\"");
                to_append = escape_seq;
                break;
            case '\\':
                strcpy(escape_seq, "\\\\");
                to_append = escape_seq;
                break;
            case '/':
                strcpy(escape_seq, "\\/");
                to_append = escape_seq;
                break;
            case '\b':
                strcpy(escape_seq, "\\b");
                to_append = escape_seq;
                break;
            case '\f':
                strcpy(escape_seq, "\\f");
                to_append = escape_seq;
                break;
            case '\n':
                strcpy(escape_seq, "\\n");
                to_append = escape_seq;
                break;
            case '\r':
                strcpy(escape_seq, "\\r");
                to_append = escape_seq;
                break;
            case '\t':
                strcpy(escape_seq, "\\t");
                to_append = escape_seq;
                break;
            default:
                /* Regular character - check if it needs escaping */
                if ((unsigned char)*ptr < 32) {
                    /* Control character - use unicode escape */
                    char unicode_escape[7];
                    snprintf(unicode_escape, sizeof(unicode_escape), "\\u%04x", (unsigned char)*ptr);
                    if (data_append(output, unicode_escape, 0) != RESULT_OK) {
                        return RESULT_ERR;
                    }
                    ptr++;
                    continue;
                } else {
                    /* Regular character */
                    char single_char[2] = {*ptr, '\0'};
                    to_append = single_char;
                }
                break;
        }
        
        if (to_append && data_append(output, to_append, 0) != RESULT_OK) {
            return RESULT_ERR;
        }
        
        ptr++;
    }
    
    return RESULT_OK;
}
