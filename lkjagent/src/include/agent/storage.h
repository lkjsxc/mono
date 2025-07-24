#ifndef LKJAGENT_AGENT_STORAGE_H
#define LKJAGENT_AGENT_STORAGE_H

#include "macro.h"
#include "std.h"
#include "types.h"
#include "utils/lkjjson.h"
#include "utils/lkjpool.h"

/**
 * Process storage add operations from agent response
 * @param pool Memory pool for allocations
 * @param agent Agent instance to modify
 * @param storage_add JSON object containing items to add
 * @return RESULT_OK on success, error code on failure
 */
__attribute__((warn_unused_result)) result_t agent_storage_add(pool_t* pool, agent_t* agent, json_value_t* storage_add);

/**
 * Process storage remove operations from agent response
 * @param pool Memory pool for allocations
 * @param agent Agent instance to modify
 * @param storage_remove JSON value containing keys to remove (string or array)
 * @return RESULT_OK on success, error code on failure
 */
__attribute__((warn_unused_result)) result_t agent_storage_remove(pool_t* pool, agent_t* agent, json_value_t* storage_remove);

#endif
