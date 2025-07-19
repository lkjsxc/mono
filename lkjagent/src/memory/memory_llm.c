/**
 * @file memory_llm.c
 * @brief LLM memory integration implementation for LKJAgent
 * 
 * This module implements the intelligent LLM-directed memory operations
 * including context analysis, paging decisions, and memory optimization
 * based on LLM responses and directives.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/memory_context.h"
#include "../include/tag_parser.h"
#include "../lkjagent.h"
#include <string.h>
#include <time.h>
#include <ctype.h>

/**
 * @defgroup Memory_LLM_Internal Internal LLM Memory Functions
 * @{
 */

/**
 * @brief Extract importance score from LLM text
 * 
 * @param text Text to analyze
 * @return Importance score (0-100), or 50 if not found
 */
static size_t extract_importance_score(const char* text) {
    if (!text) return 50;
    
    /* Look for patterns like "importance: 85", "priority: high", etc. */
    const char* importance_indicators[] = {
        "importance:",
        "priority:",
        "relevance:",
        "significance:",
        NULL
    };
    
    for (int i = 0; importance_indicators[i]; i++) {
        const char* indicator = importance_indicators[i];
        const char* pos = strstr(text, indicator);
        
        if (pos) {
            pos += strlen(indicator);
            
            /* Skip whitespace */
            while (*pos && isspace(*pos)) pos++;
            
            /* Try to parse number */
            if (isdigit(*pos)) {
                int score = atoi(pos);
                if (score >= 0 && score <= 100) {
                    return (size_t)score;
                }
            }
            
            /* Try to parse qualitative terms */
            if (strncasecmp(pos, "high", 4) == 0) return 80;
            if (strncasecmp(pos, "medium", 6) == 0) return 60;
            if (strncasecmp(pos, "low", 3) == 0) return 40;
            if (strncasecmp(pos, "critical", 8) == 0) return 95;
            if (strncasecmp(pos, "urgent", 6) == 0) return 90;
        }
    }
    
    return 50; /* Default moderate importance */
}

/**
 * @brief Parse context keys from simple tag format
 * 
 * @param text Text containing simple tags
 * @param keys Array to store found keys
 * @param max_keys Maximum number of keys to extract
 * @param key_count Pointer to store number of keys found
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t parse_context_keys_from_tags(const char* text, char keys[][MAX_TAG_SIZE], 
                                           size_t max_keys, size_t* key_count) {
    if (!text || !keys || !key_count) {
        return RESULT_ERR;
    }
    
    *key_count = 0;
    
    /* Simple approach: look for common patterns like <key:name> or [name] */
    const char* current = text;
    
    while (*current && *key_count < max_keys) {
        /* Skip whitespace */
        while (*current && isspace(*current)) {
            current++;
        }
        
        if (!*current) break;
        
        /* Look for <key:name> pattern */
        if (strncmp(current, "<key:", 5) == 0) {
            current += 5; /* Skip "<key:" */
            
            /* Extract key name */
            char* key_end = strchr(current, '>');
            if (key_end && (key_end - current) < MAX_TAG_SIZE - 1) {
                size_t key_len = key_end - current;
                strncpy(keys[*key_count], current, key_len);
                keys[*key_count][key_len] = '\0';
                (*key_count)++;
                current = key_end + 1;
                continue;
            }
        }
        
        /* Look for [name] pattern */
        if (*current == '[') {
            current++; /* Skip '[' */
            
            char* bracket_end = strchr(current, ']');
            if (bracket_end && (bracket_end - current) < MAX_TAG_SIZE - 1) {
                size_t key_len = bracket_end - current;
                strncpy(keys[*key_count], current, key_len);
                keys[*key_count][key_len] = '\0';
                (*key_count)++;
                current = bracket_end + 1;
                continue;
            }
        }
        
        /* Skip to next potential tag */
        current++;
    }
    
    return RESULT_OK;
}

