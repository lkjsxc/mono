// For usleep function
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "agent/core.h"

result_t agent_init(pool_t* pool, config_t* config, agent_t* agent) {
    // Initialize agent structure
    agent->status = config->agent_default_status;
    agent->iteration_count = 0;

    if (pool_json_object_alloc(pool, &agent->working_memory) != RESULT_OK) {
        RETURN_ERR("Failed to allocate working memory object");
    }

    if (pool_json_object_alloc(pool, &agent->storage) != RESULT_OK) {
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
    string_t* system_prompt_text;
    if (pool_string_alloc(pool, &system_prompt_text, 256) != RESULT_OK) {
        RETURN_ERR("Failed to allocate system prompt text string");
    }

    // Start with base system prompt
    if (string_copy(pool, &system_prompt_text, config->agent_prompt_system) != RESULT_OK) {
        RETURN_ERR("Failed to assign base system prompt");
    }

    // // Add status-specific prompt if available and different from system
    // string_t* status_prompt = NULL;
    // switch (agent->status) {
    //     case AGENT_STATUS_THINKING:
    //         status_prompt = config->agent_prompt_thinking;
    //         break;
    //     case AGENT_STATUS_PAGING:
    //         status_prompt = config->agent_prompt_paging;
    //         break;
    //     case AGENT_STATUS_EVALUATING:
    //         status_prompt = config->agent_prompt_evaluating;
    //         break;
    //     case AGENT_STATUS_EXECUTING:
    //         status_prompt = config->agent_prompt_executing;
    //         break;
    //     default:
    //         status_prompt = NULL;
    //         break;
    // }

    // if (status_prompt && status_prompt->size > 0) {
    //     if (string_append_str(pool, &system_prompt_text, "\n\nStatus-specific instructions: ") != RESULT_OK) {
    //         RETURN_ERR("Failed to append status separator");
    //     }
    //     if (string_append(pool, &system_prompt_text, status_prompt) != RESULT_OK) {
    //         RETURN_ERR("Failed to append status-specific prompt");
    //     }
    // }

    // Add current working memory context
    if (agent->working_memory) {
        string_t* working_memory_str;
        if (pool_string_alloc(pool, &working_memory_str, 16384) != RESULT_OK) {
            RETURN_ERR("Failed to allocate working memory string");
        }

        json_value_t* working_memory_json = (json_value_t*)agent->working_memory;
        if (json_stringify(pool, working_memory_json, &working_memory_str) == RESULT_OK && working_memory_str->size > 2) {
            if (string_append_str(pool, &system_prompt_text, "\n\nCurrent working memory: ") != RESULT_OK) {
                RETURN_ERR("Failed to append working memory separator");
            }
            if (string_append(pool, &system_prompt_text, working_memory_str) != RESULT_OK) {
                RETURN_ERR("Failed to append working memory content");
            }
        }

        if (pool_string_free(pool, working_memory_str) != RESULT_OK) {
            RETURN_ERR("Failed to free working memory string");
        }
    }

    // Add current storage context
    if (agent->storage) {
        string_t* storage_str;
        if (pool_string_alloc(pool, &storage_str, 16384) != RESULT_OK) {
            RETURN_ERR("Failed to allocate storage string");
        }

        json_value_t* storage_json = (json_value_t*)agent->storage;
        if (json_stringify(pool, storage_json, &storage_str) == RESULT_OK && storage_str->size > 2) {
            if (string_append_str(pool, &system_prompt_text, "\n\nCurrent storage: ") != RESULT_OK) {
                RETURN_ERR("Failed to append storage separator");
            }
            if (string_append(pool, &system_prompt_text, storage_str) != RESULT_OK) {
                RETURN_ERR("Failed to append storage content");
            }
        }

        if (pool_string_free(pool, storage_str) != RESULT_OK) {
            RETURN_ERR("Failed to free storage string");
        }
    }

    // // Add iteration context
    // char iteration_info[256];
    // snprintf(iteration_info, sizeof(iteration_info), "\n\nCurrent iteration: %lu/%lu", 
    //          agent->iteration_count, config->agent_max_iterate);
    // if (string_append_str(pool, &system_prompt_text, iteration_info) != RESULT_OK) {
    //     RETURN_ERR("Failed to append iteration information");
    // }

    json_value_t* content_value;
    if (json_create_string(pool, system_prompt_text->data, &content_value) != RESULT_OK) {
        RETURN_ERR("Failed to create content JSON value");
    }
    printf("config->agent_prompt_system: %s\n", config->agent_prompt_system->data);
    printf("system_prompt_text: %s\n", system_prompt_text->data);
    printf("content_value: %s\n", content_value->u.string_value->data);
    if (json_object_set(pool, system_message, "content", content_value) != RESULT_OK) {
        RETURN_ERR("Failed to set content in system message");
    }

    if (json_array_append(pool, messages_array, system_message) != RESULT_OK) {
        RETURN_ERR("Failed to append system message to array");
    }

    // Clean up system prompt text
    if (pool_string_free(pool, system_prompt_text) != RESULT_OK) {
        RETURN_ERR("Failed to free system prompt text");
    }

    if (json_object_set(pool, request_json, "messages", messages_array) != RESULT_OK) {
        RETURN_ERR("Failed to set messages in request JSON");
    }

    // Serialize JSON to string
    string_t* request_body;
    if (pool_string_alloc(pool, &request_body, 65536) != RESULT_OK) {
        RETURN_ERR("Failed to allocate request body string");
    }

    if (json_stringify(pool, request_json, &request_body) != RESULT_OK) {
        RETURN_ERR("Failed to stringify request JSON");
    }

    // Debug log the request body
    printf("Request body: %s\n", request_body->data);

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
    // Parse the response text as JSON
    json_value_t* response_json;
    if (json_parse(pool, response_text, &response_json) != RESULT_OK) {
        RETURN_ERR("Failed to parse agent response as JSON");
    }

    if (response_json->type != JSON_TYPE_OBJECT) {
        RETURN_ERR("Agent response must be a JSON object");
    }

    // Process working_memory_add operations
    json_value_t* working_memory_add = json_object_get(response_json, "working_memory_add");
    if (working_memory_add && working_memory_add->type == JSON_TYPE_OBJECT) {
        json_object_t* add_obj = working_memory_add->u.object_value;
        json_object_element_t* element = add_obj->head;

        while (element) {
            // Add each key-value pair to working memory
            if (json_object_set(pool, (json_value_t*)agent->working_memory, element->key->data, element->value) != RESULT_OK) {
                RETURN_ERR("Failed to add item to working memory");
            }
            element = element->next;
        }
    }

    // Process working_memory_remove operations
    json_value_t* working_memory_remove = json_object_get(response_json, "working_memory_remove");
    if (working_memory_remove) {
        if (working_memory_remove->type == JSON_TYPE_STRING) {
            // Remove single key
            json_value_t* working_memory_value = (json_value_t*)agent->working_memory;
            if (json_object_remove(pool, working_memory_value, working_memory_remove->u.string_value->data) != RESULT_OK) {
                RETURN_ERR("Failed to remove item from working memory");
            }
        } else if (working_memory_remove->type == JSON_TYPE_ARRAY) {
            // Remove multiple keys
            json_array_t* remove_array = working_memory_remove->u.array_value;
            json_array_element_t* element = remove_array->head;

            while (element) {
                if (element->value->type == JSON_TYPE_STRING) {
                    json_value_t* working_memory_value = (json_value_t*)agent->working_memory;
                    if (json_object_remove(pool, working_memory_value, element->value->u.string_value->data) != RESULT_OK) {
                        RETURN_ERR("Failed to remove item from working memory");
                    }
                }
                element = element->next;
            }
        }
    }

    // Process storage_add operations
    json_value_t* storage_add = json_object_get(response_json, "storage_add");
    if (storage_add && storage_add->type == JSON_TYPE_OBJECT) {
        json_object_t* add_obj = storage_add->u.object_value;
        json_object_element_t* element = add_obj->head;

        while (element) {
            // Add each key-value pair to storage
            if (json_object_set(pool, (json_value_t*)agent->storage, element->key->data, element->value) != RESULT_OK) {
                RETURN_ERR("Failed to add item to storage");
            }
            element = element->next;
        }
    }

    // Process storage_remove operations
    json_value_t* storage_remove = json_object_get(response_json, "storage_remove");
    if (storage_remove) {
        if (storage_remove->type == JSON_TYPE_STRING) {
            // Remove single key
            json_value_t* storage_value = (json_value_t*)agent->storage;
            if (json_object_remove(pool, storage_value, storage_remove->u.string_value->data) != RESULT_OK) {
                RETURN_ERR("Failed to remove item from storage");
            }
        } else if (storage_remove->type == JSON_TYPE_ARRAY) {
            // Remove multiple keys
            json_array_t* remove_array = storage_remove->u.array_value;
            json_array_element_t* element = remove_array->head;

            while (element) {
                if (element->value->type == JSON_TYPE_STRING) {
                    json_value_t* storage_value = (json_value_t*)agent->storage;
                    if (json_object_remove(pool, storage_value, element->value->u.string_value->data) != RESULT_OK) {
                        RETURN_ERR("Failed to remove item from storage");
                    }
                }
                element = element->next;
            }
        }
    }

    // Process status_change operations
    json_value_t* status_change = json_object_get(response_json, "status_change");
    if (status_change && status_change->type == JSON_TYPE_STRING) {
        const char* new_status = status_change->u.string_value->data;

        if (strcmp(new_status, "thinking") == 0) {
            agent->status = AGENT_STATUS_THINKING;
        } else if (strcmp(new_status, "paging") == 0) {
            agent->status = AGENT_STATUS_PAGING;
        } else if (strcmp(new_status, "evaluating") == 0) {
            agent->status = AGENT_STATUS_EVALUATING;
        } else if (strcmp(new_status, "executing") == 0) {
            agent->status = AGENT_STATUS_EXECUTING;
        } else {
            RETURN_ERR("Invalid agent status in response");
        }
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