/**
 * @file memory_queries.c
 * @brief Memory query engine implementation for LKJAgent
 * 
 * This module implements the advanced memory query engine with support for
 * multi-criteria queries, pattern matching, temporal filtering, and
 * relevance ranking for the autonomous agent memory system.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/memory_context.h"
#include "../lkjagent.h"
#include <string.h>
#include <time.h>
#include <fnmatch.h>
#include <sys/time.h>

/**
 * @defgroup Memory_Query_Internal Internal Memory Query Functions
 * @{
 */

/**
 * @brief Calculate relevance score for a query result
 * 
 * @param key Context key to evaluate
 * @param criteria Query criteria
 * @return Relevance score (0-100)
 */
static size_t calculate_relevance_score(const context_key_t* key, const memory_query_criteria_t* criteria) {
    if (!key || !criteria) return 0;
    
    size_t score = 0;
    size_t factors = 0;
    
    /* Importance score factor (40% weight) */
    score += (key->importance_score * 40) / 100;
    factors++;
    
    /* Recency factor (30% weight) */
    time_t current_time = time(NULL);
    time_t age = current_time - key->last_accessed;
    size_t recency_score = 0;
    
    if (age < 3600) {           /* Less than 1 hour */
        recency_score = 30;
    } else if (age < 86400) {   /* Less than 1 day */
        recency_score = 25;
    } else if (age < 604800) {  /* Less than 1 week */
        recency_score = 20;
    } else if (age < 2592000) { /* Less than 1 month */
        recency_score = 15;
    } else {
        recency_score = 10;
    }
    
    score += recency_score;
    factors++;
    
    /* Layer preference factor (20% weight) */
    size_t layer_score = 0;
    switch (key->layer) {
        case LAYER_WORKING:
            layer_score = 20; /* Highest preference for working memory */
            break;
        case LAYER_DISK:
            layer_score = 15;
            break;
        case LAYER_ARCHIVED:
            layer_score = 10; /* Lowest preference for archived */
            break;
    }
    score += layer_score;
    factors++;
    
    /* Size factor (10% weight) - smaller data scores higher for quick access */
    size_t size_score = 0;
    if (key->data_size < 1024) {           /* Small data */
        size_score = 10;
    } else if (key->data_size < 10240) {   /* Medium data */
        size_score = 8;
    } else if (key->data_size < 102400) {  /* Large data */
        size_score = 6;
    } else {                               /* Very large data */
        size_score = 4;
    }
    score += size_score;
    factors++;
    
    /* Ensure score doesn't exceed 100 */
    if (score > 100) score = 100;
    
    return score;
}

/**
 * @brief Check if context key matches query criteria
 * 
 * @param key Context key to check
 * @param criteria Query criteria
 * @return true if key matches criteria, false otherwise
 */
static bool key_matches_criteria(const context_key_t* key, const memory_query_criteria_t* criteria) {
    if (!key || !criteria) return false;
    
    /* Check key pattern match */
    if (criteria->key_pattern[0] != '\0') {
        if (fnmatch(criteria->key_pattern, key->key, 0) != 0) {
            return false;
        }
    }
    
    /* Check layer filter */
    if (criteria->layer >= 0 && criteria->layer != (int)key->layer) {
        return false;
    }
    
    /* Check importance range */
    if (key->importance_score < criteria->min_importance ||
        key->importance_score > criteria->max_importance) {
        return false;
    }
    
    /* Check time range */
    if (criteria->start_time > 0 && key->last_accessed < criteria->start_time) {
        return false;
    }
    
    if (criteria->end_time > 0 && key->last_accessed > criteria->end_time) {
        return false;
    }
    
    return true;
}

/**
 * @brief Compare two query results for sorting
 * 
 * @param a First result
 * @param b Second result
 * @return Comparison result for qsort
 */