/**
 * @brief Analyze text for context patterns
 * 
 * @param text Text to analyze
 * @param patterns Array to store found patterns
 * @param max_patterns Maximum patterns to find
 * @param pattern_count Pointer to store number of patterns found
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t analyze_context_patterns(const char* text, char patterns[][MAX_TAG_SIZE],
                                       size_t max_patterns, size_t* pattern_count) {
    if (!text || !patterns || !pattern_count) {
        return RESULT_ERR;
    }
    
    *pattern_count = 0;
    
    /* Simple pattern detection based on common LLM response structures */
    const char* pattern_indicators[] = {
        "based on",
        "according to",
        "in context of",
        "regarding",
        "about",
        "concerning",
        "related to",
        NULL
    };
    
    const char* text_lower = text; /* In a full implementation, convert to lowercase */
    
    for (int i = 0; pattern_indicators[i] && *pattern_count < max_patterns; i++) {
        const char* indicator = pattern_indicators[i];
        const char* pos = strstr(text_lower, indicator);
        
        if (pos) {
            /* Extract context after indicator */
            pos += strlen(indicator);
            while (*pos && isspace(*pos)) pos++;
            
            /* Find end of context phrase */
            const char* end = pos;
            while (*end && !ispunct(*end) && *end != '\n' && (end - pos) < MAX_TAG_SIZE - 1) {
                end++;
            }
            
            if (end > pos) {
                size_t len = end - pos;
                strncpy(patterns[*pattern_count], pos, len);
                patterns[*pattern_count][len] = '\0';
                
                /* Clean up pattern (remove extra spaces, etc.) */
                char* clean_pos = patterns[*pattern_count];
                while (*clean_pos) {
                    if (isspace(*clean_pos) && isspace(*(clean_pos + 1))) {
                        memmove(clean_pos, clean_pos + 1, strlen(clean_pos));
                    } else {
                        clean_pos++;
                    }
                }
                
                (*pattern_count)++;
            }
        }
    }
    
    return RESULT_OK;
}

/** @} */

result_t memory_llm_analyze_context(tagged_memory_t* memory, const char* llm_response,
                                   char context_keys[][MAX_TAG_SIZE], size_t max_keys, size_t* key_count) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_llm_analyze_context");
        return RESULT_ERR;
    }
    
    if (!llm_response) {
        RETURN_ERR("Null llm_response parameter in memory_llm_analyze_context");
        return RESULT_ERR;
    }
    
    if (!context_keys || !key_count) {
        RETURN_ERR("Null output parameters in memory_llm_analyze_context");
        return RESULT_ERR;
    }
    
    if (max_keys == 0) {
        *key_count = 0;
        return RESULT_OK;
    }
    
    *key_count = 0;
    
    /* Parse context keys from simple tags in the response */
    size_t tag_count = 0;
    result_t tag_result = parse_context_keys_from_tags(llm_response, context_keys, max_keys, &tag_count);
    
    if (tag_result == RESULT_OK) {
        *key_count = tag_count;
    }
    
    /* If no explicit tags found, analyze for context patterns */
    if (*key_count == 0) {
        char patterns[10][MAX_TAG_SIZE];
        size_t pattern_count = 0;
        
        if (analyze_context_patterns(llm_response, patterns, 10, &pattern_count) == RESULT_OK) {
            /* Convert patterns to context keys */
            for (size_t i = 0; i < pattern_count && *key_count < max_keys; i++) {
                /* Simple key generation from patterns */
                snprintf(context_keys[*key_count], MAX_TAG_SIZE, "context_%zu", i + 1);
                (*key_count)++;
            }
        }
    }
    
    /* If still no keys found, create a default key based on content analysis */
    if (*key_count == 0 && max_keys > 0) {
        time_t current_time = time(NULL);
        snprintf(context_keys[0], MAX_TAG_SIZE, "llm_response_%ld", current_time);
        *key_count = 1;
    }
    
    return RESULT_OK;
}

