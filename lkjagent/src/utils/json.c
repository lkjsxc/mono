#include "utils/lkjjson.h"
#include "utils/lkjpool.h"
#include "utils/lkjstring.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// Forward declarations for internal functions
static result_t json_parse_value(pool_t* pool, const char** json, json_value_t** dst);
static result_t json_parse_object(pool_t* pool, const char** json, json_value_t** dst);
static result_t json_parse_array(pool_t* pool, const char** json, json_value_t** dst);
static result_t json_parse_string(pool_t* pool, const char** json, string_t** dst);
static result_t json_parse_number(const char** json, double* dst);
static result_t json_parse_literal(const char** json, const char* literal);
static void json_skip_whitespace(const char** json);
static result_t json_stringify_value(pool_t* pool, const json_value_t* value, string_t** dst);
static result_t json_escape_string(pool_t* pool, const string_t* src, string_t** dst);
static result_t json_free_value_recursive(pool_t* pool, json_value_t* value);

// Parse JSON string into json_value_t structure
result_t json_parse(pool_t* pool, const string_t* json_string, json_value_t** dst) {
    if (!pool || !json_string || !dst) {
        return RESULT_ERR;
    }

    const char* json = json_string->data;
    json_skip_whitespace(&json);
    
    if (*json == '\0') {
        return RESULT_ERR;
    }

    return json_parse_value(pool, &json, dst);
}

// Convert json_value_t to JSON string
result_t json_stringify(pool_t* pool, const json_value_t* value, string_t** dst) {
    if (!pool || !value || !dst) {
        return RESULT_ERR;
    }

    return json_stringify_value(pool, value, dst);
}

// Create null JSON value
result_t json_create_null(pool_t* pool, json_value_t** dst) {
    if (!pool || !dst) {
        return RESULT_ERR;
    }

    if (pool_json_value_alloc(pool, dst) != RESULT_OK) {
        return RESULT_ERR;
    }

    (*dst)->type = JSON_TYPE_NULL;
    return RESULT_OK;
}

// Create object JSON value
result_t json_create_object(pool_t* pool, json_value_t** dst) {
    if (!pool || !dst) {
        return RESULT_ERR;
    }

    if (pool_json_value_alloc(pool, dst) != RESULT_OK) {
        return RESULT_ERR;
    }

    json_object_t* object;
    if (pool_json_object_alloc(pool, &object) != RESULT_OK) {
        pool_json_value_free(pool, *dst);
        return RESULT_ERR;
    }

    object->head = NULL;
    object->length = 0;

    (*dst)->type = JSON_TYPE_OBJECT;
    (*dst)->u.object_value = object;
    return RESULT_OK;
}

// Create array JSON value
result_t json_create_array(pool_t* pool, json_value_t** dst) {
    if (!pool || !dst) {
        return RESULT_ERR;
    }

    if (pool_json_value_alloc(pool, dst) != RESULT_OK) {
        return RESULT_ERR;
    }

    json_array_t* array;
    if (pool_json_array_alloc(pool, &array) != RESULT_OK) {
        pool_json_value_free(pool, *dst);
        return RESULT_ERR;
    }

    array->head = NULL;
    array->length = 0;

    (*dst)->type = JSON_TYPE_ARRAY;
    (*dst)->u.array_value = array;
    return RESULT_OK;
}

// Create boolean JSON value
result_t json_create_bool(pool_t* pool, int bool_val, json_value_t** dst) {
    if (!pool || !dst) {
        return RESULT_ERR;
    }

    if (pool_json_value_alloc(pool, dst) != RESULT_OK) {
        return RESULT_ERR;
    }

    (*dst)->type = JSON_TYPE_BOOL;
    (*dst)->u.bool_value = bool_val ? 1 : 0;
    return RESULT_OK;
}

// Create number JSON value
result_t json_create_number(pool_t* pool, double num, json_value_t** dst) {
    if (!pool || !dst) {
        return RESULT_ERR;
    }

    if (pool_json_value_alloc(pool, dst) != RESULT_OK) {
        return RESULT_ERR;
    }

    (*dst)->type = JSON_TYPE_NUMBER;
    (*dst)->u.number_value = num;
    return RESULT_OK;
}