static int compare_query_results(const void* a, const void* b) {
    const memory_query_result_t* result_a = (const memory_query_result_t*)a;
    const memory_query_result_t* result_b = (const memory_query_result_t*)b;
    
    /* Sort by relevance score in descending order */
    if (result_a->relevance_score > result_b->relevance_score) return -1;
    if (result_a->relevance_score < result_b->relevance_score) return 1;
    
    /* If relevance scores are equal, sort by importance */
    if (result_a->key.importance_score > result_b->key.importance_score) return -1;
    if (result_a->key.importance_score < result_b->key.importance_score) return 1;
    
    /* If importance scores are equal, sort by access time (most recent first) */
    if (result_a->key.last_accessed > result_b->key.last_accessed) return -1;
    if (result_a->key.last_accessed < result_b->key.last_accessed) return 1;
    
    return 0;
}

/**
 * @brief Initialize query criteria with defaults
 * 
 * @param criteria Criteria structure to initialize
 */
static void init_query_criteria(memory_query_criteria_t* criteria) {
    if (!criteria) return;
    
    memset(criteria, 0, sizeof(memory_query_criteria_t));
    criteria->layer = -1; /* All layers */
    criteria->min_importance = 0;
    criteria->max_importance = 100;
    criteria->start_time = 0; /* No time filter */
    criteria->end_time = 0;   /* No time filter */
    criteria->max_results = SIZE_MAX; /* No limit */
}

/** @} */

result_t memory_query_by_tag(tagged_memory_t* memory, const char* tag_pattern,
                            memory_query_result_t* results, size_t max_results, size_t* result_count) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_query_by_tag");
        return RESULT_ERR;
    }
    
    if (!tag_pattern || tag_pattern[0] == '\0') {
        RETURN_ERR("Invalid tag_pattern parameter in memory_query_by_tag");
        return RESULT_ERR;
    }
    
    if (!result_count) {
        RETURN_ERR("Null result_count parameter in memory_query_by_tag");
        return RESULT_ERR;
    }
    
    if (max_results > 0 && !results) {
        RETURN_ERR("Null results parameter with non-zero max_results");
        return RESULT_ERR;
    }
    
    /* Create query criteria */
    memory_query_criteria_t criteria;
    init_query_criteria(&criteria);
    strncpy(criteria.key_pattern, tag_pattern, MAX_TAG_SIZE - 1);
    criteria.key_pattern[MAX_TAG_SIZE - 1] = '\0';
    criteria.max_results = max_results;
    
    /* Execute query */
    return tagged_memory_query(memory, &criteria, results, max_results, result_count);
}

result_t memory_query_by_context_key(tagged_memory_t* memory, const char* key_name,
                                    memory_query_result_t* result) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_query_by_context_key");
        return RESULT_ERR;
    }
    
    if (!key_name || key_name[0] == '\0') {
        RETURN_ERR("Invalid key_name parameter in memory_query_by_context_key");
        return RESULT_ERR;
    }
    
    if (!result) {
        RETURN_ERR("Null result parameter in memory_query_by_context_key");
        return RESULT_ERR;
    }
    
    /* Initialize result */
    memset(result, 0, sizeof(memory_query_result_t));
    
    /* Find context key */
    if (context_key_find(memory, key_name, &result->key) != RESULT_OK) {
        return RESULT_ERR; /* Key not found */
    }
    
    /* Initialize data buffer */
    if (data_init(&result->data, 1024) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Retrieve data */
    if (tagged_memory_retrieve(memory, key_name, &result->data) != RESULT_OK) {
        data_destroy(&result->data);
        return RESULT_ERR;
    }
    
    /* Calculate relevance score */
    memory_query_criteria_t criteria;
    init_query_criteria(&criteria);
    result->relevance_score = calculate_relevance_score(&result->key, &criteria);
    
    return RESULT_OK;
}

