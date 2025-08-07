#ifndef LKJAGENT_AGENT_PROMPT_H
#define LKJAGENT_AGENT_PROMPT_H

#include "global/const.h"
#include "global/macro.h"
#include "global/std.h"
#include "global/types.h"
#include "utils/string.h"
#include "utils/pool.h"
#include "utils/object.h"

// Prompt generation functions
__attribute__((warn_unused_result)) result_t agent_prompt_generate(pool_t* pool, config_t* config, agent_t* agent, string_t** dst);

// Internal helper functions for modular prompt building
__attribute__((warn_unused_result)) result_t agent_prompt_extract_config_objects(pool_t* pool, config_t* config, agent_t* agent, object_t** agent_workingmemory, object_t** agent_state, object_t** config_agent_state, object_t** config_agent_state_base, object_t** config_agent_state_base_prompt, object_t** config_agent_state_main, object_t** config_agent_state_main_prompt);

__attribute__((warn_unused_result)) result_t agent_prompt_build_header(pool_t* pool, string_t** dst);

__attribute__((warn_unused_result)) result_t agent_prompt_append_base(pool_t* pool, string_t** dst, object_t* config_agent_state_base_prompt);

__attribute__((warn_unused_result)) result_t agent_prompt_append_state(pool_t* pool, string_t** dst, object_t* config_agent_state_main_prompt);

__attribute__((warn_unused_result)) result_t agent_prompt_append_memory(pool_t* pool, string_t** dst, object_t* agent_workingmemory);

__attribute__((warn_unused_result)) result_t agent_prompt_append_footer(pool_t* pool, string_t** dst, config_t* config);

#endif
