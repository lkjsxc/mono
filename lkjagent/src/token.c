#include "lkjagent.h"

/**
 * Initialize a token with the given buffer and capacity
 * @param token Pointer to the token to initialize
 * @param buffer Pointer to the character buffer
 * @param capacity Maximum capacity of the buffer
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result)) result_t token_init(token_t* token, char* buffer, size_t capacity) {
    if (!token || !buffer || capacity < 2) {  // Need at least 2 bytes for content + null terminator
        return RESULT_ERR;
    }
    
    token->data = buffer;
    token->size = 0;
    token->capacity = capacity;
    
    // Clear the buffer to ensure it's in a clean state
    memset(buffer, 0, capacity);
    
    return RESULT_OK;
}

/**
 * Clear the token's content
 * @param token Pointer to the token to clear
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result)) result_t token_clear(token_t* token) {
    if (token_validate(token) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    token->size = 0;
    token->data[0] = '\0';
    
    return RESULT_OK;
}

/**
 * Set the token's content from a C string
 * @param token Pointer to the token to set
 * @param str Source string to copy from
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result)) result_t token_set(token_t* token, const char* str) {
    if (token_validate(token) != RESULT_OK || !str) {
        return RESULT_ERR;
    }
    
    size_t len = strlen(str);
    return token_set_length(token, str, len);
}

/**
 * Set the token's content from a buffer with specified length
 * @param token Pointer to the token to set
 * @param buffer Source buffer to copy from
 * @param length Number of bytes to copy
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result)) result_t token_set_length(token_t* token, const char* buffer, size_t length) {
    if (token_validate(token) != RESULT_OK || !buffer) {
        return RESULT_ERR;
    }
    
    if (length >= token->capacity) {
        return RESULT_ERR; // Not enough space (need room for null terminator)
    }
    
    // Use memmove for safety in case of overlapping memory
    memmove(token->data, buffer, length);
    token->data[length] = '\0';
    token->size = length;
    
    return RESULT_OK;
}

/**
 * Append a string to the token's content
 * @param token Pointer to the token to append to
 * @param str String to append
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result)) result_t token_append(token_t* token, const char* str) {
    if (!str) {
        return RESULT_ERR;
    }
    
    size_t str_len = strlen(str);
    return token_append_length(token, str, str_len);
}

/**
 * Append a buffer with specified length to the token's content
 * @param token Pointer to the token to append to
 * @param buffer Buffer to append
 * @param length Number of bytes to append
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result)) result_t token_append_length(token_t* token, const char* buffer, size_t length) {
    if (token_validate(token) != RESULT_OK || !buffer || length == 0) {
        return RESULT_ERR;
    }
    
    if (token->size + length >= token->capacity) {
        return RESULT_ERR; // Not enough space (need room for null terminator)
    }
    
    memcpy(token->data + token->size, buffer, length);
    token->size += length;
    token->data[token->size] = '\0';
    
    return RESULT_OK;
}

/**
 * Copy content from one token to another
 * @param dest Destination token
 * @param src Source token
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result)) result_t token_copy(token_t* dest, const token_t* src) {
    if (token_validate(dest) != RESULT_OK || token_validate(src) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    return token_set_length(dest, src->data, src->size);
}

/**
 * Compare two tokens for equality
 * @param token1 First token to compare
 * @param token2 Second token to compare
 * @return 1 if equal, 0 if not equal or error
 */
int token_equals(const token_t* token1, const token_t* token2) {
    if (token_validate(token1) != RESULT_OK || token_validate(token2) != RESULT_OK) {
        return 0;
    }
    
    if (token1->size != token2->size) {
        return 0;
    }
    
    return memcmp(token1->data, token2->data, token1->size) == 0;
}

/**
 * Compare a token with a C string for equality
 * @param token Token to compare
 * @param str String to compare with
 * @return 1 if equal, 0 if not equal or error
 */
int token_equals_str(const token_t* token, const char* str) {
    if (token_validate(token) != RESULT_OK || !str) {
        return 0;
    }
    
    size_t str_len = strlen(str);
    if (token->size != str_len) {
        return 0;
    }
    
    return memcmp(token->data, str, str_len) == 0;
}

