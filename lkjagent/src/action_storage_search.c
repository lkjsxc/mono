#include "lkjagent.h"

// Helper function to check if search tags are a subset of entry tags (all search tags must be present)
static bool tags_subset_match(const data_t* search_tags, const data_t* entry_tags) {
    if (!search_tags || !entry_tags || search_tags->size == 0) {
        return true; // Empty search matches everything
    }
    
    if (entry_tags->size == 0) {
        return false; // Non-empty search can't match empty entry tags
    }
    
    // Parse search tags by commas
    const char* search_str = search_tags->data;
    uint64_t search_len = search_tags->size;
    const char* entry_str = entry_tags->data;
    uint64_t entry_len = entry_tags->size;
    
    uint64_t search_start = 0;
    
    // Check each search tag to see if it exists in entry tags
    for (uint64_t i = 0; i <= search_len; i++) {
        if (i == search_len || search_str[i] == ',') {
            if (i > search_start) {
                // Extract current search tag (trimming whitespace)
                uint64_t tag_start = search_start;
                uint64_t tag_end = i;
                
                // Trim leading whitespace
                while (tag_start < tag_end && (search_str[tag_start] == ' ' || search_str[tag_start] == '\t')) {
                    tag_start++;
                }
                
                // Trim trailing whitespace
                while (tag_end > tag_start && (search_str[tag_end - 1] == ' ' || search_str[tag_end - 1] == '\t')) {
                    tag_end--;
                }
                
                if (tag_start < tag_end) {
                    uint64_t tag_len = tag_end - tag_start;
                    bool found = false;
                    
                    // Search for this tag in entry tags
                    uint64_t entry_pos = 0;
                    while (entry_pos <= entry_len) {
                        if (entry_pos == entry_len || entry_str[entry_pos] == ',') {
                            // Found end of entry tag, compare if lengths and content match
                            uint64_t entry_tag_start = 0;
                            if (entry_pos > 0) {
                                // Find start of current entry tag after previous comma
                                for (int64_t j = entry_pos - 1; j >= 0; j--) {
                                    if (entry_str[j] == ',') {
                                        entry_tag_start = j + 1;
                                        break;
                                    }
                                }
                            }
                            
                            // Trim whitespace from entry tag
                            while (entry_tag_start < entry_pos && (entry_str[entry_tag_start] == ' ' || entry_str[entry_tag_start] == '\t')) {
                                entry_tag_start++;
                            }
                            uint64_t entry_tag_end = entry_pos;
                            while (entry_tag_end > entry_tag_start && (entry_str[entry_tag_end - 1] == ' ' || entry_str[entry_tag_end - 1] == '\t')) {
                                entry_tag_end--;
                            }
                            
                            uint64_t entry_tag_len = entry_tag_end - entry_tag_start;
                            
                            // Compare tag lengths and content
                            if (tag_len == entry_tag_len && 
                                memcmp(search_str + tag_start, entry_str + entry_tag_start, tag_len) == 0) {
                                found = true;
                                break;
                            }
                        }
                        entry_pos++;
                    }
                    
                    if (!found) {
                        return false; // This search tag was not found in entry tags
                    }
                }
            }
            search_start = i + 1;
        }
    }
    
    return true; // All search tags were found
}

// Helper function to check if value contains the search value as a substring (case-insensitive)
static bool value_contains_search(const data_t* search_value, const data_t* entry_value) {
    if (!search_value || search_value->size == 0) {
        return true; // Empty search matches everything
    }
    
    if (!entry_value || entry_value->size == 0) {
        return false; // Non-empty search can't match empty value
    }
    
    if (search_value->size > entry_value->size) {
        return false; // Search string longer than entry value
    }
    
    // Case-insensitive substring search
    for (uint64_t i = 0; i <= entry_value->size - search_value->size; i++) {
        bool match = true;
        for (uint64_t j = 0; j < search_value->size; j++) {
            char search_char = search_value->data[j];
            char entry_char = entry_value->data[i + j];
            
            // Convert to lowercase for comparison
            if (search_char >= 'A' && search_char <= 'Z') {
                search_char = search_char - 'A' + 'a';
            }
            if (entry_char >= 'A' && entry_char <= 'Z') {
                entry_char = entry_char - 'A' + 'a';
            }
            
            if (search_char != entry_char) {
                match = false;
                break;
            }
        }
        
        if (match) {
            return true;
        }
    }
    
    return false;
}

