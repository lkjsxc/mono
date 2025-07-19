/**
 * @file json_builder.h
 * @brief JSON building interface for LKJAgent
 * 
 * This header provides JSON construction capabilities for generating
 * configuration files and memory storage formats. All operations ensure
 * proper JSON formatting and escaping.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#ifndef LKJAGENT_JSON_BUILDER_H
#define LKJAGENT_JSON_BUILDER_H

#include "types.h"
#include "data.h"

/**
 * @defgroup JSON_Builder JSON Building Operations
 * @{
 */

/**
 * @brief Start building a JSON object
 * 
 * Initializes a JSON object building process. The object starts empty
 * and fields can be added using the json_add_* functions.
 * 
 * @param output Data buffer to store the JSON object being built
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note The output buffer is cleared and initialized with an empty object "{}"
 * @note Fields can be added after calling this function
 * @note The object is properly formatted and ready for use even if no fields are added
 * 
 * @warning output parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t json_obj;
 * data_init(&json_obj, 256);
 * if (json_build_object(&json_obj) == RESULT_OK) {
 *     // Object is now ready for field additions
 *     json_add_string(&json_obj, "name", "John");
 *     json_add_number(&json_obj, "age", 30);
 * }
 * data_destroy(&json_obj);
 * @endcode
 */
result_t json_build_object(data_t* output) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Start building a JSON array
 * 
 * Initializes a JSON array building process. The array starts empty
 * and elements can be added using appropriate functions.
 * 
 * @param output Data buffer to store the JSON array being built
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note The output buffer is cleared and initialized with an empty array "[]"
 * @note Elements can be added after calling this function
 * @note The array is properly formatted and ready for use even if no elements are added
 * 
 * @warning output parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t json_arr;
 * data_init(&json_arr, 256);
 * if (json_build_array(&json_arr) == RESULT_OK) {
 *     // Array is now ready for element additions
 *     // (Element addition functions would be implemented separately)
 * }
 * data_destroy(&json_arr);
 * @endcode
 */
result_t json_build_array(data_t* output) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Add string field to JSON object
 * 
 * Adds a string field to an existing JSON object. The string value is
 * properly escaped and the object structure is maintained.
 * 
 * @param json_object JSON object to modify
 * @param key Field name (key) to add
 * @param value String value to add
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note The key and value are properly JSON-escaped
 * @note If this is not the first field, a comma separator is added automatically
 * @note The key must be a valid string (non-null, non-empty)
 * @note The value can be empty string, null, or any valid string content
 * 
 * @warning json_object parameter must not be NULL and must contain a valid JSON object
 * @warning key parameter must not be NULL and must be non-empty
 * @warning value parameter must not be NULL (use empty string for empty values)
 * 
 * Example usage:
 * @code
 * data_t json_obj;
 * data_init(&json_obj, 256);
 * json_build_object(&json_obj);
 * 
 * json_add_string(&json_obj, "name", "John Doe");
 * json_add_string(&json_obj, "email", "john@example.com");
 * 
 * printf("JSON: %s\n", json_obj.data); // {"name": "John Doe", "email": "john@example.com"}
 * data_destroy(&json_obj);
 * @endcode
 */
result_t json_add_string(data_t* json_object, const char* key, const char* value) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Add number field to JSON object
 * 
 * Adds a numeric field to an existing JSON object. The number is formatted
 * appropriately for JSON representation.
 * 
 * @param json_object JSON object to modify
 * @param key Field name (key) to add
 * @param value Numeric value to add
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Integer values are formatted without decimal points
 * @note Floating-point values are formatted with appropriate precision
 * @note Special values (NaN, infinity) are handled gracefully
 * @note If this is not the first field, a comma separator is added automatically
 * 
 * @warning json_object parameter must not be NULL and must contain a valid JSON object
 * @warning key parameter must not be NULL and must be non-empty
 * 
 * Example usage:
 * @code
 * data_t json_obj;
 * data_init(&json_obj, 256);
 * json_build_object(&json_obj);
 * 
 * json_add_number(&json_obj, "age", 30);
 * json_add_number(&json_obj, "height", 5.9);
 * 
 * printf("JSON: %s\n", json_obj.data); // {"age": 30, "height": 5.9}
 * data_destroy(&json_obj);
 * @endcode
 */
result_t json_add_number(data_t* json_object, const char* key, double value) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Add boolean field to JSON object
 * 
 * Adds a boolean field to an existing JSON object using proper JSON
 * boolean representation (true/false).
 * 
 * @param json_object JSON object to modify
 * @param key Field name (key) to add
 * @param value Boolean value to add
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Boolean values are represented as literal "true" or "false" (no quotes)
 * @note If this is not the first field, a comma separator is added automatically
 * 
 * @warning json_object parameter must not be NULL and must contain a valid JSON object
 * @warning key parameter must not be NULL and must be non-empty
 * 
 * Example usage:
 * @code
 * data_t json_obj;
 * data_init(&json_obj, 256);
 * json_build_object(&json_obj);
 * 
 * json_add_boolean(&json_obj, "active", true);
 * json_add_boolean(&json_obj, "verified", false);
 * 
 * printf("JSON: %s\n", json_obj.data); // {"active": true, "verified": false}
 * data_destroy(&json_obj);
 * @endcode
 */
