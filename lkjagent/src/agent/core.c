// For usleep function
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "agent/core.h"

result_t agent_init(config_t* config, agent_t* agent) {
    // Initialize agent structure
    agent->json = NULL;
    agent->status = config->agent_default_status;
    agent->context_memory = NULL;
    agent->iteration_count = 0;

    return RESULT_OK;
}

static result_t agent_create_llm_request_with_context(pool_t* pool, config_t* config, agent_t* agent, const char* prompt, string_t** request_body) {
    json_value_t* root_obj;
    if (json_create_object(pool, &root_obj) != RESULT_OK) {
        RETURN_ERR("Failed to create root JSON object");
    }

    // Add model
    json_value_t* model_value;
    if (json_create_string(pool, config->llm_model->data, &model_value) != RESULT_OK) {
        RETURN_ERR("Failed to create model JSON string");
    }
    if (json_object_set(pool, root_obj, "model", model_value) != RESULT_OK) {
        RETURN_ERR("Failed to set model in JSON");
    }

    // Add temperature
    json_value_t* temperature_value;
    if (json_create_number(pool, config->llm_temperature, &temperature_value) != RESULT_OK) {
        RETURN_ERR("Failed to create temperature JSON number");
    }
    if (json_object_set(pool, root_obj, "temperature", temperature_value) != RESULT_OK) {
        RETURN_ERR("Failed to set temperature in JSON");
    }

    // Create messages array
    json_value_t* messages_array;
    if (json_create_array(pool, &messages_array) != RESULT_OK) {
        RETURN_ERR("Failed to create messages array");
    }

    // Add system message
    json_value_t* system_message_obj;
    if (json_create_object(pool, &system_message_obj) != RESULT_OK) {
        RETURN_ERR("Failed to create system message object");
    }

    json_value_t* role_system;
    if (json_create_string(pool, "system", &role_system) != RESULT_OK) {
        RETURN_ERR("Failed to create system role");
    }
    if (json_object_set(pool, system_message_obj, "role", role_system) != RESULT_OK) {
        RETURN_ERR("Failed to set system role");
    }

    json_value_t* content_system;
    if (json_create_string(pool, config->agent_prompt_system->data, &content_system) != RESULT_OK) {
        RETURN_ERR("Failed to create system content");
    }
    if (json_object_set(pool, system_message_obj, "content", content_system) != RESULT_OK) {
        RETURN_ERR("Failed to set system content");
    }

    if (json_array_append(pool, messages_array, system_message_obj) != RESULT_OK) {
        RETURN_ERR("Failed to append system message");
    }

    // Add context memory if available
    if (agent->context_memory && agent->context_memory->size > 0) {
        json_value_t* context_message_obj;
        if (json_create_object(pool, &context_message_obj) != RESULT_OK) {
            RETURN_ERR("Failed to create context message object");
        }

        json_value_t* role_assistant;
        if (json_create_string(pool, "assistant", &role_assistant) != RESULT_OK) {
            RETURN_ERR("Failed to create assistant role");
        }
        if (json_object_set(pool, context_message_obj, "role", role_assistant) != RESULT_OK) {
            RETURN_ERR("Failed to set assistant role");
        }

        json_value_t* content_context;
        if (json_create_string(pool, agent->context_memory->data, &content_context) != RESULT_OK) {
            RETURN_ERR("Failed to create context content");
        }
        if (json_object_set(pool, context_message_obj, "content", content_context) != RESULT_OK) {
            RETURN_ERR("Failed to set context content");
        }

        if (json_array_append(pool, messages_array, context_message_obj) != RESULT_OK) {
            RETURN_ERR("Failed to append context message");
        }
    }

    // Add user message with the current prompt
    json_value_t* user_message_obj;
    if (json_create_object(pool, &user_message_obj) != RESULT_OK) {
        RETURN_ERR("Failed to create user message object");
    }

    json_value_t* role_user;
    if (json_create_string(pool, "user", &role_user) != RESULT_OK) {
        RETURN_ERR("Failed to create user role");
    }
    if (json_object_set(pool, user_message_obj, "role", role_user) != RESULT_OK) {
        RETURN_ERR("Failed to set user role");
    }

    // Create enhanced prompt with iteration context
    string_t* enhanced_prompt;
    if (pool_string_alloc(pool, &enhanced_prompt, 4096) != RESULT_OK) {
        RETURN_ERR("Failed to allocate enhanced prompt string");
    }

    if (string_assign(pool, &enhanced_prompt, prompt) != RESULT_OK) {
        RETURN_ERR("Failed to assign base prompt");
    }

    if (agent->iteration_count > 0) {
        char iteration_info[256];
        snprintf(iteration_info, sizeof(iteration_info), "\n\nIteration: %lu/%lu", agent->iteration_count + 1, config->agent_max_iterate);
        if (string_append_str(pool, &enhanced_prompt, iteration_info) != RESULT_OK) {
            RETURN_ERR("Failed to append iteration info");
        }
    }

    json_value_t* content_user;
    if (json_create_string(pool, enhanced_prompt->data, &content_user) != RESULT_OK) {
        RETURN_ERR("Failed to create user content");
    }
    if (json_object_set(pool, user_message_obj, "content", content_user) != RESULT_OK) {
        RETURN_ERR("Failed to set user content");
    }

    if (json_array_append(pool, messages_array, user_message_obj) != RESULT_OK) {
        RETURN_ERR("Failed to append user message");
    }

    // Set messages in root object
    if (json_object_set(pool, root_obj, "messages", messages_array) != RESULT_OK) {
        RETURN_ERR("Failed to set messages in JSON");
    }

    // Convert to string
    if (json_stringify(pool, root_obj, *request_body) != RESULT_OK) {
        RETURN_ERR("Failed to stringify JSON request");
    }

    return RESULT_OK;
}

