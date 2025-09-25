#define _GNU_SOURCE
#include "lkjagent.h"

// Safe memmem implementation in case it's not available
static void* safe_memmem(const void* haystack, size_t haystack_len, const void* needle, size_t needle_len) {
    if (!haystack || !needle || needle_len == 0 || haystack_len < needle_len) {
        return NULL;
    }
    
    const char* h = (const char*)haystack;
    const char* n = (const char*)needle;
    
    for (size_t i = 0; i <= haystack_len - needle_len; i++) {
        if (memcmp(h + i, n, needle_len) == 0) {
            return (void*)(h + i);
        }
    }
    
    return NULL;
}

static __attribute__((warn_unused_result)) result_t extract_content_from_llm_response(pool_t* pool, const data_t* json_response, data_t** content) {
    object_t* response_obj = NULL;
    object_t* content_obj = NULL;
    static int missing_content_logged = 0;

    // Parse the JSON response
    if (object_parse_json(pool, &response_obj, json_response) != RESULT_OK) {
        RETURN_ERR("Failed to parse LLM response JSON");
    }

    // Try primary path: chat completion format
    if (object_provide_str(&content_obj, response_obj, "choices.0.message.content") != RESULT_OK) {
        // Fallback: legacy completion format
        if (object_provide_str(&content_obj, response_obj, "choices.0.text") != RESULT_OK) {
            // Additional fallback: some providers return delta streaming with accumulated text
            if (object_provide_str(&content_obj, response_obj, "choices.0.delta.content") != RESULT_OK) {
                // Final fallback: try a generic top-level content field
                if (object_provide_str(&content_obj, response_obj, "content") != RESULT_OK) {
                    if (!missing_content_logged) {
                        missing_content_logged = 1;
                        const char prefix[] = "LLM response missing expected fields. Payload snippet: ";
                        write(STDERR_FILENO, prefix, sizeof(prefix) - 1);
                        size_t snippet_len = json_response->size < 512 ? (size_t)json_response->size : 512;
                        if (snippet_len > 0) {
                            write(STDERR_FILENO, json_response->data, snippet_len);
                        }
                        write(STDERR_FILENO, "\n", 1);
                    }
                    if (object_destroy(pool, response_obj) != RESULT_OK) {
                        PRINT_ERR("Failed to cleanup response object when content missing");
                    }
                    RETURN_ERR("Failed to get choices array from LLM response");
                }
            }
        }
    }

    if (!content_obj || !content_obj->data) {
        if (object_destroy(pool, response_obj) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup response object when content missing");
        }
        RETURN_ERR("LLM response content field is empty");
    }

    // Check if content contains a <think> tag and extract everything after it
    data_t* processed_content = NULL;
    int64_t think_pos = data_find_str(content_obj->data, "</think>", 0);

    if (think_pos >= 0) {
        // Found </think> tag, extract everything after it
        uint64_t start_pos = think_pos + 8;  // Length of "</think>"

        if (data_create(pool, &processed_content) != RESULT_OK) {
            if (object_destroy(pool, response_obj) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup response object after processed content creation failure");
            }
            RETURN_ERR("Failed to create processed content data");
        }

        // Copy everything after </think> tag
        for (uint64_t i = start_pos; i < content_obj->data->size; i++) {
            if (data_append_char(pool, &processed_content, content_obj->data->data[i]) != RESULT_OK) {
                if (data_destroy(pool, processed_content) != RESULT_OK) {
                    PRINT_ERR("Failed to cleanup processed_content after char append error");
                }
                if (object_destroy(pool, response_obj) != RESULT_OK) {
                    PRINT_ERR("Failed to cleanup response object after char append error");
                }
                RETURN_ERR("Failed to append character to processed content");
            }
        }

        // Use the processed content (everything after </think>)
        if (data_create_data(pool, content, processed_content) != RESULT_OK) {
            if (data_destroy(pool, processed_content) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup processed_content after content copy error");
            }
            if (object_destroy(pool, response_obj) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup response object after content copy error");
            }
            RETURN_ERR("Failed to copy processed content data");
        }

        if (data_destroy(pool, processed_content) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup processed_content");
        }
    } else {
        // No </think> tag found, use original content as-is
        if (data_create_data(pool, content, content_obj->data) != RESULT_OK) {
            if (object_destroy(pool, response_obj) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup response object after content copy error");
            }
            RETURN_ERR("Failed to copy content data from LLM response");
        }
    }

    // Cleanup the parsed JSON object
    if (object_destroy(pool, response_obj) != RESULT_OK) {
        RETURN_ERR("Failed to cleanup response object");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t process_content_next_state(pool_t* pool, lkjagent_t* lkjagent, const object_t* content) {
    object_t* content_next_state = NULL;
    data_t* state_path = NULL;

    if (object_provide_str(&content_next_state, content, "agent.next_state") != RESULT_OK) {
        RETURN_ERR("Failed to get next_state from content object");
    }

    if (data_create_str(pool, &state_path, "state") != RESULT_OK) {
        RETURN_ERR("Failed to create data for next_state path");
    }

    if (object_set_data(pool, lkjagent->memory, state_path, content_next_state->data) != RESULT_OK) {
        RETURN_ERR("Failed to set data for next_state object");
    }

    if (data_destroy(pool, state_path) != RESULT_OK) {
        RETURN_ERR("Failed to cleanup next_state path");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t process_content_action(pool_t* pool, lkjagent_t* lkjagent, const object_t* content, uint64_t iteration) {
    object_t* action_obj = NULL;

    if (object_provide_str(&action_obj, content, "agent.action") != RESULT_OK) {
        RETURN_ERR("Failed to get action from content object");
    }

    if (lkjagent_action(pool, lkjagent, action_obj, iteration) != RESULT_OK) {
        RETURN_ERR("Failed to execute action");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t process_content(pool_t* pool, lkjagent_t* lkjagent, const object_t* content, uint64_t iteration) {
    if (process_content_next_state(pool, lkjagent, content) != RESULT_OK) {
        RETURN_ERR("Failed to process next_state");
    }

    if (process_content_action(pool, lkjagent, content, iteration) != RESULT_OK) {
        RETURN_ERR("Failed to process action");
    }

    return RESULT_OK;
}

// Forward declarations for auto-paging system
static result_t auto_paging_check_and_execute(pool_t* pool, lkjagent_t* lkjagent, uint64_t iteration);

result_t lkjagent_process(pool_t* pool, lkjagent_t* lkjagent, data_t* recv, uint64_t iteration) {
    data_t* content_data = NULL;
    object_t* content_obj = NULL;

    // Extract XML content from the LLM JSON response
    if (extract_content_from_llm_response(pool, recv, &content_data) != RESULT_OK) {
        RETURN_ERR("Failed to extract content from LLM response");
    }

    if (object_parse_xml(pool, &content_obj, content_data) != RESULT_OK) {
        if (data_destroy(pool, content_data) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup content data after parsing error");
        }
        RETURN_ERR("Failed to parse content data");
    }

    // Cleanup the extracted content
    if (data_destroy(pool, content_data) != RESULT_OK) {
        if (object_destroy(pool, content_obj) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup content object after content data cleanup error");
        }
        RETURN_ERR("Failed to cleanup content data");
    }

    // Process the content object
    if (process_content(pool, lkjagent, content_obj, iteration) != RESULT_OK) {
        if (object_destroy(pool, content_obj) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup content object after processing error");
        }
        RETURN_ERR("Failed to process content object");
    }

    // Cleanup the content object
    if (object_destroy(pool, content_obj) != RESULT_OK) {
        RETURN_ERR("Failed to cleanup content object");
    }

    // **INTELLIGENT AUTO-PAGING SYSTEM** - Automatically manage working memory size
    if (auto_paging_check_and_execute(pool, lkjagent, iteration) != RESULT_OK) {
        PRINT_ERR("Warning: Auto-paging failed, continuing execution");
    }

    return RESULT_OK;
}

// **INTELLIGENT AUTO-PAGING SYSTEM** 
// Automatically archives lower-priority working memory items to storage when size exceeds threshold

// Additional forward declarations
static result_t calculate_working_memory_size(pool_t* pool, object_t* working_memory, uint64_t* size);
static result_t identify_archival_candidates(pool_t* pool, object_t* working_memory, uint64_t current_iteration, object_t*** candidates, uint64_t* candidate_count);
static result_t archive_working_memory_item(pool_t* pool, lkjagent_t* lkjagent, object_t* item);
static uint64_t calculate_item_priority(const object_t* item, uint64_t current_iteration);

// Main auto-paging function
static result_t auto_paging_check_and_execute(pool_t* pool, lkjagent_t* lkjagent, uint64_t iteration) {
    // Get paging configuration
    object_t* paging_limit_enable = NULL;
    object_t* paging_limit_value = NULL;
    
    if (object_provide_str(&paging_limit_enable, lkjagent->config, "agent.paging_limit.enable") != RESULT_OK ||
        object_provide_str(&paging_limit_value, lkjagent->config, "agent.paging_limit.value") != RESULT_OK) {
        return RESULT_OK; // Paging disabled or not configured
    }
    
    if (!data_equal_str(paging_limit_enable->data, "true")) {
        return RESULT_OK; // Paging disabled
    }
    
    int64_t paging_threshold;
    if (data_toint(paging_limit_value->data, &paging_threshold) != RESULT_OK || paging_threshold <= 0) {
        return RESULT_OK; // Invalid threshold
    }
    
    // Get working memory
    object_t* working_memory = NULL;
    if (object_provide_str(&working_memory, lkjagent->memory, "working_memory") != RESULT_OK) {
        return RESULT_OK; // No working memory to page
    }
    
    // Calculate current working memory size 
    uint64_t current_size = 0;
    if (calculate_working_memory_size(pool, working_memory, &current_size) != RESULT_OK) {
        return RESULT_OK; // Can't calculate size
    }
    
    // Check if paging is needed
    if (current_size <= (uint64_t)paging_threshold) {
        return RESULT_OK; // Under threshold, no paging needed
    }
    
    // **SMART PAGING**: Identify items to archive based on priority
    object_t** archival_candidates = NULL;
    uint64_t candidate_count = 0;
    
    if (identify_archival_candidates(pool, working_memory, iteration, &archival_candidates, &candidate_count) != RESULT_OK) {
        PRINT_ERR("Failed to identify archival candidates");
        return RESULT_OK; // Continue without paging
    }
    
    // Archive lower-priority items until under threshold
    uint64_t archived_count = 0;
    uint64_t target_size = (uint64_t)(paging_threshold * 0.8); // Target 80% of threshold
    
    for (uint64_t i = 0; i < candidate_count && current_size > target_size; i++) {
        if (archive_working_memory_item(pool, lkjagent, archival_candidates[i]) == RESULT_OK) {
            archived_count++;
            // Recalculate size after archiving
            if (calculate_working_memory_size(pool, working_memory, &current_size) != RESULT_OK) {
                break;
            }
        }
    }
    
    // Set state to thinking after successful paging
    if (archived_count > 0) {
        if (object_set_str(pool, lkjagent->memory, "state", "thinking") != RESULT_OK) {
            PRINT_ERR("Failed to set state after paging");
        }
    }
    
    return RESULT_OK;
}

// Calculate total character count of working memory (JSON representation)
static result_t calculate_working_memory_size(pool_t* pool, object_t* working_memory, uint64_t* size) {
    data_t* memory_data = NULL;
    
    if (object_todata_json(pool, &memory_data, working_memory) != RESULT_OK) {
        RETURN_ERR("Failed to convert working memory to JSON");
    }
    
    *size = memory_data->size;
    
    if (data_destroy(pool, memory_data) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup memory data");
    }
    
    return RESULT_OK;
}

// **SMART PRIORITIZATION**: Identify items for archival based on age, type, and relevance
static result_t identify_archival_candidates(pool_t* pool, object_t* working_memory, uint64_t current_iteration, object_t*** candidates, uint64_t* candidate_count) {
    (void)pool; // Suppress unused parameter warning
    
    // Simple implementation using stack allocation for candidates
    #define MAX_CANDIDATES 32
    static object_t* candidate_array[MAX_CANDIDATES];
    uint64_t actual_candidates = 0;
    
    // Evaluate each item and calculate priority score  
    object_t* current = working_memory->child;
    while (current != NULL && actual_candidates < MAX_CANDIDATES) {
        uint64_t priority = calculate_item_priority(current, current_iteration);
        
        // Lower priority = more likely to be archived (threshold: 50)
        if (priority < 50) {
            candidate_array[actual_candidates] = current;
            actual_candidates++;
        }
        
        current = current->next;
    }
    
    *candidates = candidate_array;
    *candidate_count = actual_candidates;
    
    return RESULT_OK;
}

// **PRIORITY SCORING SYSTEM**: Calculate item priority (0-100, higher = keep in memory)
static uint64_t calculate_item_priority(const object_t* item, uint64_t current_iteration) {
    if (!item || !item->data) {
        return 0; // Invalid item = lowest priority
    }
    
    const char* key_str = item->data->data;
    uint64_t key_len = item->data->size;
    
    // Extract iteration number from key: "tags,iteration_N"
    uint64_t item_iteration = 0;
    bool found_iteration = false;
    
    for (uint64_t i = 0; i + 10 < key_len; i++) {
        if (memcmp(key_str + i, "iteration_", 10) == 0) {
            item_iteration = strtoull(key_str + i + 10, NULL, 10);
            found_iteration = true;
            break;
        }
    }
    
    if (!found_iteration) {
        return 25; // Unknown iteration = medium-low priority
    }
    
    // Calculate priority based on multiple factors
    uint64_t priority = 0;
    
    // **RECENCY FACTOR** (50 points): Recent items have higher priority
    if (current_iteration >= item_iteration) {
        uint64_t age = current_iteration - item_iteration;
        if (age <= 1) {
            priority += 50; // Very recent
        } else if (age <= 5) {
            priority += 30; // Recent
        } else if (age <= 15) {
            priority += 15; // Moderate age
        } else {
            priority += 5;  // Old items
        }
    } else {
        priority += 50; // Future/current iteration
    }
    
    // **CONTENT TYPE FACTOR** (30 points): Important content types get priority
    // Safe string search for non-null-terminated data
    bool has_thinking = (key_len >= 14 && safe_memmem(key_str, key_len, "thinking_notes", 14));
    bool has_evaluation = (key_len >= 16 && safe_memmem(key_str, key_len, "evaluation_notes", 16));
    bool has_search = (key_len >= 14 && safe_memmem(key_str, key_len, "search_results", 14));
    bool has_summary = (key_len >= 7 && safe_memmem(key_str, key_len, "summary", 7));
    
    if (has_thinking || has_evaluation) {
        priority += 30; // High priority content
    } else if (has_search || has_summary) {
        priority += 15; // Medium priority  
    } else {
        priority += 10; // Regular content
    }
    
    // **SIZE FACTOR** (20 points): Smaller items easier to keep
    if (item->child && item->child->data) {
        uint64_t value_size = item->child->data->size;
        if (value_size < 200) {
            priority += 20; // Small items
        } else if (value_size < 800) {
            priority += 10; // Medium items
        } else {
            priority += 0;  // Large items = good for archival
        }
    }
    
    return priority > 100 ? 100 : priority;
}

// Archive a working memory item to storage
static result_t archive_working_memory_item(pool_t* pool, lkjagent_t* lkjagent, object_t* item) {
    if (!item || !item->data || !item->child || !item->child->data) {
        return RESULT_ERR; // Invalid item
    }
    
    // Save to storage
    if (lkjagent_action_storage_save(pool, lkjagent, item->data, item->child->data) != RESULT_OK) {
        return RESULT_ERR; // Failed to archive
    }
    
    // Remove from working memory by clearing the item
    // Note: The actual unlinking happens in working_memory_remove
    return lkjagent_action_working_memory_remove(pool, lkjagent, item->data);
}
