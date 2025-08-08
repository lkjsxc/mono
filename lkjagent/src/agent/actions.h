#ifndef LKJAGENT_agent_ACTIONS_H
#define LKJAGENT_agent_ACTIONS_H

#include "global/const.h"
#include "global/macro.h"
#include "global/std.h"
#include "global/types.h"
#include "utils/string.h"
#include "utils/pool.h"
#include "utils/object.h"
#include "utils/file.h"
#include "agent/state.h"

// Action execution and memory management functions

// Main action dispatcher - routes actions based on type
__attribute__((warn_unused_result)) result_t agent_actions_dispatch(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj);

// Working memory operations
__attribute__((warn_unused_result)) result_t agent_actions_execute_working_memory_add(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj);
__attribute__((warn_unused_result)) result_t agent_actions_execute_working_memory_remove(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj);

// Storage operations
__attribute__((warn_unused_result)) result_t agent_actions_execute_storage_load(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj);
__attribute__((warn_unused_result)) result_t agent_actions_execute_storage_save(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj);
__attribute__((warn_unused_result)) result_t agent_actions_execute_storage_search(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj);

// Memory persistence
__attribute__((warn_unused_result)) result_t agent_actions_save_memory(pool_t* pool, agent_t* agent);

// Response parsing
__attribute__((warn_unused_result)) result_t agent_actions_parse_response(pool_t* pool, const string_t* response_content, object_t** response_obj);

// Helper functions for action processing

// Extract action parameters (type, tags, value)
__attribute__((warn_unused_result)) result_t agent_actions_extract_action_params(pool_t* pool, object_t* action_obj, object_t** type_obj, object_t** tags_obj, object_t** value_obj);

// Validate action parameters
__attribute__((warn_unused_result)) result_t agent_actions_validate_action_params(object_t* type_obj, object_t* tags_obj, object_t* value_obj, const char* expected_type, uint64_t value_required);

// Safe tag processing (convert spaces to underscores)
__attribute__((warn_unused_result)) result_t agent_actions_process_tags(pool_t* pool, object_t* tags_obj, string_t** processed_tags);
// Normalize storage tags: comma-separated, trimmed, lowercased, spaces->underscores, sorted ascending, deduped
__attribute__((warn_unused_result)) result_t agent_actions_normalize_storage_tags(pool_t* pool, object_t* tags_obj, string_t** processed_tags);

// Working memory access helpers
__attribute__((warn_unused_result)) result_t agent_actions_get_working_memory(pool_t* pool, agent_t* agent, object_t** working_memory);
__attribute__((warn_unused_result)) result_t agent_actions_get_storage(pool_t* pool, agent_t* agent, object_t** storage);
__attribute__((warn_unused_result)) result_t agent_actions_ensure_working_memory_exists(pool_t* pool, agent_t* agent);
__attribute__((warn_unused_result)) result_t agent_actions_ensure_storage_exists(pool_t* pool, agent_t* agent);

// Action result logging
__attribute__((warn_unused_result)) result_t agent_actions_log_result(pool_t* pool, config_t* config, agent_t* agent, const char* action_type, const char* tags, const char* result_message);

#endif