// Create string JSON value
result_t json_create_string(pool_t* pool, const string_t* string, json_value_t** dst) {
    if (!pool || !string || !dst) {
        return RESULT_ERR;
    }

    if (pool_json_value_alloc(pool, dst) != RESULT_OK) {
        return RESULT_ERR;
    }

    string_t* str_copy;
    if (string_copy(pool, &str_copy, string) != RESULT_OK) {
        pool_json_value_free(pool, *dst);
        return RESULT_ERR;
    }

    (*dst)->type = JSON_TYPE_STRING;
    (*dst)->u.string_value = str_copy;
    return RESULT_OK;
}

// Set object property by path
result_t json_object_set(pool_t* pool, json_value_t* object, const string_t* path, json_value_t* value) {
    if (!pool || !object || !path || !value || object->type != JSON_TYPE_OBJECT) {
        return RESULT_ERR;
    }

    json_object_t* obj = object->u.object_value;
    json_object_element_t* current = obj->head;
    json_object_element_t* prev = NULL;

    // Search for existing key
    while (current) {
        if (string_equal(current->key, path)) {
            // Replace existing value
            json_free_value_recursive(pool, current->value);
            current->value = value;
            return RESULT_OK;
        }
        prev = current;
        current = current->next;
    }

    // Create new element
    json_object_element_t* element;
    if (pool_json_object_element_alloc(pool, &element) != RESULT_OK) {
        return RESULT_ERR;
    }

    string_t* key_copy;
    if (string_copy(pool, &key_copy, path) != RESULT_OK) {
        pool_json_object_element_free(pool, element);
        return RESULT_ERR;
    }

    element->key = key_copy;
    element->value = value;
    element->next = NULL;

    if (prev) {
        prev->next = element;
    } else {
        obj->head = element;
    }

    obj->length++;
    return RESULT_OK;
}

// Get object property by path
result_t json_object_get(const json_value_t* object, const string_t* path, json_value_t** dst) {
    if (!object || !path || !dst || object->type != JSON_TYPE_OBJECT) {
        return RESULT_ERR;
    }

    json_object_t* obj = object->u.object_value;
    json_object_element_t* current = obj->head;

    while (current) {
        if (string_equal(current->key, path)) {
            *dst = current->value;
            return RESULT_OK;
        }
        current = current->next;
    }

    return RESULT_ERR;
}

// Append value to array
result_t json_array_append(pool_t* pool, json_value_t* array, json_value_t* value) {
    if (!pool || !array || !value || array->type != JSON_TYPE_ARRAY) {
        return RESULT_ERR;
    }

    json_array_t* arr = array->u.array_value;
    json_array_element_t* element;
    
    if (pool_json_array_element_alloc(pool, &element) != RESULT_OK) {
        return RESULT_ERR;
    }

    element->value = value;
    element->next = NULL;

    if (arr->head == NULL) {
        arr->head = element;
    } else {
        json_array_element_t* current = arr->head;
        while (current->next) {
            current = current->next;
        }
        current->next = element;
    }

    arr->length++;
    return RESULT_OK;
}

// Get array element by index
result_t json_array_get(const json_value_t* array, uint64_t index, json_value_t** dst) {
    if (!array || !dst || array->type != JSON_TYPE_ARRAY) {
        return RESULT_ERR;
    }

    json_array_t* arr = array->u.array_value;
    if (index >= arr->length) {
        return RESULT_ERR;
    }

    json_array_element_t* current = arr->head;
    for (uint64_t i = 0; i < index && current; i++) {
        current = current->next;
    }

    if (!current) {
        return RESULT_ERR;
    }

    *dst = current->value;
    return RESULT_OK;
}

// Get array length
result_t json_array_length(const json_value_t* array, uint64_t* dst) {
    if (!array || !dst || array->type != JSON_TYPE_ARRAY) {
        return RESULT_ERR;
    }

    *dst = array->u.array_value->length;
    return RESULT_OK;
}

// Get object length
result_t json_object_length(const json_value_t* object, uint64_t* dst) {
    if (!object || !dst || object->type != JSON_TYPE_OBJECT) {
        return RESULT_ERR;
    }

    *dst = object->u.object_value->length;
    return RESULT_OK;
}

