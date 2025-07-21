#ifndef LKJAGENT_POOL_H
#define LKJAGENT_POOL_H

#include "macro.h"
#include "std.h"
#include "types.h"

/**
 * Initialize the pool system
 * @param pool The pool structure to initialize
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t pool_init(pool_t* pool);

/**
 * Allocate a string from the 256-byte pool
 * @param pool The pool structure containing the pools
 * @param string Pointer to store the allocated string
 * @return RESULT_OK on success, RESULT_ERR if pool is exhausted
 */
__attribute__((warn_unused_result)) result_t pool_string256_alloc(pool_t* pool, string_t** string);

/**
 * Free a string back to the 256-byte pool
 * @param pool The pool structure containing the pools
 * @param string The string to return to the pool
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t pool_string256_free(pool_t* pool, string_t* string);

/**
 * Allocate a string from the 4096-byte pool
 * @param pool The pool structure containing the pools
 * @param string Pointer to store the allocated string
 * @return RESULT_OK on success, RESULT_ERR if pool is exhausted
 */
__attribute__((warn_unused_result)) result_t pool_string4096_alloc(pool_t* pool, string_t** string);

/**
 * Free a string back to the 4096-byte pool
 * @param pool The pool structure containing the pools
 * @param string The string to return to the pool
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t pool_string4096_free(pool_t* pool, string_t* string);

/**
 * Allocate a string from the 1048576-byte pool
 * @param pool The pool structure containing the pools
 * @param string Pointer to store the allocated string
 * @return RESULT_OK on success, RESULT_ERR if pool is exhausted
 */
__attribute__((warn_unused_result)) result_t pool_string1048576_alloc(pool_t* pool, string_t** string);

/**
 * Free a string back to the 1048576-byte pool
 * @param pool The pool structure containing the pools
 * @param string The string to return to the pool
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t pool_string1048576_free(pool_t* pool, string_t* string);

/**
 * Allocate a JSON value from the pool
 * @param pool The pool structure containing the pools
 * @param value Pointer to store the allocated value
 * @return RESULT_OK on success, RESULT_ERR if pool is exhausted
 */
__attribute__((warn_unused_result)) result_t pool_json_value_alloc(pool_t* pool, json_value_t** value);

/**
 * Free a JSON value back to the pool
 * @param pool The pool structure containing the pools
 * @param value The value to return to the pool
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t pool_json_value_free(pool_t* pool, json_value_t* value);

/**
 * Allocate a JSON object from the pool
 * @param pool The pool structure containing the pools
 * @param object Pointer to store the allocated object
 * @return RESULT_OK on success, RESULT_ERR if pool is exhausted
 */
__attribute__((warn_unused_result)) result_t pool_json_object_alloc(pool_t* pool, json_object_t** object);

/**
 * Free a JSON object back to the pool
 * @param pool The pool structure containing the pools
 * @param object The object to return to the pool
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t pool_json_object_free(pool_t* pool, json_object_t* object);

/**
 * Allocate a JSON array from the pool
 * @param pool The pool structure containing the pools
 * @param array Pointer to store the allocated array
 * @return RESULT_OK on success, RESULT_ERR if pool is exhausted
 */
__attribute__((warn_unused_result)) result_t pool_json_array_alloc(pool_t* pool, json_array_t** array);

/**
 * Free a JSON array back to the pool
 * @param pool The pool structure containing the pools
 * @param array The array to return to the pool
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t pool_json_array_free(pool_t* pool, json_array_t* array);

/**
 * Allocate a JSON object element from the pool
 * @param pool The pool structure containing the pools
 * @param element Pointer to store the allocated element
 * @return RESULT_OK on success, RESULT_ERR if pool is exhausted
 */
__attribute__((warn_unused_result)) result_t pool_json_object_element_alloc(pool_t* pool, json_object_element_t** element);

/**
 * Free a JSON object element back to the pool
 * @param pool The pool structure containing the pools
 * @param element The element to return to the pool
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t pool_json_object_element_free(pool_t* pool, json_object_element_t* element);

/**
 * Allocate a JSON array element from the pool
 * @param pool The pool structure containing the pools
 * @param element Pointer to store the allocated element
 * @return RESULT_OK on success, RESULT_ERR if pool is exhausted
 */
__attribute__((warn_unused_result)) result_t pool_json_array_element_alloc(pool_t* pool, json_array_element_t** element);

/**
 * Free a JSON array element back to the pool
 * @param pool The pool structure containing the pools
 * @param element The element to return to the pool
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t pool_json_array_element_free(pool_t* pool, json_array_element_t* element);

#endif
