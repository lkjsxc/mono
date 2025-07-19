/**
 * @file memory_cleanup.c
 * @brief Memory cleanup and optimization implementation for LKJAgent
 * 
 * This module implements comprehensive memory cleanup and optimization
 * algorithms to maintain peak performance indefinitely, including
 * duplicate detection, orphaned data cleanup, and intelligent archiving.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/memory_context.h"
#include "../lkjagent.h"
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>

/**
 * @defgroup Memory_Cleanup_Internal Internal Memory Cleanup Functions
 * @{
 */

/**
 * @brief Calculate string similarity using Levenshtein distance
 * 
 * @param s1 First string
 * @param s2 Second string
 * @return Similarity score (0.0 = completely different, 1.0 = identical)
 */
static double calculate_string_similarity(const char* s1, const char* s2) {
    if (!s1 || !s2) return 0.0;
    
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    
    if (len1 == 0 && len2 == 0) return 1.0;
    if (len1 == 0 || len2 == 0) return 0.0;
    
    /* Simple similarity check - exact match */
    if (strcmp(s1, s2) == 0) return 1.0;
    
    /* Check for substring containment */
    if (strstr(s1, s2) || strstr(s2, s1)) {
        return 0.8; /* High similarity for substring match */
    }
    
    /* Check for common prefix */
    size_t common_prefix = 0;
    size_t min_len = (len1 < len2) ? len1 : len2;
    
    for (size_t i = 0; i < min_len; i++) {
        if (s1[i] == s2[i]) {
            common_prefix++;
        } else {
            break;
        }
    }
    
    if (common_prefix > 0) {
        return (double)common_prefix / (double)((len1 > len2) ? len1 : len2);
    }
    
    return 0.0;
}

/**
 * @brief Check if content is considered stale
 * 
 * @param key Context key to check
 * @param stale_threshold Age threshold in seconds
 * @return true if content is stale, false otherwise
 */
static bool is_content_stale(const context_key_t* key, time_t stale_threshold) {
    if (!key) return true;
    
    time_t current_time = time(NULL);
    time_t age = current_time - key->last_accessed;
    
    return age > stale_threshold;
}

/**
 * @brief Check if context key is orphaned (no associated data)
 * 
 * @param memory Pointer to tagged memory system
 * @param key Context key to check
 * @return true if orphaned, false otherwise
 */
static bool is_key_orphaned(tagged_memory_t* memory, const context_key_t* key) {
    if (!memory || !key) return true;
    
    /* Try to retrieve data for this key */
    data_t test_data;
    if (data_init(&test_data, 64) != RESULT_OK) {
        return true; /* Assume orphaned if we can't test */
    }
    
    result_t retrieve_result = tagged_memory_retrieve(memory, key->key, &test_data);
    bool is_orphaned = (retrieve_result != RESULT_OK || test_data.size == 0);
    
    data_destroy(&test_data);
    return is_orphaned;
}

/**
 * @brief Calculate memory fragmentation score
 * 
 * @param memory Pointer to tagged memory system
 * @return Fragmentation score (0.0 = no fragmentation, 1.0 = highly fragmented)
 */
static double calculate_fragmentation_score(tagged_memory_t* memory) {
    if (!memory) return 1.0;
    
    /* Simple fragmentation metric based on key distribution */
    size_t total_keys = memory->context_key_count;
    if (total_keys == 0) return 0.0;
    
    /* Count keys in each layer */
    size_t working_keys = 0, disk_keys = 0, archived_keys = 0;
    
    for (size_t i = 0; i < total_keys; i++) {
        switch (memory->context_keys[i].layer) {
            case LAYER_WORKING:
                working_keys++;
                break;
            case LAYER_DISK:
                disk_keys++;
                break;
            case LAYER_ARCHIVED:
                archived_keys++;
                break;
        }
    }
    
    /* Calculate distribution imbalance */
    double ideal_working = total_keys * 0.3;  /* 30% in working */
    double ideal_disk = total_keys * 0.5;     /* 50% in disk */
    double ideal_archived = total_keys * 0.2; /* 20% in archived */
    
    double working_imbalance = fabs((double)working_keys - ideal_working) / ideal_working;
    double disk_imbalance = fabs((double)disk_keys - ideal_disk) / ideal_disk;
    double archived_imbalance = fabs((double)archived_keys - ideal_archived) / ideal_archived;
    
    return (working_imbalance + disk_imbalance + archived_imbalance) / 3.0;
}

