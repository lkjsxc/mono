// For usleep function
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "agent/core.h"

result_t agent_init(pool_t* pool, config_t* config, agent_t* agent) {
    // Initialize agent structure
    agent->status = config->agent_default_status;
    agent->iteration_count = 0;
    
    if (pool_json_object_alloc(pool, &agent->working_memory) != RESULT_OK) {
        RETURN_ERR("Failed to allocate working memory object");
    }
    
    if (pool_json_object_alloc(pool, &agent->storage) != RESULT_OK) {
        RETURN_ERR("Failed to allocate storage object");
    }

    return RESULT_OK;
}

result_t agent_step(pool_t* pool, config_t* config, agent_t* agent) {
    return RESULT_OK;
}

result_t agent_run(pool_t* pool, config_t* config, agent_t* agent) {
    printf("Starting agent with max iterations: %lu\n", config->agent_max_iterate);
    printf("Initial status: %d\n", (int)agent->status);

    while (agent->iteration_count < config->agent_max_iterate) {
        if (agent_step(pool, config, agent) != RESULT_OK) {
            RETURN_ERR("Agent step failed");
        }
        agent->iteration_count++;
    }

    return RESULT_OK;
}