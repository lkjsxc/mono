/**
 * @file context_window.c
 * @brief Context window management implementation for LKJAgent
 * 
 * This module implements intelligent context window management with
 * dynamic sizing, priority-based content selection, and LLM-optimized
 * context preparation for the autonomous agent system.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/memory_context.h"
#include "../lkjagent.h"
#include <string.h>
#include <time.h>
#include <math.h>

/**
 * @defgroup Context_Window_Internal Internal Context Window Functions
 * @{
 */

/**
 * @brief Estimate token count from character count
 * 
 * @param char_count Number of characters
 * @return Estimated number of tokens
 */
static size_t estimate_token_count(size_t char_count) {
    /* Rough approximation: 1 token ≈ 4 characters for English text */
    return (char_count + 3) / 4;
}

/**
 * @brief Calculate context priority score
 * 
 * @param key Context key to evaluate
 * @param current_state Current agent state
 * @return Priority score (higher = more important)
 */
static double calculate_context_priority(const context_key_t* key, agent_state_t current_state) {
    if (!key) return 0.0;
    
    double priority = 0.0;
    
    /* Base importance score (0-100) */
    priority += key->importance_score * 0.4; /* 40% weight */
    
    /* Recency factor */
    time_t current_time = time(NULL);
    time_t age = current_time - key->last_accessed;
    double recency_score = 0.0;
    
    if (age < 300) {        /* 5 minutes */
        recency_score = 30.0;
    } else if (age < 3600) { /* 1 hour */
        recency_score = 25.0;
    } else if (age < 86400) { /* 1 day */
        recency_score = 20.0;
    } else if (age < 604800) { /* 1 week */
        recency_score = 15.0;
    } else {
        recency_score = 10.0;
    }
    
    priority += recency_score * 0.3; /* 30% weight */
    
    /* Layer preference based on state */
    double layer_score = 0.0;
    switch (current_state) {
        case STATE_THINKING:
            /* Prefer working memory during thinking */
            layer_score = (key->layer == LAYER_WORKING) ? 20.0 : 
                         (key->layer == LAYER_DISK) ? 15.0 : 10.0;
            break;
        case STATE_EXECUTING:
            /* Prefer recent, actionable content */
            layer_score = (key->layer == LAYER_WORKING) ? 25.0 : 
                         (key->layer == LAYER_DISK) ? 10.0 : 5.0;
            break;
        case STATE_EVALUATING:
            /* Prefer diverse content for evaluation */
            layer_score = (key->layer == LAYER_WORKING) ? 20.0 : 
                         (key->layer == LAYER_DISK) ? 20.0 : 15.0;
            break;
        case STATE_PAGING:
            /* All layers equally important during paging analysis */
            layer_score = 15.0;
            break;
        default:
            layer_score = 15.0;
            break;
    }
    
    priority += layer_score * 0.2; /* 20% weight */
    
    /* Size efficiency factor (smaller data preferred for tight limits) */
    double size_score = 0.0;
    if (key->data_size < 512) {
        size_score = 10.0;
    } else if (key->data_size < 2048) {
        size_score = 8.0;
    } else if (key->data_size < 8192) {
        size_score = 6.0;
    } else {
        size_score = 4.0;
    }
    
    priority += size_score * 0.1; /* 10% weight */
    
    return priority;
}

/**
 * @brief Build state-specific context header
 * 
 * @param state Current agent state
 * @param header_buffer Buffer to store header
 * @param buffer_size Size of header buffer
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t build_context_header(agent_state_t state, char* header_buffer, size_t buffer_size) {
    if (!header_buffer || buffer_size == 0) {
        return RESULT_ERR;
    }
    
    const char* state_name;
    const char* state_description;
    
    switch (state) {
        case STATE_THINKING:
            state_name = "THINKING";
            state_description = "Analyzing situation and planning actions";
            break;
        case STATE_EXECUTING:
            state_name = "EXECUTING";
            state_description = "Executing planned actions and tasks";
            break;
        case STATE_EVALUATING:
            state_name = "EVALUATING";
            state_description = "Evaluating results and outcomes";
            break;
        case STATE_PAGING:
            state_name = "PAGING";
            state_description = "Managing memory context and paging";
            break;
        default:
            state_name = "UNKNOWN";
            state_description = "Unknown agent state";
            break;
    }
    
    time_t current_time = time(NULL);
    struct tm* tm_info = localtime(&current_time);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    int result = snprintf(header_buffer, buffer_size,
                         "=== AGENT CONTEXT ===\n"
                         "State: %s\n"
                         "Description: %s\n"
                         "Timestamp: %s\n"
                         "=====================\n\n",
                         state_name, state_description, time_str);
    
    return (result > 0 && result < (int)buffer_size) ? RESULT_OK : RESULT_ERR;
}

/**
 * @brief Estimate optimal context size for state
 * 
 * @param state Current agent state
 * @param max_tokens Maximum available tokens
 * @return Recommended context size in characters
 */