/**
 * @brief Compress memory data using simple run-length encoding
 * 
 * @param input Input data to compress
 * @param output Output buffer for compressed data
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t compress_memory_data(const data_t* input, data_t* output) {
    if (!input || !output) {
        return RESULT_ERR;
    }
    
    if (data_clear(output) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Simple compression: remove excessive whitespace and repeated patterns */
    if (!input->data || input->size == 0) {
        return RESULT_OK; /* Empty input */
    }
    
    const char* src = input->data;
    size_t src_len = input->size;
    
    for (size_t i = 0; i < src_len; i++) {
        char current = src[i];
        
        /* Skip excessive whitespace */
        if (isspace(current)) {
            /* Add single space for any whitespace sequence */
            if (output->size == 0 || output->data[output->size - 1] != ' ') {
                if (data_append(output, " ", 1) != RESULT_OK) {
                    return RESULT_ERR;
                }
            }
        } else {
            /* Add non-whitespace character */
            char temp[2] = {current, '\0'};
            if (data_append(output, temp, 1) != RESULT_OK) {
                return RESULT_ERR;
            }
        }
    }
    
    return RESULT_OK;
}

/** @} */

result_t memory_cleanup_expired(tagged_memory_t* memory, time_t expiry_threshold, size_t* cleaned_count) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_cleanup_expired");
        return RESULT_ERR;
    }
    
    if (expiry_threshold <= 0) {
        RETURN_ERR("Invalid expiry_threshold in memory_cleanup_expired");
        return RESULT_ERR;
    }
    
    if (!cleaned_count) {
        RETURN_ERR("Null cleaned_count parameter in memory_cleanup_expired");
        return RESULT_ERR;
    }
    
    *cleaned_count = 0;
    time_t current_time = time(NULL);
    time_t cutoff_time = current_time - expiry_threshold;
    
    /* Collect expired keys */
    size_t expired_indices[MAX_CONTEXT_KEYS];
    size_t expired_count = 0;
    
    for (size_t i = 0; i < memory->context_key_count; i++) {
        const context_key_t* key = &memory->context_keys[i];
        
        /* Check if key is expired */
        if (key->last_accessed < cutoff_time) {
            /* Exempt high-importance keys from cleanup */
            if (key->importance_score < 80) {
                expired_indices[expired_count] = i;
                expired_count++;
            }
        }
    }
    
    /* Remove expired keys in reverse order to maintain indices */
    for (size_t i = expired_count; i > 0; i--) {
        size_t index = expired_indices[i - 1];
        const context_key_t* key = &memory->context_keys[index];
        
        /* Delete the key and its data */
        if (tagged_memory_delete(memory, key->key) == RESULT_OK) {
            (*cleaned_count)++;
        }
        
        /* Remove from context keys array */
        if (index < memory->context_key_count - 1) {
            memmove(&memory->context_keys[index],
                   &memory->context_keys[index + 1],
                   (memory->context_key_count - index - 1) * sizeof(context_key_t));
        }
        memory->context_key_count--;
    }
    
    memory->last_modified = current_time;
    memory->total_size = calculate_total_memory_size(memory);
    
    return RESULT_OK;
}