/**
 * Check if a token is empty
 * @param token Token to check
 * @return 1 if empty, 0 if not empty or error
 */
int token_is_empty(const token_t* token) {
    if (token_validate(token) != RESULT_OK) {
        return 1; // Consider invalid tokens as empty
    }
    
    return (token->size == 0);
}

/**
 * Get the remaining capacity of a token
 * @param token Token to check
 * @return Available capacity or -1 on error
 */
int token_available_space(const token_t* token) {
    if (token_validate(token) != RESULT_OK) {
        return -1;
    }
    
    return (int)(token->capacity - token->size - 1); // -1 for null terminator
}

/**
 * Validate that a token is in a consistent state
 * @param token Token to validate
 * @return RESULT_OK if valid, RESULT_ERR if invalid
 */
__attribute__((warn_unused_result)) result_t token_validate(const token_t* token) {
    if (!token || !token->data) {
        return RESULT_ERR;
    }
    
    if (token->capacity <= 0) {
        return RESULT_ERR;
    }
    
    if (token->size >= token->capacity) {
        return RESULT_ERR;
    }
    
    // Check null termination
    if (token->data[token->size] != '\0') {
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

/**
 * Find the first occurrence of a substring in a token
 * @param token Token to search in
 * @param needle Substring to search for
 * @param position Pointer to store the position of the found substring (0-based)
 * @return RESULT_OK if found, RESULT_ERR if not found or error
 */
__attribute__((warn_unused_result)) result_t token_find(const token_t* token, const char* needle, size_t* position) {
    if (token_validate(token) != RESULT_OK || !needle || !position) {
        return RESULT_ERR;
    }
    
    if (token_is_empty(token)) {
        return RESULT_ERR;
    }
    
    size_t needle_len = strlen(needle);
    if (needle_len == 0 || needle_len > token->size) {
        return RESULT_ERR;
    }
    
    for (size_t i = 0; i <= token->size - needle_len; i++) {
        if (memcmp(token->data + i, needle, needle_len) == 0) {
            *position = i;
            return RESULT_OK;
        }
    }
    
    return RESULT_ERR; // Not found
}

/**
 * Extract a substring from a token
 * @param token Source token
 * @param start Starting position (0-based)
 * @param length Length of substring to extract
 * @param dest Destination token to store the substring
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result)) result_t token_substring(const token_t* token, size_t start, size_t length, token_t* dest) {
    if (token_validate(token) != RESULT_OK || token_validate(dest) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (start >= token->size || length == 0) {
        return RESULT_ERR;
    }
    
    // Adjust length if it extends beyond the token
    if (start + length > token->size) {
        length = token->size - start;
    }
    
    return token_set_length(dest, token->data + start, length);
}

/**
 * Remove leading and trailing whitespace from a token
 * @param token Token to trim
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result)) result_t token_trim(token_t* token) {
    if (token_validate(token) != RESULT_OK || token_is_empty(token)) {
        return RESULT_OK; // Nothing to trim
    }
    
    // Find first non-whitespace character
    size_t start = 0;
    while (start < token->size && (token->data[start] == ' ' || 
                                   token->data[start] == '\t' || 
                                   token->data[start] == '\n' || 
                                   token->data[start] == '\r')) {
        start++;
    }
    
    // If all characters are whitespace
    if (start == token->size) {
        return token_clear(token);
    }
    
    // Find last non-whitespace character
    size_t end = token->size - 1;
    while (end > start && (token->data[end] == ' ' || 
                           token->data[end] == '\t' || 
                           token->data[end] == '\n' || 
                           token->data[end] == '\r')) {
        end--;
    }
    
    // Calculate new length
    size_t new_length = end - start + 1;
    
    // Move trimmed content to the beginning if needed
    if (start > 0) {
        memmove(token->data, token->data + start, new_length);
    }
    
    token->data[new_length] = '\0';
    token->size = new_length;
    
    return RESULT_OK;
}