static size_t estimate_optimal_context_size(agent_state_t state, size_t max_tokens) {
    /* Convert tokens to characters (4 chars per token average) */
    size_t max_chars = max_tokens * 4;
    
    /* State-specific optimization */
    double efficiency_factor = 1.0;
    
    switch (state) {
        case STATE_THINKING:
            /* Dense context for analysis */
            efficiency_factor = 0.9;
            break;
        case STATE_EXECUTING:
            /* Focused context for action */
            efficiency_factor = 0.8;
            break;
        case STATE_EVALUATING:
            /* Comprehensive context for evaluation */
            efficiency_factor = 0.95;
            break;
        case STATE_PAGING:
            /* Full context for paging decisions */
            efficiency_factor = 1.0;
            break;
        default:
            efficiency_factor = 0.85;
            break;
    }
    
    return (size_t)(max_chars * efficiency_factor);
}

/** @} */

result_t context_window_calculate(tagged_memory_t* memory, context_window_info_t* info) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in context_window_calculate");
        return RESULT_ERR;
    }
    
    if (!info) {
        RETURN_ERR("Null info parameter in context_window_calculate");
        return RESULT_ERR;
    }
    
    /* Clear info structure */
    memset(info, 0, sizeof(context_window_info_t));
    
    /* Calculate current context size */
    info->current_size = memory->working_memory.size + memory->disk_memory.size;
    
    /* Count context keys */
    info->key_count = memory->context_key_count;
    
    /* Estimate token count */
    info->estimated_tokens = estimate_token_count(info->current_size);
    
    /* Set maximum size (would typically come from configuration) */
    info->max_size = 512 * 1024; /* Default 512KB limit */
    
    /* Calculate utilization */
    if (info->max_size > 0) {
        info->utilization = ((double)info->current_size / info->max_size) * 100.0;
    } else {
        info->utilization = 0.0;
    }
    
    return RESULT_OK;
}

