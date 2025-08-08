#include "agent/http.h"

result_t agent_http_send_receive(pool_t* pool, config_t* config, const string_t* prompt, string_t** response_content) {
    string_t* request_string = NULL;
    string_t* response_string = NULL;
    string_t* content_type = NULL;
    string_t* endpoint_url = NULL;

    if (agent_http_create_resources(pool, &request_string, &response_string, &content_type) != RESULT_OK) {
        RETURN_ERR("Failed to create HTTP resources");
    }

    if (agent_http_build_endpoint_url(pool, config, &endpoint_url) != RESULT_OK) {
        result_t tmp_cleanup = agent_http_cleanup_resources(pool, request_string, response_string, content_type);
        if (tmp_cleanup != RESULT_OK) {
            printf("Warning: HTTP cleanup failed after endpoint URL build error\n");
        }
        RETURN_ERR("Failed to build endpoint URL");
    }

    if (string_copy_string(pool, &request_string, prompt) != RESULT_OK) {
        result_t tmp1 = string_destroy(pool, endpoint_url);
        if (tmp1 != RESULT_OK) {
            printf("Warning: Failed to destroy endpoint_url after request build error\n");
        }
        result_t tmp2 = agent_http_cleanup_resources(pool, request_string, response_string, content_type);
        if (tmp2 != RESULT_OK) {
            printf("Warning: HTTP cleanup failed after request build error\n");
        }
        RETURN_ERR("Failed to copy prompt to request string");
    }

    if (http_post(pool, endpoint_url, content_type, request_string, &response_string) != RESULT_OK) {
        result_t tmp1 = string_destroy(pool, endpoint_url);
        if (tmp1 != RESULT_OK) {
            printf("Warning: Failed to destroy endpoint_url after HTTP POST error\n");
        }
        result_t tmp2 = agent_http_cleanup_resources(pool, request_string, response_string, content_type);
        if (tmp2 != RESULT_OK) {
            printf("Warning: HTTP cleanup failed after HTTP POST error\n");
        }
        RETURN_ERR("Failed to send HTTP POST request to LLM");
    }

    if (agent_http_extract_response_content(pool, response_string, response_content) != RESULT_OK) {
        result_t tmp1 = string_destroy(pool, endpoint_url);
        if (tmp1 != RESULT_OK) {
            printf("Warning: Failed to destroy endpoint_url after response extraction error\n");
        }
        result_t tmp2 = agent_http_cleanup_resources(pool, request_string, response_string, content_type);
        if (tmp2 != RESULT_OK) {
            printf("Warning: HTTP cleanup failed after response extraction error\n");
        }
        RETURN_ERR("Failed to extract content from LLM response");
    }

    if (string_destroy(pool, endpoint_url) != RESULT_OK) {
        RETURN_ERR("Failed to destroy endpoint URL");
    }

    if (agent_http_cleanup_resources(pool, request_string, response_string, content_type) != RESULT_OK) {
        RETURN_ERR("Failed to cleanup HTTP resources");
    }

    return RESULT_OK;
}

result_t agent_http_create_resources(pool_t* pool, string_t** request_string, string_t** response_string, string_t** content_type) {
    if (string_create(pool, request_string) != RESULT_OK) {
        RETURN_ERR("Failed to create HTTP request string");
    }

    if (string_create(pool, response_string) != RESULT_OK) {
        result_t tmp = string_destroy(pool, *request_string);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy request_string after response string create error\n");
        }
        RETURN_ERR("Failed to create HTTP response string");
    }

    if (string_create_str(pool, content_type, "application/json") != RESULT_OK) {
        result_t tmp1 = string_destroy(pool, *request_string);
        if (tmp1 != RESULT_OK) {
            printf("Warning: Failed to destroy request_string after content_type create error\n");
        }
        result_t tmp2 = string_destroy(pool, *response_string);
        if (tmp2 != RESULT_OK) {
            printf("Warning: Failed to destroy response_string after content_type create error\n");
        }
        RETURN_ERR("Failed to create HTTP content type string");
    }

    return RESULT_OK;
}

result_t agent_http_cleanup_resources(pool_t* pool, string_t* request_string, string_t* response_string, string_t* content_type) {
    result_t cleanup_result = RESULT_OK;

    if (request_string != NULL && string_destroy(pool, request_string) != RESULT_OK) {
        cleanup_result = RESULT_ERR;
    }
    if (response_string != NULL && string_destroy(pool, response_string) != RESULT_OK) {
        cleanup_result = RESULT_ERR;
    }
    if (content_type != NULL && string_destroy(pool, content_type) != RESULT_OK) {
        cleanup_result = RESULT_ERR;
    }

    if (cleanup_result != RESULT_OK) {
        RETURN_ERR("Failed to cleanup one or more HTTP resources");
    }

    return RESULT_OK;
}

