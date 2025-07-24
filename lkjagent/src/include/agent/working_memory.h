#ifndef LKJAGENT_AGENT_WORKING_MEMORY_H
#define LKJAGENT_AGENT_WORKING_MEMORY_H

#include "macro.h"
#include "std.h"
#include "types.h"
#include "utils/lkjjson.h"
#include "utils/lkjpool.h"

/**
 * Process working memory add operations from agent response
 * @param pool Memory pool for allocations
 * @param agent Agent instance to modify
 * @param working_memory_add JSON object containing items to add
 * @return RESULT_OK on success, error code on failure
 */
__attribute__((warn_unused_result)) result_t agent_working_memory_add(pool_t* pool, agent_t* agent, json_value_t* working_memory_add);

/**
 * Process working memory remove operations from agent response
 * @param pool Memory pool for allocations
 * @param agent Agent instance to modify
 * @param working_memory_remove JSON value containing keys to remove (string or array)
 * @return RESULT_OK on success, error code on failure
 */
__attribute__((warn_unused_result)) result_t agent_working_memory_remove(pool_t* pool, agent_t* agent, json_value_t* working_memory_remove);

#endif