static result_t agent_send_llm_request(pool_t* pool, config_t* config, agent_t* agent, const char* prompt, string_t** response_content) {
    string_t* request_body;
    if (pool_string_alloc(pool, &request_body, 65536) != RESULT_OK) {
        RETURN_ERR("Failed to allocate request body string");
    }

    if (agent_create_llm_request_with_context(pool, config, agent, prompt, &request_body) != RESULT_OK) {
        if (pool_string_free(pool, request_body) != RESULT_OK) {
            RETURN_ERR("Failed to free request body after create failure");
        }
        RETURN_ERR("Failed to create LLM request");
    }

    printf("Sending request to LLM: %.*s\n", (int)request_body->size, request_body->data);

    http_response_t response;
    if (http_response_init(pool, &response) != RESULT_OK) {
        if (pool_string_free(pool, request_body) != RESULT_OK) {
            RETURN_ERR("Failed to free request body after response init failure");
        }
        RETURN_ERR("Failed to initialize HTTP response");
    }

    result_t http_result = http_post_json(pool, config->llm_endpoint->data, request_body->data, &response);

    if (http_result != RESULT_OK) {
        printf("LLM endpoint not available, using mock response...\n");
        if (pool_string_free(pool, request_body) != RESULT_OK) {
            RETURN_ERR("Failed to free request body after HTTP failure");
        }

        // Create a mock response based on the current prompt
        if (pool_string_alloc(pool, response_content, 1024) != RESULT_OK) {
            RETURN_ERR("Failed to allocate mock response content string");
        }

        if (strstr(prompt, "thinking")) {
            if (string_assign(pool, response_content, "I am analyzing the current situation and considering the best approach. [PAGING]") != RESULT_OK) {
                RETURN_ERR("Failed to assign thinking mock response");
            }
        } else if (strstr(prompt, "paging")) {
            if (string_assign(pool, response_content, "I am gathering more information about the environment and available resources. [EVALUATING]") != RESULT_OK) {
                RETURN_ERR("Failed to assign paging mock response");
            }
        } else if (strstr(prompt, "evaluating")) {
            if (string_assign(pool, response_content, "Based on my analysis, I will now proceed with the planned actions. [EXECUTING]") != RESULT_OK) {
                RETURN_ERR("Failed to assign evaluating mock response");
            }
        } else if (strstr(prompt, "executing")) {
            if (string_assign(pool, response_content, "Task completed successfully. Ready for the next challenge. [COMPLETE]") != RESULT_OK) {
                RETURN_ERR("Failed to assign executing mock response");
            }
        } else {
            if (string_assign(pool, response_content, "I am ready to assist you with your request. [THINKING]") != RESULT_OK) {
                RETURN_ERR("Failed to assign default mock response");
            }
        }

        return RESULT_OK;
    }

    printf("Received response with status: %d\n", response.status_code);

    if (response.status_code != 200) {
        if (pool_string_free(pool, request_body) != RESULT_OK) {
            RETURN_ERR("Failed to free request body after status code error");
        }
        RETURN_ERR("LLM returned non-200 status code");
    }

    if (!response.body || response.body->size == 0) {
        if (pool_string_free(pool, request_body) != RESULT_OK) {
            RETURN_ERR("Failed to free request body after empty response");
        }
        RETURN_ERR("LLM returned empty response");
    }

    printf("Raw LLM response: %.*s\n", (int)response.body->size, response.body->data);

    // Try to parse the response JSON to extract the content
    json_value_t* response_json;
    result_t parse_result = json_parse(pool, response.body, &response_json);

    if (parse_result == RESULT_OK && response_json->type == JSON_TYPE_OBJECT) {
        // Full JSON parsing succeeded
        json_value_t* choices = json_object_get(response_json, "choices");
        if (choices && choices->type == JSON_TYPE_ARRAY) {
            json_value_t* first_choice = json_array_get(choices, 0);
            if (first_choice && first_choice->type == JSON_TYPE_OBJECT) {
                json_value_t* message = json_object_get(first_choice, "message");
                if (message && message->type == JSON_TYPE_OBJECT) {
                    json_value_t* content = json_object_get(message, "content");
                    if (content && content->type == JSON_TYPE_STRING) {
                        if (pool_string_alloc(pool, response_content, content->u.string_value->size + 1) != RESULT_OK) {
                            if (pool_string_free(pool, request_body) != RESULT_OK) {
                                RETURN_ERR("Failed to free request body after content allocation failure");
                            }
                            RETURN_ERR("Failed to allocate response content string");
                        }

                        if (string_assign(pool, response_content, content->u.string_value->data) != RESULT_OK) {
                            if (pool_string_free(pool, request_body) != RESULT_OK) {
                                RETURN_ERR("Failed to free request body after content assignment failure");
                            }
                            RETURN_ERR("Failed to assign response content");
                        }

                        if (pool_string_free(pool, request_body) != RESULT_OK) {
                            RETURN_ERR("Failed to free request body string");
                        }

                        return RESULT_OK;
                    }
                }
            }
        }
    }

    // Fallback: extract content using simple string parsing
    printf("JSON parsing failed, trying simple content extraction...\n");

    const char* content_start = strstr(response.body->data, "\"content\": \"");
    if (!content_start) {
        if (pool_string_free(pool, request_body) != RESULT_OK) {
            RETURN_ERR("Failed to free request body after content extraction failure");
        }
        RETURN_ERR("Could not find content field in response");
    }

    content_start += 12;  // Skip '"content": "'
    const char* content_end = content_start;

    // Find the end of the content string, handling escape sequences
    while (*content_end && !(*content_end == '"' && *(content_end - 1) != '\\')) {
        content_end++;
    }

    if (!*content_end) {
        if (pool_string_free(pool, request_body) != RESULT_OK) {
            RETURN_ERR("Failed to free request body after end content extraction failure");
        }
        RETURN_ERR("Could not find end of content field");
    }

    size_t content_length = content_end - content_start;

    if (pool_string_alloc(pool, response_content, content_length + 1) != RESULT_OK) {
        if (pool_string_free(pool, request_body) != RESULT_OK) {
            RETURN_ERR("Failed to free request body after response allocation failure");
        }
        RETURN_ERR("Failed to allocate response content string");
    }

    if (string_append_data(pool, response_content, content_start, content_length) != RESULT_OK) {
        if (pool_string_free(pool, request_body) != RESULT_OK) {
            RETURN_ERR("Failed to free request body after response copy failure");
        }
        RETURN_ERR("Failed to copy response content");
    }

    if (pool_string_free(pool, request_body) != RESULT_OK) {
        RETURN_ERR("Failed to free request body string");
    }

    return RESULT_OK;
}

