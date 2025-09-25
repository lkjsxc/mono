#include "lkjagent.h"
#include <string.h>
#include <ctype.h>

// Helper function to compare two data_t strings lexicographically
static int32_t data_compare_lexical(const data_t* a, const data_t* b) {
    if (!a || !b) {
        if (!a && !b) return 0;
        return a ? 1 : -1;
    }
    
    uint64_t min_size = a->size < b->size ? a->size : b->size;
    int result = memcmp(a->data, b->data, min_size);
    
    if (result != 0) {
        return result;
    }
    
    // If the compared parts are equal, the shorter string comes first
    if (a->size < b->size) return -1;
    if (a->size > b->size) return 1;
    return 0;
}

// **OPTIMIZED TAG TRIMMING** - Fast string trimming without excessive allocations
static result_t trim_segment(pool_t* pool, const char* start, uint64_t length, data_t** trimmed) {
    // Find trimmed boundaries efficiently
    const char* begin = start;
    const char* end = start + length;
    
    // Trim leading whitespace
    while (begin < end && (*begin == ' ' || *begin == '\t' || *begin == '\n' || *begin == '\r')) {
        begin++;
    }
    
    // Trim trailing whitespace  
    while (end > begin && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\n' || end[-1] == '\r')) {
        end--;
    }
    
    uint64_t trimmed_length = (uint64_t)(end - begin);
    
    if (trimmed_length == 0) {
        *trimmed = NULL; // Empty tag after trimming
        return RESULT_OK;
    }
    
    // Create trimmed data efficiently (single allocation + copy)
    if (data_create(pool, trimmed) != RESULT_OK) {
        RETURN_ERR("Failed to create trimmed tag data");
    }
    
    // Reserve capacity to avoid reallocations
    if ((*trimmed)->capacity < trimmed_length && 
        pool_data_realloc(pool, trimmed, trimmed_length) != RESULT_OK) {
        if (data_destroy(pool, *trimmed) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup trimmed data after realloc error");
        }
        RETURN_ERR("Failed to allocate capacity for trimmed tag");
    }
    
    // Fast copy using memcpy
    memcpy((*trimmed)->data, begin, trimmed_length);
    (*trimmed)->size = trimmed_length;
    
    return RESULT_OK;
}

// **ULTRA-OPTIMIZED TAG SORTING** - Streamlined parsing, sorting, and deduplication
result_t tags_sort(pool_t* pool, data_t** sorted_tags_array, const data_t* unsorted_tags) {
    if (!pool || !sorted_tags_array || !unsorted_tags) {
        RETURN_ERR("Invalid parameters for tags_sort");
    }
    
    // Initialize output array
    sorted_tags_array[0] = NULL;
    
    if (unsorted_tags->size == 0) {
        return RESULT_OK; // Empty input, empty output
    }
    
    // **EFFICIENT TAG PARSING** - Single pass with minimal allocations
    data_t* tags[MAX_TAGS] = {NULL};
    uint64_t tag_count = 0;
    
    const char* input = unsorted_tags->data;
    uint64_t input_size = unsorted_tags->size;
    uint64_t start = 0;
    
    for (uint64_t i = 0; i <= input_size; i++) {
        if (i == input_size || input[i] == ',') {
            if (tag_count >= MAX_TAGS - 1) {
                // Cleanup and return error
                for (uint64_t j = 0; j < tag_count; j++) {
                    if (tags[j] && data_destroy(pool, tags[j]) != RESULT_OK) {
                        PRINT_ERR("Failed to cleanup tag during max count error");
                    }
                }
                RETURN_ERR("Too many tags (exceeds MAX_TAGS limit)");
            }
            
            if (i > start) {
                data_t* trimmed_tag = NULL;
                if (trim_segment(pool, input + start, i - start, &trimmed_tag) == RESULT_OK && trimmed_tag) {
                    tags[tag_count++] = trimmed_tag;
                }
            }
            
            start = i + 1;
        }
    }
    
    if (tag_count == 0) {
        return RESULT_OK; // No valid tags found
    }
    
    // **OPTIMIZED SORTING** - Insertion sort with early termination for pre-sorted data
    for (uint64_t i = 1; i < tag_count; i++) {
        data_t* key = tags[i];
        uint64_t j = i;
        
        // Early termination if already in correct position
        if (data_compare_lexical(tags[i-1], key) <= 0) {
            continue;
        }
        
        while (j > 0 && data_compare_lexical(tags[j - 1], key) > 0) {
            tags[j] = tags[j - 1];
            j--;
        }
        tags[j] = key;
    }
    
    // **EFFICIENT DEDUPLICATION** - Single pass with in-place removal
    uint64_t unique_count = 1; // First element is always unique
    sorted_tags_array[0] = tags[0];
    
    for (uint64_t i = 1; i < tag_count; i++) {
        // Check if different from previous unique tag
        if (data_compare_lexical(sorted_tags_array[unique_count - 1], tags[i]) != 0) {
            sorted_tags_array[unique_count++] = tags[i];
        } else {
            // Duplicate - destroy it
            if (data_destroy(pool, tags[i]) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup duplicate tag");
            }
        }
    }
    
    // Null-terminate the result
    sorted_tags_array[unique_count] = NULL;
    
    return RESULT_OK;
}

// Helper function to convert sorted tag array back to comma-separated string
result_t tags_array_to_string(pool_t* pool, data_t** tags_array, data_t** output) {
    if (!pool || !tags_array || !output) {
        RETURN_ERR("Invalid parameters for tags_array_to_string");
    }
    
    if (data_create(pool, output) != RESULT_OK) {
        RETURN_ERR("Failed to create output data for tag string");
    }
    
    bool first = true;
    for (uint64_t i = 0; tags_array[i] != NULL; i++) {
        if (!first) {
            if (data_append_char(pool, output, ',') != RESULT_OK) {
                if (data_destroy(pool, *output) != RESULT_OK) {
                    PRINT_ERR("Failed to cleanup output during comma append error");
                }
                RETURN_ERR("Failed to append comma separator");
            }
        }
        first = false;
        
        if (data_append_data(pool, output, tags_array[i]) != RESULT_OK) {
            if (data_destroy(pool, *output) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup output during tag append error");
            }
            RETURN_ERR("Failed to append tag to output string");
        }
    }
    
    return RESULT_OK;
}
