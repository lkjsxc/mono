// For usleep function
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "agent/core.h"

result_t agent_init(pool_t* pool, config_t* config, agent_t* agent) {
    // Initialize agent structure
    agent->status = config->agent_default_status;
    agent->iteration_count = 0;

    if (pool_json_value_alloc(pool, &agent->working_memory) != RESULT_OK) {
        RETURN_ERR("Failed to allocate working memory object");
    }

    if (pool_json_value_alloc(pool, &agent->storage) != RESULT_OK) {
        RETURN_ERR("Failed to allocate storage object");
    }

    return RESULT_OK;
}

static result_t agent_request(pool_t* pool, config_t* config, agent_t* agent, string_t** response_text) {
    // Create JSON request payload
    json_value_t* request_json;
    if (json_create_object(pool, &request_json) != RESULT_OK) {
        RETURN_ERR("Failed to create request JSON object");
    }

    // Add model field
    json_value_t* model_value;
    if (json_create_string(pool, config->llm_model->data, &model_value) != RESULT_OK) {
        RETURN_ERR("Failed to create model JSON value");
    }
    if (json_object_set(pool, request_json, "model", model_value) != RESULT_OK) {
        RETURN_ERR("Failed to set model in request JSON");
    }

    // Add temperature field
    json_value_t* temperature_value;
    if (json_create_number(pool, config->llm_temperature, &temperature_value) != RESULT_OK) {
        RETURN_ERR("Failed to create temperature JSON value");
    }
    if (json_object_set(pool, request_json, "temperature", temperature_value) != RESULT_OK) {
        RETURN_ERR("Failed to set temperature in request JSON");
    }

    // Create messages array
    json_value_t* messages_array;
    if (json_create_array(pool, &messages_array) != RESULT_OK) {
        RETURN_ERR("Failed to create messages array");
    }

    // Add system message
    json_value_t* system_message;
    if (json_create_object(pool, &system_message) != RESULT_OK) {
        RETURN_ERR("Failed to create system message object");
    }

    json_value_t* role_value;
    if (json_create_string(pool, "system", &role_value) != RESULT_OK) {
        RETURN_ERR("Failed to create role JSON value");
    }
    if (json_object_set(pool, system_message, "role", role_value) != RESULT_OK) {
        RETURN_ERR("Failed to set role in system message");
    }

    // Construct comprehensive system prompt
    json_value_t* system_prompt;
    if (json_create_object(pool, &system_prompt) != RESULT_OK) {
        RETURN_ERR("Failed to create system prompt object");
    }

    if (json_deep_copy(pool, config->agent_prompt_system, &system_prompt) != RESULT_OK) {
        RETURN_ERR("Failed to copy system prompt JSON object");
    }

    // Add current working memory context
    if (json_object_set(pool, system_prompt, "working_memory", agent->working_memory) != RESULT_OK) {
        RETURN_ERR("Failed to set working memory in system prompt");
    }

    // Add current storage context
    // implement later

    // Convert system prompt JSON to string for LLM content
    string_t* content_string;
    if (pool_string_alloc(pool, &content_string, 4096) != RESULT_OK) {
        RETURN_ERR("Failed to allocate content string");
    }

    if (json_stringify(pool, system_prompt, &content_string) != RESULT_OK) {
        RETURN_ERR("Failed to stringify system prompt JSON");
    }

    json_value_t* content_value;
    if (json_create_string(pool, content_string->data, &content_value) != RESULT_OK) {
        RETURN_ERR("Failed to create content string value");
    }

    if (json_object_set(pool, system_message, "content", content_value) != RESULT_OK) {
        RETURN_ERR("Failed to set content in system message");
    }

    // Clean up content string
    if (pool_string_free(pool, content_string) != RESULT_OK) {
        RETURN_ERR("Failed to free content string");
    }

    // Add system message to messages array
    if (json_array_append(pool, messages_array, system_message) != RESULT_OK) {
        RETURN_ERR("Failed to add system message to messages array");
    }

    // Add messages array to request JSON
    if (json_object_set(pool, request_json, "messages", messages_array) != RESULT_OK) {
        RETURN_ERR("Failed to set messages array in request JSON");
    }

    // Serialize JSON to string
    string_t* request_body;
    if (pool_string_alloc(pool, &request_body, 65536) != RESULT_OK) {
        RETURN_ERR("Failed to allocate request body string");
    }

    if (json_stringify(pool, request_json, &request_body) != RESULT_OK) {
        RETURN_ERR("Failed to stringify request JSON");
    }

    // Make HTTP request
    http_response_t response;
    if (http_post_json(pool, config->llm_endpoint->data, request_body->data, &response) != RESULT_OK) {
        RETURN_ERR("Failed to send HTTP request to LLM endpoint");
    }

    // Check HTTP status
    if (response.status_code != 200) {
        RETURN_ERR("LLM endpoint returned non-200 status code");
    }

    // Parse response JSON to extract content
    json_value_t* response_json;
    if (json_parse(pool, response.body, &response_json) != RESULT_OK) {
        RETURN_ERR("Failed to parse LLM response JSON");
    }

    // Extract choices[0].message.content
    json_value_t* choices = json_object_get(response_json, "choices");
    if (!choices || choices->type != JSON_TYPE_ARRAY) {
        RETURN_ERR("LLM response missing choices array");
    }

    json_value_t* first_choice = json_array_get(choices, 0);
    if (!first_choice || first_choice->type != JSON_TYPE_OBJECT) {
        RETURN_ERR("LLM response missing first choice");
    }

    json_value_t* message = json_object_get(first_choice, "message");
    if (!message || message->type != JSON_TYPE_OBJECT) {
        RETURN_ERR("LLM response missing message object");
    }

    json_value_t* content = json_object_get(message, "content");
    if (!content || content->type != JSON_TYPE_STRING) {
        RETURN_ERR("LLM response missing content string");
    }

    // Copy content to response_text
    if (string_assign(pool, response_text, content->u.string_value->data) != RESULT_OK) {
        RETURN_ERR("Failed to assign LLM response content");
    }

    return RESULT_OK;
}