result_t memory_query_by_importance(tagged_memory_t* memory, size_t min_importance, size_t max_importance,
                                   memory_query_result_t* results, size_t max_results, size_t* result_count) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_query_by_importance");
        return RESULT_ERR;
    }
    
    if (min_importance > 100 || max_importance > 100 || min_importance > max_importance) {
        RETURN_ERR("Invalid importance range in memory_query_by_importance");
        return RESULT_ERR;
    }
    
    if (!result_count) {
        RETURN_ERR("Null result_count parameter in memory_query_by_importance");
        return RESULT_ERR;
    }
    
    if (max_results > 0 && !results) {
        RETURN_ERR("Null results parameter with non-zero max_results");
        return RESULT_ERR;
    }
    
    /* Create query criteria */
    memory_query_criteria_t criteria;
    init_query_criteria(&criteria);
    criteria.min_importance = min_importance;
    criteria.max_importance = max_importance;
    criteria.max_results = max_results;
    
    /* Execute query */
    return tagged_memory_query(memory, &criteria, results, max_results, result_count);
}

result_t memory_query_by_timerange(tagged_memory_t* memory, time_t start_time, time_t end_time,
                                  memory_query_result_t* results, size_t max_results, size_t* result_count) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_query_by_timerange");
        return RESULT_ERR;
    }
    
    if (start_time > 0 && end_time > 0 && start_time > end_time) {
        RETURN_ERR("Invalid time range in memory_query_by_timerange");
        return RESULT_ERR;
    }
    
    if (!result_count) {
        RETURN_ERR("Null result_count parameter in memory_query_by_timerange");
        return RESULT_ERR;
    }
    
    if (max_results > 0 && !results) {
        RETURN_ERR("Null results parameter with non-zero max_results");
        return RESULT_ERR;
    }
    
    /* Create query criteria */
    memory_query_criteria_t criteria;
    init_query_criteria(&criteria);
    criteria.start_time = start_time;
    criteria.end_time = end_time;
    criteria.max_results = max_results;
    
    /* Execute query */
    return tagged_memory_query(memory, &criteria, results, max_results, result_count);
}

result_t memory_query_related(tagged_memory_t* memory, const char* reference_key,
                             memory_query_result_t* results, size_t max_results, size_t* result_count) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_query_related");
        return RESULT_ERR;
    }
    
    if (!reference_key || reference_key[0] == '\0') {
        RETURN_ERR("Invalid reference_key parameter in memory_query_related");
        return RESULT_ERR;
    }
    
    if (!result_count) {
        RETURN_ERR("Null result_count parameter in memory_query_related");
        return RESULT_ERR;
    }
    
    if (max_results > 0 && !results) {
        RETURN_ERR("Null results parameter with non-zero max_results");
        return RESULT_ERR;
    }
    
    /* Find reference key to get its characteristics */
    context_key_t ref_key;
    if (context_key_find(memory, reference_key, &ref_key) != RESULT_OK) {
        RETURN_ERR("Reference key not found in memory_query_related");
        return RESULT_ERR;
    }
    
    /* Create pattern based on reference key */
    char pattern[MAX_TAG_SIZE];
    
    /* Simple heuristic: look for keys with common prefixes or suffixes */
    const char* underscore = strchr(ref_key.key, '_');
    if (underscore) {
        /* Use prefix before underscore */
        size_t prefix_len = underscore - ref_key.key;
        snprintf(pattern, sizeof(pattern), "%.*s*", (int)prefix_len, ref_key.key);
    } else {
        /* Use first half of key name */
        size_t key_len = strlen(ref_key.key);
        size_t half_len = key_len / 2;
        if (half_len > 0) {
            snprintf(pattern, sizeof(pattern), "%.*s*", (int)half_len, ref_key.key);
        } else {
            /* Fallback to exact match */
            strncpy(pattern, ref_key.key, sizeof(pattern) - 1);
            pattern[sizeof(pattern) - 1] = '\0';
        }
    }
    
    /* Create query criteria for related items */
    memory_query_criteria_t criteria;
    init_query_criteria(&criteria);
    strncpy(criteria.key_pattern, pattern, MAX_TAG_SIZE - 1);
    criteria.key_pattern[MAX_TAG_SIZE - 1] = '\0';
    criteria.layer = (int)ref_key.layer; /* Same layer preference */
    criteria.max_results = max_results;
    
    /* Execute query */
    result_t query_result = tagged_memory_query(memory, &criteria, results, max_results, result_count);
    
    /* Remove the reference key itself from results */
    if (query_result == RESULT_OK && *result_count > 0) {
        size_t write_index = 0;
        for (size_t read_index = 0; read_index < *result_count; read_index++) {
            if (strcmp(results[read_index].key.key, reference_key) != 0) {
                if (write_index != read_index) {
                    results[write_index] = results[read_index];
                }
                write_index++;
            } else {
                /* Clean up data for removed result */
                data_destroy(&results[read_index].data);
            }
        }
        *result_count = write_index;
    }
    
    return query_result;
}

