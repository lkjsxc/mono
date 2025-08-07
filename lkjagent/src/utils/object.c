#include "utils/object.h"

// Helper function to process escape sequences when parsing JSON strings
static result_t process_escape_sequences(pool_t* pool, const char* input, size_t input_len, string_t** output) {
    if (pool_string_alloc(pool, output, input_len + 1) != RESULT_OK) {
        RETURN_ERR("Failed to allocate output string for escape processing");
    }

    const char* src = input;
    const char* src_end = input + input_len;
    char* dst = (*output)->data;
    size_t dst_pos = 0;

    while (src < src_end) {
        if (*src == '\\' && src + 1 < src_end) {
            // Process escape sequence
            switch (src[1]) {
                case '"':
                    dst[dst_pos++] = '"';
                    break;
                case '\\':
                    dst[dst_pos++] = '\\';
                    break;
                case '/':
                    dst[dst_pos++] = '/';
                    break;
                case 'b':
                    dst[dst_pos++] = '\b';
                    break;
                case 'f':
                    dst[dst_pos++] = '\f';
                    break;
                case 'n':
                    dst[dst_pos++] = '\n';
                    break;
                case 'r':
                    dst[dst_pos++] = '\r';
                    break;
                case 't':
                    dst[dst_pos++] = '\t';
                    break;
                case 'u':
                    // Unicode escape sequence \uXXXX
                    if (src + 5 < src_end) {
                        // For now, just copy the unicode sequence as-is
                        // Full unicode processing would require UTF-8 conversion
                        dst[dst_pos++] = '\\';
                        dst[dst_pos++] = 'u';
                        dst[dst_pos++] = src[2];
                        dst[dst_pos++] = src[3];
                        dst[dst_pos++] = src[4];
                        dst[dst_pos++] = src[5];
                        src += 4;  // Will be incremented by 2 below
                    } else {
                        // Invalid unicode sequence, treat as literal
                        dst[dst_pos++] = *src;
                    }
                    break;
                default:
                    // Unknown escape sequence, keep the backslash and character
                    dst[dst_pos++] = '\\';
                    dst[dst_pos++] = src[1];
                    break;
            }
            src += 2;
        } else {
            // Regular character
            dst[dst_pos++] = *src;
            src++;
        }
    }

    (*output)->size = dst_pos;
    return RESULT_OK;
}

// Helper function to determine if a string represents a JSON primitive (number, boolean, null)
// Returns true if the value should NOT be quoted in JSON output
static bool is_json_primitive(const string_t* str) {
    if (!str || str->size == 0) {
        return false;
    }

    const char* data = str->data;
    size_t size = str->size;

    // Check for null
    if (string_equal_str(str, "null")) {
        return true;
    }

    // Check for boolean values
    if (string_equal_str(str, "true") || string_equal_str(str, "false")) {
        return true;
    }

    // Check for numbers (integer or floating point)
    size_t i = 0;
    
    // Optional negative sign
    if (data[i] == '-') {
        i++;
    }
    
    if (i >= size) {
        return false;  // Just a minus sign
    }
    
    // Must have at least one digit
    if (data[i] < '0' || data[i] > '9') {
        return false;
    }
    
    // Handle integer part
    if (data[i] == '0') {
        i++;  // Single zero
    } else {
        // Non-zero digit followed by optional digits
        while (i < size && data[i] >= '0' && data[i] <= '9') {
            i++;
        }
    }
    
    // Optional decimal part
    if (i < size && data[i] == '.') {
        i++;
        if (i >= size || data[i] < '0' || data[i] > '9') {
            return false;  // Decimal point must be followed by digits
        }
        while (i < size && data[i] >= '0' && data[i] <= '9') {
            i++;
        }
    }
    
    // Optional exponent part
    if (i < size && (data[i] == 'e' || data[i] == 'E')) {
        i++;
        if (i < size && (data[i] == '+' || data[i] == '-')) {
            i++;
        }
        if (i >= size || data[i] < '0' || data[i] > '9') {
            return false;  // Exponent must have digits
        }
        while (i < size && data[i] >= '0' && data[i] <= '9') {
            i++;
        }
    }
    
    // Must have consumed the entire string
    return (i == size);
}

// Helper function to validate object integrity before JSON generation
// Returns true if the object has valid structure for JSON serialization
static bool validate_object_for_json(const object_t* obj) {
    if (!obj) {
        return true;  // NULL objects are valid (will be serialized as "null")
    }

    // If object has a string, ensure it's properly allocated and null-terminated
    if (obj->string) {
        if (!obj->string->data) {
            return false;  // String data pointer is NULL
        }
        // Check that the string data is null-terminated (defensive check)
        if (obj->string->size > 0 && obj->string->data[obj->string->size] != '\0') {
            return false;  // String is not properly null-terminated
        }
    }

    // Recursively check children
    const object_t* child = obj->child;
    while (child) {
        if (!validate_object_for_json(child)) {
            return false;
        }
        child = child->next;
    }

    return true;
}

