#ifndef LKJAGENT_AGENT_RESPONSE_PARSER_H
#define LKJAGENT_AGENT_RESPONSE_PARSER_H

#include "macro.h"
#include "std.h"
#include "types.h"
#include "utils/lkjjson.h"
#include "utils/lkjpool.h"
#include "utils/lkjstring.h"

/**
 * Parse agent response text and extract JSON payload after </think> tag
 * @param pool Memory pool for allocations
 * @param response_text Raw response text from LLM
 * @param response_json Output parameter for parsed JSON object
 * @return RESULT_OK on success, error code on failure
 */
__attribute__((warn_unused_result)) result_t agent_parse_response(pool_t* pool, const string_t* response_text, json_value_t** response_json);

#endif
