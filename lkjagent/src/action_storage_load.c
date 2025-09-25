#include "lkjagent.h"

// Forward declaration
static bool storage_tags_match(const data_t* search_tags, const data_t* entry_tags);

// **FIXED STORAGE LOAD** - Now works with the correct unified storage format
// Loads data from storage and adds to working memory using efficient matching

result_t lkjagent_action_storage_load(pool_t* pool, lkjagent_t* lkjagent, data_t* tags, uint64_t iteration) {
    // Input validation
    if (!pool || !lkjagent || !tags) {
        RETURN_ERR("Invalid parameters: pool, lkjagent, or tags is NULL");
    }
    
    // Get storage object
    object_t* storage = NULL;
    if (object_provide_str(&storage, lkjagent->memory, "storage") != RESULT_OK) {
        // Storage doesn't exist - that's OK, just return without error
        return RESULT_OK;
    }
    
    if (!storage->child) {
        // Storage is empty - that's OK, just return
        return RESULT_OK;
    }
    
    // **EFFICIENT STORAGE SEARCH** - O(n) single pass through storage entries
    // Storage format: each child has key=tags, value=data (direct key-value pairs)
    object_t* current = storage->child;
    uint32_t matches_loaded = 0;
    
    while (current != NULL) {
        if (current->data && current->child && current->child->data) {
            // current->data = tags (key)
            // current->child->data = value (data)
            
            bool match = storage_tags_match(tags, current->data);
            
            if (match) {
                // Found matching entry - add to working memory
                if (lkjagent_action_working_memory_add(pool, lkjagent, current->data, current->child->data, iteration) != RESULT_OK) {
                    // Don't fail entire operation if one add fails
                    PRINT_ERR("Warning: Failed to add loaded entry to working memory");
                } else {
                    matches_loaded++;
                }
            }
        }
        
        current = current->next;
    }
    
    return RESULT_OK;
}

// **OPTIMIZED TAG MATCHING** - Efficient subset matching for storage operations
// Returns true if search_tags is a subset of entry_tags (all search tags must be present)
static bool storage_tags_match(const data_t* search_tags, const data_t* entry_tags) {
    if (!search_tags || search_tags->size == 0) {
        return true; // Empty search matches everything
    }
    
    if (!entry_tags || entry_tags->size == 0) {
        return false; // Non-empty search can't match empty entry
    }
    
    // Handle exact match case (common optimization)
    if (data_equal_data(search_tags, entry_tags)) {
        return true;
    }
    
    // Parse search tags and check each one exists in entry tags
    const char* search_str = search_tags->data;
    const char* entry_str = entry_tags->data;
    uint64_t search_len = search_tags->size;
    uint64_t entry_len = entry_tags->size;
    
    // Simple but efficient comma-separated tag subset matching
    uint64_t search_pos = 0;
    while (search_pos < search_len) {
        // Find next tag in search string
        uint64_t tag_start = search_pos;
        uint64_t tag_end = search_pos;
        
        // Skip leading whitespace
        while (tag_start < search_len && (search_str[tag_start] == ' ' || search_str[tag_start] == '\t')) {
            tag_start++;
        }
        
        // Find end of tag
        tag_end = tag_start;
        while (tag_end < search_len && search_str[tag_end] != ',') {
            tag_end++;
        }
        
        // Trim trailing whitespace
        while (tag_end > tag_start && (search_str[tag_end - 1] == ' ' || search_str[tag_end - 1] == '\t')) {
            tag_end--;
        }
        
        if (tag_end > tag_start) {
            // Check if this tag exists in entry tags
            uint64_t tag_len = tag_end - tag_start;
            bool found = false;
            
            // Search for tag in entry string
            for (uint64_t i = 0; i <= entry_len; i++) {
                if (i == entry_len || entry_str[i] == ',') {
                    // Found tag boundary, check if it matches
                    uint64_t entry_tag_start = (i == 0) ? 0 : i - tag_len;
                    if (entry_tag_start > 0) {
                        // Find actual start after comma
                        for (uint64_t j = entry_tag_start; j > 0; j--) {
                            if (entry_str[j - 1] == ',') {
                                entry_tag_start = j;
                                break;
                            }
                        }
                    }
                    
                    // Skip whitespace
                    while (entry_tag_start < i && (entry_str[entry_tag_start] == ' ' || entry_str[entry_tag_start] == '\t')) {
                        entry_tag_start++;
                    }
                    
                    uint64_t entry_tag_end = i;
                    while (entry_tag_end > entry_tag_start && (entry_str[entry_tag_end - 1] == ' ' || entry_str[entry_tag_end - 1] == '\t')) {
                        entry_tag_end--;
                    }
                    
                    if (entry_tag_end - entry_tag_start == tag_len && 
                        memcmp(search_str + tag_start, entry_str + entry_tag_start, tag_len) == 0) {
                        found = true;
                        break;
                    }
                }
            }
            
            if (!found) {
                return false; // Search tag not found in entry
            }
        }
        
        // Move to next search tag
        search_pos = (tag_end < search_len) ? tag_end + 1 : search_len;
    }
    
    return true; // All search tags found
}