// Helper function to escape characters when serializing to JSON
static result_t escape_json_string(pool_t* pool, const string_t* input, string_t** output) {
    // Estimate output size (worst case: every character needs escaping)
    size_t estimated_size = input->size * 2 + 1;
    if (pool_string_alloc(pool, output, estimated_size) != RESULT_OK) {
        RETURN_ERR("Failed to allocate output string for JSON escaping");
    }

    const char* src = input->data;
    const char* src_end = input->data + input->size;
    char* dst = (*output)->data;
    size_t dst_pos = 0;

    while (src < src_end) {
        switch (*src) {
            case '"':
                dst[dst_pos++] = '\\';
                dst[dst_pos++] = '"';
                break;
            case '\\':
                dst[dst_pos++] = '\\';
                dst[dst_pos++] = '\\';
                break;
            case '\b':
                dst[dst_pos++] = '\\';
                dst[dst_pos++] = 'b';
                break;
            case '\f':
                dst[dst_pos++] = '\\';
                dst[dst_pos++] = 'f';
                break;
            case '\n':
                dst[dst_pos++] = '\\';
                dst[dst_pos++] = 'n';
                break;
            case '\r':
                dst[dst_pos++] = '\\';
                dst[dst_pos++] = 'r';
                break;
            case '\t':
                dst[dst_pos++] = '\\';
                dst[dst_pos++] = 't';
                break;
            default:
                if (*src < 0x20) {
                    // Control characters - escape as \uXXXX
                    dst[dst_pos++] = '\\';
                    dst[dst_pos++] = 'u';
                    dst[dst_pos++] = '0';
                    dst[dst_pos++] = '0';
                    // Convert to hex
                    char hex[] = "0123456789ABCDEF";
                    dst[dst_pos++] = hex[(*src >> 4) & 0x0F];
                    dst[dst_pos++] = hex[*src & 0x0F];
                } else {
                    // Regular character
                    dst[dst_pos++] = *src;
                }
                break;
        }
        src++;
    }

    (*output)->size = dst_pos;
    return RESULT_OK;
}

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

    start++;  // Skip opening quote
    const char* current = start;

    // Find the closing quote (handle escapes)
    while (current < end && *current != '"') {
        if (*current == '\\' && current + 1 < end) {
            current += 2;  // Skip escaped character
        } else {
            current++;
        }
    }

    if (current >= end) {
        RETURN_ERR("Unterminated JSON string");
    }

    // Process escape sequences in the string content
    size_t raw_length = current - start;
    if (process_escape_sequences(pool, start, raw_length, result) != RESULT_OK) {
        RETURN_ERR("Failed to process escape sequences in JSON string");
    }

    *json = current + 1;  // Skip closing quote
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

    current++;  // Skip opening brace
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

    current++;  // Skip opening bracket
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

        // Create a temporary null-terminated string from the segment
        size_t length = current - start;
        char temp_str[length + 1];
        memcpy(temp_str, start, length);
        temp_str[length] = '\0';
        
        if (string_create_str(pool, &((*result)->string), temp_str) != RESULT_OK) {
            if (pool_object_free(pool, *result) != RESULT_OK) {
                RETURN_ERR("Failed to cleanup object after string creation failure");
            }
            RETURN_ERR("Failed to create string for primitive value");
        }
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

// Helper function to skip whitespace in XML parsing
static const char* skip_xml_whitespace(const char* xml, const char* end) {
    while (xml < end && (*xml == ' ' || *xml == '\t' || *xml == '\n' || *xml == '\r')) {
        xml++;
    }
    return xml;
}

// Helper function to parse XML tag name
static result_t parse_xml_tag_name(pool_t* pool, const char** xml, const char* end, string_t** result) {
    const char* start = *xml;
    const char* current = start;

    // Tag name starts with letter or underscore, followed by alphanumeric, hyphens, underscores, or periods
    if (current >= end || (!isalpha(*current) && *current != '_')) {
        RETURN_ERR("Invalid XML tag name start");
    }

    while (current < end && (isalnum(*current) || *current == '-' || *current == '_' || *current == '.' || *current == ':')) {
        current++;
    }

    size_t length = current - start;
    if (length == 0) {
        RETURN_ERR("Empty XML tag name");
    }

    // Create a temporary null-terminated string from the tag name segment
    char temp_str[length + 1];
    memcpy(temp_str, start, length);
    temp_str[length] = '\0';
    
    if (string_create_str(pool, result, temp_str) != RESULT_OK) {
        RETURN_ERR("Failed to create string for XML tag name");
    }

    *xml = current;
    return RESULT_OK;
}