static result_t agent_execute(pool_t* pool, __attribute__((unused)) config_t* config, agent_t* agent, const string_t* response_text) {
    // Parse response and extract JSON payload
    json_value_t* response_json;
    if (agent_parse_response(pool, response_text, &response_json) != RESULT_OK) {
        RETURN_ERR("Failed to parse agent response");
    }

    // Process working_memory_add operations
    json_value_t* working_memory_add = json_object_get(response_json, "working_memory_add");
    if (agent_working_memory_add(pool, agent, working_memory_add) != RESULT_OK) {
        if (pool_json_value_free(pool, response_json) != RESULT_OK) {
            RETURN_ERR("Failed to free response JSON value and process working memory add");
        }
        RETURN_ERR("Failed to process working memory add operations");
    }

    // Process working_memory_remove operations
    json_value_t* working_memory_remove = json_object_get(response_json, "working_memory_remove");
    if (agent_working_memory_remove(pool, agent, working_memory_remove) != RESULT_OK) {
        if (pool_json_value_free(pool, response_json) != RESULT_OK) {
            RETURN_ERR("Failed to free response JSON value and process working memory remove");
        }
        RETURN_ERR("Failed to process working memory remove operations");
    }

    // Process storage_add operations
    json_value_t* storage_add = json_object_get(response_json, "storage_add");
    if (agent_storage_add(pool, agent, storage_add) != RESULT_OK) {
        if (pool_json_value_free(pool, response_json) != RESULT_OK) {
            RETURN_ERR("Failed to free response JSON value and process storage add");
        }
        RETURN_ERR("Failed to process storage add operations");
    }

    // Process storage_remove operations
    json_value_t* storage_remove = json_object_get(response_json, "storage_remove");
    if (agent_storage_remove(pool, agent, storage_remove) != RESULT_OK) {
        if (pool_json_value_free(pool, response_json) != RESULT_OK) {
            RETURN_ERR("Failed to free response JSON value and process storage remove");
        }
        RETURN_ERR("Failed to process storage remove operations");
    }

    // Process status_change operations
    json_value_t* status_change = json_object_get(response_json, "status_change");
    if (agent_status_change(agent, status_change) != RESULT_OK) {
        if (pool_json_value_free(pool, response_json) != RESULT_OK) {
            RETURN_ERR("Failed to free response JSON value and process status change");
        }
        RETURN_ERR("Failed to process status change operations");
    }

    if (pool_json_value_free(pool, response_json) != RESULT_OK) {
        RETURN_ERR("Failed to free response JSON value");
    }

    return RESULT_OK;
}

result_t agent_step(pool_t* pool, config_t* config, agent_t* agent) {
    string_t* response_text;
    if (pool_string_alloc(pool, &response_text, 1048576) != RESULT_OK) {
        RETURN_ERR("Failed to allocate response text string");
    }

    if (agent_request(pool, config, agent, &response_text) != RESULT_OK) {
        RETURN_ERR("Agent request failed");
    }

    if (agent_execute(pool, config, agent, response_text) != RESULT_OK) {
        RETURN_ERR("Agent execution failed");
    }

    if (pool_string_free(pool, response_text) != RESULT_OK) {
        RETURN_ERR("Failed to free response text string");
    }

    return RESULT_OK;
}

result_t agent_run(pool_t* pool, config_t* config, agent_t* agent) {
    while (agent->iteration_count < config->agent_max_iterate) {
        if (agent_step(pool, config, agent) != RESULT_OK) {
            RETURN_ERR("Agent step failed");
        }
        agent->iteration_count++;
    }

    return RESULT_OK;
}