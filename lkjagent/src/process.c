#include "lkjagent.h"

static __attribute__((warn_unused_result)) result_t extract_content_from_llm_response(pool_t* pool, const data_t* json_response, data_t** content) {
    object_t* response_obj = NULL;
    object_t* choices_obj = NULL;
    object_t* message_obj = NULL;
    object_t* content_obj = NULL;

    // Parse the JSON response
    if (object_parse_json(pool, &response_obj, json_response) != RESULT_OK) {
        RETURN_ERR("Failed to parse LLM response JSON");
    }

    // Navigate to choices[0].message.content
    if (object_provide_str(&choices_obj, response_obj, "choices") != RESULT_OK) {
        result_t cleanup = object_destroy(pool, response_obj);
        if (cleanup != RESULT_OK) {
            PRINT_ERR("Failed to cleanup response object after choices extraction error");
        }
        RETURN_ERR("Failed to get choices array from LLM response");
    }

    // Get first element of choices array (choices[0])
    if (!choices_obj->child) {
        result_t cleanup = object_destroy(pool, response_obj);
        if (cleanup != RESULT_OK) {
            PRINT_ERR("Failed to cleanup response object after choices child error");
        }
        RETURN_ERR("Choices array is empty in LLM response");
    }

    // Get message object from choices[0]
    if (object_provide_str(&message_obj, choices_obj->child, "message") != RESULT_OK) {
        result_t cleanup = object_destroy(pool, response_obj);
        if (cleanup != RESULT_OK) {
            PRINT_ERR("Failed to cleanup response object after message extraction error");
        }
        RETURN_ERR("Failed to get message from first choice in LLM response");
    }

    // Get content from message
    if (object_provide_str(&content_obj, message_obj, "content") != RESULT_OK) {
        result_t cleanup = object_destroy(pool, response_obj);
        if (cleanup != RESULT_OK) {
            PRINT_ERR("Failed to cleanup response object after content extraction error");
        }
        RETURN_ERR("Failed to get content from message in LLM response");
    }

    // Create a copy of the content data
    if (data_create_data(pool, content, content_obj->data) != RESULT_OK) {
        result_t cleanup = object_destroy(pool, response_obj);
        if (cleanup != RESULT_OK) {
            PRINT_ERR("Failed to cleanup response object after content copy error");
        }
        RETURN_ERR("Failed to copy content data from LLM response");
    }

    // Cleanup the parsed JSON object
    if (object_destroy(pool, response_obj) != RESULT_OK) {
        RETURN_ERR("Failed to cleanup response object");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t process_content_next_state(pool_t* pool, lkjagent_t* lkjagent, const object_t* content) {
    object_t* content_next_state = NULL;
    data_t* next_state_path = NULL;

    if (object_provide_str(&content_next_state, content, "agent.next_state") != RESULT_OK) {
        RETURN_ERR("Failed to get next_state from content object");
    }

    if (data_create_str(pool, &next_state_path, "next_state") != RESULT_OK) {
        RETURN_ERR("Failed to create data for next_state path");
    }

    if (object_set_data(pool, lkjagent->memory, next_state_path, content_next_state->data) != RESULT_OK) {
        RETURN_ERR("Failed to set data for next_state object");
    }

    if (data_destroy(pool, next_state_path) != RESULT_OK) {
        RETURN_ERR("Failed to cleanup next_state path");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t process_content_action(pool_t* pool, lkjagent_t* lkjagent, const object_t* content, uint64_t iteration) {
    object_t* action_obj = NULL;

    if (object_provide_str(&action_obj, content, "agent.action") != RESULT_OK) {
        RETURN_ERR("Failed to get action from content object");
    }

    if (lkjagent_action(pool, lkjagent, action_obj, iteration) != RESULT_OK) {
        RETURN_ERR("Failed to execute action");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t process_content(pool_t* pool, lkjagent_t* lkjagent, const object_t* content, uint64_t iteration) {
    if (process_content_next_state(pool, lkjagent, content) != RESULT_OK) {
        RETURN_ERR("Failed to process next_state");
    }

    if (process_content_action(pool, lkjagent, content, iteration) != RESULT_OK) {
        RETURN_ERR("Failed to process action");
    }

    return RESULT_OK;
}

result_t lkjagent_process(pool_t* pool, lkjagent_t* lkjagent, data_t* recv, uint64_t iteration) {
    data_t* content_data = NULL;
    object_t* content_obj = NULL;

    // Extract XML content from the LLM JSON response
    if (extract_content_from_llm_response(pool, recv, &content_data) != RESULT_OK) {
        RETURN_ERR("Failed to extract content from LLM response");
    }

    if (object_parse_xml(pool, &content_obj, content_data) != RESULT_OK) {
        if (data_destroy(pool, content_data) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup content data after parsing error");
        }
        RETURN_ERR("Failed to parse content data");
    }

    // Cleanup the extracted content
    if (data_destroy(pool, content_data) != RESULT_OK) {
        if (object_destroy(pool, content_obj) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup content object after content data cleanup error");
        }
        RETURN_ERR("Failed to cleanup content data");
    }

    // Process the content object
    if (process_content(pool, lkjagent, content_obj, iteration) != RESULT_OK) {
        if (object_destroy(pool, content_obj) != RESULT_OK) {
            PRINT_ERR("Failed to cleanup content object after processing error");
        }
        RETURN_ERR("Failed to process content object");
    }

    // Cleanup the content object
    if (object_destroy(pool, content_obj) != RESULT_OK) {
        RETURN_ERR("Failed to cleanup content object");
    }

    return RESULT_OK;
}
