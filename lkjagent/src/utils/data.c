/**
 * @file data.c
 * @brief Data buffer management implementation for LKJAgent
 * 
 * This module provides safe, bounds-checked data buffer operations with
 * comprehensive error handling and memory safety guarantees. All functions
 * maintain null termination and prevent buffer overflows.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/data.h"
#include "../lkjagent.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/**
 * @defgroup Data_Internal Internal Data Management Functions
 * @{
 */

/**
 * @brief Internal function to ensure buffer has minimum capacity
 * 
 * @param data Pointer to data_t structure
 * @param min_capacity Minimum required capacity
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t data_ensure_capacity(data_t* data, size_t min_capacity) {
    if (!data || !data->data) {
        RETURN_ERR("Invalid data structure");
        return RESULT_ERR;
    }
    
    if (data->capacity >= min_capacity) {
        return RESULT_OK;
    }
    
    /* Calculate new capacity with growth factor of 1.5 and minimum increment */
    size_t new_capacity = data->capacity;
    if (new_capacity == 0) {
        new_capacity = 64; /* Minimum starting capacity */
    }
    
    while (new_capacity < min_capacity) {
        size_t next_capacity = new_capacity + (new_capacity >> 1); /* 1.5x growth */
        if (next_capacity <= new_capacity) {
            /* Overflow detected */
            RETURN_ERR("Capacity overflow in buffer resize");
            return RESULT_ERR;
        }
        new_capacity = next_capacity;
    }
    
    /* Ensure we don't exceed reasonable limits */
    if (new_capacity > MAX_DATA_SIZE) {
        RETURN_ERR("Buffer size exceeds maximum limit");
        return RESULT_ERR;
    }
    
    char* new_data = realloc(data->data, new_capacity);
    if (!new_data) {
        RETURN_ERR("Memory allocation failed during buffer resize");
        return RESULT_ERR;
    }
    
    data->data = new_data;
    data->capacity = new_capacity;
    
    return RESULT_OK;
}

/**
 * @brief Internal function to find word boundaries for smart trimming
 * 
 * @param text Text to search
 * @param start_pos Starting position
 * @param direction 1 for forward, -1 for backward
 * @return Position of word boundary or start_pos if not found
 */
static size_t find_word_boundary(const char* text, size_t start_pos, int direction) {
    if (!text) return start_pos;
    
    size_t text_len = strlen(text);
    if (start_pos >= text_len) return text_len;
    
    size_t pos = start_pos;
    bool found_word = false;
    
    while (pos < text_len && pos != SIZE_MAX) {
        char c = text[pos];
        bool is_word_char = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
                           (c >= '0' && c <= '9') || c == '_';
        
        if (!is_word_char && found_word) {
            return pos;
        }
        if (is_word_char) {
            found_word = true;
        }
        
        if (direction > 0) {
            pos++;
        } else {
            if (pos == 0) break;
            pos--;
        }
    }
    
    return start_pos;
}

/** @} */

result_t data_init(data_t* data, size_t initial_capacity) {
    if (!data) {
        RETURN_ERR("Null pointer passed to data_init");
        return RESULT_ERR;
    }
    
    if (initial_capacity == 0) {
        RETURN_ERR("Initial capacity must be greater than zero");
        return RESULT_ERR;
    }
    
    if (initial_capacity > MAX_DATA_SIZE) {
        RETURN_ERR("Initial capacity exceeds maximum data size");
        return RESULT_ERR;
    }
    
    /* Ensure minimum capacity for null terminator */
    if (initial_capacity < 1) {
        initial_capacity = 1;
    }
    
    data->data = malloc(initial_capacity);
    if (!data->data) {
        RETURN_ERR("Memory allocation failed in data_init");
        return RESULT_ERR;
    }
    
    data->data[0] = '\0';
    data->size = 0;
    data->capacity = initial_capacity;
    
    return RESULT_OK;
}