// Delete JSON value and free memory
result_t json_delete(pool_t* pool, json_value_t* value) {
    if (!pool || !value) {
        return RESULT_ERR;
    }

    return json_free_value_recursive(pool, value);
}

// Remove object property by path
result_t json_object_remove(pool_t* pool, json_value_t* object, const string_t* path) {
    if (!pool || !object || !path || object->type != JSON_TYPE_OBJECT) {
        return RESULT_ERR;
    }

    json_object_t* obj = object->u.object_value;
    json_object_element_t* current = obj->head;
    json_object_element_t* prev = NULL;

    while (current) {
        if (string_equal(current->key, path)) {
            if (prev) {
                prev->next = current->next;
            } else {
                obj->head = current->next;
            }

            json_free_value_recursive(pool, current->value);
            pool_string_free(pool, current->key);
            pool_json_object_element_free(pool, current);
            obj->length--;
            return RESULT_OK;
        }
        prev = current;
        current = current->next;
    }

    return RESULT_ERR;
}

// Remove array element by index
result_t json_array_remove(pool_t* pool, json_value_t* array, uint64_t index) {
    if (!pool || !array || array->type != JSON_TYPE_ARRAY) {
        return RESULT_ERR;
    }

    json_array_t* arr = array->u.array_value;
    if (index >= arr->length) {
        return RESULT_ERR;
    }

    json_array_element_t* current = arr->head;
    json_array_element_t* prev = NULL;

    for (uint64_t i = 0; i < index && current; i++) {
        prev = current;
        current = current->next;
    }

    if (!current) {
        return RESULT_ERR;
    }

    if (prev) {
        prev->next = current->next;
    } else {
        arr->head = current->next;
    }

    json_free_value_recursive(pool, current->value);
    pool_json_array_element_free(pool, current);
    arr->length--;
    return RESULT_OK;
}

// Deep copy JSON value
result_t json_deep_copy(pool_t* pool, const json_value_t* src, json_value_t** dst) {
    if (!pool || !src || !dst) {
        return RESULT_ERR;
    }

    switch (src->type) {
        case JSON_TYPE_NULL:
            return json_create_null(pool, dst);

        case JSON_TYPE_BOOL:
            return json_create_bool(pool, src->u.bool_value, dst);

        case JSON_TYPE_NUMBER:
            return json_create_number(pool, src->u.number_value, dst);

        case JSON_TYPE_STRING:
            return json_create_string(pool, src->u.string_value, dst);

        case JSON_TYPE_ARRAY: {
            if (json_create_array(pool, dst) != RESULT_OK) {
                return RESULT_ERR;
            }

            json_array_element_t* current = src->u.array_value->head;
            while (current) {
                json_value_t* copied_value;
                if (json_deep_copy(pool, current->value, &copied_value) != RESULT_OK) {
                    json_free_value_recursive(pool, *dst);
                    return RESULT_ERR;
                }

                if (json_array_append(pool, *dst, copied_value) != RESULT_OK) {
                    json_free_value_recursive(pool, copied_value);
                    json_free_value_recursive(pool, *dst);
                    return RESULT_ERR;
                }

                current = current->next;
            }
            return RESULT_OK;
        }

        case JSON_TYPE_OBJECT: {
            if (json_create_object(pool, dst) != RESULT_OK) {
                return RESULT_ERR;
            }

            json_object_element_t* current = src->u.object_value->head;
            while (current) {
                json_value_t* copied_value;
                if (json_deep_copy(pool, current->value, &copied_value) != RESULT_OK) {
                    json_free_value_recursive(pool, *dst);
                    return RESULT_ERR;
                }

                if (json_object_set(pool, *dst, current->key, copied_value) != RESULT_OK) {
                    json_free_value_recursive(pool, copied_value);
                    json_free_value_recursive(pool, *dst);
                    return RESULT_ERR;
                }

                current = current->next;
            }
            return RESULT_OK;
        }

        default:
            return RESULT_ERR;
    }
}

// Internal function implementations

static void json_skip_whitespace(const char** json) {
    while (**json && isspace(**json)) {
        (*json)++;
    }
}

