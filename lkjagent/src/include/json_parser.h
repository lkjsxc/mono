/**
 * @file json_parser.h
 * @brief JSON parsing interface for LKJAgent
 * 
 * This header provides a lightweight JSON parser interface designed for
 * parsing configuration files and memory storage formats. The parser
 * handles basic JSON structures with robust error handling.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#ifndef LKJAGENT_JSON_PARSER_H
#define LKJAGENT_JSON_PARSER_H

#include "types.h"
#include "data.h"

/**
 * @defgroup JSON_Parser JSON Parsing Operations
 * @{
 */

/**
 * @brief JSON value types
 */
typedef enum {
    JSON_TYPE_NULL = 0,
    JSON_TYPE_BOOLEAN,
    JSON_TYPE_NUMBER,
    JSON_TYPE_STRING,
    JSON_TYPE_ARRAY,
    JSON_TYPE_OBJECT
} json_type_t;

/**
 * @brief JSON value structure
 */
typedef struct {
    json_type_t type;
    union {
        bool boolean_value;
        double number_value;
        data_t string_value;
    } value;
} json_value_t;

/**
 * @brief Parse JSON object from string
 * 
 * Parses a JSON object from the input string and validates its structure.
 * This is the main entry point for JSON parsing operations.
 * 
 * @param json_string Input JSON string to parse
 * @param parsed_object Data buffer to store parsed object representation
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note The parsed object is stored as a formatted string representation
 * @note Nested objects and arrays are supported
 * @note Invalid JSON syntax will result in parsing failure
 * @note The output buffer is cleared before parsing
 * 
 * @warning json_string parameter must not be NULL
 * @warning parsed_object parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t parsed;
 * data_init(&parsed, 1024);
 * const char* json = "{\"key\": \"value\", \"number\": 42}";
 * if (json_parse_object(json, &parsed) == RESULT_OK) {
 *     printf("Parsed JSON: %s\n", parsed.data);
 * }
 * data_destroy(&parsed);
 * @endcode
 */
result_t json_parse_object(const char* json_string, data_t* parsed_object) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Parse JSON array from string
 * 
 * Parses a JSON array from the input string and validates its structure.
 * Handles arrays containing mixed value types.
 * 
 * @param json_string Input JSON string containing an array
 * @param parsed_array Data buffer to store parsed array representation
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note The parsed array is stored as a formatted string representation
 * @note Nested arrays and objects within the array are supported
 * @note Empty arrays are valid and parse successfully
 * @note The output buffer is cleared before parsing
 * 
 * @warning json_string parameter must not be NULL
 * @warning parsed_array parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t parsed;
 * data_init(&parsed, 512);
 * const char* json = "[\"item1\", 42, true, null]";
 * if (json_parse_array(json, &parsed) == RESULT_OK) {
 *     printf("Parsed array: %s\n", parsed.data);
 * }
 * data_destroy(&parsed);
 * @endcode
 */
