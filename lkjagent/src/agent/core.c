// For usleep function
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "agent/core.h"

result_t agent_init(pool_t* pool, config_t* config, agent_t* agent) {
    if (json_create_object(pool, &agent->data) != RESULT_OK) {
        RETURN_ERR("Failed to create agent data object");
    }
    if(json_object_set_string(pool, agent->data, "state", config->agent_default_state->data) != RESULT_OK) {
        RETURN_ERR("Failed to set agent default state");
    }

    return RESULT_OK;
}

result_t agent_step(pool_t* pool, config_t* config, agent_t* agent) {
    string_t* response;
    if (pool_string_alloc(pool, &response, 256) != RESULT_OK) {
        RETURN_ERR("Failed to allocate response text string");
    }

    if (agent_request(pool, config, agent, &response) != RESULT_OK) {
        RETURN_ERR("Agent request failed");
    }

    if (agent_process(pool, config, agent, response) != RESULT_OK) {
        RETURN_ERR("Agent LLM response processing failed");
    }

    if (pool_string_free(pool, response) != RESULT_OK) {
        RETURN_ERR("Failed to free response text string");
    }

    return RESULT_OK;
}

result_t agent_run(pool_t* pool, config_t* config, agent_t* agent) {
    for (uint64_t i = 0; i < config->agent_max_iterate; i++) {
        if (agent_step(pool, config, agent) != RESULT_OK) {
            RETURN_ERR("Agent step failed");
        }
    }

    return RESULT_OK;
}