result_t context_window_trim(tagged_memory_t* memory, size_t max_size, bool preserve_recent) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in context_window_trim");
        return RESULT_ERR;
    }
    
    if (max_size == 0) {
        RETURN_ERR("Invalid max_size parameter in context_window_trim");
        return RESULT_ERR;
    }
    
    /* Check if trimming is needed */
    context_window_info_t current_info;
    if (context_window_calculate(memory, &current_info) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (current_info.current_size <= max_size) {
        return RESULT_OK; /* No trimming needed */
    }
    
    /* Calculate how much to trim */
    size_t target_size = max_size * 0.9; /* Trim to 90% of limit for buffer */
    size_t bytes_to_trim = current_info.current_size - target_size;
    
    /* Create priority-sorted list of context keys */
    context_key_t* sorted_keys = malloc(memory->context_key_count * sizeof(context_key_t));
    if (!sorted_keys) {
        RETURN_ERR("Memory allocation failed in context_window_trim");
        return RESULT_ERR;
    }
    
    /* Copy and sort keys by priority (lowest priority first for trimming) */
    memcpy(sorted_keys, memory->context_keys, memory->context_key_count * sizeof(context_key_t));
    
    /* Simple bubble sort by priority (ascending - lowest priority first) */
    for (size_t i = 0; i < memory->context_key_count - 1; i++) {
        for (size_t j = 0; j < memory->context_key_count - i - 1; j++) {
            double priority_j = calculate_context_priority(&sorted_keys[j], STATE_THINKING);
            double priority_j1 = calculate_context_priority(&sorted_keys[j + 1], STATE_THINKING);
            
            /* Special handling for recent content preservation */
            if (preserve_recent) {
                time_t current_time = time(NULL);
                bool j_is_recent = (current_time - sorted_keys[j].last_accessed) < 3600;
                bool j1_is_recent = (current_time - sorted_keys[j + 1].last_accessed) < 3600;
                
                /* Don't trim recent content */
                if (j_is_recent && !j1_is_recent) {
                    priority_j += 50.0; /* Boost recent content priority */
                }
                if (j1_is_recent && !j_is_recent) {
                    priority_j1 += 50.0;
                }
            }
            
            if (priority_j > priority_j1) {
                context_key_t temp = sorted_keys[j];
                sorted_keys[j] = sorted_keys[j + 1];
                sorted_keys[j + 1] = temp;
            }
        }
    }
    
    /* Move lowest priority items to disk or archive */
    size_t trimmed_bytes = 0;
    for (size_t i = 0; i < memory->context_key_count && trimmed_bytes < bytes_to_trim; i++) {
        context_key_t* key = &sorted_keys[i];
        
        /* Skip high-importance items */
        if (key->importance_score >= 80) {
            continue;
        }
        
        /* Skip very recent items if preserve_recent is true */
        if (preserve_recent) {
            time_t current_time = time(NULL);
            if ((current_time - key->last_accessed) < 3600) { /* Less than 1 hour */
                continue;
            }
        }
        
        /* Move to appropriate layer based on current layer */
        if (key->layer == LAYER_WORKING) {
            /* Move working memory to disk */
            if (context_key_move_layer(memory, key->key, LAYER_DISK) == RESULT_OK) {
                trimmed_bytes += key->data_size;
            }
        } else if (key->layer == LAYER_DISK) {
            /* Archive disk memory */
            if (context_key_archive(memory, key->key) == RESULT_OK) {
                trimmed_bytes += key->data_size;
            }
        }
        /* Don't trim archived items further */
    }
    
    free(sorted_keys);
    
    /* If still over limit, perform more aggressive trimming */
    if (trimmed_bytes < bytes_to_trim) {
        /* Trim oldest content from working memory */
        if (memory->working_memory.size > target_size / 2) {
            size_t chars_to_remove = memory->working_memory.size - (target_size / 2);
            result_t trim_result = data_trim_front(&memory->working_memory, chars_to_remove);
            if (trim_result != RESULT_OK) {
                /* Log error but continue */
            }
        }
    }
    
    /* Update memory statistics */
    memory->last_modified = time(NULL);
    memory->total_size = calculate_total_memory_size(memory);
    
    return RESULT_OK;
}

result_t context_window_prioritize(tagged_memory_t* memory, size_t max_context_keys) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in context_window_prioritize");
        return RESULT_ERR;
    }
    
    if (max_context_keys == 0) {
        RETURN_ERR("Invalid max_context_keys parameter in context_window_prioritize");
        return RESULT_ERR;
    }
    
    /* If we're already under the limit, no action needed */
    if (memory->context_key_count <= max_context_keys) {
        return RESULT_OK;
    }
    
    /* Create priority-sorted list of context keys */
    context_key_t* sorted_keys = malloc(memory->context_key_count * sizeof(context_key_t));
    if (!sorted_keys) {
        RETURN_ERR("Memory allocation failed in context_window_prioritize");
        return RESULT_ERR;
    }
    
    /* Copy and sort keys by priority (highest priority first) */
    memcpy(sorted_keys, memory->context_keys, memory->context_key_count * sizeof(context_key_t));
    
    /* Sort by priority (descending - highest priority first) */
    for (size_t i = 0; i < memory->context_key_count - 1; i++) {
        for (size_t j = 0; j < memory->context_key_count - i - 1; j++) {
            double priority_j = calculate_context_priority(&sorted_keys[j], STATE_THINKING);
            double priority_j1 = calculate_context_priority(&sorted_keys[j + 1], STATE_THINKING);
            
            if (priority_j < priority_j1) {
                context_key_t temp = sorted_keys[j];
                sorted_keys[j] = sorted_keys[j + 1];
                sorted_keys[j + 1] = temp;
            }
        }
    }
    
    /* Keep top priority keys in working memory */
    size_t working_count = max_context_keys / 2; /* Half in working memory */
    size_t disk_count = max_context_keys - working_count;
    
    for (size_t i = 0; i < memory->context_key_count; i++) {
        context_key_t* key = &sorted_keys[i];
        
        if (i < working_count) {
            /* Move to working memory */
            if (key->layer != LAYER_WORKING) {
                context_key_move_layer(memory, key->key, LAYER_WORKING);
            }
        } else if (i < working_count + disk_count) {
            /* Move to disk memory */
            if (key->layer != LAYER_DISK) {
                context_key_move_layer(memory, key->key, LAYER_DISK);
            }
        } else {
            /* Archive low-priority keys */
            if (key->layer != LAYER_ARCHIVED) {
                context_key_archive(memory, key->key);
            }
        }
    }
    
    free(sorted_keys);
    
    memory->last_modified = time(NULL);
    
    return RESULT_OK;
}

