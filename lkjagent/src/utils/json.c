#include "../lkjagent.h"

// JSON parsing constants and configuration
#define JSON_MAX_DEPTH 32
#define JSON_MAX_KEY_LEN 256
#define JSON_MAX_STRING_LEN 2048
#define JSON_MAX_NUMBER_LEN 64

// JSON value types
typedef enum {
    JSON_TYPE_NULL = 0,
    JSON_TYPE_BOOL,
    JSON_TYPE_NUMBER,
    JSON_TYPE_STRING,
    JSON_TYPE_OBJECT,
    JSON_TYPE_ARRAY
} json_type_t;

// JSON value structure for parsing state
typedef struct {
    json_type_t type;
    union {
        int bool_val;
        double number_val;
        token_t string_val;
    } value;
} json_value_t;

// JSON parser state structure
typedef struct {
    const char* input;
    size_t position;
    size_t length;
    int depth;
    char error_msg[256];
} json_parser_t;

// Forward declarations for recursive parsing functions
static result_t json_parse_value(json_parser_t* parser, json_value_t* value);
static result_t json_parse_string(json_parser_t* parser, token_t* result);
static result_t json_parse_number(json_parser_t* parser, double* result);
static result_t json_parse_object(json_parser_t* parser);
static result_t json_parse_array(json_parser_t* parser);

/**
 * Skip whitespace characters in JSON input
 * @param parser Pointer to parser state
 */
static void json_skip_whitespace(json_parser_t* parser) {
    if (!parser || !parser->input) {
        return;
    }

    while (parser->position < parser->length) {
        char c = parser->input[parser->position];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            parser->position++;
        } else {
            break;
        }
    }
}

/**
 * Peek at the current character without advancing position
 * @param parser Pointer to parser state
 * @return Current character or '\0' if at end
 */
static char json_peek_char(json_parser_t* parser) {
    if (!parser || parser->position >= parser->length) {
        return '\0';
    }
    return parser->input[parser->position];
}

/**
 * Get the current character and advance position
 * @param parser Pointer to parser state
 * @return Current character or '\0' if at end
 */
static char json_next_char(json_parser_t* parser) {
    if (!parser || parser->position >= parser->length) {
        return '\0';
    }
    return parser->input[parser->position++];
}

/**
 * Set an error message in the parser state
 * @param parser Pointer to parser state
 * @param message Error message to set
 */
static void json_set_error(json_parser_t* parser, const char* message) {
    if (!parser || !message) {
        return;
    }

    size_t msg_len = strlen(message);
    size_t max_len = sizeof(parser->error_msg) - 1;
    
    if (msg_len > max_len) {
        msg_len = max_len;
    }
    
    memcpy(parser->error_msg, message, msg_len);
    parser->error_msg[msg_len] = '\0';
}

/**
 * Check if a character is a valid hex digit
 * @param c Character to check
 * @return 1 if valid hex digit, 0 otherwise
 */
static int json_is_hex_digit(char c) {
    return (c >= '0' && c <= '9') || 
           (c >= 'a' && c <= 'f') || 
           (c >= 'A' && c <= 'F');
}