static agent_status_t agent_parse_next_status(const string_t* response) {
    // Look for explicit status indicators in the response
    if (string_find(response, "[THINKING]") >= 0) {
        return AGENT_STATUS_THINKING;
    } else if (string_find(response, "[PAGING]") >= 0) {
        return AGENT_STATUS_PAGING;
    } else if (string_find(response, "[EVALUATING]") >= 0) {
        return AGENT_STATUS_EVALUATING;
    } else if (string_find(response, "[EXECUTING]") >= 0) {
        return AGENT_STATUS_EXECUTING;
    }

    // If no explicit status found, return unknown status (will use default transition)
    return (agent_status_t)-1;
}

result_t agent_run(pool_t* pool, config_t* config, agent_t* agent) {
    printf("Agent is running...\n");

    // Initialize agent context memory
    if (!agent->context_memory) {
        if (pool_string_alloc(pool, &agent->context_memory, 4096) != RESULT_OK) {
            RETURN_ERR("Failed to allocate agent context memory");
        }
        if (string_assign(pool, &agent->context_memory, "Agent initialized and ready to assist.") != RESULT_OK) {
            RETURN_ERR("Failed to initialize context memory");
        }
    }

    for (uint64_t i = 0; i < config->agent_max_iterate; i++) {
        agent->iteration_count = i;
        printf("--- Iteration %lu ---\n", i);

        const char* current_prompt = NULL;
        const char* status_name = NULL;

        if (agent->status == AGENT_STATUS_THINKING) {
            current_prompt = config->agent_prompt_thinking->data;
            status_name = "THINKING";
        } else if (agent->status == AGENT_STATUS_PAGING) {
            current_prompt = config->agent_prompt_paging->data;
            status_name = "PAGING";
        } else if (agent->status == AGENT_STATUS_EVALUATING) {
            current_prompt = config->agent_prompt_evaluating->data;
            status_name = "EVALUATING";
        } else if (agent->status == AGENT_STATUS_EXECUTING) {
            current_prompt = config->agent_prompt_executing->data;
            status_name = "EXECUTING";
        } else {
            RETURN_ERR("Unknown agent status");
        }

        printf("Status: %s\n", status_name);
        printf("Prompt: %s\n", current_prompt);

        string_t* response_content;
        if (agent_send_llm_request(pool, config, agent, current_prompt, &response_content) != RESULT_OK) {
            printf("Failed to get response from LLM, continuing...\n");
            // Continue to next iteration instead of failing completely
            continue;
        }

        printf("LLM Response: %.*s\n", (int)response_content->size, response_content->data);

        // Update context memory with latest response (keep it bounded)
        if (agent->context_memory->size < 2048) {  // Leave room for more content
            if (string_append_str(pool, &agent->context_memory, " -> ") != RESULT_OK ||
                string_append_str(pool, &agent->context_memory, status_name) != RESULT_OK) {
                printf("Warning: Failed to update context memory\n");
            } else {
                // Add a very truncated version of the response to memory
                size_t response_len = response_content->size;
                if (response_len > 100)
                    response_len = 100;  // Limit to 100 chars

                if (string_append_data(pool, &agent->context_memory, response_content->data, response_len) != RESULT_OK) {
                    printf("Warning: Failed to append response to context memory\n");
                }
                if (response_content->size > 100) {
                    if (string_append_str(pool, &agent->context_memory, "...") != RESULT_OK) {
                        printf("Warning: Failed to append truncation marker\n");
                    }
                }
            }
        }

        // Determine next status based on response
        agent_status_t next_status = agent_parse_next_status(response_content);

        // State transition logic
        if (next_status != (agent_status_t)-1) {
            // Explicit status transition requested
            agent->status = next_status;
        } else {
            // Default cycle progression
            switch (agent->status) {
                case AGENT_STATUS_THINKING:
                    agent->status = AGENT_STATUS_PAGING;
                    break;
                case AGENT_STATUS_PAGING:
                    agent->status = AGENT_STATUS_EVALUATING;
                    break;
                case AGENT_STATUS_EVALUATING:
                    agent->status = AGENT_STATUS_EXECUTING;
                    break;
                case AGENT_STATUS_EXECUTING:
                    agent->status = AGENT_STATUS_THINKING;
                    break;
            }
        }

        // Check if response indicates completion
        if (string_find(response_content, "[COMPLETE]") >= 0 ||
            string_find(response_content, "[DONE]") >= 0) {
            printf("Agent indicates completion, stopping iterations.\n");
            if (pool_string_free(pool, response_content) != RESULT_OK) {
                RETURN_ERR("Failed to free response content string");
            }
            break;
        }

        if (pool_string_free(pool, response_content) != RESULT_OK) {
            RETURN_ERR("Failed to free response content string");
        }

        // Add a small delay between iterations
        printf("Waiting before next iteration...\n\n");
        usleep(1000000);  // 1 second delay
    }

    printf("Agent run completed\n");
    printf("Final context memory: %.*s\n", (int)agent->context_memory->size, agent->context_memory->data);
    return RESULT_OK;
}