result_t context_window_prepare_llm(tagged_memory_t* memory, agent_state_t state,
                                   data_t* context_buffer, size_t max_tokens) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in context_window_prepare_llm");
        return RESULT_ERR;
    }
    
    if (!context_buffer) {
        RETURN_ERR("Null context_buffer parameter in context_window_prepare_llm");
        return RESULT_ERR;
    }
    
    if (max_tokens == 0) {
        RETURN_ERR("Invalid max_tokens parameter in context_window_prepare_llm");
        return RESULT_ERR;
    }
    
    /* Clear context buffer */
    if (data_clear(context_buffer) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Calculate optimal context size */
    size_t optimal_size = estimate_optimal_context_size(state, max_tokens);
    
    /* Build context header */
    char header[512];
    if (build_context_header(state, header, sizeof(header)) == RESULT_OK) {
        data_append(context_buffer, header, 0);
    }
    
    /* Get priority-sorted context keys */
    context_key_t* sorted_keys = malloc(memory->context_key_count * sizeof(context_key_t));
    if (!sorted_keys) {
        RETURN_ERR("Memory allocation failed in context_window_prepare_llm");
        return RESULT_ERR;
    }
    
    memcpy(sorted_keys, memory->context_keys, memory->context_key_count * sizeof(context_key_t));
    
    /* Sort by priority for the current state */
    for (size_t i = 0; i < memory->context_key_count - 1; i++) {
        for (size_t j = 0; j < memory->context_key_count - i - 1; j++) {
            double priority_j = calculate_context_priority(&sorted_keys[j], state);
            double priority_j1 = calculate_context_priority(&sorted_keys[j + 1], state);
            
            if (priority_j < priority_j1) {
                context_key_t temp = sorted_keys[j];
                sorted_keys[j] = sorted_keys[j + 1];
                sorted_keys[j + 1] = temp;
            }
        }
    }
    
    /* Add context content based on priority and size limits */
    size_t current_size = context_buffer->size;
    
    for (size_t i = 0; i < memory->context_key_count && current_size < optimal_size; i++) {
        context_key_t* key = &sorted_keys[i];
        
        /* Skip if this would exceed our size limit significantly */
        if (current_size + key->data_size > optimal_size * 1.1) {
            continue;
        }
        
        /* Add context key section */
        char key_section[256];
        snprintf(key_section, sizeof(key_section),
                 "\n--- Context: %s (importance: %zu, layer: %s) ---\n",
                 key->key, key->importance_score,
                 (key->layer == LAYER_WORKING) ? "working" :
                 (key->layer == LAYER_DISK) ? "disk" : "archived");
        
        if (data_append(context_buffer, key_section, 0) != RESULT_OK) {
            break;
        }
        
        /* Retrieve and add context data */
        data_t key_data;
        if (data_init(&key_data, key->data_size + 256) == RESULT_OK) {
            if (tagged_memory_retrieve(memory, key->key, &key_data) == RESULT_OK) {
                if (data_append(context_buffer, key_data.data, 0) == RESULT_OK) {
                    if (data_append(context_buffer, "\n--- End Context ---\n", 0) == RESULT_OK) {
                        current_size = context_buffer->size;
                    }
                }
            }
            data_destroy(&key_data);
        }
    }
    
    free(sorted_keys);
    
    /* Add context footer */
    char footer[256];
    snprintf(footer, sizeof(footer),
             "\n=== CONTEXT SUMMARY ===\n"
             "Total size: %zu characters\n"
             "Estimated tokens: %zu\n"
             "Context keys included: %zu\n"
             "=====================\n",
             context_buffer->size, estimate_token_count(context_buffer->size),
             memory->context_key_count);
    
    data_append(context_buffer, footer, 0);
    
    return RESULT_OK;
}