result_t agent_http_extract_response_content(pool_t* pool, const string_t* response_json, string_t** content) {
    object_t* response_obj = NULL;
    object_t* choices_array = NULL;
    object_t* first_choice = NULL;
    object_t* message_obj = NULL;
    object_t* content_obj = NULL;

    if (object_parse_json(pool, &response_obj, response_json) != RESULT_OK) {
        // Dump raw response for debugging
        printf("[HTTP] Error: Failed to parse LLM response JSON. Raw size=%lu\n", (unsigned long)response_json->size);
        printf("[HTTP] Raw: %.*s\n", (int)((response_json->size < 2048) ? response_json->size : 2048), response_json->data);
        RETURN_ERR("Failed to parse LLM response JSON");
    }

    if (object_provide_str(pool, &choices_array, response_obj, "choices") != RESULT_OK) {
        printf("[HTTP] Error: Missing 'choices' in response. Top-level keys present will be printed below.\n");
        // Try to print top-level keys
        object_t* child = response_obj->child;
        while (child) {
            if (child->string && child->string->data) {
                printf("[HTTP] Top key: %.*s\n", (int)child->string->size, child->string->data);
            }
            child = child->next;
        }
        result_t tmp = object_destroy(pool, response_obj);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy response_obj after missing choices\n");
        }
        RETURN_ERR("Failed to get choices array from LLM response");
    }

    if (object_provide_str(pool, &first_choice, choices_array, "[0]") != RESULT_OK) {
        printf("[HTTP] Error: Missing first element in 'choices'. Dumping choices..\n");
        // Best-effort small dump
        string_t* dump = NULL;
        if (string_create(pool, &dump) == RESULT_OK) {
            if (object_tostring_json(pool, &dump, choices_array) == RESULT_OK) {
                printf("[HTTP] choices: %.*s\n", (int)((dump->size < 2048) ? dump->size : 2048), dump->data);
            }
            if (string_destroy(pool, dump) != RESULT_OK) {
                printf("Warning: Failed to destroy dump string for choices\n");
            }
        }
        result_t tmp = object_destroy(pool, response_obj);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy response_obj after missing first choice\n");
        }
        RETURN_ERR("Failed to get first choice from LLM response");
    }

    if (object_provide_str(pool, &message_obj, first_choice, "message") != RESULT_OK) {
        printf("[HTTP] Error: Missing 'message' in choice. Dump choice..\n");
        string_t* dump = NULL;
        if (string_create(pool, &dump) == RESULT_OK) {
            if (object_tostring_json(pool, &dump, first_choice) == RESULT_OK) {
                printf("[HTTP] choice[0]: %.*s\n", (int)((dump->size < 2048) ? dump->size : 2048), dump->data);
            }
            if (string_destroy(pool, dump) != RESULT_OK) {
                printf("Warning: Failed to destroy dump string for choice[0]\n");
            }
        }
        result_t tmp = object_destroy(pool, response_obj);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy response_obj after missing message\n");
        }
        RETURN_ERR("Failed to get message object from LLM response");
    }

    if (object_provide_str(pool, &content_obj, message_obj, "content") != RESULT_OK) {
        printf("[HTTP] Error: Missing 'content' in message. Dump message..\n");
        string_t* dump = NULL;
        if (string_create(pool, &dump) == RESULT_OK) {
            if (object_tostring_json(pool, &dump, message_obj) == RESULT_OK) {
                printf("[HTTP] message: %.*s\n", (int)((dump->size < 2048) ? dump->size : 2048), dump->data);
            }
            if (string_destroy(pool, dump) != RESULT_OK) {
                printf("Warning: Failed to destroy dump string for message\n");
            }
        }
        result_t tmp = object_destroy(pool, response_obj);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy response_obj after missing content\n");
        }
        RETURN_ERR("Failed to get content from LLM response message");
    }

    if (string_create_string(pool, content, content_obj->string) != RESULT_OK) {
        result_t tmp = object_destroy(pool, response_obj);
        if (tmp != RESULT_OK) {
            printf("Warning: Failed to destroy response_obj after content copy error\n");
        }
        RETURN_ERR("Failed to create copy of LLM response content");
    }

    if (object_destroy(pool, response_obj) != RESULT_OK) {
        RETURN_ERR("Failed to destroy response object after content extraction");
    }

    return RESULT_OK;
}

result_t agent_http_build_endpoint_url(pool_t* pool, config_t* config, string_t** endpoint_url) {
    object_t* llm_config = NULL;
    object_t* endpoint_obj = NULL;
    object_t* model_obj = NULL;
    object_t* temperature_obj = NULL;

    if (agent_http_extract_llm_config(pool, config, &llm_config, &endpoint_obj, &model_obj, &temperature_obj) != RESULT_OK) {
        RETURN_ERR("Failed to extract LLM configuration");
    }

    if (string_create_string(pool, endpoint_url, endpoint_obj->string) != RESULT_OK) {
        RETURN_ERR("Failed to create endpoint URL string");
    }

    return RESULT_OK;
}

result_t agent_http_extract_llm_config(pool_t* pool, config_t* config, object_t** llm_config, object_t** endpoint_obj, object_t** model_obj, object_t** temperature_obj) {
    if (object_provide_str(pool, llm_config, config->data, "llm") != RESULT_OK) {
        RETURN_ERR("Failed to get LLM configuration from config");
    }

    if (object_provide_str(pool, endpoint_obj, *llm_config, "endpoint") != RESULT_OK) {
        RETURN_ERR("Failed to get LLM endpoint from configuration");
    }

    if (object_provide_str(pool, model_obj, *llm_config, "model") != RESULT_OK) {
        RETURN_ERR("Failed to get LLM model from configuration");
    }

    if (object_provide_str(pool, temperature_obj, *llm_config, "temperature") != RESULT_OK) {
        *temperature_obj = NULL;
    }

    return RESULT_OK;
}
