#ifndef LKJAGENT_AGENT_STATUS_H
#define LKJAGENT_AGENT_STATUS_H

#include "macro.h"
#include "std.h"
#include "types.h"
#include "utils/lkjjson.h"

/**
 * Process status change operations from agent response
 * @param agent Agent instance to modify
 * @param status_change JSON string containing new status
 * @return RESULT_OK on success, error code on failure
 */
__attribute__((warn_unused_result)) result_t agent_status_change(agent_t* agent, json_value_t* status_change);

#endif
