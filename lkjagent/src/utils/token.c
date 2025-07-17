/**
 * @file token.c
 * @brief Safe string token management implementation
 *
 * This module provides safe string handling using bounded buffers to prevent
 * buffer overflows. All string operations are performed with capacity validation
 * and explicit error handling.
 *
 * Key features:
 * - Stack-based allocation only (no malloc/free)
 * - Comprehensive parameter validation
 * - Bounded buffer operations
 * - Null termination guarantee
 * - Memory-efficient design
 */

#include "../lkjagent.h"

/**
 * @brief Initialize a token with a static buffer
 *
 * Sets up a token structure with the provided buffer, ensuring it's properly
 * initialized and ready for safe string operations.
 *
 * @param token Pointer to token structure to initialize
 * @param buffer Character buffer to use for storage
 * @param capacity Maximum buffer size (minimum 2 bytes for null termination)
 * @return RESULT_OK on success, RESULT_ERR on invalid parameters
 */
__attribute__((warn_unused_result))
result_t token_init(token_t* token, char* buffer, size_t capacity) {
    if (!token) {
        RETURN_ERR("token_init: NULL token pointer");
        return RESULT_ERR;
    }
    
    if (!buffer) {
        RETURN_ERR("token_init: NULL buffer pointer");
        return RESULT_ERR;
    }
    
    if (capacity < 2) {
        RETURN_ERR("token_init: Buffer capacity too small (minimum 2 bytes)");
        return RESULT_ERR;
    }
    
    token->data = buffer;
    token->capacity = capacity;
    token->size = 0;
    
    // Clear buffer and ensure null termination
    memset(buffer, 0, capacity);
    buffer[0] = '\0';
    
    return RESULT_OK;
}

/**
 * @brief Set token content from a C string
 *
 * Safely copies the provided string into the token's buffer, ensuring
 * null termination and capacity bounds are respected.
 *
 * @param token Target token to set
 * @param str Source null-terminated string
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t token_set(token_t* token, const char* str) {
    if (!token) {
        RETURN_ERR("token_set: NULL token pointer");
        return RESULT_ERR;
    }
    
    if (!token->data) {
        RETURN_ERR("token_set: Token not initialized");
        return RESULT_ERR;
    }
    
    if (!str) {
        RETURN_ERR("token_set: NULL string pointer");
        return RESULT_ERR;
    }
    
    size_t str_len = strlen(str);
    
    // Check if string fits (reserve space for null terminator)
    if (str_len >= token->capacity) {
        RETURN_ERR("token_set: String too long for token capacity");
        return RESULT_ERR;
    }
    
    // Copy string and ensure null termination
    strncpy(token->data, str, token->capacity - 1);
    token->data[token->capacity - 1] = '\0';
    token->size = str_len;
    
    return RESULT_OK;
}

/**
 * @brief Set token content from buffer with specified length
 *
 * Copies exactly the specified number of bytes from the source buffer,
 * handling binary data correctly while maintaining null termination.
 *
 * @param token Target token
 * @param buffer Source buffer
 * @param length Number of bytes to copy
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t token_set_length(token_t* token, const char* buffer, size_t length) {
    if (!token) {
        RETURN_ERR("token_set_length: NULL token pointer");
        return RESULT_ERR;
    }
    
    if (!token->data) {
        RETURN_ERR("token_set_length: Token not initialized");
        return RESULT_ERR;
    }
    
    if (!buffer) {
        RETURN_ERR("token_set_length: NULL buffer pointer");
        return RESULT_ERR;
    }
    
    // Check if data fits (reserve space for null terminator)
    if (length >= token->capacity) {
        RETURN_ERR("token_set_length: Data too long for token capacity");
        return RESULT_ERR;
    }
    
    // Copy data and ensure null termination
    memcpy(token->data, buffer, length);
    token->data[length] = '\0';
    token->size = length;
    
    return RESULT_OK;
}

/**
 * @brief Append string to existing token content
 *
 * Safely appends the provided string to the current token content,
 * ensuring capacity bounds and null termination are maintained.
 *
 * @param token Target token to append to
 * @param str String to append
 * @return RESULT_OK on success, RESULT_ERR on insufficient space
 */
__attribute__((warn_unused_result))
result_t token_append(token_t* token, const char* str) {
    if (!token) {
        RETURN_ERR("token_append: NULL token pointer");
        return RESULT_ERR;
    }
    
    if (!token->data) {
        RETURN_ERR("token_append: Token not initialized");
        return RESULT_ERR;
    }
    
    if (!str) {
        RETURN_ERR("token_append: NULL string pointer");
        return RESULT_ERR;
    }
    
    size_t str_len = strlen(str);
    size_t new_size = token->size + str_len;
    
    // Check if combined string fits
    if (new_size >= token->capacity) {
        RETURN_ERR("token_append: Insufficient space to append string");
        return RESULT_ERR;
    }
    
    // Append string and update size
    strncpy(token->data + token->size, str, token->capacity - token->size - 1);
    token->data[new_size] = '\0';
    token->size = new_size;
    
    return RESULT_OK;
}

