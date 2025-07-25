#ifndef LKJAGENT_AGENT_PROCESS_H
#define LKJAGENT_AGENT_PROCESS_H

#include "macro.h"
#include "std.h"
#include "types.h"
#include "utils/lkjpool.h"
#include "utils/lkjstring.h"
#include "agent/response_parser.h"
#include "agent/working_memory.h"
#include "agent/storage.h"

/**
 * Process LLM response data and apply operations to agent state
 * This function parses the LLM response text and processes various operations
 * such as working memory modifications, storage operations, and status changes.
 * 
 * Supports both new action-based format with tags and legacy direct operations format.
 * 
 * @param pool Memory pool for allocations
 * @param config Configuration (currently unused but kept for future extensions)
 * @param agent Agent instance to modify
 * @param response_text Raw response text from LLM containing JSON operations
 * @return RESULT_OK on success, error code on failure
 */
__attribute__((warn_unused_result)) result_t agent_process(pool_t* pool, config_t* config, agent_t* agent, const string_t* response_text);

#endif
