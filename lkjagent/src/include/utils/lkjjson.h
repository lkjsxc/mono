#ifndef LKJAGENT_LKJJSON_H
#define LKJAGENT_LKJJSON_H

#include "macro.h"
#include "std.h"
#include "types.h"
#include "utils/lkjstring.h"
#include "utils/lkjpool.h"

/**
 * Parse JSON from a string
 * @param pool Memory pool for allocations
 * @param json_string String containing JSON data
 * @param value Pointer to store the parsed JSON value
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t json_parse(pool_t* pool, const string_t* json_string, json_value_t** value);

/**
 * Serialize JSON value to string
 * @param pool Memory pool for allocations
 * @param value JSON value to serialize
 * @param output String to store the serialized JSON
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t json_stringify(pool_t* pool, const json_value_t* value, string_t** output);

/**
 * Create a new JSON null value
 * @param pool Memory pool for allocations
 * @param value Pointer to store the new value
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t json_create_null(pool_t* pool, json_value_t** value);

/**
 * Create a new JSON boolean value
 * @param pool Memory pool for allocations
 * @param bool_val Boolean value
 * @param value Pointer to store the new value
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t json_create_bool(pool_t* pool, int bool_val, json_value_t** value);

/**
 * Create a new JSON number value
 * @param pool Memory pool for allocations
 * @param number_val Number value
 * @param value Pointer to store the new value
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t json_create_number(pool_t* pool, double number_val, json_value_t** value);

/**
 * Create a new JSON string value
 * @param pool Memory pool for allocations
 * @param string_val String value
 * @param value Pointer to store the new value
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t json_create_string(pool_t* pool, const char* string_val, json_value_t** value);

/**
 * Create a new JSON object
 * @param pool Memory pool for allocations
 * @param value Pointer to store the new value
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t json_create_object(pool_t* pool, json_value_t** value);

/**
 * Create a new JSON array
 * @param pool Memory pool for allocations
 * @param value Pointer to store the new value
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t json_create_array(pool_t* pool, json_value_t** value);

/**
 * Add a key-value pair to a JSON object
 * @param pool Memory pool for allocations
 * @param object JSON object value
 * @param key Key string
 * @param value Value to add
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t json_object_set(pool_t* pool, json_value_t* object, const char* key, json_value_t* value);

/**
 * Add a string value to a JSON object (convenience function)
 * @param pool Memory pool for allocations
 * @param object JSON object value
 * @param key Key string
 * @param string_val String value to add
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t json_object_set_string(pool_t* pool, json_value_t* object, const char* key, const char* string_val);

/**
 * Get a value from a JSON object by key
 * @param object JSON object value
 * @param key Key string
 * @return Pointer to the value, or NULL if not found
 */
json_value_t* json_object_get(const json_value_t* object, const char* key);

/**
 * Add a value to a JSON array
 * @param pool Memory pool for allocations
 * @param array JSON array value
 * @param value Value to add
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t json_array_append(pool_t* pool, json_value_t* array, json_value_t* value);

/**
 * Get a value from a JSON array by index
 * @param array JSON array value
 * @param index Array index
 * @return Pointer to the value, or NULL if index is out of bounds
 */
json_value_t* json_array_get(const json_value_t* array, uint64_t index);

/**
 * Get the length of a JSON array
 * @param array JSON array value
 * @return Array length, or 0 if not an array
 */
uint64_t json_array_length(const json_value_t* array);

/**
 * Get the length of a JSON object
 * @param object JSON object value
 * @return Object length, or 0 if not an object
 */
uint64_t json_object_length(const json_value_t* object);

/**
 * Delete (free) a JSON value and all its nested content
 * @param pool Memory pool for deallocations
 * @param value JSON value to delete
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t json_delete(pool_t* pool, json_value_t* value);

/**
 * Remove and delete a key-value pair from a JSON object
 * @param pool Memory pool for deallocations
 * @param object JSON object value
 * @param key Key string to remove
 * @return RESULT_OK on success, RESULT_ERR on error (including key not found)
 */
__attribute__((warn_unused_result)) result_t json_object_remove(pool_t* pool, json_value_t* object, const char* key);

/**
 * Remove and delete a value from a JSON array at the specified index
 * @param pool Memory pool for deallocations
 * @param array JSON array value
 * @param index Array index to remove
 * @return RESULT_OK on success, RESULT_ERR on error (including index out of bounds)
 */
__attribute__((warn_unused_result)) result_t json_array_remove(pool_t* pool, json_value_t* array, uint64_t index);

/**
 * Deep copy a JSON value, recursively copying all nested objects and arrays.
 * 
 * @param pool The memory pool.
 * @param src The source JSON value to copy.
 * @param dst Pointer to store the copied JSON value.
 * @return RESULT_OK on success, or an error code on failure.
 */
__attribute__((warn_unused_result)) result_t json_deep_copy(pool_t* pool, const json_value_t* src, json_value_t** dst);

#endif