result_t json_parse_array(const char* json_string, data_t* parsed_array) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Parse and extract string value from JSON
 * 
 * Parses a JSON string value and extracts the unescaped string content.
 * Handles JSON string escaping properly.
 * 
 * @param json_string Input JSON string value (including quotes)
 * @param output Data buffer to store unescaped string value
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Input should be a complete JSON string including surrounding quotes
 * @note JSON escape sequences are properly unescaped (\\n, \\", \\\\, etc.)
 * @note Unicode escape sequences (\\uXXXX) are supported
 * @note The output buffer is cleared before extracting the value
 * 
 * @warning json_string parameter must not be NULL
 * @warning output parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t result;
 * data_init(&result, 256);
 * const char* json_str = "\"Hello\\nWorld\"";
 * if (json_parse_string(json_str, &result) == RESULT_OK) {
 *     printf("Unescaped: %s\n", result.data); // "Hello\nWorld"
 * }
 * data_destroy(&result);
 * @endcode
 */
result_t json_parse_string(const char* json_string, data_t* output) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Parse and extract number value from JSON
 * 
 * Parses a JSON number value and converts it to a double precision value.
 * Handles both integer and floating-point numbers.
 * 
 * @param json_string Input JSON number string
 * @param output Pointer to store parsed number value
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Supports both integer and floating-point number formats
 * @note Scientific notation (e.g., 1.23e-4) is supported
 * @note Range validation is performed to detect overflow
 * @note Leading/trailing whitespace is ignored
 * 
 * @warning json_string parameter must not be NULL
 * @warning output parameter must not be NULL
 * 
 * Example usage:
 * @code
 * double value;
 * if (json_parse_number("42.5", &value) == RESULT_OK) {
 *     printf("Parsed number: %f\n", value); // 42.500000
 * }
 * 
 * if (json_parse_number("1.23e-4", &value) == RESULT_OK) {
 *     printf("Scientific: %f\n", value); // 0.000123
 * }
 * @endcode
 */
result_t json_parse_number(const char* json_string, double* output) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Parse and extract boolean value from JSON
 * 
 * Parses a JSON boolean value (true or false) and converts it to a C boolean.
 * 
 * @param json_string Input JSON boolean string
 * @param output Pointer to store parsed boolean value
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Accepts exactly "true" or "false" (case-sensitive)
 * @note Leading/trailing whitespace is ignored
 * @note Any other string results in parsing failure
 * 
 * @warning json_string parameter must not be NULL
 * @warning output parameter must not be NULL
 * 
 * Example usage:
 * @code
 * bool value;
 * if (json_parse_boolean("true", &value) == RESULT_OK) {
 *     printf("Boolean value: %s\n", value ? "true" : "false");
 * }
 * @endcode
 */
result_t json_parse_boolean(const char* json_string, bool* output) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Find and extract value for a key in JSON object
 * 
 * Searches for a specific key in a JSON object and extracts its value.
 * The value is returned as a string that can be further parsed.
 * 
 * @param json_object JSON object string to search
 * @param key Key name to search for
 * @param value Data buffer to store the found value
 * @return RESULT_OK on success, RESULT_ERR if key not found or parsing error
 * 
 * @note The key search is case-sensitive
 * @note The returned value includes the complete JSON value (with quotes for strings)
 * @note Nested object/array values are returned as complete JSON strings
 * @note The output buffer is cleared before storing the found value
 * 
 * @warning json_object parameter must not be NULL
 * @warning key parameter must not be NULL
 * @warning value parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t value;
 * data_init(&value, 256);
 * const char* json = "{\"name\": \"John\", \"age\": 30}";
 * if (json_find_key(json, "name", &value) == RESULT_OK) {
 *     printf("Found value: %s\n", value.data); // "John"
 * }
 * data_destroy(&value);
 * @endcode
 */
result_t json_find_key(const char* json_object, const char* key, data_t* value) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Validate JSON structure without full parsing
 * 
 * Performs structural validation of JSON input to check for syntax errors
 * without the overhead of full parsing. Useful for input validation.
 * 
 * @param json_string JSON string to validate
 * @return RESULT_OK if valid JSON, RESULT_ERR if invalid
 * 
 * @note Checks for: balanced brackets/braces, proper quoting, valid syntax
 * @note Does not validate semantic correctness, only syntactic structure
 * @note More efficient than full parsing for validation-only use cases
 * @note Supports all JSON value types: null, boolean, number, string, array, object
 * 
 * @warning json_string parameter must not be NULL
 * 
 * Example usage:
 * @code
 * const char* json1 = "{\"valid\": true}";
 * const char* json2 = "{\"invalid\": }";
 * 
 * if (json_validate_structure(json1) == RESULT_OK) {
 *     printf("JSON1 is valid\n");
 * }
 * 
 * if (json_validate_structure(json2) != RESULT_OK) {
 *     printf("JSON2 is invalid\n");
 * }
 * @endcode
 */
result_t json_validate_structure(const char* json_string) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Parse memory.json unified storage format
 * 
 * Specialized parser for the memory.json format used in LKJAgent unified
 * memory storage. Extracts working and disk memory layers.
 * 
 * @param json_content JSON content from memory.json file
 * @param working_memory Data buffer to store working memory content
 * @param disk_memory Data buffer to store disk memory content
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Expected format: {"working_memory": "...", "disk_memory": "..."}
 * @note Both memory layers are optional and may be empty
 * @note Content strings are unescaped and ready for use
 * @note Output buffers are cleared before storing extracted content
 * 
 * @warning json_content parameter must not be NULL
 * @warning working_memory parameter must not be NULL and must be initialized
 * @warning disk_memory parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t working, disk;
 * data_init(&working, 1024);
 * data_init(&disk, 1024);
 * 
 * const char* memory_json = "{\"working_memory\": \"Current context\", \"disk_memory\": \"Archived data\"}";
 * if (json_parse_memory_format(memory_json, &working, &disk) == RESULT_OK) {
 *     printf("Working: %s\n", working.data);
 *     printf("Disk: %s\n", disk.data);
 * }
 * 
 * data_destroy(&working);
 * data_destroy(&disk);
 * @endcode
 */
result_t json_parse_memory_format(const char* json_content, data_t* working_memory, data_t* disk_memory) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Parse context_keys.json directory format
 * 
 * Specialized parser for the context_keys.json format used to store
 * context key metadata and directory information.
 * 
 * @param json_content JSON content from context_keys.json file
 * @param context_keys Array to store parsed context key structures
 * @param max_keys Maximum number of context keys to parse
 * @param parsed_count Pointer to store number of keys actually parsed
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Expected format: array of objects with key metadata
 * @note Each object should contain: key, layer, importance_score, last_accessed
 * @note Invalid or incomplete entries are skipped with warnings
 * @note The context_keys array is filled with valid context_key_t structures
 * 
 * @warning json_content parameter must not be NULL
 * @warning context_keys parameter must not be NULL
 * @warning parsed_count parameter must not be NULL
 * @warning max_keys must be greater than 0
 * 
 * Example usage:
 * @code
 * context_key_t keys[10];
 * size_t count;
 * const char* keys_json = "[{\"key\": \"user_data\", \"layer\": 0, \"importance_score\": 85}]";
 * if (json_parse_context_keys_format(keys_json, keys, 10, &count) == RESULT_OK) {
 *     printf("Parsed %zu context keys\n", count);
 * }
 * @endcode
 */
result_t json_parse_context_keys_format(const char* json_content, context_key_t* context_keys, size_t max_keys, size_t* parsed_count) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 4)));

/** @} */

#endif /* LKJAGENT_JSON_PARSER_H */