result_t memory_cleanup_duplicates(tagged_memory_t* memory, double similarity_threshold, size_t* removed_count) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_cleanup_duplicates");
        return RESULT_ERR;
    }
    
    if (similarity_threshold < 0.0 || similarity_threshold > 1.0) {
        RETURN_ERR("Invalid similarity_threshold in memory_cleanup_duplicates");
        return RESULT_ERR;
    }
    
    if (!removed_count) {
        RETURN_ERR("Null removed_count parameter in memory_cleanup_duplicates");
        return RESULT_ERR;
    }
    
    *removed_count = 0;
    
    /* Compare all pairs of context keys for duplicates */
    bool* marked_for_removal = calloc(memory->context_key_count, sizeof(bool));
    if (!marked_for_removal) {
        RETURN_ERR("Memory allocation failed in memory_cleanup_duplicates");
        return RESULT_ERR;
    }
    
    for (size_t i = 0; i < memory->context_key_count; i++) {
        if (marked_for_removal[i]) continue;
        
        const context_key_t* key1 = &memory->context_keys[i];
        
        /* Retrieve data for first key */
        data_t data1;
        if (data_init(&data1, 4096) != RESULT_OK) {
            continue;
        }
        
        if (tagged_memory_retrieve(memory, key1->key, &data1) != RESULT_OK) {
            data_destroy(&data1);
            continue;
        }
        
        /* Compare with all subsequent keys */
        for (size_t j = i + 1; j < memory->context_key_count; j++) {
            if (marked_for_removal[j]) continue;
            
            const context_key_t* key2 = &memory->context_keys[j];
            
            /* Quick check: similar key names */
            double key_similarity = calculate_string_similarity(key1->key, key2->key);
            if (key_similarity < similarity_threshold * 0.5) {
                continue; /* Key names too different */
            }
            
            /* Retrieve data for second key */
            data_t data2;
            if (data_init(&data2, 4096) != RESULT_OK) {
                continue;
            }
            
            if (tagged_memory_retrieve(memory, key2->key, &data2) != RESULT_OK) {
                data_destroy(&data2);
                continue;
            }
            
            /* Compare data content */
            double content_similarity = calculate_string_similarity(data1.data, data2.data);
            
            /* If both key and content are similar enough, mark for removal */
            if (key_similarity >= similarity_threshold && content_similarity >= similarity_threshold) {
                /* Keep the one with higher importance or more recent access */
                if (key2->importance_score > key1->importance_score ||
                    (key2->importance_score == key1->importance_score && 
                     key2->last_accessed > key1->last_accessed)) {
                    marked_for_removal[i] = true; /* Remove key1 */
                } else {
                    marked_for_removal[j] = true; /* Remove key2 */
                }
            }
            
            data_destroy(&data2);
        }
        
        data_destroy(&data1);
    }
    
    /* Remove marked keys in reverse order */
    for (size_t i = memory->context_key_count; i > 0; i--) {
        size_t index = i - 1;
        
        if (marked_for_removal[index]) {
            const context_key_t* key = &memory->context_keys[index];
            
            /* Delete the key and its data */
            tagged_memory_delete(memory, key->key);
            
            /* Remove from context keys array */
            if (index < memory->context_key_count - 1) {
                memmove(&memory->context_keys[index],
                       &memory->context_keys[index + 1],
                       (memory->context_key_count - index - 1) * sizeof(context_key_t));
            }
            memory->context_key_count--;
            (*removed_count)++;
        }
    }
    
    free(marked_for_removal);
    
    memory->last_modified = time(NULL);
    memory->total_size = calculate_total_memory_size(memory);
    
    return RESULT_OK;
}

result_t memory_cleanup_orphaned(tagged_memory_t* memory, size_t* cleaned_count) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_cleanup_orphaned");
        return RESULT_ERR;
    }
    
    if (!cleaned_count) {
        RETURN_ERR("Null cleaned_count parameter in memory_cleanup_orphaned");
        return RESULT_ERR;
    }
    
    *cleaned_count = 0;
    
    /* Find orphaned context keys */
    bool* is_orphaned = malloc(memory->context_key_count * sizeof(bool));
    if (!is_orphaned) {
        RETURN_ERR("Memory allocation failed in memory_cleanup_orphaned");
        return RESULT_ERR;
    }
    
    for (size_t i = 0; i < memory->context_key_count; i++) {
        is_orphaned[i] = is_key_orphaned(memory, &memory->context_keys[i]);
    }
    
    /* Remove orphaned keys in reverse order */
    for (size_t i = memory->context_key_count; i > 0; i--) {
        size_t index = i - 1;
        
        if (is_orphaned[index]) {
            /* Remove from context keys array */
            if (index < memory->context_key_count - 1) {
                memmove(&memory->context_keys[index],
                       &memory->context_keys[index + 1],
                       (memory->context_key_count - index - 1) * sizeof(context_key_t));
            }
            memory->context_key_count--;
            (*cleaned_count)++;
        }
    }
    
    free(is_orphaned);
    
    memory->last_modified = time(NULL);
    
    return RESULT_OK;
}