result_t memory_query_summary(tagged_memory_t* memory, const memory_query_criteria_t* criteria,
                             data_t* summary_buffer) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_query_summary");
        return RESULT_ERR;
    }
    
    if (!criteria) {
        RETURN_ERR("Null criteria parameter in memory_query_summary");
        return RESULT_ERR;
    }
    
    if (!summary_buffer) {
        RETURN_ERR("Null summary_buffer parameter in memory_query_summary");
        return RESULT_ERR;
    }
    
    /* Clear summary buffer */
    if (data_clear(summary_buffer) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Execute query to get matching keys */
    const size_t max_summary_results = 100;
    memory_query_result_t* results = malloc(max_summary_results * sizeof(memory_query_result_t));
    if (!results) {
        RETURN_ERR("Memory allocation failed in memory_query_summary");
        return RESULT_ERR;
    }
    
    size_t result_count;
    result_t query_result = tagged_memory_query(memory, criteria, results, max_summary_results, &result_count);
    
    if (query_result != RESULT_OK) {
        free(results);
        return query_result;
    }
    
    /* Build summary */
    char summary_header[256];
    snprintf(summary_header, sizeof(summary_header),
             "Memory Query Summary\n"
             "==================\n"
             "Pattern: %s\n"
             "Layer: %s\n"
             "Importance: %zu-%zu\n"
             "Results: %zu\n\n",
             criteria->key_pattern[0] ? criteria->key_pattern : "*",
             (criteria->layer < 0) ? "all" : 
             (criteria->layer == LAYER_WORKING) ? "working" :
             (criteria->layer == LAYER_DISK) ? "disk" : "archived",
             criteria->min_importance, criteria->max_importance,
             result_count);
    
    if (data_append(summary_buffer, summary_header, 0) != RESULT_OK) {
        /* Cleanup results */
        for (size_t i = 0; i < result_count; i++) {
            data_destroy(&results[i].data);
        }
        free(results);
        return RESULT_ERR;
    }
    
    /* Add individual result summaries */
    for (size_t i = 0; i < result_count; i++) {
        char result_summary[512];
        
        /* Format timestamp */
        struct tm* tm_info = localtime(&results[i].key.last_accessed);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        
        snprintf(result_summary, sizeof(result_summary),
                 "%zu. Key: %s\n"
                 "   Layer: %s | Importance: %zu | Size: %zu bytes\n"
                 "   Last Accessed: %s | Relevance: %zu%%\n"
                 "   Preview: %.100s%s\n\n",
                 i + 1, results[i].key.key,
                 (results[i].key.layer == LAYER_WORKING) ? "working" :
                 (results[i].key.layer == LAYER_DISK) ? "disk" : "archived",
                 results[i].key.importance_score, results[i].key.data_size,
                 time_str, results[i].relevance_score,
                 results[i].data.data ? results[i].data.data : "",
                 (results[i].data.size > 100) ? "..." : "");
        
        if (data_append(summary_buffer, result_summary, 0) != RESULT_OK) {
            break; /* Stop on error */
        }
    }
    
    /* Cleanup results */
    for (size_t i = 0; i < result_count; i++) {
        data_destroy(&results[i].data);
    }
    free(results);
    
    return RESULT_OK;
}