/**
 * Parse a JSON string value
 * @param parser Pointer to parser state
 * @param result Token to store the parsed string
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t json_parse_string(json_parser_t* parser, token_t* result) {
    if (!parser || token_validate(result) != RESULT_OK) {
        return RESULT_ERR;
    }

    json_skip_whitespace(parser);
    
    if (json_peek_char(parser) != '"') {
        json_set_error(parser, "Expected '\"' at start of string");
        return RESULT_ERR;
    }
    
    json_next_char(parser); // Skip opening quote
    
    if (token_clear(result) != RESULT_OK) {
        return RESULT_ERR;
    }

    while (parser->position < parser->length) {
        char c = json_next_char(parser);
        
        if (c == '"') {
            // End of string
            return RESULT_OK;
        }
        
        if (c == '\\') {
            // Escape sequence
            if (parser->position >= parser->length) {
                json_set_error(parser, "Unterminated escape sequence");
                return RESULT_ERR;
            }
            
            char escaped = json_next_char(parser);
            char to_append;
            
            switch (escaped) {
                case '"': to_append = '"'; break;
                case '\\': to_append = '\\'; break;
                case '/': to_append = '/'; break;
                case 'b': to_append = '\b'; break;
                case 'f': to_append = '\f'; break;
                case 'n': to_append = '\n'; break;
                case 'r': to_append = '\r'; break;
                case 't': to_append = '\t'; break;
                case 'u': {
                    // Unicode escape sequence \uXXXX
                    if (parser->position + 4 > parser->length) {
                        json_set_error(parser, "Incomplete unicode escape");
                        return RESULT_ERR;
                    }
                    
                    // Validate all 4 hex digits
                    for (int i = 0; i < 4; i++) {
                        if (!json_is_hex_digit(parser->input[parser->position + i])) {
                            json_set_error(parser, "Invalid unicode escape");
                            return RESULT_ERR;
                        }
                    }
                    
                    // For simplicity, we'll just skip unicode for now
                    // In a full implementation, you'd convert to UTF-8
                    parser->position += 4;
                    to_append = '?'; // Placeholder
                    break;
                }
                default:
                    json_set_error(parser, "Invalid escape sequence");
                    return RESULT_ERR;
            }
            
            // Append the escaped character
            char temp_str[2] = {to_append, '\0'};
            if (token_append(result, temp_str) != RESULT_OK) {
                json_set_error(parser, "String too long");
                return RESULT_ERR;
            }
        } else if (c < 0x20) {
            // Control characters must be escaped
            json_set_error(parser, "Unescaped control character in string");
            return RESULT_ERR;
        } else {
            // Regular character
            char temp_str[2] = {c, '\0'};
            if (token_append(result, temp_str) != RESULT_OK) {
                json_set_error(parser, "String too long");
                return RESULT_ERR;
            }
        }
    }
    
    json_set_error(parser, "Unterminated string");
    return RESULT_ERR;
}

/**
 * Parse a JSON number value
 * @param parser Pointer to parser state
 * @param result Pointer to store the parsed number
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t json_parse_number(json_parser_t* parser, double* result) {
    if (!parser || !result) {
        return RESULT_ERR;
    }

    json_skip_whitespace(parser);
    
    char number_str[JSON_MAX_NUMBER_LEN];
    size_t num_pos = 0;
    
    // Parse the number character by character
    while (parser->position < parser->length && num_pos < sizeof(number_str) - 1) {
        char c = json_peek_char(parser);
        
        if ((c >= '0' && c <= '9') || c == '-' || c == '+' || 
            c == '.' || c == 'e' || c == 'E') {
            number_str[num_pos++] = json_next_char(parser);
        } else {
            break;
        }
    }
    
    if (num_pos == 0) {
        json_set_error(parser, "Invalid number format");
        return RESULT_ERR;
    }
    
    number_str[num_pos] = '\0';
    
    char* endptr;
    *result = strtod(number_str, &endptr);
    
    if (endptr == number_str || *endptr != '\0') {
        json_set_error(parser, "Invalid number format");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

/**
 * Parse a JSON literal (true, false, null)
 * @param parser Pointer to parser state
 * @param expected Expected literal string
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t json_parse_literal(json_parser_t* parser, const char* expected) {
    if (!parser || !expected) {
        return RESULT_ERR;
    }

    json_skip_whitespace(parser);
    
    size_t expected_len = strlen(expected);
    
    if (parser->position + expected_len > parser->length) {
        json_set_error(parser, "Unexpected end of input");
        return RESULT_ERR;
    }
    
    if (strncmp(parser->input + parser->position, expected, expected_len) != 0) {
        json_set_error(parser, "Invalid literal");
        return RESULT_ERR;
    }
    
    parser->position += expected_len;
    return RESULT_OK;
}

/**
 * Parse a JSON object
 * @param parser Pointer to parser state
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t json_parse_object(json_parser_t* parser) {
    if (!parser) {
        return RESULT_ERR;
    }

    json_skip_whitespace(parser);
    
    if (json_next_char(parser) != '{') {
        json_set_error(parser, "Expected '{'");
        return RESULT_ERR;
    }
    
    parser->depth++;
    if (parser->depth > JSON_MAX_DEPTH) {
        json_set_error(parser, "Maximum nesting depth exceeded");
        return RESULT_ERR;
    }
    
    json_skip_whitespace(parser);
    
    // Handle empty object
    if (json_peek_char(parser) == '}') {
        json_next_char(parser);
        parser->depth--;
        return RESULT_OK;
    }
    
    // Parse key-value pairs
    int first_pair = 1;
    while (parser->position < parser->length) {
        if (!first_pair) {
            json_skip_whitespace(parser);
            if (json_peek_char(parser) == '}') {
                json_next_char(parser);
                parser->depth--;
                return RESULT_OK;
            }
            
            if (json_next_char(parser) != ',') {
                json_set_error(parser, "Expected ',' or '}' in object");
                return RESULT_ERR;
            }
        }
        first_pair = 0;
        
        // Parse key (must be a string)
        char key_buffer[JSON_MAX_KEY_LEN];
        token_t key_token;
        if (token_init(&key_token, key_buffer, sizeof(key_buffer)) != RESULT_OK) {
            return RESULT_ERR;
        }
        
        if (json_parse_string(parser, &key_token) != RESULT_OK) {
            return RESULT_ERR;
        }
        
        // Expect colon
        json_skip_whitespace(parser);
        if (json_next_char(parser) != ':') {
            json_set_error(parser, "Expected ':' after object key");
            return RESULT_ERR;
        }
        
        // Parse value
        json_value_t value;
        if (json_parse_value(parser, &value) != RESULT_OK) {
            return RESULT_ERR;
        }
        
        json_skip_whitespace(parser);
    }
    
    json_set_error(parser, "Unterminated object");
    return RESULT_ERR;
}

/**
 * Parse a JSON array
 * @param parser Pointer to parser state
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t json_parse_array(json_parser_t* parser) {
    if (!parser) {
        return RESULT_ERR;
    }

    json_skip_whitespace(parser);
    
    if (json_next_char(parser) != '[') {
        json_set_error(parser, "Expected '['");
        return RESULT_ERR;
    }
    
    parser->depth++;
    if (parser->depth > JSON_MAX_DEPTH) {
        json_set_error(parser, "Maximum nesting depth exceeded");
        return RESULT_ERR;
    }
    
    json_skip_whitespace(parser);
    
    // Handle empty array
    if (json_peek_char(parser) == ']') {
        json_next_char(parser);
        parser->depth--;
        return RESULT_OK;
    }
    
    // Parse array elements
    int first_element = 1;
    while (parser->position < parser->length) {
        if (!first_element) {
            json_skip_whitespace(parser);
            if (json_peek_char(parser) == ']') {
                json_next_char(parser);
                parser->depth--;
                return RESULT_OK;
            }
            
            if (json_next_char(parser) != ',') {
                json_set_error(parser, "Expected ',' or ']' in array");
                return RESULT_ERR;
            }
        }
        first_element = 0;
        
        // Parse value
        json_value_t value;
        if (json_parse_value(parser, &value) != RESULT_OK) {
            return RESULT_ERR;
        }
        
        json_skip_whitespace(parser);
    }
    
    json_set_error(parser, "Unterminated array");
    return RESULT_ERR;
}

/**
 * Parse any JSON value
 * @param parser Pointer to parser state
 * @param value Pointer to store the parsed value
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t json_parse_value(json_parser_t* parser, json_value_t* value) {
    if (!parser || !value) {
        return RESULT_ERR;
    }

    json_skip_whitespace(parser);
    
    char c = json_peek_char(parser);
    
    switch (c) {
        case '"': {
            // String value
            char string_buffer[JSON_MAX_STRING_LEN];
            if (token_init(&value->value.string_val, string_buffer, sizeof(string_buffer)) != RESULT_OK) {
                return RESULT_ERR;
            }
            
            value->type = JSON_TYPE_STRING;
            return json_parse_string(parser, &value->value.string_val);
        }
        
        case '{':
            // Object value
            value->type = JSON_TYPE_OBJECT;
            return json_parse_object(parser);
            
        case '[':
            // Array value
            value->type = JSON_TYPE_ARRAY;
            return json_parse_array(parser);
            
        case 't':
            // true literal
            value->type = JSON_TYPE_BOOL;
            value->value.bool_val = 1;
            return json_parse_literal(parser, "true");
            
        case 'f':
            // false literal
            value->type = JSON_TYPE_BOOL;
            value->value.bool_val = 0;
            return json_parse_literal(parser, "false");
            
        case 'n':
            // null literal
            value->type = JSON_TYPE_NULL;
            return json_parse_literal(parser, "null");
            
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            // Number value
            value->type = JSON_TYPE_NUMBER;
            return json_parse_number(parser, &value->value.number_val);
            
        default:
            json_set_error(parser, "Unexpected character");
            return RESULT_ERR;
    }
}

// Public JSON API functions

/**
 * Validate if a token contains valid JSON
 * @param json_token Token containing JSON data
 * @return RESULT_OK if valid JSON, RESULT_ERR otherwise
 */
