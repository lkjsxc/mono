#ifndef LKJAGENT_AGENT_CORE_H
#define LKJAGENT_AGENT_CORE_H

#include "global/const.h"
#include "global/macro.h"
#include "global/std.h"
#include "global/types.h"
#include "utils/string.h"
#include "utils/pool.h"
#include "utils/object.h"
#include "utils/http.h"

// Import sub-modules
#include "agent/state.h"
#include "agent/prompt.h"
#include "agent/http.h"
#include "agent/actions.h"

// Main agent processing function - orchestrates the full LLM interaction cycle
__attribute__((warn_unused_result)) result_t lkjagent_agent(pool_t* pool, config_t* config, agent_t* agent);

// Main execution function that processes LLM responses and executes agent actions
__attribute__((warn_unused_result)) result_t lkjagent_agent_execute(pool_t* pool, config_t* config, agent_t* agent, const string_t* recv);

#endif
