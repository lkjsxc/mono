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

#endif