// Action handler for storage_search - searches storage for entries matching tags and/or value criteria
result_t lkjagent_action_storage_search(pool_t* pool, lkjagent_t* lkjagent, data_t* tags, data_t* value, uint64_t iteration) {
    if (pool == NULL || lkjagent == NULL) {
        RETURN_ERR("Invalid parameters (pool or lkjagent is NULL)");
    }
    
    // Get storage object from memory
    object_t* storage = NULL;
    if (object_provide_str(&storage, lkjagent->memory, "storage") != RESULT_OK) {
        // No storage exists, save this information to working memory
        data_t* no_storage_tags = NULL;
        data_t* no_storage_value = NULL;
        
        if (data_create_str(pool, &no_storage_tags, "search_results,summary") == RESULT_OK &&
            data_create_str(pool, &no_storage_value, "found 0 matches - no storage exists") == RESULT_OK) {
            if (lkjagent_action_working_memory_add(pool, lkjagent, no_storage_tags, no_storage_value, iteration) != RESULT_OK) {
                PRINT_ERR("Failed to save no-storage message to working memory");
            }
        }
        
        if (no_storage_tags && data_destroy(pool, no_storage_tags) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup no_storage_tags");
        }
        if (no_storage_value && data_destroy(pool, no_storage_value) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup no_storage_value");
        }
        
        return RESULT_OK;
    }
    
    if (!storage->child) {
        // Storage exists but is empty, save this information to working memory
        data_t* empty_storage_tags = NULL;
        data_t* empty_storage_value = NULL;
        
        if (data_create_str(pool, &empty_storage_tags, "search_results,summary") == RESULT_OK &&
            data_create_str(pool, &empty_storage_value, "found 0 matches - storage is empty") == RESULT_OK) {
            if (lkjagent_action_working_memory_add(pool, lkjagent, empty_storage_tags, empty_storage_value, iteration) != RESULT_OK) {
                PRINT_ERR("Failed to save empty-storage message to working memory");
            }
        }
        
        if (empty_storage_tags && data_destroy(pool, empty_storage_tags) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup empty_storage_tags");
        }
        if (empty_storage_value && data_destroy(pool, empty_storage_value) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup empty_storage_value");
        }
        
        return RESULT_OK;
    }
    
    // Search through all storage entries (key-value pairs)
    object_t* current = storage->child;
    uint64_t matches_found = 0;
    while (current != NULL) {
        if (current->data && current->data->data && current->child && current->child->data) {
            // current->data is the key: "tags,iteration_N"
            // current->child->data is the value
            data_t* key = current->data;
            data_t* val = current->child->data;

            // Find the last comma (before iteration_N)
            int64_t last_comma = -1;
            for (int64_t i = key->size - 1; i >= 0; i--) {
                if (key->data[i] == ',') {
                    last_comma = i;
                    break;
                }
            }
            if (last_comma <= 0) {
                current = current->next;
                continue; // Malformed key
            }

            // Extract tags (everything before last comma)
            data_t* entry_tags = NULL;
            if (data_create(pool, &entry_tags) != RESULT_OK) {
                current = current->next;
                continue;
            }
            for (int64_t i = 0; i < last_comma; i++) {
                if (data_append_char(pool, &entry_tags, key->data[i]) != RESULT_OK) {
                    if(data_destroy(pool, entry_tags) != RESULT_OK){
                        PRINT_ERR("Failed to cleanup entry_tags");
                    }
                    entry_tags = NULL;
                    break;
                }
            }
            if (!entry_tags) {
                current = current->next;
                continue;
            }

            // Match tags and value
            bool tags_match = (tags == NULL || tags->size == 0) ? true : tags_subset_match(tags, entry_tags);
            bool value_match = (value == NULL || value->size == 0) ? true : value_contains_search(value, val);
            if (tags_match && value_match) {
                if (lkjagent_action_working_memory_add(pool, lkjagent, entry_tags, val, iteration) == RESULT_OK) {
                    matches_found++;
                } else {
                    PRINT_ERR("Failed to add search result to working memory");
                }
            }
            if (entry_tags && data_destroy(pool, entry_tags) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup entry_tags during search iteration");
            }
        }
        current = current->next;
    }
    
    // Search completed successfully - save search results summary to working memory
    data_t* search_summary_tags = NULL;
    data_t* search_summary_value = NULL;
    
    // Create tags for the search summary entry
    if (data_create_str(pool, &search_summary_tags, "search_results,summary") != RESULT_OK) {
        PRINT_ERR("Failed to create search summary tags");
        return RESULT_OK; // Don't fail the entire search if we can't save summary
    }
    
    // Create the search summary value with details about the search operation
    if (data_create(pool, &search_summary_value) != RESULT_OK) {
        if (data_destroy(pool, search_summary_tags) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup search_summary_tags after value creation error");
        }
        PRINT_ERR("Failed to create search summary value");
        return RESULT_OK; // Don't fail the entire search if we can't save summary
    }
    
    // Build search summary: "found X matches for tags:[tags] value:[value]" or "no matches found for..."
    char matches_buffer[64];
    if (matches_found == 0) {
        snprintf(matches_buffer, sizeof(matches_buffer), "no matches found for ");
    } else {
        snprintf(matches_buffer, sizeof(matches_buffer), "found %lu matches for ", matches_found);
    }
    
    if (data_append_str(pool, &search_summary_value, matches_buffer) != RESULT_OK) {
        if (data_destroy(pool, search_summary_tags) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup search_summary_tags after append error");
        }
        if (data_destroy(pool, search_summary_value) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup search_summary_value after append error");
        }
        PRINT_ERR("Failed to append matches count to search summary");
        return RESULT_OK;
    }
    
    if (data_append_str(pool, &search_summary_value, "tags:[") != RESULT_OK) {
        if (data_destroy(pool, search_summary_tags) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup search_summary_tags after append error");
        }
        if (data_destroy(pool, search_summary_value) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup search_summary_value after append error");
        }
        PRINT_ERR("Failed to append tags prefix to search summary");
        return RESULT_OK;
    }
    
    if (tags && tags->size > 0) {
        if (data_append_data(pool, &search_summary_value, tags) != RESULT_OK) {
            if (data_destroy(pool, search_summary_tags) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup search_summary_tags after append error");
            }
            if (data_destroy(pool, search_summary_value) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup search_summary_value after append error");
            }
            PRINT_ERR("Failed to append search tags to search summary");
            return RESULT_OK;
        }
    } else {
        if (data_append_str(pool, &search_summary_value, "any") != RESULT_OK) {
            if (data_destroy(pool, search_summary_tags) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup search_summary_tags after append error");
            }
            if (data_destroy(pool, search_summary_value) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup search_summary_value after append error");
            }
            PRINT_ERR("Failed to append 'any' for empty search tags");
            return RESULT_OK;
        }
    }
    
    if (data_append_str(pool, &search_summary_value, "] value:[") != RESULT_OK) {
        if (data_destroy(pool, search_summary_tags) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup search_summary_tags after append error");
        }
        if (data_destroy(pool, search_summary_value) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup search_summary_value after append error");
        }
        PRINT_ERR("Failed to append value prefix to search summary");
        return RESULT_OK;
    }
    
    if (value && value->size > 0) {
        if (data_append_data(pool, &search_summary_value, value) != RESULT_OK) {
            if (data_destroy(pool, search_summary_tags) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup search_summary_tags after append error");
            }
            if (data_destroy(pool, search_summary_value) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup search_summary_value after append error");
            }
            PRINT_ERR("Failed to append search value to search summary");
            return RESULT_OK;
        }
    } else {
        if (data_append_str(pool, &search_summary_value, "any") != RESULT_OK) {
            if (data_destroy(pool, search_summary_tags) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup search_summary_tags after append error");
            }
            if (data_destroy(pool, search_summary_value) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup search_summary_value after append error");
            }
            PRINT_ERR("Failed to append 'any' for empty search value");
            return RESULT_OK;
        }
    }
    
    if (data_append_str(pool, &search_summary_value, "]") != RESULT_OK) {
        if (data_destroy(pool, search_summary_tags) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup search_summary_tags after append error");
        }
        if (data_destroy(pool, search_summary_value) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup search_summary_value after append error");
        }
        PRINT_ERR("Failed to append closing bracket to search summary");
        return RESULT_OK;
    }
    
    // Save the search summary to working memory
    if (lkjagent_action_working_memory_add(pool, lkjagent, search_summary_tags, search_summary_value, iteration) != RESULT_OK) {
        PRINT_ERR("Failed to save search summary to working memory");
    }
    
    // Cleanup search summary data
    if (data_destroy(pool, search_summary_tags) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup search_summary_tags");
    }
    if (data_destroy(pool, search_summary_value) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup search_summary_value");
    }
    
    return RESULT_OK;
}