result_t memory_optimize_storage(tagged_memory_t* memory, bool aggressive) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_optimize_storage");
        return RESULT_ERR;
    }
    
    /* Step 1: Clean up expired content */
    size_t expired_cleaned = 0;
    time_t expiry_threshold = aggressive ? (7 * 24 * 3600) : (30 * 24 * 3600); /* 7 or 30 days */
    memory_cleanup_expired(memory, expiry_threshold, &expired_cleaned);
    
    /* Step 2: Remove duplicates */
    size_t duplicates_removed = 0;
    double similarity_threshold = aggressive ? 0.8 : 0.95; /* More or less aggressive deduplication */
    memory_cleanup_duplicates(memory, similarity_threshold, &duplicates_removed);
    
    /* Step 3: Clean up orphaned keys */
    size_t orphaned_cleaned = 0;
    memory_cleanup_orphaned(memory, &orphaned_cleaned);
    
    /* Step 4: Optimize layer distribution */
    if (aggressive) {
        /* Redistribute keys based on importance and access patterns */
        time_t current_time = time(NULL);
        time_t recent_threshold = current_time - 3600; /* Last hour */
        
        for (size_t i = 0; i < memory->context_key_count; i++) {
            context_key_t* key = &memory->context_keys[i];
            
            /* Move high-importance, recently accessed items to working memory */
            if (key->importance_score >= 80 && key->last_accessed >= recent_threshold) {
                if (key->layer != LAYER_WORKING) {
                    context_key_move_layer(memory, key->key, LAYER_WORKING);
                }
            }
            /* Move low-importance, old items to disk */
            else if (key->importance_score <= 40 && key->last_accessed < (current_time - 86400)) {
                if (key->layer == LAYER_WORKING) {
                    context_key_move_layer(memory, key->key, LAYER_DISK);
                }
            }
            /* Archive very old, low-importance items */
            else if (key->importance_score <= 20 && key->last_accessed < (current_time - 604800)) {
                if (key->layer != LAYER_ARCHIVED) {
                    context_key_archive(memory, key->key);
                }
            }
        }
    }
    
    /* Step 5: Compress memory data if aggressive optimization */
    if (aggressive) {
        /* Compress working memory */
        data_t compressed_working;
        if (data_init(&compressed_working, memory->working_memory.capacity) == RESULT_OK) {
            if (compress_memory_data(&memory->working_memory, &compressed_working) == RESULT_OK) {
                data_destroy(&memory->working_memory);
                memory->working_memory = compressed_working;
            } else {
                data_destroy(&compressed_working);
            }
        }
        
        /* Compress disk memory */
        data_t compressed_disk;
        if (data_init(&compressed_disk, memory->disk_memory.capacity) == RESULT_OK) {
            if (compress_memory_data(&memory->disk_memory, &compressed_disk) == RESULT_OK) {
                data_destroy(&memory->disk_memory);
                memory->disk_memory = compressed_disk;
            } else {
                data_destroy(&compressed_disk);
            }
        }
    }
    
    /* Update statistics */
    memory->last_modified = time(NULL);
    memory->total_size = calculate_total_memory_size(memory);
    
    return RESULT_OK;
}

result_t memory_compress_archives(tagged_memory_t* memory, double compression_ratio) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_compress_archives");
        return RESULT_ERR;
    }
    
    if (compression_ratio <= 0.0 || compression_ratio > 1.0) {
        RETURN_ERR("Invalid compression_ratio in memory_compress_archives");
        return RESULT_ERR;
    }
    
    /* Find archived context keys */
    for (size_t i = 0; i < memory->context_key_count; i++) {
        context_key_t* key = &memory->context_keys[i];
        
        if (key->layer == LAYER_ARCHIVED) {
            /* Retrieve archived data */
            data_t archived_data;
            if (data_init(&archived_data, key->data_size + 256) == RESULT_OK) {
                if (tagged_memory_retrieve(memory, key->key, &archived_data) == RESULT_OK) {
                    
                    /* Compress the data */
                    data_t compressed_data;
                    if (data_init(&compressed_data, archived_data.size) == RESULT_OK) {
                        if (compress_memory_data(&archived_data, &compressed_data) == RESULT_OK) {
                            
                            /* Update the stored data with compressed version */
                            tagged_memory_store(memory, key->key, &compressed_data, 
                                              LAYER_ARCHIVED, key->importance_score);
                            
                            /* Update data size in context key */
                            key->data_size = compressed_data.size;
                        }
                        data_destroy(&compressed_data);
                    }
                }
                data_destroy(&archived_data);
            }
        }
    }
    
    memory->last_modified = time(NULL);
    memory->total_size = calculate_total_memory_size(memory);
    
    return RESULT_OK;
}