result_t memory_query_optimize(tagged_memory_t* memory, memory_query_criteria_t* criteria) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_query_optimize");
        return RESULT_ERR;
    }
    
    if (!criteria) {
        RETURN_ERR("Null criteria parameter in memory_query_optimize");
        return RESULT_ERR;
    }
    
    /* Optimize query criteria based on memory statistics */
    memory_stats_t stats;
    if (tagged_memory_get_stats(memory, &stats) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* If no pattern specified and many keys exist, suggest focusing on working memory */
    if (criteria->key_pattern[0] == '\0' && stats.context_key_count > 100) {
        criteria->layer = LAYER_WORKING;
    }
    
    /* If looking for recent data, adjust time range based on current activity */
    if (criteria->start_time == 0 && criteria->end_time == 0) {
        time_t current_time = time(NULL);
        
        /* Focus on data from last week for performance */
        if (stats.context_key_count > 500) {
            criteria->start_time = current_time - (7 * 24 * 3600);
        }
    }
    
    /* Limit results for performance if not specified */
    if (criteria->max_results == SIZE_MAX) {
        if (stats.context_key_count > 1000) {
            criteria->max_results = 50; /* Reasonable limit for large datasets */
        } else {
            criteria->max_results = 100;
        }
    }
    
    /* Adjust importance thresholds based on distribution */
    if (criteria->min_importance == 0 && criteria->max_importance == 100) {
        /* For large datasets, focus on higher importance items */
        if (stats.context_key_count > 200) {
            criteria->min_importance = 50;
        }
    }
    
    return RESULT_OK;
}

result_t tagged_memory_query(tagged_memory_t* memory, const memory_query_criteria_t* criteria,
                            memory_query_result_t* results, size_t max_results, size_t* result_count) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in tagged_memory_query");
        return RESULT_ERR;
    }
    
    if (!criteria) {
        RETURN_ERR("Null criteria parameter in tagged_memory_query");
        return RESULT_ERR;
    }
    
    if (!result_count) {
        RETURN_ERR("Null result_count parameter in tagged_memory_query");
        return RESULT_ERR;
    }
    
    if (max_results > 0 && !results) {
        RETURN_ERR("Null results parameter with non-zero max_results");
        return RESULT_ERR;
    }
    
    *result_count = 0;
    
    /* Determine effective maximum results */
    size_t effective_max = MIN(max_results, criteria->max_results);
    if (effective_max == 0) {
        return RESULT_OK; /* No results requested */
    }
    
    /* Collect matching results */
    size_t collected = 0;
    
    for (size_t i = 0; i < memory->context_key_count && collected < effective_max; i++) {
        const context_key_t* key = &memory->context_keys[i];
        
        /* Check if key matches criteria */
        if (!key_matches_criteria(key, criteria)) {
            continue;
        }
        
        /* Initialize result */
        memory_query_result_t* result = &results[collected];
        memset(result, 0, sizeof(memory_query_result_t));
        
        /* Copy key data */
        result->key = *key;
        
        /* Initialize data buffer */
        if (data_init(&result->data, 1024) != RESULT_OK) {
            continue; /* Skip this result on initialization failure */
        }
        
        /* Retrieve data */
        if (tagged_memory_retrieve(memory, key->key, &result->data) != RESULT_OK) {
            data_destroy(&result->data);
            continue; /* Skip this result on retrieval failure */
        }
        
        /* Calculate relevance score */
        result->relevance_score = calculate_relevance_score(key, criteria);
        
        collected++;
    }
    
    *result_count = collected;
    
    /* Sort results by relevance */
    if (collected > 1) {
        qsort(results, collected, sizeof(memory_query_result_t), compare_query_results);
    }
    
    return RESULT_OK;
}
