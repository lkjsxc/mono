#include "agent/http.h"

// Helper function to create and initialize HTTP request resources
result_t agent_http_create_resources(pool_t* pool, string_t** send_string, string_t** content_type, string_t** recv_http_string) {
    // Create send string
    if (string_create(pool, send_string) != RESULT_OK) {
        RETURN_ERR("Failed to create send string");
    }

    // Create content type string
    if (string_create_str(pool, content_type, "application/json") != RESULT_OK) {
        if (string_destroy(pool, *send_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy send string after content_type failure");
        }
        RETURN_ERR("Failed to create content type string");
    }

    // Create response string
    if (string_create(pool, recv_http_string) != RESULT_OK) {
        if (string_destroy(pool, *send_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy send string after response creation failure");
        }
        if (string_destroy(pool, *content_type) != RESULT_OK) {
            RETURN_ERR("Failed to destroy content type after response creation failure");
        }
        RETURN_ERR("Failed to create response string");
    }

    return RESULT_OK;
}

// Helper function to extract content from LLM response
result_t agent_http_extract_response_content(pool_t* pool, object_t* recv_http_object, object_t** recv_content_object) {
    object_t* choices_obj;
    object_t* first_choice_obj;
    object_t* message_obj;

    // Navigate the JSON structure step by step: choices -> first element -> message -> content
    if (object_provide_str(pool, &choices_obj, recv_http_object, "choices") != RESULT_OK) {
        RETURN_ERR("Failed to get choices array from HTTP response");
    }

    // Get the first choice (assuming it's named "0" or just use the first child)
    if (choices_obj->child == NULL) {
        RETURN_ERR("Choices array is empty");
    }
    first_choice_obj = choices_obj->child;

    if (object_provide_str(pool, &message_obj, first_choice_obj, "message") != RESULT_OK) {
        RETURN_ERR("Failed to get message from first choice");
    }

    if (object_provide_str(pool, recv_content_object, message_obj, "content") != RESULT_OK) {
        RETURN_ERR("Failed to get content from message");
    }

    return RESULT_OK;
}

// Helper function to clean up all HTTP-related resources
result_t agent_http_cleanup_resources(pool_t* pool, string_t* send_string, string_t* content_type, string_t* recv_http_string, object_t* recv_http_object) {
    result_t cleanup_result = RESULT_OK;

    if (send_string && string_destroy(pool, send_string) != RESULT_OK) {
        cleanup_result = RESULT_ERR;
    }
    if (content_type && string_destroy(pool, content_type) != RESULT_OK) {
        cleanup_result = RESULT_ERR;
    }
    if (recv_http_string && string_destroy(pool, recv_http_string) != RESULT_OK) {
        cleanup_result = RESULT_ERR;
    }
    if (recv_http_object && object_destroy(pool, recv_http_object) != RESULT_OK) {
        cleanup_result = RESULT_ERR;
    }

    if (cleanup_result != RESULT_OK) {
        RETURN_ERR("Failed to clean up one or more HTTP resources");
    }

    return RESULT_OK;
}

// High-level function to send HTTP request and receive response
result_t agent_http_send_receive(pool_t* pool, config_t* config, string_t* prompt, string_t** response_content) {
    string_t* content_type = NULL;
    string_t* recv_http_string = NULL;
    object_t* recv_http_object = NULL;
    object_t* recv_content_object = NULL;
    object_t* url_object = NULL;

    // Create content type string
    if (string_create_str(pool, &content_type, "application/json") != RESULT_OK) {
        RETURN_ERR("Failed to create content type string");
    }

    // Create response string
    if (string_create(pool, &recv_http_string) != RESULT_OK) {
        if (string_destroy(pool, content_type) != RESULT_OK) {
            RETURN_ERR("Failed to destroy content type after response creation failure");
        }
        RETURN_ERR("Failed to create response string");
    }

    // Get URL from config
    if (object_provide_str(pool, &url_object, config->data, "llm.endpoint") != RESULT_OK) {
        if (agent_http_cleanup_resources(pool, NULL, content_type, recv_http_string, NULL) != RESULT_OK) {
            RETURN_ERR("Failed to cleanup HTTP resources after URL retrieval failure");
        }
        RETURN_ERR("Failed to get URL from config");
    }

    // Send HTTP POST request
    if (http_post(pool, url_object->string, content_type, prompt, &recv_http_string) != RESULT_OK) {
        if (agent_http_cleanup_resources(pool, NULL, content_type, recv_http_string, NULL) != RESULT_OK) {
            RETURN_ERR("Failed to cleanup HTTP resources after HTTP POST failure");
        }
        RETURN_ERR("Failed to send HTTP POST request");
    }

    // Parse JSON response
    if (object_parse_json(pool, &recv_http_object, recv_http_string) != RESULT_OK) {
        if (agent_http_cleanup_resources(pool, NULL, content_type, recv_http_string, NULL) != RESULT_OK) {
            RETURN_ERR("Failed to cleanup HTTP resources after JSON parse failure");
        }
        RETURN_ERR("Failed to parse HTTP response JSON");
    }

    // Extract content from response
    if (agent_http_extract_response_content(pool, recv_http_object, &recv_content_object) != RESULT_OK) {
        if (agent_http_cleanup_resources(pool, NULL, content_type, recv_http_string, recv_http_object) != RESULT_OK) {
            RETURN_ERR("Failed to cleanup HTTP resources after content extraction failure");
        }
        RETURN_ERR("Failed to extract content from LLM response");
    }

    // Create response content string
    if (string_create_string(pool, response_content, recv_content_object->string) != RESULT_OK) {
        if (agent_http_cleanup_resources(pool, NULL, content_type, recv_http_string, recv_http_object) != RESULT_OK) {
            RETURN_ERR("Failed to cleanup HTTP resources after response creation failure");
        }
        RETURN_ERR("Failed to create response content string");
    }

    // Cleanup resources
    if (agent_http_cleanup_resources(pool, NULL, content_type, recv_http_string, recv_http_object) != RESULT_OK) {
        RETURN_ERR("Failed to cleanup HTTP resources");
    }

    return RESULT_OK;
}
