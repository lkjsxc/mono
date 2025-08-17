#include "lkjagent.h"

static __attribute__((warn_unused_result)) result_t lkjagent_action(pool_t* pool, lkjagent_t* lkjagent, object_t* content, uint64_t iteration) {
    object_t* action_type = NULL;
    object_t* action_tags = NULL;
    object_t* action_value = NULL;

    if (object_provide_str(&action_type, content, "agent.action.type") != RESULT_OK) {
        RETURN_ERR("Failed to get action type from content object");
    }

    if (object_provide_str(&action_tags, content, "agent.action.tags") != RESULT_OK) {
        RETURN_ERR("Failed to get action tags from content object");
    }

    if (object_provide_str(&action_value, content, "agent.action.value") != RESULT_OK) {
        RETURN_ERR("Failed to get action value from content object");
    }

    if (data_equal_data(action_type->data, "storage_save") == RESULT_OK) {
        
    } else if (data_equal_data(action_type->data, "storage_load") == RESULT_OK) {

    }

    return RESULT_OK;
}