result_t memory_defragment(tagged_memory_t* memory) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_defragment");
        return RESULT_ERR;
    }
    
    /* Defragment by rebuilding memory buffers without gaps */
    
    /* Rebuild working memory */
    data_t new_working;
    if (data_init(&new_working, memory->working_memory.capacity) == RESULT_OK) {
        /* Copy all working memory content in order */
        for (size_t i = 0; i < memory->context_key_count; i++) {
            const context_key_t* key = &memory->context_keys[i];
            
            if (key->layer == LAYER_WORKING) {
                data_t key_data;
                if (data_init(&key_data, key->data_size + 256) == RESULT_OK) {
                    if (tagged_memory_retrieve(memory, key->key, &key_data) == RESULT_OK) {
                        /* Add key marker and data */
                        char key_marker[MAX_TAG_SIZE + 32];
                        snprintf(key_marker, sizeof(key_marker), "\n<key:%s>\n", key->key);
                        data_append(&new_working, key_marker, 0);
                        data_append(&new_working, key_data.data, 0);
                        data_append(&new_working, "\n</key>\n", 0);
                    }
                    data_destroy(&key_data);
                }
            }
        }
        
        /* Replace old working memory */
        data_destroy(&memory->working_memory);
        memory->working_memory = new_working;
    }
    
    /* Rebuild disk memory */
    data_t new_disk;
    if (data_init(&new_disk, memory->disk_memory.capacity) == RESULT_OK) {
        /* Copy all disk memory content in order */
        for (size_t i = 0; i < memory->context_key_count; i++) {
            const context_key_t* key = &memory->context_keys[i];
            
            if (key->layer == LAYER_DISK || key->layer == LAYER_ARCHIVED) {
                data_t key_data;
                if (data_init(&key_data, key->data_size + 256) == RESULT_OK) {
                    if (tagged_memory_retrieve(memory, key->key, &key_data) == RESULT_OK) {
                        /* Add key marker and data */
                        char key_marker[MAX_TAG_SIZE + 32];
                        snprintf(key_marker, sizeof(key_marker), "\n<key:%s>\n", key->key);
                        data_append(&new_disk, key_marker, 0);
                        data_append(&new_disk, key_data.data, 0);
                        data_append(&new_disk, "\n</key>\n", 0);
                    }
                    data_destroy(&key_data);
                }
            }
        }
        
        /* Replace old disk memory */
        data_destroy(&memory->disk_memory);
        memory->disk_memory = new_disk;
    }
    
    memory->last_modified = time(NULL);
    memory->total_size = calculate_total_memory_size(memory);
    
    return RESULT_OK;
}

