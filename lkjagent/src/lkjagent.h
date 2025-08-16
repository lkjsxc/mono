#ifndef LKJAGENT_H
#define LKJAGENT_H

#include "lkjlib/lkjlib.h"

#define CONFIG_PATH "/data/config.json"
#define MEMORY_PATH "/data/memory.json"

typedef struct lkjagent_t {
    object_t* config;
    object_t* memory;
} lkjagent_t;

__attribute__((warn_unused_result)) result_t lkjagent_request(pool_t* pool, lkjagent_t* lkjagent, data_t** dst);

#endif