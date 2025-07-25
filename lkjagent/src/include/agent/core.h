#ifndef LKJAGENT_AGENT_CORE_H
#define LKJAGENT_AGENT_CORE_H

#include "macro.h"
#include "std.h"
#include "types.h"
#include "utils/lkjconfig.h"
#include "utils/lkjhttp.h"
#include "utils/lkjjson.h"
#include "utils/lkjpool.h"
#include "utils/lkjstring.h"
#include "agent/response_parser.h"
#include "agent/working_memory.h"
#include "agent/storage.h"
#include "agent/request.h"
#include "agent/process.h"

__attribute__((warn_unused_result)) result_t agent_init(pool_t* pool, config_t* config, agent_t* agent);
__attribute__((warn_unused_result)) result_t agent_step(pool_t* pool, config_t* config, agent_t* agent);
__attribute__((warn_unused_result)) result_t agent_run(pool_t* pool, config_t* config, agent_t* agent);

#endif