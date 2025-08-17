#ifndef LKJAGENT_H
#define LKJAGENT_H

#include "lkjlib/lkjlib.h"

#define CONFIG_PATH "/data/config.json"
#define MEMORY_PATH "/data/memory.json"
#define MAX_TAGS 32

typedef struct lkjagent_t {
    object_t* config;
    object_t* memory;
} lkjagent_t;

__attribute__((warn_unused_result)) result_t lkjagent_request(pool_t* pool, lkjagent_t* lkjagent, data_t** dst);
__attribute__((warn_unused_result)) result_t lkjagent_process(pool_t* pool, lkjagent_t* lkjagent, data_t* recv, uint64_t iteration);
__attribute__((warn_unused_result)) result_t lkjagent_action(pool_t* pool, lkjagent_t* lkjagent, object_t* action, uint64_t iteration);

// Action function declarations
__attribute__((warn_unused_result)) result_t lkjagent_action_working_memory_add(pool_t* pool, lkjagent_t* lkjagent, data_t* tags, data_t* value, uint64_t iteration);
__attribute__((warn_unused_result)) result_t lkjagent_action_working_memory_remove(pool_t* pool, lkjagent_t* lkjagent, data_t* tags);
__attribute__((warn_unused_result)) result_t lkjagent_action_storage_save(pool_t* pool, lkjagent_t* lkjagent, data_t* tags, data_t* value, uint64_t iteration);
__attribute__((warn_unused_result)) result_t lkjagent_action_storage_load(pool_t* pool, lkjagent_t* lkjagent, data_t* tags, uint64_t iteration);
__attribute__((warn_unused_result)) result_t lkjagent_action_storage_search(pool_t* pool, lkjagent_t* lkjagent, data_t* tags, data_t* value, uint64_t iteration);

// Tag utility functions
__attribute__((warn_unused_result)) result_t sort_tags(pool_t* pool, const data_t* input_tags, data_t** sorted_tags);
__attribute__((warn_unused_result)) result_t tags_sort(pool_t* pool, data_t** sorted_tags, const data_t* unsorted_tags);
__attribute__((warn_unused_result)) result_t tags_array_to_string(pool_t* pool, data_t** tags_array, data_t** output);

#endif