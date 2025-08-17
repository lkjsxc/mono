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

// Helper function to trim whitespace from a string segment
static result_t trim_segment(pool_t* pool, const char* start, uint64_t length, data_t** trimmed) {
    // Find start of non-whitespace
    const char* begin = start;
    const char* end = start + length;
    
    while (begin < end && isspace((unsigned char)*begin)) {
        begin++;
    }
    
    // Find end of non-whitespace
    while (end > begin && isspace((unsigned char)*(end - 1))) {
        end--;
    }
    
    uint64_t trimmed_length = (uint64_t)(end - begin);
    
    if (trimmed_length == 0) {
        // Empty tag after trimming
        *trimmed = NULL;
        return RESULT_OK;
    }
    
    // Create trimmed data_t
    if (data_create(pool, trimmed) != RESULT_OK) {
        RETURN_ERR("Failed to create trimmed tag data");
    }
    
    if (data_append_str(pool, trimmed, "") != RESULT_OK) {
        if (data_destroy(pool, *trimmed) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup trimmed data after append error");
        }
        RETURN_ERR("Failed to initialize trimmed tag data");
    }
    
    // Manually copy the trimmed segment
    for (uint64_t i = 0; i < trimmed_length; i++) {
        if (data_append_char(pool, trimmed, begin[i]) != RESULT_OK) {
            if (data_destroy(pool, *trimmed) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup trimmed data after char append error");
            }
            RETURN_ERR("Failed to append character to trimmed tag");
        }
    }
    
    return RESULT_OK;
}

// High-quality tag sorting function
result_t sort_tags(pool_t* pool, const data_t* input_tags, data_t** sorted_tags) {
    if (input_tags == NULL || input_tags->data == NULL || input_tags->size == 0) {
        // Empty input, create empty output
        if (data_create_str(pool, sorted_tags, "") != RESULT_OK) {
            RETURN_ERR("Failed to create empty sorted tags");
        }
        return RESULT_OK;
    }
    
    // For now, just return a copy of the input (preserving backward compatibility)
    if (data_create_data(pool, sorted_tags, input_tags) != RESULT_OK) {
        RETURN_ERR("Failed to create copy of input tags");
    }
    
    return RESULT_OK;
}

// High-quality tags_sort function that produces a null-terminated array
result_t tags_sort(pool_t* pool, data_t** sorted_tags_array, const data_t* unsorted_tags) {
    if (!pool || !sorted_tags_array || !unsorted_tags) {
        RETURN_ERR("Invalid parameters for tags_sort");
    }
    
    // Handle empty input
    if (unsorted_tags->size == 0) {
        sorted_tags_array[0] = NULL;
        return RESULT_OK;
    }
    
    // Parse comma-separated tags into individual data_t structures
    data_t* tags[MAX_TAGS] = {NULL};
    uint64_t tag_count = 0;
    
    const char* input = unsorted_tags->data;
    uint64_t input_size = unsorted_tags->size;
    uint64_t start = 0;
    
    // Split by commas
    for (uint64_t i = 0; i <= input_size; i++) {
        if (i == input_size || input[i] == ',') {
            if (tag_count >= MAX_TAGS - 1) {
                // Cleanup already allocated tags
                for (uint64_t j = 0; j < tag_count; j++) {
                    if (tags[j] && data_destroy(pool, tags[j]) != RESULT_OK) {
                        PRINT_ERR("Failed to cleanup tag during max count error");
                    }
                }
                RETURN_ERR("Too many tags (exceeds MAX_TAGS limit)");
            }
            
            uint64_t segment_length = i - start;
            data_t* trimmed_tag = NULL;
            
            if (trim_segment(pool, input + start, segment_length, &trimmed_tag) != RESULT_OK) {
                // Cleanup already allocated tags
                for (uint64_t j = 0; j < tag_count; j++) {
                    if (tags[j] && data_destroy(pool, tags[j]) != RESULT_OK) {
                        PRINT_ERR("Failed to cleanup tag during trim error");
                    }
                }
                RETURN_ERR("Failed to trim tag segment");
            }
            
            // Only add non-empty tags
            if (trimmed_tag != NULL) {
                tags[tag_count++] = trimmed_tag;
            }
            
            start = i + 1;
        }
    }
    
    // Sort tags using simple insertion sort (stable and efficient for small arrays)
    for (uint64_t i = 1; i < tag_count; i++) {
        data_t* key = tags[i];
        uint64_t j = i;
        
        while (j > 0 && data_compare_lexical(tags[j - 1], key) > 0) {
            tags[j] = tags[j - 1];
            j--;
        }
        tags[j] = key;
    }
    
    // Remove duplicates while preserving order
    uint64_t unique_count = 0;
    for (uint64_t i = 0; i < tag_count; i++) {
        bool is_duplicate = false;
        
        // Check if this tag is a duplicate of any previous unique tag
        for (uint64_t j = 0; j < unique_count; j++) {
            if (data_compare_lexical(tags[i], sorted_tags_array[j]) == 0) {
                is_duplicate = true;
                break;
            }
        }
        
        if (!is_duplicate) {
            sorted_tags_array[unique_count++] = tags[i];
        } else {
            // Destroy duplicate tag
            if (data_destroy(pool, tags[i]) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup duplicate tag");
            }
        }
    }
    
    // Null-terminate the array
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