result_t memory_llm_identify_keys(const char* llm_response, char identified_keys[][MAX_TAG_SIZE],
                                 size_t max_keys, size_t* key_count) {
    if (!llm_response) {
        RETURN_ERR("Null llm_response parameter in memory_llm_identify_keys");
        return RESULT_ERR;
    }
    
    if (!identified_keys || !key_count) {
        RETURN_ERR("Null output parameters in memory_llm_identify_keys");
        return RESULT_ERR;
    }
    
    *key_count = 0;
    
    /* Look for explicit memory references in LLM response */
    const char* memory_indicators[] = {
        "remember",
        "recall",
        "store",
        "save",
        "context:",
        "memory:",
        "key:",
        NULL
    };
    
    const char* pos = llm_response;
    
    for (int i = 0; memory_indicators[i] && *key_count < max_keys; i++) {
        const char* indicator = memory_indicators[i];
        pos = strstr(llm_response, indicator);
        
        while (pos && *key_count < max_keys) {
            pos += strlen(indicator);
            
            /* Skip whitespace */
            while (*pos && isspace(*pos)) pos++;
            
            /* Extract potential key */
            if (*pos == '"' || *pos == '\'') {
                /* Quoted key */
                char quote = *pos;
                pos++;
                const char* start = pos;
                
                while (*pos && *pos != quote && (pos - start) < MAX_TAG_SIZE - 1) {
                    pos++;
                }
                
                if (*pos == quote) {
                    size_t len = pos - start;
                    strncpy(identified_keys[*key_count], start, len);
                    identified_keys[*key_count][len] = '\0';
                    (*key_count)++;
                }
            } else {
                /* Unquoted key - take until whitespace or punctuation */
                const char* start = pos;
                
                while (*pos && !isspace(*pos) && !ispunct(*pos) && (pos - start) < MAX_TAG_SIZE - 1) {
                    pos++;
                }
                
                if (pos > start) {
                    size_t len = pos - start;
                    strncpy(identified_keys[*key_count], start, len);
                    identified_keys[*key_count][len] = '\0';
                    (*key_count)++;
                }
            }
            
            /* Look for next occurrence */
            pos = strstr(pos, indicator);
        }
    }
    
    return RESULT_OK;
}

