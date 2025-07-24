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

static result_t agent_request(pool_t* pool, config_t* config, agent_t* agent, string_t** response_text) {
    return RESULT_OK;
}

static result_t agent_execute(pool_t* pool, config_t* config, agent_t* agent, const string_t* response_text) {
    return RESULT_OK;
}

result_t agent_step(pool_t* pool, config_t* config, agent_t* agent) {
    string_t* response_text;
    if(pool_string_alloc(pool, &response_text, 1048576) != RESULT_OK) {
        RETURN_ERR("Failed to allocate response text string");
    }

    if (agent_request(pool, config, agent, &response_text) != RESULT_OK) {
        RETURN_ERR("Agent request failed");
    }

    if (agent_execute(pool, config, agent, response_text) != RESULT_OK) {
        RETURN_ERR("Agent execution failed");
    }

    if (pool_string_free(pool, response_text) != RESULT_OK) {
        RETURN_ERR("Failed to free response text string");
    }

    return RESULT_OK;
}

result_t agent_run(pool_t* pool, config_t* config, agent_t* agent) {
    while (agent->iteration_count < config->agent_max_iterate) {
        if (agent_step(pool, config, agent) != RESULT_OK) {
            RETURN_ERR("Agent step failed");
        }
        agent->iteration_count++;
    }

    return RESULT_OK;
}