__attribute__((warn_unused_result)) result_t json_validate(const token_t* json_token) {
    if (token_validate(json_token) != RESULT_OK || token_is_empty(json_token)) {
        return RESULT_ERR;
    }

    json_parser_t parser = {
        .input = json_token->data,
        .position = 0,
        .length = json_token->size,
        .depth = 0,
        .error_msg = {0}
    };

    json_value_t root_value;
    if (json_parse_value(&parser, &root_value) != RESULT_OK) {
        return RESULT_ERR;
    }

    // Check that we've consumed all input (except whitespace)
    json_skip_whitespace(&parser);
    if (parser.position < parser.length) {
        return RESULT_ERR; // Extra data after valid JSON
    }

    return RESULT_OK;
}

/**
 * Extract a string value from JSON by key path
 * @param json_token Token containing JSON data
 * @param key_path Dot-separated path to the desired key (e.g., "object.key")
 * @param result Token to store the extracted string value
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result)) result_t json_get_string(const token_t* json_token, const char* key_path, token_t* result) {
    if (token_validate(json_token) != RESULT_OK || !key_path || token_validate(result) != RESULT_OK) {
        return RESULT_ERR;
    }

    // For this basic implementation, we'll do simple string searching
    // In a full implementation, you'd parse the JSON structure properly
    
    // Find the key in the JSON
    size_t key_pos;
    char search_pattern[JSON_MAX_KEY_LEN + 10]; // Extra space for quotes and colon
    int len = snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", key_path);
    
    if (len < 0 || (size_t)len >= sizeof(search_pattern)) {
        return RESULT_ERR;
    }
    
    if (token_find(json_token, search_pattern, &key_pos) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Move to the value part (after the colon)
    size_t value_start = key_pos + strlen(search_pattern);
    
    // Skip whitespace
    while (value_start < json_token->size && 
           (json_token->data[value_start] == ' ' || 
            json_token->data[value_start] == '\t' ||
            json_token->data[value_start] == '\n' ||
            json_token->data[value_start] == '\r')) {
        value_start++;
    }
    
    // Check if it's a string value (starts with quote)
    if (value_start >= json_token->size || json_token->data[value_start] != '"') {
        return RESULT_ERR;
    }
    
    // Find the end quote
    size_t value_end = value_start + 1;
    while (value_end < json_token->size && json_token->data[value_end] != '"') {
        if (json_token->data[value_end] == '\\') {
            value_end++; // Skip escaped character
        }
        value_end++;
    }
    
    if (value_end >= json_token->size) {
        return RESULT_ERR; // Unterminated string
    }
    
    // Extract the string value (without quotes)
    size_t string_len = value_end - value_start - 1;
    if (string_len >= result->capacity) {
        return RESULT_ERR; // String too long
    }
    
    if (token_clear(result) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    return token_set_length(result, json_token->data + value_start + 1, string_len);
}

/**
 * Extract a number value from JSON by key path
 * @param json_token Token containing JSON data
 * @param key_path Dot-separated path to the desired key
 * @param result Pointer to store the extracted number value
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result)) result_t json_get_number(const token_t* json_token, const char* key_path, double* result) {
    if (token_validate(json_token) != RESULT_OK || !key_path || !result) {
        return RESULT_ERR;
    }

    // Find the key in the JSON
    size_t key_pos;
    char search_pattern[JSON_MAX_KEY_LEN + 10];
    int len = snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", key_path);
    
    if (len < 0 || (size_t)len >= sizeof(search_pattern)) {
        return RESULT_ERR;
    }
    
    if (token_find(json_token, search_pattern, &key_pos) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Create a temporary parser to parse the number
    json_parser_t parser = {
        .input = json_token->data,
        .position = key_pos + strlen(search_pattern),
        .length = json_token->size,
        .depth = 0,
        .error_msg = {0}
    };
    
    return json_parse_number(&parser, result);
}

/**
 * Create a simple JSON object with string key-value pairs
 * @param result Token to store the generated JSON
 * @param keys Array of key strings
 * @param values Array of value strings
 * @param count Number of key-value pairs
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result)) result_t json_create_object(token_t* result, const char* keys[], const char* values[], size_t count) {
    if (token_validate(result) != RESULT_OK || !keys || !values) {
        return RESULT_ERR;
    }

    if (token_clear(result) != RESULT_OK) {
        return RESULT_ERR;
    }

    if (token_append(result, "{") != RESULT_OK) {
        return RESULT_ERR;
    }

    for (size_t i = 0; i < count; i++) {
        if (!keys[i] || !values[i]) {
            return RESULT_ERR;
        }

        if (i > 0) {
            if (token_append(result, ",") != RESULT_OK) {
                return RESULT_ERR;
            }
        }

        // Add key
        if (token_append(result, "\"") != RESULT_OK ||
            token_append(result, keys[i]) != RESULT_OK ||
            token_append(result, "\":\"") != RESULT_OK ||
            token_append(result, values[i]) != RESULT_OK ||
            token_append(result, "\"") != RESULT_OK) {
            return RESULT_ERR;
        }
    }

    if (token_append(result, "}") != RESULT_OK) {
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * Pretty print JSON with proper indentation
 * @param input Token containing JSON data
 * @param output Token to store the formatted JSON
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result)) result_t json_format(const token_t* input, token_t* output) {
    if (token_validate(input) != RESULT_OK || token_validate(output) != RESULT_OK) {
        return RESULT_ERR;
    }

    // First validate the input JSON
    if (json_validate(input) != RESULT_OK) {
        return RESULT_ERR;
    }

    if (token_clear(output) != RESULT_OK) {
        return RESULT_ERR;
    }

    // Simple formatting: add newlines and basic indentation
    int indent_level = 0;
    int in_string = 0;
    int escape_next = 0;
    
    for (size_t i = 0; i < input->size; i++) {
        char c = input->data[i];
        
        if (escape_next) {
            // Previous character was escape, so this one is escaped
            char temp_str[2] = {c, '\0'};
            if (token_append(output, temp_str) != RESULT_OK) {
                return RESULT_ERR;
            }
            escape_next = 0;
            continue;
        }
        
        if (c == '\\' && in_string) {
            escape_next = 1;
            char temp_str[2] = {c, '\0'};
            if (token_append(output, temp_str) != RESULT_OK) {
                return RESULT_ERR;
            }
            continue;
        }
        
        if (c == '"' && !escape_next) {
            in_string = !in_string;
        }
        
        if (!in_string) {
            if (c == '{' || c == '[') {
                char temp_str[2] = {c, '\0'};
                if (token_append(output, temp_str) != RESULT_OK ||
                    token_append(output, "\n") != RESULT_OK) {
                    return RESULT_ERR;
                }
                indent_level++;
                
                // Add indentation for next line
                for (int j = 0; j < indent_level * 2; j++) {
                    if (token_append(output, " ") != RESULT_OK) {
                        return RESULT_ERR;
                    }
                }
                continue;
            } else if (c == '}' || c == ']') {
                if (token_append(output, "\n") != RESULT_OK) {
                    return RESULT_ERR;
                }
                indent_level--;
                
                // Add indentation
                for (int j = 0; j < indent_level * 2; j++) {
                    if (token_append(output, " ") != RESULT_OK) {
                        return RESULT_ERR;
                    }
                }
                
                char temp_str[2] = {c, '\0'};
                if (token_append(output, temp_str) != RESULT_OK) {
                    return RESULT_ERR;
                }
                continue;
            } else if (c == ',') {
                char temp_str[2] = {c, '\0'};
                if (token_append(output, temp_str) != RESULT_OK ||
                    token_append(output, "\n") != RESULT_OK) {
                    return RESULT_ERR;
                }
                
                // Add indentation for next line
                for (int j = 0; j < indent_level * 2; j++) {
                    if (token_append(output, " ") != RESULT_OK) {
                        return RESULT_ERR;
                    }
                }
                continue;
            } else if (c == ':') {
                if (token_append(output, ": ") != RESULT_OK) {
                    return RESULT_ERR;
                }
                continue;
            }
        }
        
        // Regular character
        char temp_str[2] = {c, '\0'};
        if (token_append(output, temp_str) != RESULT_OK) {
            return RESULT_ERR;
        }
    }
    
    return RESULT_OK;
}