static result_t json_parse_value(pool_t* pool, const char** json, json_value_t** dst) {
    json_skip_whitespace(json);

    switch (**json) {
        case '{':
            return json_parse_object(pool, json, dst);
        case '[':
            return json_parse_array(pool, json, dst);
        case '"':
            {
                string_t* str;
                if (json_parse_string(pool, json, &str) != RESULT_OK) {
                    return RESULT_ERR;
                }
                return json_create_string(pool, str, dst);
            }
        case 't':
            if (json_parse_literal(json, "true") == RESULT_OK) {
                return json_create_bool(pool, 1, dst);
            }
            return RESULT_ERR;
        case 'f':
            if (json_parse_literal(json, "false") == RESULT_OK) {
                return json_create_bool(pool, 0, dst);
            }
            return RESULT_ERR;
        case 'n':
            if (json_parse_literal(json, "null") == RESULT_OK) {
                return json_create_null(pool, dst);
            }
            return RESULT_ERR;
        default:
            if (**json == '-' || isdigit(**json)) {
                double num;
                if (json_parse_number(json, &num) == RESULT_OK) {
                    return json_create_number(pool, num, dst);
                }
            }
            return RESULT_ERR;
    }
}

static result_t json_parse_object(pool_t* pool, const char** json, json_value_t** dst) {
    if (**json != '{') {
        return RESULT_ERR;
    }
    (*json)++;

    if (json_create_object(pool, dst) != RESULT_OK) {
        return RESULT_ERR;
    }

    json_skip_whitespace(json);
    if (**json == '}') {
        (*json)++;
        return RESULT_OK;
    }

    while (1) {
        json_skip_whitespace(json);

        // Parse key
        if (**json != '"') {
            json_free_value_recursive(pool, *dst);
            return RESULT_ERR;
        }

        string_t* key;
        if (json_parse_string(pool, json, &key) != RESULT_OK) {
            json_free_value_recursive(pool, *dst);
            return RESULT_ERR;
        }

        json_skip_whitespace(json);
        if (**json != ':') {
            pool_string_free(pool, key);
            json_free_value_recursive(pool, *dst);
            return RESULT_ERR;
        }
        (*json)++;

        // Parse value
        json_value_t* value;
        if (json_parse_value(pool, json, &value) != RESULT_OK) {
            pool_string_free(pool, key);
            json_free_value_recursive(pool, *dst);
            return RESULT_ERR;
        }

        if (json_object_set(pool, *dst, key, value) != RESULT_OK) {
            pool_string_free(pool, key);
            json_free_value_recursive(pool, value);
            json_free_value_recursive(pool, *dst);
            return RESULT_ERR;
        }

        pool_string_free(pool, key);

        json_skip_whitespace(json);
        if (**json == '}') {
            (*json)++;
            return RESULT_OK;
        } else if (**json == ',') {
            (*json)++;
        } else {
            json_free_value_recursive(pool, *dst);
            return RESULT_ERR;
        }
    }
}

static result_t json_parse_array(pool_t* pool, const char** json, json_value_t** dst) {
    if (**json != '[') {
        return RESULT_ERR;
    }
    (*json)++;

    if (json_create_array(pool, dst) != RESULT_OK) {
        return RESULT_ERR;
    }

    json_skip_whitespace(json);
    if (**json == ']') {
        (*json)++;
        return RESULT_OK;
    }

    while (1) {
        json_value_t* value;
        if (json_parse_value(pool, json, &value) != RESULT_OK) {
            json_free_value_recursive(pool, *dst);
            return RESULT_ERR;
        }

        if (json_array_append(pool, *dst, value) != RESULT_OK) {
            json_free_value_recursive(pool, value);
            json_free_value_recursive(pool, *dst);
            return RESULT_ERR;
        }

        json_skip_whitespace(json);
        if (**json == ']') {
            (*json)++;
            return RESULT_OK;
        } else if (**json == ',') {
            (*json)++;
        } else {
            json_free_value_recursive(pool, *dst);
            return RESULT_ERR;
        }
    }
}