result_t context_window_manage_overflow(tagged_memory_t* memory, size_t max_size) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in context_window_manage_overflow");
        return RESULT_ERR;
    }
    
    /* Check current size */
    context_window_info_t info;
    if (context_window_calculate(memory, &info) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (info.current_size <= max_size) {
        return RESULT_OK; /* No overflow */
    }
    
    /* Calculate overflow amount */
    size_t overflow = info.current_size - max_size;
    
    /* Progressive overflow management strategy */
    /* Note: overflow amount is %zu bytes */
    (void)overflow; /* Acknowledge variable usage */
    
    /* Step 1: Archive old, low-importance items */
    size_t cleaned_count;
    time_t week_ago = time(NULL) - (7 * 24 * 3600);
    context_key_cleanup_expired(memory, week_ago, true, &cleaned_count);
    
    /* Check if enough was cleaned */
    if (context_window_calculate(memory, &info) == RESULT_OK) {
        if (info.current_size <= max_size) {
            return RESULT_OK;
        }
    }
    
    /* Step 2: Move working memory items to disk */
    memory_query_criteria_t criteria = {0};
    criteria.layer = LAYER_WORKING;
    criteria.max_importance = 60; /* Medium and low importance */
    criteria.max_results = 20;
    
    memory_query_result_t* results = malloc(20 * sizeof(memory_query_result_t));
    if (results) {
        size_t result_count;
        if (tagged_memory_query(memory, &criteria, results, 20, &result_count) == RESULT_OK) {
            for (size_t i = 0; i < result_count; i++) {
                context_key_move_layer(memory, results[i].key.key, LAYER_DISK);
                data_destroy(&results[i].data);
            }
        }
        free(results);
    }
    
    /* Check if enough was moved */
    if (context_window_calculate(memory, &info) == RESULT_OK) {
        if (info.current_size <= max_size) {
            return RESULT_OK;
        }
    }
    
    /* Step 3: Aggressive trimming if still over limit */
    return context_window_trim(memory, max_size, true);
}

result_t context_window_preserve_recent(tagged_memory_t* memory, time_t preserve_threshold) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in context_window_preserve_recent");
        return RESULT_ERR;
    }
    
    time_t current_time = time(NULL);
    time_t cutoff_time = current_time - preserve_threshold;
    
    /* Move recent items to working memory */
    for (size_t i = 0; i < memory->context_key_count; i++) {
        context_key_t* key = &memory->context_keys[i];
        
        if (key->last_accessed >= cutoff_time) {
            /* This is recent content - ensure it's in working memory */
            if (key->layer != LAYER_WORKING) {
                context_key_move_layer(memory, key->key, LAYER_WORKING);
            }
            
            /* Boost importance of recent content */
            if (key->importance_score < 70) {
                size_t boosted_importance = key->importance_score + 20;
                if (boosted_importance > 100) boosted_importance = 100;
                context_key_update_importance(memory, key->key, boosted_importance);
            }
        }
    }
    
    return RESULT_OK;
}

result_t context_window_optimize(tagged_memory_t* memory, agent_state_t state) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in context_window_optimize");
        return RESULT_ERR;
    }
    
    /* State-specific optimization */
    switch (state) {
        case STATE_THINKING:
            /* Optimize for analytical thinking */
            context_window_prioritize(memory, 50);
            context_window_preserve_recent(memory, 3600); /* Last hour */
            break;
            
        case STATE_EXECUTING:
            /* Optimize for action execution */
            context_window_prioritize(memory, 30);
            context_window_preserve_recent(memory, 1800); /* Last 30 minutes */
            break;
            
        case STATE_EVALUATING:
            /* Optimize for comprehensive evaluation */
            context_window_prioritize(memory, 80);
            context_window_preserve_recent(memory, 7200); /* Last 2 hours */
            break;
            
        case STATE_PAGING:
            /* Optimize for memory management */
            tagged_memory_compact(memory, true);
            break;
            
        default:
            /* Default optimization */
            context_window_prioritize(memory, 40);
            context_window_preserve_recent(memory, 3600);
            break;
    }
    
    return RESULT_OK;
}