/**
 * @brief Copy content from source token to destination token
 *
 * Performs a safe copy operation between tokens, handling capacity
 * constraints and ensuring proper null termination.
 *
 * @param dest Destination token
 * @param src Source token
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t token_copy(token_t* dest, const token_t* src) {
    if (!dest) {
        RETURN_ERR("token_copy: NULL destination token");
        return RESULT_ERR;
    }
    
    if (!src) {
        RETURN_ERR("token_copy: NULL source token");
        return RESULT_ERR;
    }
    
    if (!dest->data || !src->data) {
        RETURN_ERR("token_copy: Uninitialized token");
        return RESULT_ERR;
    }
    
    // Check if source content fits in destination
    if (src->size >= dest->capacity) {
        RETURN_ERR("token_copy: Source token too large for destination");
        return RESULT_ERR;
    }
    
    // Copy content
    memcpy(dest->data, src->data, src->size);
    dest->data[src->size] = '\0';
    dest->size = src->size;
    
    return RESULT_OK;
}

/**
 * @brief Compare two tokens for equality
 *
 * Performs byte-by-byte comparison of token contents.
 *
 * @param token1 First token to compare
 * @param token2 Second token to compare
 * @return 1 if tokens are equal, 0 if not equal or error
 */
int token_equals(const token_t* token1, const token_t* token2) {
    if (!token1 || !token2) {
        return 0;
    }
    
    if (!token1->data || !token2->data) {
        return 0;
    }
    
    if (token1->size != token2->size) {
        return 0;
    }
    
    return memcmp(token1->data, token2->data, token1->size) == 0;
}

/**
 * @brief Compare token with C string for equality
 *
 * Compares token content with a null-terminated string.
 *
 * @param token Token to compare
 * @param str String to compare with
 * @return 1 if token equals string, 0 if not equal or error
 */
int token_equals_str(const token_t* token, const char* str) {
    if (!token || !str) {
        return 0;
    }
    
    if (!token->data) {
        return 0;
    }
    
    size_t str_len = strlen(str);
    if (token->size != str_len) {
        return 0;
    }
    
    return strncmp(token->data, str, str_len) == 0;
}

/**
 * @brief Find substring in token content
 *
 * Searches for the first occurrence of a substring within the token.
 *
 * @param token Token to search in
 * @param needle String to find
 * @param position Pointer to store found position
 * @return RESULT_OK if found, RESULT_ERR if not found or error
 */
__attribute__((warn_unused_result))
result_t token_find(const token_t* token, const char* needle, size_t* position) {
    if (!token || !needle || !position) {
        RETURN_ERR("token_find: NULL parameter");
        return RESULT_ERR;
    }
    
    if (!token->data) {
        RETURN_ERR("token_find: Token not initialized");
        return RESULT_ERR;
    }
    
    char* found = strstr(token->data, needle);
    if (!found) {
        RETURN_ERR("token_find: Substring not found");
        return RESULT_ERR;
    }
    
    *position = found - token->data;
    return RESULT_OK;
}

/**
 * @brief Extract substring from token
 *
 * Creates a substring from the specified range within the source token.
 *
 * @param token Source token
 * @param start Starting position
 * @param length Number of characters to extract
 * @param dest Destination token for substring
 * @return RESULT_OK on success, RESULT_ERR on invalid range or error
 */
__attribute__((warn_unused_result))
result_t token_substring(const token_t* token, size_t start, size_t length, token_t* dest) {
    if (!token || !dest) {
        RETURN_ERR("token_substring: NULL parameter");
        return RESULT_ERR;
    }
    
    if (!token->data || !dest->data) {
        RETURN_ERR("token_substring: Uninitialized token");
        return RESULT_ERR;
    }
    
    // Validate range
    if (start >= token->size) {
        RETURN_ERR("token_substring: Start position beyond token size");
        return RESULT_ERR;
    }
    
    if (start + length > token->size) {
        RETURN_ERR("token_substring: Length extends beyond token");
        return RESULT_ERR;
    }
    
    // Check destination capacity
    if (length >= dest->capacity) {
        RETURN_ERR("token_substring: Destination token too small");
        return RESULT_ERR;
    }
    
    // Extract substring
    memcpy(dest->data, token->data + start, length);
    dest->data[length] = '\0';
    dest->size = length;
    
    return RESULT_OK;
}

/**
 * @brief Clear token content
 *
 * Resets the token to empty state while preserving its buffer allocation.
 *
 * @param token Token to clear
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result))
result_t token_clear(token_t* token) {
    if (!token) {
        RETURN_ERR("token_clear: NULL token pointer");
        return RESULT_ERR;
    }
    
    if (!token->data) {
        RETURN_ERR("token_clear: Token not initialized");
        return RESULT_ERR;
    }
    
    token->data[0] = '\0';
    token->size = 0;
    
    return RESULT_OK;
}

/**
 * @brief Check if token is empty
 *
 * @param token Token to check
 * @return 1 if empty, 0 if not empty or error
 */
int token_is_empty(const token_t* token) {
    if (!token || !token->data) {
        return 1;
    }
    
    return token->size == 0;
}

/**
 * @brief Get remaining capacity in token
 *
 * @param token Token to check
 * @return Number of characters that can still be added (excluding null terminator)
 */
size_t token_remaining_capacity(const token_t* token) {
    if (!token || !token->data) {
        return 0;
    }
    
    return token->capacity - token->size - 1; // -1 for null terminator
}