result_t data_set(data_t* data, const char* source, size_t max_size) {
    if (!data) {
        RETURN_ERR("Null data pointer passed to data_set");
        return RESULT_ERR;
    }
    
    if (!source) {
        RETURN_ERR("Null source pointer passed to data_set");
        return RESULT_ERR;
    }
    
    if (!data->data) {
        RETURN_ERR("Uninitialized data structure passed to data_set");
        return RESULT_ERR;
    }
    
    size_t source_len = strlen(source);
    
    /* Apply size limit if specified */
    if (max_size > 0 && source_len > max_size) {
        source_len = max_size;
    }
    
    /* Ensure capacity for string plus null terminator */
    if (data_ensure_capacity(data, source_len + 1) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Copy the data */
    if (source_len > 0) {
        memcpy(data->data, source, source_len);
    }
    data->data[source_len] = '\0';
    data->size = source_len;
    
    return RESULT_OK;
}

result_t data_append(data_t* data, const char* source, size_t max_total_size) {
    if (!data) {
        RETURN_ERR("Null data pointer passed to data_append");
        return RESULT_ERR;
    }
    
    if (!source) {
        RETURN_ERR("Null source pointer passed to data_append");
        return RESULT_ERR;
    }
    
    if (!data->data) {
        RETURN_ERR("Uninitialized data structure passed to data_append");
        return RESULT_ERR;
    }
    
    size_t source_len = strlen(source);
    size_t new_total_size = data->size + source_len;
    
    /* Apply total size limit if specified */
    if (max_total_size > 0 && new_total_size > max_total_size) {
        if (max_total_size <= data->size) {
            /* No room for any additional data */
            return RESULT_OK;
        }
        source_len = max_total_size - data->size;
        new_total_size = max_total_size;
    }
    
    /* Ensure capacity for new total size plus null terminator */
    if (data_ensure_capacity(data, new_total_size + 1) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Append the data */
    if (source_len > 0) {
        memcpy(data->data + data->size, source, source_len);
    }
    data->data[new_total_size] = '\0';
    data->size = new_total_size;
    
    return RESULT_OK;
}

result_t data_trim_front(data_t* data, size_t chars_to_remove) {
    if (!data) {
        RETURN_ERR("Null data pointer passed to data_trim_front");
        return RESULT_ERR;
    }
    
    if (!data->data) {
        RETURN_ERR("Uninitialized data structure passed to data_trim_front");
        return RESULT_ERR;
    }
    
    if (chars_to_remove == 0) {
        return RESULT_OK;
    }
    
    if (chars_to_remove >= data->size) {
        /* Remove all content */
        data->data[0] = '\0';
        data->size = 0;
        return RESULT_OK;
    }
    
    /* Shift remaining content to front */
    size_t remaining_size = data->size - chars_to_remove;
    memmove(data->data, data->data + chars_to_remove, remaining_size);
    data->data[remaining_size] = '\0';
    data->size = remaining_size;
    
    return RESULT_OK;
}

result_t data_trim_context(data_t* data, size_t max_context_size, size_t preserve_suffix_size) {
    if (!data) {
        RETURN_ERR("Null data pointer passed to data_trim_context");
        return RESULT_ERR;
    }
    
    if (!data->data) {
        RETURN_ERR("Uninitialized data structure passed to data_trim_context");
        return RESULT_ERR;
    }
    
    if (max_context_size == 0) {
        RETURN_ERR("Maximum context size must be greater than zero");
        return RESULT_ERR;
    }
    
    if (preserve_suffix_size >= max_context_size) {
        RETURN_ERR("Preserve suffix size must be less than maximum context size");
        return RESULT_ERR;
    }
    
    if (data->size <= max_context_size) {
        /* No trimming needed */
        return RESULT_OK;
    }
    
    if (preserve_suffix_size == 0) {
        /* Smart trimming: preserve beginning and end, remove middle */
        size_t prefix_size = max_context_size / 2;
        size_t suffix_size = max_context_size - prefix_size;
        
        /* Find good break points at word boundaries */
        prefix_size = find_word_boundary(data->data, prefix_size, -1);
        size_t suffix_start = data->size - suffix_size;
        suffix_start = find_word_boundary(data->data, suffix_start, 1);
        
        /* Ensure we don't exceed max_context_size */
        suffix_size = data->size - suffix_start;
        if (prefix_size + suffix_size > max_context_size) {
            if (prefix_size > suffix_size) {
                prefix_size = max_context_size - suffix_size;
            } else {
                suffix_size = max_context_size - prefix_size;
                suffix_start = data->size - suffix_size;
            }
        }
        
        /* Copy suffix after prefix */
        if (suffix_size > 0 && suffix_start < data->size) {
            memmove(data->data + prefix_size, data->data + suffix_start, suffix_size);
        }
        
        size_t new_size = prefix_size + suffix_size;
        data->data[new_size] = '\0';
        data->size = new_size;
    } else {
        /* Simple trimming: preserve specified suffix */
        if (preserve_suffix_size > data->size) {
            preserve_suffix_size = data->size;
        }
        
        size_t prefix_size = max_context_size - preserve_suffix_size;
        size_t suffix_start = data->size - preserve_suffix_size;
        
        /* Copy suffix after prefix */
        if (preserve_suffix_size > 0) {
            memmove(data->data + prefix_size, data->data + suffix_start, preserve_suffix_size);
        }
        
        data->data[max_context_size] = '\0';
        data->size = max_context_size;
    }
    
    return RESULT_OK;
}

result_t data_clear(data_t* data) {
    if (!data) {
        RETURN_ERR("Null data pointer passed to data_clear");
        return RESULT_ERR;
    }
    
    if (!data->data) {
        RETURN_ERR("Uninitialized data structure passed to data_clear");
        return RESULT_ERR;
    }
    
    data->data[0] = '\0';
    data->size = 0;
    
    return RESULT_OK;
}

result_t data_copy(data_t* dest, const data_t* source) {
    if (!dest) {
        RETURN_ERR("Null destination pointer passed to data_copy");
        return RESULT_ERR;
    }
    
    if (!source) {
        RETURN_ERR("Null source pointer passed to data_copy");
        return RESULT_ERR;
    }
    
    if (!source->data) {
        RETURN_ERR("Uninitialized source structure passed to data_copy");
        return RESULT_ERR;
    }
    
    /* Initialize destination with optimal capacity */
    size_t copy_capacity = source->size > 0 ? source->size + 1 : 64;
    if (data_init(dest, copy_capacity) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Copy the content */
    if (source->size > 0) {
        memcpy(dest->data, source->data, source->size);
    }
    dest->data[source->size] = '\0';
    dest->size = source->size;
    
    return RESULT_OK;
}

result_t data_validate(const data_t* data) {
    if (!data) {
        RETURN_ERR("Null data pointer passed to data_validate");
        return RESULT_ERR;
    }
    
    if (!data->data) {
        RETURN_ERR("Data structure has null data pointer");
        return RESULT_ERR;
    }
    
    if (data->size > data->capacity) {
        RETURN_ERR("Data size exceeds capacity");
        return RESULT_ERR;
    }
    
    if (data->capacity == 0) {
        RETURN_ERR("Data capacity is zero but data pointer is not null");
        return RESULT_ERR;
    }
    
    if (data->capacity > MAX_DATA_SIZE) {
        RETURN_ERR("Data capacity exceeds maximum allowed size");
        return RESULT_ERR;
    }
    
    /* Check null termination if size > 0 or capacity allows */
    if (data->size < data->capacity && data->data[data->size] != '\0') {
        RETURN_ERR("Data buffer is not null-terminated");
        return RESULT_ERR;
    }
    
    /* Basic memory accessibility check */
    volatile char test_read = data->data[0];
    (void)test_read; /* Suppress unused variable warning */
    
    if (data->capacity > 1) {
        volatile char test_read_end = data->data[data->capacity - 1];
        (void)test_read_end;
    }
    
    return RESULT_OK;
}

void data_destroy(data_t* data) {
    if (!data) {
        return;
    }
    
    if (data->data) {
        free(data->data);
    }
    
    data->data = NULL;
    data->size = 0;
    data->capacity = 0;
}
