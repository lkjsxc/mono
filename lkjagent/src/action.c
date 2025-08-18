#include "lkjagent.h"

result_t lkjagent_action(pool_t* pool, lkjagent_t* lkjagent, object_t* action, uint64_t iteration) {
    object_t* action_type = NULL;
    object_t* action_tags = NULL;
    object_t* action_value = NULL;
    data_t* sorted_tags_array[MAX_TAGS] = {NULL};
    data_t* sorted_tags_string = NULL;

    if (object_provide_str(&action_type, action, "type") != RESULT_OK) {
        RETURN_ERR("Failed to get action type from action object");
    }

    if (object_provide_str(&action_tags, action, "tags") != RESULT_OK) {
        RETURN_ERR("Failed to get action tags from action object");
    }

    if (object_provide_str(&action_value, action, "value") != RESULT_OK) {
        RETURN_ERR("Failed to get action value from action object");
    }

    // Sort tags into array format
    if (tags_sort(pool, sorted_tags_array, action_tags->data) != RESULT_OK) {
        RETURN_ERR("Failed to sort action tags");
    }

    // Convert sorted array back to comma-separated string for existing action functions
    if (tags_array_to_string(pool, sorted_tags_array, &sorted_tags_string) != RESULT_OK) {
        // Cleanup sorted_tags_array
        for (uint64_t i = 0; i < MAX_TAGS && sorted_tags_array[i] != NULL; i++) {
            if (data_destroy(pool, sorted_tags_array[i]) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup sorted tag array element during conversion error");
            }
        }
        RETURN_ERR("Failed to convert sorted tags array to string");
    }

    // Dispatch to appropriate action handler based on type
    if (data_equal_str(action_type->data, "working_memory_add")) {
        if (lkjagent_action_working_memory_add(pool, lkjagent, sorted_tags_string, action_value->data, iteration) != RESULT_OK) {
            // Cleanup
            for (uint64_t i = 0; i < MAX_TAGS && sorted_tags_array[i] != NULL; i++) {
                if (data_destroy(pool, sorted_tags_array[i]) != RESULT_OK) {
                    PRINT_ERR("Failed to cleanup sorted tag array element");
                }
            }
            if (data_destroy(pool, sorted_tags_string) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup sorted_tags_string after working_memory_add error");
            }
            RETURN_ERR("Failed to perform working_memory_add action");
        }
    } else if (data_equal_str(action_type->data, "working_memory_remove")) {
        if (lkjagent_action_working_memory_remove(pool, lkjagent, sorted_tags_string) != RESULT_OK) {
            // Cleanup
            for (uint64_t i = 0; i < MAX_TAGS && sorted_tags_array[i] != NULL; i++) {
                if (data_destroy(pool, sorted_tags_array[i]) != RESULT_OK) {
                    PRINT_ERR("Failed to cleanup sorted tag array element");
                }
            }
            if (data_destroy(pool, sorted_tags_string) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup sorted_tags_string after working_memory_remove error");
            }
            RETURN_ERR("Failed to perform working_memory_remove action");
        }
    } else if (data_equal_str(action_type->data, "storage_save")) {
        if (lkjagent_action_storage_save(pool, lkjagent, sorted_tags_string, action_value->data) != RESULT_OK) {
            // Cleanup
            for (uint64_t i = 0; i < MAX_TAGS && sorted_tags_array[i] != NULL; i++) {
                if (data_destroy(pool, sorted_tags_array[i]) != RESULT_OK) {
                    PRINT_ERR("Failed to cleanup sorted tag array element");
                }
            }
            if (data_destroy(pool, sorted_tags_string) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup sorted_tags_string after storage_save error");
            }
            RETURN_ERR("Failed to perform storage_save action");
        }
    } else if (data_equal_str(action_type->data, "storage_load")) {
        if (lkjagent_action_storage_load(pool, lkjagent, sorted_tags_string, iteration) != RESULT_OK) {
            // Cleanup
            for (uint64_t i = 0; i < MAX_TAGS && sorted_tags_array[i] != NULL; i++) {
                if (data_destroy(pool, sorted_tags_array[i]) != RESULT_OK) {
                    PRINT_ERR("Failed to cleanup sorted tag array element");
                }
            }
            if (data_destroy(pool, sorted_tags_string) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup sorted_tags_string after storage_load error");
            }
            RETURN_ERR("Failed to perform storage_load action");
        }
    } else if (data_equal_str(action_type->data, "storage_search")) {
        if (lkjagent_action_storage_search(pool, lkjagent, sorted_tags_string, action_value->data, iteration) != RESULT_OK) {
            // Cleanup
            for (uint64_t i = 0; i < MAX_TAGS && sorted_tags_array[i] != NULL; i++) {
                if (data_destroy(pool, sorted_tags_array[i]) != RESULT_OK) {
                    PRINT_ERR("Failed to cleanup sorted tag array element");
                }
            }
            if (data_destroy(pool, sorted_tags_string) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup sorted_tags_string after storage_search error");
            }
            RETURN_ERR("Failed to perform storage_search action");
        }
    } else {
        // Cleanup
        for (uint64_t i = 0; i < MAX_TAGS && sorted_tags_array[i] != NULL; i++) {
            if (data_destroy(pool, sorted_tags_array[i]) != RESULT_OK) {
                PRINT_ERR("Failed to cleanup sorted tag array element");
            }
        }
        if (data_destroy(pool, sorted_tags_string) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup sorted_tags_string after unknown action type");
        }
        RETURN_ERR("Unknown action type");
    }

    // Cleanup sorted tags array and string
    for (uint64_t i = 0; i < MAX_TAGS && sorted_tags_array[i] != NULL; i++) {
        if (data_destroy(pool, sorted_tags_array[i]) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup sorted tag array element");
        }
    }
    if (data_destroy(pool, sorted_tags_string) != RESULT_OK) {
        PRINT_ERR("Failed to cleanup sorted tags string");
    }

    return RESULT_OK;
}