// Helper function to parse XML text content (between tags)
static result_t parse_xml_text_content(pool_t* pool, const char** xml, const char* end, string_t** result) {
    const char* start = *xml;
    const char* current = start;

    // Find the next '<' or end of input
    while (current < end && *current != '<') {
        current++;
    }

    size_t length = current - start;

    // Trim whitespace from both ends
    while (length > 0 && (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')) {
        start++;
        length--;
    }
    while (length > 0 && (start[length - 1] == ' ' || start[length - 1] == '\t' || start[length - 1] == '\n' || start[length - 1] == '\r')) {
        length--;
    }

    if (length == 0) {
        *result = NULL;
        *xml = current;
        return RESULT_OK;
    }

    // Create a temporary null-terminated string from the text content segment
    char temp_str[length + 1];
    memcpy(temp_str, start, length);
    temp_str[length] = '\0';
    
    if (string_create_str(pool, result, temp_str) != RESULT_OK) {
        RETURN_ERR("Failed to create string for XML text content");
    }

    *xml = current;
    return RESULT_OK;
}

// Forward declaration for recursive parsing
static result_t parse_xml_element(pool_t* pool, const char** xml, const char* end, object_t** result);

// Helper function to parse XML attributes and child elements
static result_t parse_xml_content(pool_t* pool, const char** xml, const char* end, const string_t* tag_name, object_t** result) {
    const char* current = *xml;
    current = skip_xml_whitespace(current, end);

    // Check for self-closing tag
    if (current < end && *current == '/') {
        current++;
        current = skip_xml_whitespace(current, end);
        if (current >= end || *current != '>') {
            RETURN_ERR("Expected '>' after '/' in self-closing XML tag");
        }
        *xml = current + 1;
        return RESULT_OK;
    }

    // Expect '>'
    if (current >= end || *current != '>') {
        RETURN_ERR("Expected '>' after XML tag name");
    }
    current++;

    object_t* first_child = NULL;
    object_t* last_child = NULL;
    string_t* text_accumulator = NULL;

    while (current < end) {
        current = skip_xml_whitespace(current, end);

        if (current >= end) {
            RETURN_ERR("Unexpected end of XML input");
        }

        if (*current == '<') {
            current++;
            if (current < end && *current == '/') {
                // End tag
                current++;
                current = skip_xml_whitespace(current, end);

                // Parse closing tag name
                string_t* closing_tag;
                if (parse_xml_tag_name(pool, &current, end, &closing_tag) != RESULT_OK) {
                    RETURN_ERR("Failed to parse XML closing tag name");
                }

                // Verify tag names match
                if (!string_equal_string(tag_name, closing_tag)) {
                    if (string_destroy(pool, closing_tag) != RESULT_OK) {
                        RETURN_ERR("Failed to destroy closing tag string");
                    }
                    RETURN_ERR("XML closing tag does not match opening tag");
                }

                if (string_destroy(pool, closing_tag) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy closing tag string");
                }

                current = skip_xml_whitespace(current, end);
                if (current >= end || *current != '>') {
                    RETURN_ERR("Expected '>' after XML closing tag name");
                }
                current++;
                break;
            } else {
                // Child element
                current--; // Back up to include the '<'
                object_t* child;
                if (parse_xml_element(pool, &current, end, &child) != RESULT_OK) {
                    RETURN_ERR("Failed to parse XML child element");
                }

                if (!first_child) {
                    first_child = child;
                    last_child = child;
                } else {
                    last_child->next = child;
                    last_child = child;
                }
            }
        } else {
            // Text content
            string_t* text_content;
            if (parse_xml_text_content(pool, &current, end, &text_content) != RESULT_OK) {
                RETURN_ERR("Failed to parse XML text content");
            }

            if (text_content && text_content->size > 0) {
                if (!text_accumulator) {
                    text_accumulator = text_content;
                } else {
                    // Append to existing text
                    if (string_append_string(pool, &text_accumulator, text_content) != RESULT_OK) {
                        if (string_destroy(pool, text_content) != RESULT_OK) {
                            RETURN_ERR("Failed to destroy text content string");
                        }
                        RETURN_ERR("Failed to append text content");
                    }
                    if (string_destroy(pool, text_content) != RESULT_OK) {
                        RETURN_ERR("Failed to destroy appended text content string");
                    }
                }
            } else if (text_content) {
                if (string_destroy(pool, text_content) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy empty text content string");
                }
            }
        }
    }

    // Handle mixed content: if we have both text and child elements
    if (text_accumulator && first_child) {
        RETURN_ERR("Mixed content (text and elements) not supported in this XML parser");
    } else if (text_accumulator) {
        // Only text content - store directly in the result
        (*result)->string = text_accumulator;
    } else if (first_child) {
        // Only child elements - store as children
        (*result)->child = first_child;
    }
    // Empty element case is handled by having neither string nor child

    *xml = current;
    return RESULT_OK;
}

// Helper function to parse a complete XML element
static result_t parse_xml_element(pool_t* pool, const char** xml, const char* end, object_t** result) {
    const char* current = *xml;
    current = skip_xml_whitespace(current, end);

    if (current >= end || *current != '<') {
        RETURN_ERR("Expected '<' at start of XML element");
    }

    current++;

    // Parse tag name
    string_t* tag_name;
    if (parse_xml_tag_name(pool, &current, end, &tag_name) != RESULT_OK) {
        RETURN_ERR("Failed to parse XML tag name");
    }

    // Create object for element content
    object_t* element_content;
    if (pool_object_alloc(pool, &element_content) != RESULT_OK) {
        if (string_destroy(pool, tag_name) != RESULT_OK) {
            RETURN_ERR("Failed to destroy tag name string");
        }
        RETURN_ERR("Failed to allocate object for XML element content");
    }

    element_content->string = NULL;
    element_content->child = NULL;
    element_content->next = NULL;

    // Parse content (attributes and children)
    if (parse_xml_content(pool, &current, end, tag_name, &element_content) != RESULT_OK) {
        RETURN_ERR("Failed to parse XML element content");
    }

    // Create key-value pair object (like JSON objects)
    if (pool_object_alloc(pool, result) != RESULT_OK) {
        if (string_destroy(pool, tag_name) != RESULT_OK) {
            RETURN_ERR("Failed to destroy tag name string");
        }
        RETURN_ERR("Failed to allocate object for XML key-value pair");
    }

    (*result)->string = tag_name;  // Tag name is the key
    (*result)->child = element_content;  // Content is the value
    (*result)->next = NULL;

    *xml = current;
    return RESULT_OK;
}

result_t object_parse_xml(pool_t* pool, object_t** dst, const string_t* src) {
    if (!pool || !dst || !src) {
        RETURN_ERR("Invalid parameters for XML parsing");
    }

    if (src->size == 0) {
        RETURN_ERR("Empty XML string");
    }

    const char* xml = src->data;
    const char* end = src->data + src->size;
    const char* current = skip_xml_whitespace(xml, end);

    // Skip XML declaration if present
    if (current < end && *current == '<') {
        if (current + 1 < end && current[1] == '?') {
            // Skip XML declaration
            while (current < end && !(current[0] == '?' && current + 1 < end && current[1] == '>')) {
                current++;
            }
            if (current < end) {
                current += 2;  // Skip "?>"
            }
            current = skip_xml_whitespace(current, end);
        }
    }

    // Create a root container object to hold all parsed XML elements
    if (pool_object_alloc(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to allocate root object for XML");
    }

    (*dst)->string = NULL;
    (*dst)->child = NULL;
    (*dst)->next = NULL;

    // Parse all root-level XML elements
    object_t* first_child = NULL;
    object_t* last_child = NULL;

    while (current < end) {
        current = skip_xml_whitespace(current, end);
        
        // Check if we've reached the end
        if (current >= end) {
            break;
        }

        // Parse XML element if we encounter a '<'
        if (*current == '<') {
            // Skip comments and processing instructions
            if (current + 1 < end && current[1] == '!') {
                // Skip comment <!-- ... -->
                if (current + 4 < end && strncmp(current, "<!--", 4) == 0) {
                    current += 4;
                    while (current + 2 < end && strncmp(current, "-->", 3) != 0) {
                        current++;
                    }
                    if (current + 2 < end) {
                        current += 3; // Skip "-->"
                    }
                } else {
                    // Skip other declarations like <!DOCTYPE>
                    while (current < end && *current != '>') {
                        current++;
                    }
                    if (current < end) {
                        current++; // Skip '>'
                    }
                }
                continue;
            }

            // Parse XML element
            object_t* element;
            if (parse_xml_element(pool, &current, end, &element) != RESULT_OK) {
                RETURN_ERR("Failed to parse XML element");
            }

            // Add to children list
            if (!first_child) {
                first_child = element;
                last_child = element;
            } else {
                last_child->next = element;
                last_child = element;
            }
        } else {
            // Skip any text content at root level (whitespace, etc.)
            while (current < end && *current != '<') {
                current++;
            }
        }
    }

    // Set the parsed elements as children of the root container
    (*dst)->child = first_child;

    return RESULT_OK;
}

// Helper function to convert object to JSON string recursively
static result_t object_to_json_recursive(pool_t* pool, string_t** dst, const object_t* src, int depth) {
    if (!src) {
        return string_append_str(pool, dst, "null");
    }

    if (src->string && !src->child) {
        // Leaf node - just a value (check if it's a primitive or string)
        if (is_json_primitive(src->string)) {
            // Number, boolean, or null - don't quote
            return string_append_string(pool, dst, src->string);
        } else {
            // String value - add quotes and escape special characters
            string_t* escaped_string;
            if (escape_json_string(pool, src->string, &escaped_string) != RESULT_OK) {
                RETURN_ERR("Failed to escape string for JSON output");
            }

            if (string_append_char(pool, dst, '"') != RESULT_OK) {
                if (string_destroy(pool, escaped_string) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy escaped string");
                }
                RETURN_ERR("Failed to append opening quote");
            }
            if (string_append_string(pool, dst, escaped_string) != RESULT_OK) {
                if (string_destroy(pool, escaped_string) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy escaped string");
                }
                RETURN_ERR("Failed to append escaped string value");
            }
            if (string_append_char(pool, dst, '"') != RESULT_OK) {
                if (string_destroy(pool, escaped_string) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy escaped string");
                }
                RETURN_ERR("Failed to append closing quote");
            }

            if (string_destroy(pool, escaped_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy escaped string");
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
                string_t* escaped_key;
                if (escape_json_string(pool, child->string, &escaped_key) != RESULT_OK) {
                    RETURN_ERR("Failed to escape key for JSON output");
                }

                if (string_append_char(pool, dst, '"') != RESULT_OK) {
                    if (string_destroy(pool, escaped_key) != RESULT_OK) {
                        RETURN_ERR("Failed to destroy escaped key");
                    }
                    RETURN_ERR("Failed to append opening quote for key");
                }
                if (string_append_string(pool, dst, escaped_key) != RESULT_OK) {
                    if (string_destroy(pool, escaped_key) != RESULT_OK) {
                        RETURN_ERR("Failed to destroy escaped key");
                    }
                    RETURN_ERR("Failed to append escaped key");
                }
                if (string_append_str(pool, dst, "\":") != RESULT_OK) {
                    if (string_destroy(pool, escaped_key) != RESULT_OK) {
                        RETURN_ERR("Failed to destroy escaped key");
                    }
                    RETURN_ERR("Failed to append colon");
                }

                if (string_destroy(pool, escaped_key) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy escaped key");
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
    if (string_clear(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to create destination string");
    }

    // Validate object integrity before JSON generation
    if (!validate_object_for_json(src)) {
        RETURN_ERR("Object contains invalid data that cannot be safely serialized to JSON");
    }

    return object_to_json_recursive(pool, dst, src, 0);
}

// Helper function to escape characters when serializing to XML
static result_t escape_xml_string(pool_t* pool, const string_t* input, string_t** output) {
    // Estimate output size (worst case: every character needs escaping with spaces)
    size_t estimated_size = input->size * 8 + 1;  // " &quot; " is 8 chars
    if (pool_string_alloc(pool, output, estimated_size) != RESULT_OK) {
        RETURN_ERR("Failed to allocate output string for XML escaping");
    }

    const char* src = input->data;
    const char* src_end = input->data + input->size;
    char* dst = (*output)->data;
    size_t dst_pos = 0;

    while (src < src_end) {
        switch (*src) {
            case '<':
                // Expand to " &lt; "
                memcpy(dst + dst_pos, " &lt; ", 6);
                dst_pos += 6;
                break;
            case '>':
                // Expand to " &gt; "
                memcpy(dst + dst_pos, " &gt; ", 6);
                dst_pos += 6;
                break;
            case '&':
                // Expand to " &amp; "
                memcpy(dst + dst_pos, " &amp; ", 7);
                dst_pos += 7;
                break;
            case '"':
                // Expand to " &quot; "
                memcpy(dst + dst_pos, " &quot; ", 8);
                dst_pos += 8;
                break;
            case '\'':
                // Expand to " &apos; "
                memcpy(dst + dst_pos, " &apos; ", 8);
                dst_pos += 8;
                break;
            default:
                if (*src < 0x20 && *src != '\t' && *src != '\n' && *src != '\r') {
                    // Control characters - skip them for XML safety
                    break;
                } else {
                    // Regular character
                    dst[dst_pos++] = *src;
                }
                break;
        }
        src++;
    }

    (*output)->size = dst_pos;
    return RESULT_OK;
}

// Helper structure to hold children for sorting
typedef struct {
    object_t* child;
    char* key;
} sorted_child_t;

// Comparison function for sorting children by key name
static int compare_children(const void* a, const void* b) {
    const sorted_child_t* child_a = (const sorted_child_t*)a;
    const sorted_child_t* child_b = (const sorted_child_t*)b;
    return strcmp(child_a->key, child_b->key);
}

// Helper function to collect and sort children by their keys
static result_t collect_and_sort_children(const object_t* parent, sorted_child_t** sorted_children, size_t* count) {
    if (!parent || !parent->child) {
        *sorted_children = NULL;
        *count = 0;
        return RESULT_OK;
    }

    // First pass: count children
    *count = 0;
    object_t* child = parent->child;
    while (child) {
        if (child->string) {
            (*count)++;
        }
        child = child->next;
    }

    if (*count == 0) {
        *sorted_children = NULL;
        return RESULT_OK;
    }

    // Allocate array for sorted children
    *sorted_children = malloc(*count * sizeof(sorted_child_t));
    if (!*sorted_children) {
        RETURN_ERR("Failed to allocate memory for sorted children array");
    }

    // Second pass: collect children and their keys
    size_t i = 0;
    child = parent->child;
    while (child && i < *count) {
        if (child->string) {
            (*sorted_children)[i].child = child;
            
            // Convert key to C string for sorting
            (*sorted_children)[i].key = malloc(child->string->size + 1);
            if (!(*sorted_children)[i].key) {
                // Clean up allocated keys
                for (size_t j = 0; j < i; j++) {
                    free((*sorted_children)[j].key);
                }
                free(*sorted_children);
                RETURN_ERR("Failed to allocate memory for child key");
            }
            memcpy((*sorted_children)[i].key, child->string->data, child->string->size);
            (*sorted_children)[i].key[child->string->size] = '\0';
            i++;
        }
        child = child->next;
    }

    // Sort the children by key name
    qsort(*sorted_children, *count, sizeof(sorted_child_t), compare_children);

    return RESULT_OK;
}

// Helper function to free sorted children array
static void free_sorted_children(sorted_child_t* sorted_children, size_t count) {
    if (sorted_children) {
        for (size_t i = 0; i < count; i++) {
            free(sorted_children[i].key);
        }
        free(sorted_children);
    }
}

// Helper function to convert object to XML string recursively
static result_t object_to_xml_recursive(pool_t* pool, string_t** dst, const object_t* src, int depth, const char* element_name) {
    if (!src) {
        // Null value - represent as empty element
        if (string_append_str(pool, dst, "<") != RESULT_OK) {
            RETURN_ERR("Failed to append opening tag for null");
        }
        if (string_append_str(pool, dst, element_name) != RESULT_OK) {
            RETURN_ERR("Failed to append element name for null");
        }
        if (string_append_str(pool, dst, "/>") != RESULT_OK) {
            RETURN_ERR("Failed to append self-closing tag for null");
        }
        return RESULT_OK;
    }

    if (src->string && !src->child) {
        // Leaf node - just a value
        string_t* escaped_string;
        if (escape_xml_string(pool, src->string, &escaped_string) != RESULT_OK) {
            RETURN_ERR("Failed to escape string for XML output");
        }

        if (string_append_str(pool, dst, "<") != RESULT_OK) {
            if (string_destroy(pool, escaped_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy escaped string");
            }
            RETURN_ERR("Failed to append opening tag");
        }
        if (string_append_str(pool, dst, element_name) != RESULT_OK) {
            if (string_destroy(pool, escaped_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy escaped string");
            }
            RETURN_ERR("Failed to append element name");
        }
        if (string_append_str(pool, dst, ">") != RESULT_OK) {
            if (string_destroy(pool, escaped_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy escaped string");
            }
            RETURN_ERR("Failed to append closing bracket for opening tag");
        }
        if (string_append_string(pool, dst, escaped_string) != RESULT_OK) {
            if (string_destroy(pool, escaped_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy escaped string");
            }
            RETURN_ERR("Failed to append escaped string value");
        }
        if (string_append_str(pool, dst, "</") != RESULT_OK) {
            if (string_destroy(pool, escaped_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy escaped string");
            }
            RETURN_ERR("Failed to append opening of closing tag");
        }
        if (string_append_str(pool, dst, element_name) != RESULT_OK) {
            if (string_destroy(pool, escaped_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy escaped string");
            }
            RETURN_ERR("Failed to append element name for closing tag");
        }
        if (string_append_str(pool, dst, ">") != RESULT_OK) {
            if (string_destroy(pool, escaped_string) != RESULT_OK) {
                RETURN_ERR("Failed to destroy escaped string");
            }
            RETURN_ERR("Failed to append closing tag");
        }

        if (string_destroy(pool, escaped_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy escaped string");
        }
        return RESULT_OK;
    } else if (src->child) {
        object_t* first_child = src->child;

        // Check if this is an object (has key-value pairs) or array
        if (first_child && first_child->string) {
            // Object with key-value pairs
            if (string_append_str(pool, dst, "<") != RESULT_OK) {
                RETURN_ERR("Failed to append opening tag for object");
            }
            if (string_append_str(pool, dst, element_name) != RESULT_OK) {
                RETURN_ERR("Failed to append element name for object");
            }
            if (string_append_str(pool, dst, ">") != RESULT_OK) {
                RETURN_ERR("Failed to append closing bracket for object opening tag");
            }

            // Collect and sort children by their keys
            sorted_child_t* sorted_children;
            size_t child_count;
            if (collect_and_sort_children(src, &sorted_children, &child_count) != RESULT_OK) {
                RETURN_ERR("Failed to collect and sort children");
            }

            // Process children in sorted order
            for (size_t i = 0; i < child_count; i++) {
                object_t* child = sorted_children[i].child;
                if (child->string) {
                    // Use the key as the element name
                    string_t* key_escaped;
                    if (escape_xml_string(pool, child->string, &key_escaped) != RESULT_OK) {
                        free_sorted_children(sorted_children, child_count);
                        RETURN_ERR("Failed to escape key for XML element name");
                    }

                    // Convert key to C string for element name
                    char* key_cstr = malloc(key_escaped->size + 1);
                    if (!key_cstr) {
                        if (string_destroy(pool, key_escaped) != RESULT_OK) {
                            free_sorted_children(sorted_children, child_count);
                            RETURN_ERR("Failed to destroy escaped key");
                        }
                        free_sorted_children(sorted_children, child_count);
                        RETURN_ERR("Failed to allocate memory for key string");
                    }
                    memcpy(key_cstr, key_escaped->data, key_escaped->size);
                    key_cstr[key_escaped->size] = '\0';

                    if (object_to_xml_recursive(pool, dst, child->child, depth + 1, key_cstr) != RESULT_OK) {
                        free(key_cstr);
                        if (string_destroy(pool, key_escaped) != RESULT_OK) {
                            free_sorted_children(sorted_children, child_count);
                            RETURN_ERR("Failed to destroy escaped key");
                        }
                        free_sorted_children(sorted_children, child_count);
                        RETURN_ERR("Failed to convert child to XML");
                    }

                    free(key_cstr);
                    if (string_destroy(pool, key_escaped) != RESULT_OK) {
                        free_sorted_children(sorted_children, child_count);
                        RETURN_ERR("Failed to destroy escaped key");
                    }
                }
            }

            // Clean up sorted children array
            free_sorted_children(sorted_children, child_count);

            if (string_append_str(pool, dst, "</") != RESULT_OK) {
                RETURN_ERR("Failed to append opening of closing tag for object");
            }
            if (string_append_str(pool, dst, element_name) != RESULT_OK) {
                RETURN_ERR("Failed to append element name for object closing tag");
            }
            if (string_append_str(pool, dst, ">") != RESULT_OK) {
                RETURN_ERR("Failed to append closing tag for object");
            }
        } else {
            // Array - use numeric indices as element names
            if (string_append_str(pool, dst, "<") != RESULT_OK) {
                RETURN_ERR("Failed to append opening tag for array");
            }
            if (string_append_str(pool, dst, element_name) != RESULT_OK) {
                RETURN_ERR("Failed to append element name for array");
            }
            if (string_append_str(pool, dst, ">") != RESULT_OK) {
                RETURN_ERR("Failed to append closing bracket for array opening tag");
            }

            object_t* child = first_child;
            int index = 0;
            while (child) {
                char item_name[32];
                snprintf(item_name, sizeof(item_name), "item%d", index);

                if (object_to_xml_recursive(pool, dst, child, depth + 1, item_name) != RESULT_OK) {
                    RETURN_ERR("Failed to convert array element to XML");
                }

                child = child->next;
                index++;
            }

            if (string_append_str(pool, dst, "</") != RESULT_OK) {
                RETURN_ERR("Failed to append opening of closing tag for array");
            }
            if (string_append_str(pool, dst, element_name) != RESULT_OK) {
                RETURN_ERR("Failed to append element name for array closing tag");
            }
            if (string_append_str(pool, dst, ">") != RESULT_OK) {
                RETURN_ERR("Failed to append closing tag for array");
            }
        }
        return RESULT_OK;
    } else {
        // Empty object or null
        if (string_append_str(pool, dst, "<") != RESULT_OK) {
            RETURN_ERR("Failed to append opening tag for empty");
        }
        if (string_append_str(pool, dst, element_name) != RESULT_OK) {
            RETURN_ERR("Failed to append element name for empty");
        }
        if (string_append_str(pool, dst, "/>") != RESULT_OK) {
            RETURN_ERR("Failed to append self-closing tag for empty");
        }
        return RESULT_OK;
    }
}

result_t object_tostring_xml(pool_t* pool, string_t** dst, const object_t* src) {
    if (!pool || !dst) {
        RETURN_ERR("Invalid parameters for XML serialization");
    }

    if (string_clear(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to create destination string");
    }

    // For root level, handle objects/arrays differently to avoid wrapping tags
    if (src && src->child) {
        object_t* first_child = src->child;

        // Check if this is an object (has key-value pairs) or array
        if (first_child && first_child->string) {
            // Object with key-value pairs - output children directly in sorted order
            sorted_child_t* sorted_children;
            size_t child_count;
            if (collect_and_sort_children(src, &sorted_children, &child_count) != RESULT_OK) {
                RETURN_ERR("Failed to collect and sort root-level children");
            }

            // Process children in sorted order
            for (size_t i = 0; i < child_count; i++) {
                object_t* child = sorted_children[i].child;
                if (child->string) {
                    // Use the key as the element name
                    string_t* key_escaped;
                    if (escape_xml_string(pool, child->string, &key_escaped) != RESULT_OK) {
                        free_sorted_children(sorted_children, child_count);
                        RETURN_ERR("Failed to escape key for XML element name");
                    }

                    // Convert key to C string for element name
                    char* key_cstr = malloc(key_escaped->size + 1);
                    if (!key_cstr) {
                        if (string_destroy(pool, key_escaped) != RESULT_OK) {
                            free_sorted_children(sorted_children, child_count);
                            RETURN_ERR("Failed to destroy escaped key");
                        }
                        free_sorted_children(sorted_children, child_count);
                        RETURN_ERR("Failed to allocate memory for key string");
                    }
                    memcpy(key_cstr, key_escaped->data, key_escaped->size);
                    key_cstr[key_escaped->size] = '\0';

                    if (object_to_xml_recursive(pool, dst, child->child, 1, key_cstr) != RESULT_OK) {
                        free(key_cstr);
                        if (string_destroy(pool, key_escaped) != RESULT_OK) {
                            free_sorted_children(sorted_children, child_count);
                            RETURN_ERR("Failed to destroy escaped key");
                        }
                        free_sorted_children(sorted_children, child_count);
                        RETURN_ERR("Failed to convert child to XML");
                    }

                    free(key_cstr);
                    if (string_destroy(pool, key_escaped) != RESULT_OK) {
                        free_sorted_children(sorted_children, child_count);
                        RETURN_ERR("Failed to destroy escaped key");
                    }
                }
            }

            // Clean up sorted children array
            free_sorted_children(sorted_children, child_count);
            return RESULT_OK;
        } else {
            // Array - output items directly
            object_t* child = first_child;
            int index = 0;
            while (child) {
                char item_name[32];
                snprintf(item_name, sizeof(item_name), "item%d", index);

                if (object_to_xml_recursive(pool, dst, child, 1, item_name) != RESULT_OK) {
                    RETURN_ERR("Failed to convert array element to XML");
                }

                child = child->next;
                index++;
            }
            return RESULT_OK;
        }
    } else {
        // Single value or null - use a default element name
        return object_to_xml_recursive(pool, dst, src, 0, "value");
    }
}

// Helper function to find an object by path
static object_t* find_object_by_path(const object_t* object, const string_t* path) {
    if (!object || !path || path->size == 0) {
        return NULL;
    }

    const char* path_data = path->data;
    size_t path_len = path->size;
    size_t current_pos = 0;
    const object_t* current_obj = object;

    while (current_pos < path_len && current_obj) {
        // Find the next delimiter (. or [)
        size_t segment_start = current_pos;
        size_t segment_end = current_pos;

        // Find end of current segment
        while (segment_end < path_len) {
            char c = path_data[segment_end];
            if (c == '.' || c == '[') {
                break;
            }
            segment_end++;
        }

        // Check if this is an array access
        if (segment_end < path_len && path_data[segment_end] == '[') {
            // Find the closing bracket
            size_t bracket_start = segment_end + 1;
            size_t bracket_end = bracket_start;
            while (bracket_end < path_len && path_data[bracket_end] != ']') {
                bracket_end++;
            }

            if (bracket_end >= path_len) {
                return NULL;  // Malformed path - no closing bracket
            }

            // Parse the array index
            int index = 0;
            for (size_t i = bracket_start; i < bracket_end; i++) {
                char c = path_data[i];
                if (c < '0' || c > '9') {
                    return NULL;  // Invalid array index
                }
                index = index * 10 + (c - '0');
            }

            // Navigate to the array element
            current_obj = current_obj->child;  // Enter the array
            for (int i = 0; i < index && current_obj; i++) {
                current_obj = current_obj->next;
            }

            // Move past the bracket notation
            current_pos = bracket_end + 1;
        } else {
            // Object property access
            if (segment_end == segment_start) {
                return NULL;  // Empty segment
            }

            // Create a temporary string for comparison
            object_t* found_child = NULL;
            object_t* child = current_obj->child;

            // Create temporary null-terminated string for the path segment
            size_t segment_len = segment_end - segment_start;
            char temp_segment[segment_len + 1];
            memcpy(temp_segment, path_data + segment_start, segment_len);
            temp_segment[segment_len] = '\0';

            while (child) {
                if (child->string && string_equal_str(child->string, temp_segment)) {
                    found_child = child;
                    break;
                }
                child = child->next;
            }

            if (!found_child) {
                return NULL;  // Property not found
            }

            current_obj = found_child->child;  // Navigate to the property value
            current_pos = segment_end;
        }

        // Skip the delimiter if we're not at the end
        if (current_pos < path_len && (path_data[current_pos] == '.' || path_data[current_pos] == '[')) {
            if (path_data[current_pos] == '.') {
                current_pos++;  // Skip the dot
            }
            // For '[', we handle it in the next iteration
        }
    }

    return (object_t*)current_obj;
}

result_t object_get(object_t** dst, const object_t* object, const string_t* path) {
    if (!dst || !object || !path) {
        RETURN_ERR("Invalid parameters for object get");
    }

    *dst = find_object_by_path(object, path);
    if (*dst == NULL) {
        RETURN_ERR("Object not found at specified path");
    }

    return RESULT_OK;
}

result_t object_set(pool_t* pool, object_t* object, const string_t* path, object_t* value) {
    if (!pool || !object || !path || !value) {
        RETURN_ERR("Invalid parameters for object set");
    }

    if (path->size == 0) {
        RETURN_ERR("Empty path in object set");
    }

    const char* path_data = path->data;
    size_t path_len = path->size;
    size_t current_pos = 0;
    object_t* current_obj = object;

    // Navigate to the parent of the target location
    while (current_pos < path_len) {
        // Find the next delimiter
        size_t segment_start = current_pos;
        size_t segment_end = current_pos;

        while (segment_end < path_len && path_data[segment_end] != '.') {
            segment_end++;
        }

        // Check if this is the last segment
        bool is_last_segment = (segment_end == path_len);

        if (segment_end == segment_start) {
            RETURN_ERR("Empty segment in path");
        }

        // Look for existing child with this key
        object_t* found_child = NULL;
        object_t* child = current_obj->child;

        // Create temporary null-terminated string for the path segment
        size_t segment_len = segment_end - segment_start;
        char temp_segment[segment_len + 1];
        memcpy(temp_segment, path_data + segment_start, segment_len);
        temp_segment[segment_len] = '\0';

        while (child) {
            if (child->string && string_equal_str(child->string, temp_segment)) {
                found_child = child;
                break;
            }
            child = child->next;
        }

        if (is_last_segment) {
            // This is the target key - set the value
            if (found_child) {
                // Update existing value
                if (found_child->child) {
                    if (object_destroy(pool, found_child->child) != RESULT_OK) {
                        RETURN_ERR("Failed to destroy existing value");
                    }
                }
                found_child->child = value;
            } else {
                // Create new key-value pair
                object_t* new_child;
                if (object_create(pool, &new_child) != RESULT_OK) {
                    RETURN_ERR("Failed to create new child object");
                }

                // Set the key name by creating a temporary null-terminated string
                char temp_key[256];
                size_t key_len = segment_end - segment_start;
                if (key_len >= sizeof(temp_key)) {
                    if (object_destroy(pool, new_child) != RESULT_OK) {
                        RETURN_ERR("Failed to destroy new_child after key name too long error");
                    }
                    RETURN_ERR("Key name too long");
                }
                memcpy(temp_key, path_data + segment_start, key_len);
                temp_key[key_len] = '\0';

                if (string_create_str(pool, &new_child->string, temp_key) != RESULT_OK) {
                    if (object_destroy(pool, new_child) != RESULT_OK) {
                        RETURN_ERR("Failed to destroy new_child after key string creation failure");
                    }
                    RETURN_ERR("Failed to create key string");
                }

                // Set the value
                new_child->child = value;

                // Add to parent's children
                if (!current_obj->child) {
                    current_obj->child = new_child;
                } else {
                    // Find the last child and append
                    object_t* last_child = current_obj->child;
                    while (last_child->next) {
                        last_child = last_child->next;
                    }
                    last_child->next = new_child;
                }
            }
            return RESULT_OK;
        } else {
            // Not the last segment - navigate deeper
            if (!found_child) {
                // Create intermediate object
                object_t* new_child;
                if (object_create(pool, &new_child) != RESULT_OK) {
                    RETURN_ERR("Failed to create intermediate object");
                }

                // Set the key name
                char temp_key[256];
                size_t key_len = segment_end - segment_start;
                if (key_len >= sizeof(temp_key)) {
                    if (object_destroy(pool, new_child) != RESULT_OK) {
                        RETURN_ERR("Failed to destroy new_child after intermediate key name too long error");
                    }
                    RETURN_ERR("Intermediate key name too long");
                }
                memcpy(temp_key, path_data + segment_start, key_len);
                temp_key[key_len] = '\0';

                if (string_create_str(pool, &new_child->string, temp_key) != RESULT_OK) {
                    if (object_destroy(pool, new_child) != RESULT_OK) {
                        RETURN_ERR("Failed to destroy new_child after intermediate key string creation failure");
                    }
                    RETURN_ERR("Failed to create intermediate key string");
                }

                // Create container object for the intermediate level
                object_t* container;
                if (object_create(pool, &container) != RESULT_OK) {
                    if (object_destroy(pool, new_child) != RESULT_OK) {
                        RETURN_ERR("Failed to destroy new_child after intermediate container creation failure");
                    }
                    RETURN_ERR("Failed to create intermediate container");
                }

                new_child->child = container;

                // Add to parent's children
                if (!current_obj->child) {
                    current_obj->child = new_child;
                } else {
                    object_t* last_child = current_obj->child;
                    while (last_child->next) {
                        last_child = last_child->next;
                    }
                    last_child->next = new_child;
                }

                current_obj = container;
            } else {
                // Navigate to existing child
                if (!found_child->child) {
                    // Create container if it doesn't exist
                    if (object_create(pool, &found_child->child) != RESULT_OK) {
                        RETURN_ERR("Failed to create container for existing key");
                    }
                }
                current_obj = found_child->child;
            }

            current_pos = segment_end + 1; // Skip the dot
        }
    }

    return RESULT_OK;
}

result_t object_set_string(pool_t* pool, object_t* object, const string_t* path, const string_t* value) {
    // Create a string object from the value and call object_set
    object_t* value_object;
    if (object_create(pool, &value_object) != RESULT_OK) {
        RETURN_ERR("Failed to create value object");
    }

    if (string_create_string(pool, &(value_object->string), value) != RESULT_OK) {
        if (object_destroy(pool, value_object) != RESULT_OK) {
            // Log error but continue
        }
        RETURN_ERR("Failed to create value string");
    }

    return object_set(pool, object, path, value_object);
}

result_t object_provide_string(object_t** dst, const object_t* object, const string_t* path) {
    *dst = find_object_by_path(object, path);
    if (*dst == NULL) {
        RETURN_ERR("Object not found at specified path");
    }
    return RESULT_OK;
}

result_t object_provide_str(pool_t* pool, object_t** dst, const object_t* object, const char* path) {
    string_t* path_str;
    if (string_create_str(pool, &path_str, path) != RESULT_OK) {
        RETURN_ERR("Failed to create string from path");
    }

    result_t result = object_provide_string(dst, object, path_str);

    // Always try to destroy the path string, but don't let cleanup failures cascade
    if (string_destroy(pool, path_str) != RESULT_OK) {
        // Log the cleanup failure but don't propagate it
        printf("Warning: Failed to destroy temporary path string for '%s'\n", path);
    }

    if (result != RESULT_OK) {
        RETURN_ERR("Failed to provide object by string path");
    }
    return RESULT_OK;
}