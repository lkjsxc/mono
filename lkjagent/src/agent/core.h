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

// Main agent processing function - orchestrates the full LLM interaction cycle
__attribute__((warn_unused_result)) result_t lkjagent_agent(pool_t* pool, config_t* config, agent_t* agent);

#endif
