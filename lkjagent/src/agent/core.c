// For usleep function
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "agent/core.h"

result_t agent_init(config_t* config, agent_t* agent) {
    // Initialize agent structure
    agent->status = config->agent_default_status;
    agent->iteration_count = 0;

    return RESULT_OK;
}

result_t agent_run(pool_t* pool, config_t* config, agent_t* agent) {

    while(agent->iteration_count < config->agent_max_iterate) {
        
        // Generate request json
        json_value_t* request_json = NULL;
        if(json_create_object(pool, &request_json) != RESULT_OK) {
            RETURN_ERR("Failed to create request JSON object");
        }



        agent->iteration_count++;
    }

    return RESULT_OK;
}