result_t memory_llm_request_paging(tagged_memory_t* memory, agent_state_t current_state,
                                  size_t context_limit, data_t* paging_request) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_llm_request_paging");
        return RESULT_ERR;
    }
    
    if (!paging_request) {
        RETURN_ERR("Null paging_request parameter in memory_llm_request_paging");
        return RESULT_ERR;
    }
    
    if (context_limit == 0) {
        RETURN_ERR("Invalid context_limit in memory_llm_request_paging");
        return RESULT_ERR;
    }
    
    /* Clear request buffer */
    if (data_clear(paging_request) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Get current memory statistics */
    memory_stats_t stats;
    if (tagged_memory_get_stats(memory, &stats) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Calculate context window info */
    context_window_info_t window_info;
    if (context_window_calculate(memory, &window_info) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Build paging request */
    char request_header[] = 
        "MEMORY PAGING REQUEST\n"
        "==================\n\n"
        "Current System State:\n";
    
    if (data_append(paging_request, request_header, 0) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Add state information */
    char state_info[512];
    const char* state_name = (current_state == STATE_THINKING) ? "THINKING" :
                            (current_state == STATE_EXECUTING) ? "EXECUTING" :
                            (current_state == STATE_EVALUATING) ? "EVALUATING" :
                            (current_state == STATE_PAGING) ? "PAGING" : "UNKNOWN";
    
    snprintf(state_info, sizeof(state_info),
             "- Agent State: %s\n"
             "- Context Limit: %zu tokens\n"
             "- Current Context Size: %zu characters\n"
             "- Context Utilization: %.1f%%\n"
             "- Total Memory Keys: %zu\n\n",
             state_name, context_limit, window_info.current_size,
             window_info.utilization, stats.context_key_count);
    
    if (data_append(paging_request, state_info, 0) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Add memory layer breakdown */
    char layer_info[512];
    snprintf(layer_info, sizeof(layer_info),
             "Memory Layer Status:\n"
             "- Working Memory: %zu bytes\n"
             "- Disk Memory: %zu bytes\n"
             "- Total Size: %zu bytes\n\n",
             stats.working_size, stats.disk_size, stats.total_size);
    
    if (data_append(paging_request, layer_info, 0) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Add context keys summary */
    if (data_append(paging_request, "High Priority Context Keys:\n", 0) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Get high-importance keys */
    memory_query_criteria_t criteria = {0};
    criteria.min_importance = 70;
    criteria.max_importance = 100;
    criteria.layer = -1; /* All layers */
    criteria.max_results = 20;
    
    memory_query_result_t* query_results = malloc(20 * sizeof(memory_query_result_t));
    if (query_results) {
        size_t result_count;
        if (tagged_memory_query(memory, &criteria, query_results, 20, &result_count) == RESULT_OK) {
            for (size_t i = 0; i < result_count; i++) {
                char key_summary[256];
                snprintf(key_summary, sizeof(key_summary),
                         "- %s (importance: %zu, layer: %s)\n",
                         query_results[i].key.key,
                         query_results[i].key.importance_score,
                         (query_results[i].key.layer == LAYER_WORKING) ? "working" :
                         (query_results[i].key.layer == LAYER_DISK) ? "disk" : "archived");
                
                if (data_append(paging_request, key_summary, 0) != RESULT_OK) {
                    /* Continue processing other keys even if one fails */
                }
                data_destroy(&query_results[i].data);
            }
        }
        free(query_results);
    }
    
    /* Add paging request */
    char paging_prompt[] = 
        "\nPAGING DIRECTIVE REQUEST:\n"
        "Please analyze the current memory state and provide paging directives using simple tags:\n"
        "- Use <move_to_disk:key_name> to move keys to disk storage\n"
        "- Use <move_to_working:key_name> to move keys to working memory\n"
        "- Use <archive:key_name> to archive old keys\n"
        "- Use <importance:key_name:score> to update importance scores\n"
        "- Use <delete:key_name> to remove obsolete keys\n\n"
        "Focus on optimizing memory for the current agent state while preserving important context.\n";
    
    if (data_append(paging_request, paging_prompt, 0) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t memory_llm_process_directives(tagged_memory_t* memory, const char* llm_response) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_llm_process_directives");
        return RESULT_ERR;
    }
    
    if (!llm_response) {
        RETURN_ERR("Null llm_response parameter in memory_llm_process_directives");
        return RESULT_ERR;
    }
    
    /* Parse simple tag directives from LLM response */
    data_t response_data;
    if (data_init(&response_data, strlen(llm_response) + 1) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (data_set(&response_data, llm_response, 0) != RESULT_OK) {
        data_destroy(&response_data);
        return RESULT_ERR;
    }
    
    /* Simply process the LLM response directly for directives */
    const char* current = llm_response;
    char line_buffer[512];
    
    /* Process line by line looking for directives */
    while (*current) {
        /* Extract line */
        const char* line_end = strchr(current, '\n');
        size_t line_len;
        
        if (line_end) {
            line_len = line_end - current;
        } else {
            line_len = strlen(current);
        }
        
        if (line_len < sizeof(line_buffer) - 1) {
            strncpy(line_buffer, current, line_len);
            line_buffer[line_len] = '\0';
            
            /* Skip whitespace */
            char* directive = line_buffer;
            while (*directive && isspace(*directive)) directive++;
            
            /* Process move_to_disk directives */
            if (strncmp(directive, "move_to_disk:", 13) == 0) {
                char* key_name = directive + 13;
                if (*key_name) {
                    result_t move_result = context_key_move_layer(memory, key_name, LAYER_DISK);
                    if (move_result != RESULT_OK) {
                        /* Log error but continue processing */
                    }
                }
            }
            /* Process move_to_working directives */
            else if (strncmp(directive, "move_to_working:", 16) == 0) {
                char* key_name = directive + 16;
                if (*key_name) {
                    result_t move_result = context_key_move_layer(memory, key_name, LAYER_WORKING);
                    if (move_result != RESULT_OK) {
                        /* Log error but continue processing */
                    }
                }
            }
            /* Process archive directives */
            else if (strncmp(directive, "archive:", 8) == 0) {
                char* key_name = directive + 8;
                if (*key_name) {
                    result_t archive_result = context_key_archive(memory, key_name);
                    if (archive_result != RESULT_OK) {
                        /* Log error but continue processing */
                    }
                }
            }
            /* Process importance directives */
            else if (strncmp(directive, "importance:", 11) == 0) {
                char* params = directive + 11;
                char* key_name = strtok(params, ":");
                char* score_str = strtok(NULL, ":");
                
                if (key_name && score_str) {
                    int score = atoi(score_str);
                    if (score >= 0 && score <= 100) {
                        result_t importance_result = context_key_update_importance(memory, key_name, (size_t)score);
                        if (importance_result != RESULT_OK) {
                            /* Log error but continue processing */
                        }
                    }
                }
            }
            /* Process delete directives */
            else if (strncmp(directive, "delete:", 7) == 0) {
                char* key_name = directive + 7;
                if (*key_name) {
                    result_t delete_result = tagged_memory_delete(memory, key_name);
                    if (delete_result != RESULT_OK) {
                        /* Log error but continue processing */
                    }
                }
            }
        }
        
        /* Move to next line */
        if (line_end) {
            current = line_end + 1;
        } else {
            break;
        }
    }
    
    data_destroy(&response_data);
    
    return RESULT_OK;
}

result_t memory_llm_evaluate_importance(const char* content, const char* context, size_t* importance_score) {
    if (!content) {
        RETURN_ERR("Null content parameter in memory_llm_evaluate_importance");
        return RESULT_ERR;
    }
    
    if (!importance_score) {
        RETURN_ERR("Null importance_score parameter in memory_llm_evaluate_importance");
        return RESULT_ERR;
    }
    
    /* Simple heuristic-based importance evaluation */
    int score = 50; /* Base score */
    
    /* Length factor */
    size_t content_len = strlen(content);
    if (content_len > 1000) {
        score += 10; /* Longer content may be more important */
    } else if (content_len < 100) {
        score -= 10; /* Very short content may be less important */
    }
    
    /* Keyword-based scoring */
    const char* high_importance_keywords[] = {
        "critical", "important", "urgent", "error", "failure", "success", "result", NULL
    };
    
    const char* medium_importance_keywords[] = {
        "note", "remember", "consider", "think", "analyze", NULL
    };
    
    for (int i = 0; high_importance_keywords[i]; i++) {
        if (strstr(content, high_importance_keywords[i])) {
            score += 15;
            break; /* Only count once */
        }
    }
    
    for (int i = 0; medium_importance_keywords[i]; i++) {
        if (strstr(content, medium_importance_keywords[i])) {
            score += 10;
            break; /* Only count once */
        }
    }
    
    /* Context-based adjustments */
    if (context) {
        if (strstr(context, "critical") || strstr(context, "high priority")) {
            score += 20;
        } else if (strstr(context, "low priority") || strstr(context, "optional")) {
            score -= 15;
        }
    }
    
    /* Extract explicit importance from content */
    size_t extracted_score = extract_importance_score(content);
    if (extracted_score != 50) { /* Non-default score found */
        score = extracted_score;
    }
    
    /* Ensure score is within bounds */
    if (score > 100) score = 100;
    if (score < 0) score = 0;
    
    *importance_score = (size_t)score;
    
    return RESULT_OK;
}

result_t memory_llm_suggest_relationships(tagged_memory_t* memory, const char* base_key,
                                        char related_keys[][MAX_TAG_SIZE], size_t max_keys, size_t* key_count) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_llm_suggest_relationships");
        return RESULT_ERR;
    }
    
    if (!base_key || base_key[0] == '\0') {
        RETURN_ERR("Invalid base_key parameter in memory_llm_suggest_relationships");
        return RESULT_ERR;
    }
    
    if (!related_keys || !key_count) {
        RETURN_ERR("Null output parameters in memory_llm_suggest_relationships");
        return RESULT_ERR;
    }
    
    *key_count = 0;
    
    /* Use existing memory query for related items */
    memory_query_result_t* results = malloc(max_keys * sizeof(memory_query_result_t));
    if (!results) {
        RETURN_ERR("Memory allocation failed in memory_llm_suggest_relationships");
        return RESULT_ERR;
    }
    
    size_t result_count;
    result_t query_result = memory_query_related(memory, base_key, results, max_keys, &result_count);
    
    if (query_result == RESULT_OK) {
        /* Extract key names from results */
        for (size_t i = 0; i < result_count && *key_count < max_keys; i++) {
            strncpy(related_keys[*key_count], results[i].key.key, MAX_TAG_SIZE - 1);
            related_keys[*key_count][MAX_TAG_SIZE - 1] = '\0';
            (*key_count)++;
        }
    }
    
    /* Cleanup results */
    for (size_t i = 0; i < result_count; i++) {
        data_destroy(&results[i].data);
    }
    free(results);
    
    return query_result;
}

result_t memory_llm_optimize_storage(tagged_memory_t* memory, agent_state_t current_state) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_llm_optimize_storage");
        return RESULT_ERR;
    }
    
    /* State-specific optimization strategies */
    switch (current_state) {
        case STATE_THINKING:
            /* During thinking, prioritize working memory for quick access */
            /* Move high-importance, recently accessed items to working memory */
            {
                memory_query_criteria_t criteria = {0};
                criteria.min_importance = 80;
                criteria.layer = LAYER_DISK;
                criteria.start_time = time(NULL) - 3600; /* Last hour */
                
                memory_query_result_t results[10];
                size_t result_count;
                
                if (tagged_memory_query(memory, &criteria, results, 10, &result_count) == RESULT_OK) {
                    for (size_t i = 0; i < result_count; i++) {
                        result_t move_result = context_key_move_layer(memory, results[i].key.key, LAYER_WORKING);
                        if (move_result != RESULT_OK) {
                            /* Log error but continue */
                        }
                        data_destroy(&results[i].data);
                    }
                }
            }
            break;
            
        case STATE_EXECUTING:
            /* During execution, focus on immediate context */
            /* Archive old, low-importance items */
            {
                size_t cleaned_count;
                result_t cleanup_result = context_key_cleanup_expired(memory, 7 * 24 * 3600, true, &cleaned_count); /* 7 days */
                if (cleanup_result != RESULT_OK) {
                    /* Log error but continue */
                }
            }
            break;
            
        case STATE_EVALUATING:
            /* During evaluation, maintain diverse context */
            /* Balance between layers based on importance */
            {
                memory_query_criteria_t criteria = {0};
                criteria.min_importance = 0;
                criteria.max_importance = 50;
                criteria.layer = LAYER_WORKING;
                
                memory_query_result_t results[20];
                size_t result_count;
                
                if (tagged_memory_query(memory, &criteria, results, 20, &result_count) == RESULT_OK) {
                    for (size_t i = 0; i < result_count; i++) {
                        result_t move_result = context_key_move_layer(memory, results[i].key.key, LAYER_DISK);
                        if (move_result != RESULT_OK) {
                            /* Log error but continue */
                        }
                        data_destroy(&results[i].data);
                    }
                }
            }
            break;
            
        case STATE_PAGING:
            /* During paging, perform comprehensive optimization */
            {
                result_t compact_result = tagged_memory_compact(memory, true);
                if (compact_result != RESULT_OK) {
                    /* Log error but continue */
                }
            }
            break;
            
        default:
            /* Default optimization */
            {
                result_t compact_result = tagged_memory_compact(memory, false);
                if (compact_result != RESULT_OK) {
                    /* Log error but continue */
                }
            }
            break;
    }
    
    return RESULT_OK;
}