result_t json_add_boolean(data_t* json_object, const char* key, bool value) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Add null field to JSON object
 * 
 * Adds a null field to an existing JSON object using proper JSON
 * null representation.
 * 
 * @param json_object JSON object to modify
 * @param key Field name (key) to add
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Null values are represented as literal "null" (no quotes)
 * @note If this is not the first field, a comma separator is added automatically
 * 
 * @warning json_object parameter must not be NULL and must contain a valid JSON object
 * @warning key parameter must not be NULL and must be non-empty
 * 
 * Example usage:
 * @code
 * data_t json_obj;
 * data_init(&json_obj, 256);
 * json_build_object(&json_obj);
 * 
 * json_add_string(&json_obj, "name", "John");
 * json_add_null(&json_obj, "middle_name");
 * 
 * printf("JSON: %s\n", json_obj.data); // {"name": "John", "middle_name": null}
 * data_destroy(&json_obj);
 * @endcode
 */
result_t json_add_null(data_t* json_object, const char* key) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Build memory.json unified storage format
 * 
 * Creates a properly formatted memory.json file content with working
 * and disk memory layers in the expected format.
 * 
 * @param working_memory Content for working memory layer
 * @param disk_memory Content for disk memory layer
 * @param output Data buffer to store the generated JSON
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Generated format: {"working_memory": "...", "disk_memory": "..."}
 * @note Memory content is properly JSON-escaped
 * @note Empty memory layers are represented as empty strings
 * @note The output is a complete, valid JSON object ready for file storage
 * 
 * @warning working_memory parameter must not be NULL (can be empty)
 * @warning disk_memory parameter must not be NULL (can be empty)
 * @warning output parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t memory_json;
 * data_init(&memory_json, 1024);
 * 
 * const char* working = "Current user session data";
 * const char* disk = "Historical context and logs";
 * 
 * if (json_build_memory(working, disk, &memory_json) == RESULT_OK) {
 *     printf("Memory JSON: %s\n", memory_json.data);
 *     // Can now save memory_json.data to memory.json file
 * }
 * data_destroy(&memory_json);
 * @endcode
 */
result_t json_build_memory(const char* working_memory, const char* disk_memory, data_t* output) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Build context_keys.json directory format
 * 
 * Creates a properly formatted context_keys.json file content from an
 * array of context key structures.
 * 
 * @param context_keys Array of context key structures to serialize
 * @param key_count Number of context keys in the array
 * @param output Data buffer to store the generated JSON array
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Generated format: array of objects with key metadata
 * @note Each object contains: key, layer, importance_score, last_accessed, data_size
 * @note All fields are properly validated and formatted
 * @note The output is a complete, valid JSON array ready for file storage
 * 
 * @warning context_keys parameter must not be NULL if key_count > 0
 * @warning output parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * context_key_t keys[2];
 * strcpy(keys[0].key, "user_session");
 * keys[0].layer = LAYER_WORKING;
 * keys[0].importance_score = 85;
 * keys[0].last_accessed = time(NULL);
 * keys[0].data_size = 1024;
 * 
 * strcpy(keys[1].key, "historical_data");
 * keys[1].layer = LAYER_DISK;
 * keys[1].importance_score = 60;
 * keys[1].last_accessed = time(NULL) - 3600;
 * keys[1].data_size = 4096;
 * 
 * data_t keys_json;
 * data_init(&keys_json, 512);
 * 
 * if (json_build_context_keys(keys, 2, &keys_json) == RESULT_OK) {
 *     printf("Keys JSON: %s\n", keys_json.data);
 * }
 * data_destroy(&keys_json);
 * @endcode
 */
result_t json_build_context_keys(const context_key_t* context_keys, size_t key_count, data_t* output) __attribute__((warn_unused_result)) __attribute__((nonnull(3)));

/**
 * @brief Build configuration JSON format
 * 
 * Creates a properly formatted configuration JSON from a config structure.
 * This is used for saving configuration to files.
 * 
 * @param config Configuration structure to serialize
 * @param output Data buffer to store the generated JSON
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note All configuration fields are included in the output
 * @note Sensitive information (like API keys) is handled appropriately
 * @note The output is a complete, valid JSON object ready for file storage
 * @note Empty or unset fields are represented appropriately
 * 
 * @warning config parameter must not be NULL
 * @warning output parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * config_t config;
 * // ... initialize config structure ...
 * 
 * data_t config_json;
 * data_init(&config_json, 1024);
 * 
 * if (json_build_config(&config, &config_json) == RESULT_OK) {
 *     printf("Config JSON: %s\n", config_json.data);
 * }
 * data_destroy(&config_json);
 * @endcode
 */
result_t json_build_config(const config_t* config, data_t* output) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Escape string for JSON representation
 * 
 * Properly escapes a string for inclusion in JSON, handling all necessary
 * escape sequences and special characters.
 * 
 * @param input String to escape
 * @param output Data buffer to store escaped string
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Handles all JSON escape sequences: \", \\, \/, \b, \f, \n, \r, \t
 * @note Control characters are properly escaped
 * @note Unicode characters are preserved where possible
 * @note The output does not include surrounding quotes
 * 
 * @warning input parameter must not be NULL
 * @warning output parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t escaped;
 * data_init(&escaped, 256);
 * 
 * const char* raw = "Line 1\nLine 2\tTabbed\nQuote: \"Hello\"";
 * if (json_escape_string(raw, &escaped) == RESULT_OK) {
 *     printf("Escaped: %s\n", escaped.data);
 *     // Output: Line 1\\nLine 2\\tTabbed\\nQuote: \\\"Hello\\\"
 * }
 * data_destroy(&escaped);
 * @endcode
 */
result_t json_escape_string(const char* input, data_t* output) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/** @} */

#endif /* LKJAGENT_JSON_BUILDER_H */