static result_t json_parse_string(pool_t* pool, const char** json, string_t** dst) {
    if (**json != '"') {
        return RESULT_ERR;
    }
    (*json)++;

    const char* start = *json;
    const char* end = start;
    
    // Find end of string
    while (*end && *end != '"') {
        if (*end == '\\') {
            end++;
            if (*end) end++;
        } else {
            end++;
        }
    }

    if (*end != '"') {
        return RESULT_ERR;
    }

    uint64_t length = end - start;
    
    if (pool_string_alloc(pool, dst, length + 1) != RESULT_OK) {
        return RESULT_ERR;
    }

    // Copy and unescape string
    char* dest = (*dst)->data;
    const char* src = start;
    uint64_t dest_len = 0;

    while (src < end) {
        if (*src == '\\' && src + 1 < end) {
            src++;
            switch (*src) {
                case '"': *dest++ = '"'; break;
                case '\\': *dest++ = '\\'; break;
                case '/': *dest++ = '/'; break;
                case 'b': *dest++ = '\b'; break;
                case 'f': *dest++ = '\f'; break;
                case 'n': *dest++ = '\n'; break;
                case 'r': *dest++ = '\r'; break;
                case 't': *dest++ = '\t'; break;
                case 'u':
                    // Simple unicode escape (only supports basic ASCII range)
                    if (src + 4 < end) {
                        src += 4;
                        *dest++ = '?'; // Placeholder for unicode
                    } else {
                        *dest++ = '?';
                    }
                    break;
                default:
                    *dest++ = *src;
                    break;
            }
            src++;
            dest_len++;
        } else {
            *dest++ = *src++;
            dest_len++;
        }
    }

    *dest = '\0';
    (*dst)->size = dest_len;

    *json = end + 1; // Skip closing quote
    return RESULT_OK;
}

static result_t json_parse_number(const char** json, double* dst) {
    char* endptr;
    *dst = strtod(*json, &endptr);
    
    if (endptr == *json) {
        return RESULT_ERR;
    }

    *json = endptr;
    return RESULT_OK;
}

static result_t json_parse_literal(const char** json, const char* literal) {
    uint64_t len = strlen(literal);
    if (strncmp(*json, literal, len) == 0) {
        *json += len;
        return RESULT_OK;
    }
    return RESULT_ERR;
}

static result_t json_stringify_value(pool_t* pool, const json_value_t* value, string_t** dst) {
    switch (value->type) {
        case JSON_TYPE_NULL:
            return string_assign(pool, dst, "null");

        case JSON_TYPE_BOOL:
            return string_assign(pool, dst, value->u.bool_value ? "true" : "false");

        case JSON_TYPE_NUMBER: {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%.17g", value->u.number_value);
            return string_assign(pool, dst, buffer);
        }

        case JSON_TYPE_STRING: {
            string_t* escaped;
            if (json_escape_string(pool, value->u.string_value, &escaped) != RESULT_OK) {
                return RESULT_ERR;
            }

            if (string_assign(pool, dst, "\"") != RESULT_OK) {
                pool_string_free(pool, escaped);
                return RESULT_ERR;
            }

            if (string_append(pool, dst, escaped) != RESULT_OK) {
                pool_string_free(pool, escaped);
                return RESULT_ERR;
            }

            if (string_append_str(pool, dst, "\"") != RESULT_OK) {
                pool_string_free(pool, escaped);
                return RESULT_ERR;
            }

            pool_string_free(pool, escaped);
            return RESULT_OK;
        }

        case JSON_TYPE_ARRAY: {
            if (string_assign(pool, dst, "[") != RESULT_OK) {
                return RESULT_ERR;
            }

            json_array_element_t* current = value->u.array_value->head;
            int first = 1;

            while (current) {
                if (!first) {
                    if (string_append_str(pool, dst, ",") != RESULT_OK) {
                        return RESULT_ERR;
                    }
                }

                string_t* element_str;
                if (json_stringify_value(pool, current->value, &element_str) != RESULT_OK) {
                    return RESULT_ERR;
                }

                if (string_append(pool, dst, element_str) != RESULT_OK) {
                    pool_string_free(pool, element_str);
                    return RESULT_ERR;
                }

                pool_string_free(pool, element_str);
                current = current->next;
                first = 0;
            }

            return string_append_str(pool, dst, "]");
        }

        case JSON_TYPE_OBJECT: {
            if (string_assign(pool, dst, "{") != RESULT_OK) {
                return RESULT_ERR;
            }

            json_object_element_t* current = value->u.object_value->head;
            int first = 1;

            while (current) {
                if (!first) {
                    if (string_append_str(pool, dst, ",") != RESULT_OK) {
                        return RESULT_ERR;
                    }
                }

                // Add key
                string_t* escaped_key;
                if (json_escape_string(pool, current->key, &escaped_key) != RESULT_OK) {
                    return RESULT_ERR;
                }

                if (string_append_str(pool, dst, "\"") != RESULT_OK ||
                    string_append(pool, dst, escaped_key) != RESULT_OK ||
                    string_append_str(pool, dst, "\":") != RESULT_OK) {
                    pool_string_free(pool, escaped_key);
                    return RESULT_ERR;
                }

                pool_string_free(pool, escaped_key);

                // Add value
                string_t* value_str;
                if (json_stringify_value(pool, current->value, &value_str) != RESULT_OK) {
                    return RESULT_ERR;
                }

                if (string_append(pool, dst, value_str) != RESULT_OK) {
                    pool_string_free(pool, value_str);
                    return RESULT_ERR;
                }

                pool_string_free(pool, value_str);
                current = current->next;
                first = 0;
            }

            return string_append_str(pool, dst, "}");
        }

        default:
            return RESULT_ERR;
    }
}

