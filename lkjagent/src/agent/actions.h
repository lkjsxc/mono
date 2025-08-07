#ifndef LKJAGENT_AGENT_ACTIONS_H
#define LKJAGENT_AGENT_ACTIONS_H

#include "global/const.h"
#include "global/macro.h"
#include "global/std.h"
#include "global/types.h"
#include "utils/string.h"
#include "utils/pool.h"
#include "utils/object.h"

// Action execution functions
__attribute__((warn_unused_result)) result_t agent_actions_execute_working_memory_add(pool_t* pool, agent_t* agent, object_t* action);

__attribute__((warn_unused_result)) result_t agent_actions_execute_working_memory_remove(pool_t* pool, agent_t* agent, object_t* action);

__attribute__((warn_unused_result)) result_t agent_actions_execute_storage_load(pool_t* pool, agent_t* agent, object_t* action);

__attribute__((warn_unused_result)) result_t agent_actions_execute_storage_save(pool_t* pool, agent_t* agent, object_t* action);

// Response parsing and action dispatching
__attribute__((warn_unused_result)) result_t agent_actions_parse_response(pool_t* pool, const string_t* recv, object_t** response_obj);

__attribute__((warn_unused_result)) result_t agent_actions_dispatch(pool_t* pool, agent_t* agent, object_t* action_obj);

// Memory persistence
__attribute__((warn_unused_result)) result_t agent_actions_save_memory(pool_t* pool, agent_t* agent);

#endif
