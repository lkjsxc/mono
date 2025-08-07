#include "agent/http.h"

// High-level HTTP communication wrapper
result_t agent_http_send_receive(pool_t* pool, config_t* config, const string_t* prompt, string_t** response_content) {
    string_t* request_string = NULL;
    string_t* response_string = NULL;
    string_t* content_type = NULL;
    string_t* endpoint_url = NULL;
    
    // Phase 1: Create and initialize resources
    if (agent_http_create_resources(pool, &request_string, &response_string, &content_type) != RESULT_OK) {
        RETURN_ERR("Failed to create HTTP resources");
    }
    
    // Phase 2: Build endpoint URL from configuration
    if (agent_http_build_endpoint_url(pool, config, &endpoint_url) != RESULT_OK) {
        if (agent_http_cleanup_resources(pool, request_string, response_string, content_type) != RESULT_OK) {
            RETURN_ERR("Failed to cleanup resources after endpoint URL build failure");
        }
        RETURN_ERR("Failed to build endpoint URL");
    }
    
    // Phase 3: Copy prompt to request string
    if (string_copy_string(pool, &request_string, prompt) != RESULT_OK) {
        if (string_destroy(pool, endpoint_url) != RESULT_OK ||
            agent_http_cleanup_resources(pool, request_string, response_string, content_type) != RESULT_OK) {
            RETURN_ERR("Failed to cleanup resources after prompt copy failure");
        }
        RETURN_ERR("Failed to copy prompt to request string");
    }
    
    // Phase 4: Send HTTP POST request to LLM endpoint
    if (http_post(pool, endpoint_url, content_type, request_string, &response_string) != RESULT_OK) {
        if (string_destroy(pool, endpoint_url) != RESULT_OK ||
            agent_http_cleanup_resources(pool, request_string, response_string, content_type) != RESULT_OK) {
            RETURN_ERR("Failed to cleanup resources after HTTP POST failure");
        }
        RETURN_ERR("Failed to send HTTP POST request to LLM");
    }
    
    // Phase 5: Extract content from LLM response
    if (agent_http_extract_response_content(pool, response_string, response_content) != RESULT_OK) {
        if (string_destroy(pool, endpoint_url) != RESULT_OK ||
            agent_http_cleanup_resources(pool, request_string, response_string, content_type) != RESULT_OK) {
            RETURN_ERR("Failed to cleanup resources after response extraction failure");
        }
        RETURN_ERR("Failed to extract content from LLM response");
    }
    
    // Phase 6: Clean up resources
    if (string_destroy(pool, endpoint_url) != RESULT_OK) {
        RETURN_ERR("Failed to destroy endpoint URL");
    }
    
    if (agent_http_cleanup_resources(pool, request_string, response_string, content_type) != RESULT_OK) {
        RETURN_ERR("Failed to cleanup HTTP resources");
    }
    
    return RESULT_OK;
}