static result_t json_escape_string(pool_t* pool, const string_t* src, string_t** dst) {
    if (pool_string_alloc(pool, dst, src->size * 2 + 1) != RESULT_OK) {
        return RESULT_ERR;
    }

    char* dest = (*dst)->data;
    const char* source = src->data;
    uint64_t dest_len = 0;

    for (uint64_t i = 0; i < src->size; i++) {
        switch (source[i]) {
            case '"':
                *dest++ = '\\'; *dest++ = '"'; dest_len += 2;
                break;
            case '\\':
                *dest++ = '\\'; *dest++ = '\\'; dest_len += 2;
                break;
            case '\b':
                *dest++ = '\\'; *dest++ = 'b'; dest_len += 2;
                break;
            case '\f':
                *dest++ = '\\'; *dest++ = 'f'; dest_len += 2;
                break;
            case '\n':
                *dest++ = '\\'; *dest++ = 'n'; dest_len += 2;
                break;
            case '\r':
                *dest++ = '\\'; *dest++ = 'r'; dest_len += 2;
                break;
            case '\t':
                *dest++ = '\\'; *dest++ = 't'; dest_len += 2;
                break;
            default:
                if (source[i] < 32) {
                    // Control characters
                    dest += sprintf(dest, "\\u%04x", (unsigned char)source[i]);
                    dest_len += 6;
                } else {
                    *dest++ = source[i];
                    dest_len++;
                }
                break;
        }
    }

    *dest = '\0';
    (*dst)->size = dest_len;
    return RESULT_OK;
}

static result_t json_free_value_recursive(pool_t* pool, json_value_t* value) {
    if (!value) {
        return RESULT_OK;
    }

    switch (value->type) {
        case JSON_TYPE_STRING:
            if (value->u.string_value) {
                pool_string_free(pool, value->u.string_value);
            }
            break;

        case JSON_TYPE_ARRAY:
            if (value->u.array_value) {
                json_array_element_t* current = value->u.array_value->head;
                while (current) {
                    json_array_element_t* next = current->next;
                    json_free_value_recursive(pool, current->value);
                    pool_json_array_element_free(pool, current);
                    current = next;
                }
                pool_json_array_free(pool, value->u.array_value);
            }
            break;

        case JSON_TYPE_OBJECT:
            if (value->u.object_value) {
                json_object_element_t* current = value->u.object_value->head;
                while (current) {
                    json_object_element_t* next = current->next;
                    pool_string_free(pool, current->key);
                    json_free_value_recursive(pool, current->value);
                    pool_json_object_element_free(pool, current);
                    current = next;
                }
                pool_json_object_free(pool, value->u.object_value);
            }
            break;

        default:
            // NULL, BOOL, NUMBER don't need special cleanup
            break;
    }

    return pool_json_value_free(pool, value);
}
