// For usleep function
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "agent/core.h"

result_t agent_init(pool_t* pool, config_t* config, agent_t* agent) {
    // Initialize agent structure
    agent->status = config->agent_default_status;
    agent->iteration_count = 0;

    if (json_create_object(pool, &agent->working_memory) != RESULT_OK) {
        RETURN_ERR("Failed to create working memory object");
    }

    if (json_create_object(pool, &agent->storage) != RESULT_OK) {
        RETURN_ERR("Failed to create storage object");
    }

    return RESULT_OK;
}

result_t agent_step(pool_t* pool, config_t* config, agent_t* agent) {
    string_t* response_text;
    if (pool_string_alloc(pool, &response_text, 256) != RESULT_OK) {
        RETURN_ERR("Failed to allocate response text string");
    }

    if (agent_request(pool, config, agent, &response_text) != RESULT_OK) {
        RETURN_ERR("Agent request failed");
    }

    if (agent_process(pool, config, agent, response_text) != RESULT_OK) {
        RETURN_ERR("Agent LLM response processing failed");
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