// Create and initialize HTTP request/response resources
result_t agent_http_create_resources(pool_t* pool, string_t** request_string, string_t** response_string, string_t** content_type) {
    // Create request string
    if (string_create(pool, request_string) != RESULT_OK) {
        RETURN_ERR("Failed to create HTTP request string");
    }
    
    // Create response string
    if (string_create(pool, response_string) != RESULT_OK) {
        if (string_destroy(pool, *request_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request string after response string creation failure");
        }
        RETURN_ERR("Failed to create HTTP response string");
    }
    
    // Create content type string
    if (string_create_str(pool, content_type, "application/json") != RESULT_OK) {
        if (string_destroy(pool, *request_string) != RESULT_OK ||
            string_destroy(pool, *response_string) != RESULT_OK) {
            RETURN_ERR("Failed to destroy strings after content type creation failure");
        }
        RETURN_ERR("Failed to create HTTP content type string");
    }
    
    return RESULT_OK;
}

// Clean up HTTP resources
result_t agent_http_cleanup_resources(pool_t* pool, string_t* request_string, string_t* response_string, string_t* content_type) {
    result_t cleanup_result = RESULT_OK;
    
    // Clean up request string
    if (request_string != NULL && string_destroy(pool, request_string) != RESULT_OK) {
        cleanup_result = RESULT_ERR;
    }
    
    // Clean up response string
    if (response_string != NULL && string_destroy(pool, response_string) != RESULT_OK) {
        cleanup_result = RESULT_ERR;
    }
    
    // Clean up content type string
    if (content_type != NULL && string_destroy(pool, content_type) != RESULT_OK) {
        cleanup_result = RESULT_ERR;
    }
    
    if (cleanup_result != RESULT_OK) {
        RETURN_ERR("Failed to cleanup one or more HTTP resources");
    }
    
    return RESULT_OK;
}

// Extract content from LLM API response JSON
result_t agent_http_extract_response_content(pool_t* pool, const string_t* response_json, string_t** content) {
    object_t* response_obj = NULL;
    object_t* choices_array = NULL;
    object_t* first_choice = NULL;
    object_t* message_obj = NULL;
    object_t* content_obj = NULL;
    
    // Parse the JSON response
    if (object_parse_json(pool, &response_obj, response_json) != RESULT_OK) {
        RETURN_ERR("Failed to parse LLM response JSON");
    }
    
    // Navigate through the response structure
    // response.choices[0].message.content
    
    // Get choices array
    if (object_provide_str(pool, &choices_array, response_obj, "choices") != RESULT_OK) {
        if (object_destroy(pool, response_obj) != RESULT_OK) {
            RETURN_ERR("Failed to destroy response object after choices extraction failure");
        }
        RETURN_ERR("Failed to get choices array from LLM response");
    }
    
    // Get first choice (index 0)
    if (object_provide_str(pool, &first_choice, choices_array, "[0]") != RESULT_OK) {
        if (object_destroy(pool, response_obj) != RESULT_OK) {
            RETURN_ERR("Failed to destroy response object after first choice extraction failure");
        }
        RETURN_ERR("Failed to get first choice from LLM response");
    }
    
    // Get message object
    if (object_provide_str(pool, &message_obj, first_choice, "message") != RESULT_OK) {
        if (object_destroy(pool, response_obj) != RESULT_OK) {
            RETURN_ERR("Failed to destroy response object after message extraction failure");
        }
        RETURN_ERR("Failed to get message object from LLM response");
    }
    
    // Get content string
    if (object_provide_str(pool, &content_obj, message_obj, "content") != RESULT_OK) {
        if (object_destroy(pool, response_obj) != RESULT_OK) {
            RETURN_ERR("Failed to destroy response object after content extraction failure");
        }
        RETURN_ERR("Failed to get content from LLM response message");
    }
    
    // Create a copy of the content string for the caller
    if (string_create_string(pool, content, content_obj->string) != RESULT_OK) {
        if (object_destroy(pool, response_obj) != RESULT_OK) {
            RETURN_ERR("Failed to destroy response object after content copy failure");
        }
        RETURN_ERR("Failed to create copy of LLM response content");
    }
    
    // Clean up the parsed response object
    if (object_destroy(pool, response_obj) != RESULT_OK) {
        RETURN_ERR("Failed to destroy response object after content extraction");
    }
    
    return RESULT_OK;
}

// Build endpoint URL from configuration
result_t agent_http_build_endpoint_url(pool_t* pool, config_t* config, string_t** endpoint_url) {
    object_t* llm_config = NULL;
    object_t* endpoint_obj = NULL;
    object_t* model_obj = NULL;
    object_t* temperature_obj = NULL;
    
    // Extract LLM configuration
    if (agent_http_extract_llm_config(pool, config, &llm_config, &endpoint_obj, &model_obj, &temperature_obj) != RESULT_OK) {
        RETURN_ERR("Failed to extract LLM configuration");
    }
    
    // Create endpoint URL string from configuration
    if (string_create_string(pool, endpoint_url, endpoint_obj->string) != RESULT_OK) {
        RETURN_ERR("Failed to create endpoint URL string");
    }
    
    return RESULT_OK;
}

// Validate and extract LLM configuration parameters
result_t agent_http_extract_llm_config(pool_t* pool, config_t* config, object_t** llm_config, object_t** endpoint_obj, object_t** model_obj, object_t** temperature_obj) {
    // Get LLM configuration section
    if (object_provide_str(pool, llm_config, config->data, "llm") != RESULT_OK) {
        RETURN_ERR("Failed to get LLM configuration from config");
    }
    
    // Extract endpoint URL
    if (object_provide_str(pool, endpoint_obj, *llm_config, "endpoint") != RESULT_OK) {
        RETURN_ERR("Failed to get LLM endpoint from configuration");
    }
    
    // Extract model name
    if (object_provide_str(pool, model_obj, *llm_config, "model") != RESULT_OK) {
        RETURN_ERR("Failed to get LLM model from configuration");
    }
    
    // Extract temperature (optional)
    if (object_provide_str(pool, temperature_obj, *llm_config, "temperature") != RESULT_OK) {
        // Temperature is optional, set to NULL if not found
        *temperature_obj = NULL;
    }
    
    return RESULT_OK;
}