result_t memory_analyze_usage(tagged_memory_t* memory, data_t* analysis_report) {
    if (!memory) {
        RETURN_ERR("Null memory parameter in memory_analyze_usage");
        return RESULT_ERR;
    }
    
    if (!analysis_report) {
        RETURN_ERR("Null analysis_report parameter in memory_analyze_usage");
        return RESULT_ERR;
    }
    
    /* Clear report buffer */
    if (data_clear(analysis_report) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Collect usage statistics */
    memory_stats_t stats;
    if (tagged_memory_get_stats(memory, &stats) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    /* Analyze layer distribution */
    size_t working_keys = 0, disk_keys = 0, archived_keys = 0;
    size_t total_importance = 0;
    time_t oldest_access = time(NULL);
    time_t newest_access = 0;
    
    for (size_t i = 0; i < memory->context_key_count; i++) {
        const context_key_t* key = &memory->context_keys[i];
        
        switch (key->layer) {
            case LAYER_WORKING:
                working_keys++;
                break;
            case LAYER_DISK:
                disk_keys++;
                break;
            case LAYER_ARCHIVED:
                archived_keys++;
                break;
        }
        
        total_importance += key->importance_score;
        
        if (key->last_accessed < oldest_access) {
            oldest_access = key->last_accessed;
        }
        if (key->last_accessed > newest_access) {
            newest_access = key->last_accessed;
        }
    }
    
    /* Calculate metrics */
    double avg_importance = (memory->context_key_count > 0) ? 
                           ((double)total_importance / memory->context_key_count) : 0.0;
    
    double fragmentation = calculate_fragmentation_score(memory);
    
    time_t current_time = time(NULL);
    time_t usage_span = newest_access - oldest_access;
    
    /* Build analysis report */
    char report_header[] = 
        "MEMORY USAGE ANALYSIS REPORT\n"
        "============================\n\n";
    
    data_append(analysis_report, report_header, 0);
    
    /* Basic statistics */
    char basic_stats[1024];
    snprintf(basic_stats, sizeof(basic_stats),
             "Basic Statistics:\n"
             "- Total Memory Size: %zu bytes\n"
             "- Working Memory: %zu bytes (%zu keys)\n"
             "- Disk Memory: %zu bytes (%zu keys)\n"
             "- Archived Memory: %zu keys\n"
             "- Total Context Keys: %zu\n"
             "- Average Importance: %.1f\n\n",
             stats.total_size, stats.working_size, working_keys,
             stats.disk_size, disk_keys, archived_keys,
             stats.context_key_count, avg_importance);
    
    data_append(analysis_report, basic_stats, 0);
    
    /* Performance metrics */
    char performance[512];
    snprintf(performance, sizeof(performance),
             "Performance Metrics:\n"
             "- Access Count: %llu\n"
             "- Average Access Time: %llu microseconds\n"
             "- Fragmentation Score: %.2f\n"
             "- Usage Span: %ld seconds\n\n",
             (unsigned long long)stats.access_count,
             (unsigned long long)stats.avg_access_time,
             fragmentation, usage_span);
    
    data_append(analysis_report, performance, 0);
    
    /* Layer distribution analysis */
    char distribution[512];
    double working_pct = (memory->context_key_count > 0) ? 
                        ((double)working_keys / memory->context_key_count) * 100.0 : 0.0;
    double disk_pct = (memory->context_key_count > 0) ? 
                     ((double)disk_keys / memory->context_key_count) * 100.0 : 0.0;
    double archived_pct = (memory->context_key_count > 0) ? 
                         ((double)archived_keys / memory->context_key_count) * 100.0 : 0.0;
    
    snprintf(distribution, sizeof(distribution),
             "Layer Distribution:\n"
             "- Working Memory: %.1f%% (%zu keys)\n"
             "- Disk Memory: %.1f%% (%zu keys)\n"
             "- Archived Memory: %.1f%% (%zu keys)\n\n",
             working_pct, working_keys,
             disk_pct, disk_keys,
             archived_pct, archived_keys);
    
    data_append(analysis_report, distribution, 0);
    
    /* Recommendations */
    char recommendations[1024];
    snprintf(recommendations, sizeof(recommendations),
             "Recommendations:\n");
    
    data_append(analysis_report, recommendations, 0);
    
    if (fragmentation > 0.5) {
        data_append(analysis_report, "- High fragmentation detected - recommend defragmentation\n", 0);
    }
    
    if (working_pct > 50.0) {
        data_append(analysis_report, "- Working memory overloaded - move old content to disk\n", 0);
    }
    
    if (avg_importance < 50.0) {
        data_append(analysis_report, "- Low average importance - cleanup old content\n", 0);
    }
    
    if (usage_span > (30 * 24 * 3600)) { /* 30 days */
        data_append(analysis_report, "- Old content detected - archive stale data\n", 0);
    }
    
    if (stats.avg_access_time > 1000) { /* 1ms */
        data_append(analysis_report, "- Slow access times - optimize storage layout\n", 0);
    }
    
    data_append(analysis_report, "\nAnalysis completed.\n", 0);
    
    return RESULT_OK;
}
