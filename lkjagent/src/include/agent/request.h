#ifndef LKJAGENT_AGENT_REQUEST_H
#define LKJAGENT_AGENT_REQUEST_H

#include "macro.h"
#include "std.h"
#include "types.h"
#include "utils/lkjconfig.h"
#include "utils/lkjhttp.h"
#include "utils/lkjjson.h"
#include "utils/lkjpool.h"
#include "utils/lkjstring.h"

/**
 * Send a request to the LLM API and retrieve the response
 * @param pool Memory pool for allocations
 * @param config Configuration containing LLM settings
 * @param agent Agent instance containing current state
 * @param response_text Output parameter for the LLM response text
 * @return RESULT_OK on success, error code on failure
 */
__attribute__((warn_unused_result)) result_t agent_request(pool_t* pool, config_t* config, agent_t* agent, string_t** response_text);

#endif
