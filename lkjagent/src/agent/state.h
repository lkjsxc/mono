#ifndef LKJAGENT_agent_STATE_H
#define LKJAGENT_agent_STATE_H

#include "global/const.h"
#include "global/macro.h"
#include "global/std.h"
#include "global/types.h"
#include "utils/string.h"
#include "utils/pool.h"
#include "utils/object.h"

// State management and transition functions

// Automatic transition from executing state to evaluating state
__attribute__((warn_unused_result)) result_t agent_state_auto_transition(pool_t* pool, config_t* config, agent_t* agent);

// Regular state update with thinking log management
__attribute__((warn_unused_result)) result_t agent_state_update_and_log(pool_t* pool, config_t* config, agent_t* agent, object_t* response_obj);

// Memory-aware transition handling for evaluating state
__attribute__((warn_unused_result)) result_t agent_state_handle_evaluation_transition(pool_t* pool, config_t* config, agent_t* agent, object_t* response_obj);

// Token counting and memory estimation
__attribute__((warn_unused_result)) result_t agent_state_estimate_tokens(pool_t* pool, agent_t* agent, uint64_t* token_count);

// Internal helper functions for state management

// Extract next state from LLM response
__attribute__((warn_unused_result)) result_t agent_state_extract_next_state(pool_t* pool, object_t* response_obj, object_t** next_state_obj);

// Update agent state with new value
__attribute__((warn_unused_result)) result_t agent_state_update_state(pool_t* pool, agent_t* agent, const char* new_state);

// Handle thinking log entries with rotation
__attribute__((warn_unused_result)) result_t agent_state_manage_think_log(pool_t* pool, config_t* config, agent_t* agent, object_t* response_obj);

// Handle evaluation log entries with rotation
__attribute__((warn_unused_result)) result_t agent_state_manage_evaluation_log(pool_t* pool, config_t* config, agent_t* agent, object_t* response_obj);

// Handle execution log entries with rotation
__attribute__((warn_unused_result)) result_t agent_state_manage_execution_log(pool_t* pool, config_t* config, agent_t* agent, const char* action_type, const char* tags, const char* result_message);

// Check if memory limits require paging
__attribute__((warn_unused_result)) result_t agent_state_check_memory_limits(pool_t* pool, config_t* config, agent_t* agent, uint64_t* requires_paging);

// Execute paging operation to manage memory overflow
__attribute__((warn_unused_result)) result_t agent_state_execute_paging(pool_t* pool, config_t* config, agent_t* agent);

// Synchronize all logs with working memory for consistent access
__attribute__((warn_unused_result)) result_t agent_state_sync_logs_to_working_memory(pool_t* pool, agent_t* agent);

#endif
