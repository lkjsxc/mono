#ifndef ACTION_H
#define ACTION_H

#include "../lkjlib/lkjlib.h"

// Action types
typedef enum {
    ACTION_WORKING_MEMORY_ADD,
    ACTION_WORKING_MEMORY_REMOVE,
    ACTION_STORAGE_LOAD,
    ACTION_STORAGE_SAVE,
    ACTION_STORAGE_SEARCH,
    ACTION_UNKNOWN
} action_type_t;

// Action structure parsed from XML
typedef struct {
    action_type_t type;
    data_t* tags;
    data_t* value;  // May be NULL for some action types
} action_t;

// Action processing functions
__attribute__((warn_unused_result)) result_t action_parse_xml(pool_t* pool, action_t* action, const object_t* xml_obj);
__attribute__((warn_unused_result)) result_t action_execute(pool_t* pool, action_t* action, object_t* agent_memory);
__attribute__((warn_unused_result)) result_t action_cleanup(pool_t* pool, action_t* action);

// Individual action handlers
__attribute__((warn_unused_result)) result_t action_working_memory_add(pool_t* pool, object_t* agent_memory, const data_t* tags, const data_t* value);
__attribute__((warn_unused_result)) result_t action_working_memory_remove(pool_t* pool, object_t* agent_memory, const data_t* tags);
__attribute__((warn_unused_result)) result_t action_storage_load(pool_t* pool, object_t* agent_memory, const data_t* tags);
__attribute__((warn_unused_result)) result_t action_storage_save(pool_t* pool, object_t* agent_memory, const data_t* tags, const data_t* value);
__attribute__((warn_unused_result)) result_t action_storage_search(pool_t* pool, object_t* agent_memory, const data_t* tags);

#endif
