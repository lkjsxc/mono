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

result_t agent_init(pool_t* pool, config_t* config, agent_t* agent);
result_t agent_step(pool_t* pool, config_t* config, agent_t* agent);
result_t agent_run(pool_t* pool, config_t* config, agent_t* agent);

#endif