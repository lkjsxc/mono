#ifndef LKJAGENT_AGENT_STATE_H
#define LKJAGENT_AGENT_STATE_H

#include "global/const.h"
#include "global/macro.h"
#include "global/std.h"
#include "global/types.h"
#include "utils/string.h"
#include "utils/pool.h"
#include "utils/object.h"

// State management functions
__attribute__((warn_unused_result)) result_t agent_state_auto_transition(pool_t* pool, config_t* config, agent_t* agent);

__attribute__((warn_unused_result)) result_t agent_state_update_and_log(pool_t* pool, agent_t* agent, object_t* response_obj);

// Helper function for token estimation
size_t agent_state_estimate_tokens(const object_t* working_memory);

#endif
