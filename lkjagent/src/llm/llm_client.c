/**
 * @file llm_client.c
 * @brief LLM client implementation for LMStudio communication
 * 
 * This module implements the high-level LLM client interface for communicating
 * with LMStudio and other compatible LLM services. It provides request construction,
 * response processing, and model management with comprehensive error handling.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/llm/llm_client.h"
#include "../include/json_parser.h"
#include "../include/json_builder.h"
#include "../lkjagent.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

/**
 * @defgroup LLM_Client_Internal Internal LLM Client Functions
 * @{
 */

/**
 * @brief Build JSON request payload for LLM
 * 
 * @param prompt Prompt text to send
 * @param params Request parameters
 * @param request_json Buffer to store JSON request
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t build_llm_request(const char* prompt, const llm_request_params_t* params, data_t* request_json) {
    if (!prompt || !params || !request_json) {
        RETURN_ERR("Invalid parameters for LLM request building");
        return RESULT_ERR;
    }
    
    /* Start JSON object */
    if (json_build_object(request_json) != RESULT_OK) {
        RETURN_ERR("Failed to initialize JSON object for LLM request");
        return RESULT_ERR;
    }
    
    /* Add model */
    char model_field[256];
    snprintf(model_field, sizeof(model_field), "\"model\": \"%s\",", params->model);
    if (data_append(request_json, model_field, 0) != RESULT_OK) {
        RETURN_ERR("Failed to add model field to LLM request");
        return RESULT_ERR;
    }
    
    /* Add prompt */
    char prompt_field[64];
    snprintf(prompt_field, sizeof(prompt_field), "\"prompt\": \"");
    if (data_append(request_json, prompt_field, 0) != RESULT_OK) {
        RETURN_ERR("Failed to add prompt field start to LLM request");
        return RESULT_ERR;
    }
    
    /* Escape prompt content */
    for (const char* p = prompt; *p; p++) {
        if (*p == '"' || *p == '\\') {
            if (data_append(request_json, "\\", 1) != RESULT_OK) {
                RETURN_ERR("Failed to escape character in prompt");
                return RESULT_ERR;
            }
        }
        if (*p == '\n') {
            if (data_append(request_json, "\\n", 2) != RESULT_OK) {
                RETURN_ERR("Failed to escape newline in prompt");
                return RESULT_ERR;
            }
        } else if (*p == '\r') {
            if (data_append(request_json, "\\r", 2) != RESULT_OK) {
                RETURN_ERR("Failed to escape carriage return in prompt");
                return RESULT_ERR;
            }
        } else if (*p == '\t') {
            if (data_append(request_json, "\\t", 2) != RESULT_OK) {
                RETURN_ERR("Failed to escape tab in prompt");
                return RESULT_ERR;
            }
        } else {
            if (data_append(request_json, p, 1) != RESULT_OK) {
                RETURN_ERR("Failed to append prompt character");
                return RESULT_ERR;
            }
        }
    }
    
    if (data_append(request_json, "\",", 2) != RESULT_OK) {
        RETURN_ERR("Failed to close prompt field in LLM request");
        return RESULT_ERR;
    }
    
    /* Add max_tokens */
    char max_tokens_field[64];
    snprintf(max_tokens_field, sizeof(max_tokens_field), "\"max_tokens\": %u,", params->max_tokens);
    if (data_append(request_json, max_tokens_field, 0) != RESULT_OK) {
        RETURN_ERR("Failed to add max_tokens field to LLM request");
        return RESULT_ERR;
    }
    
    /* Add temperature */
    char temp_field[64];
    snprintf(temp_field, sizeof(temp_field), "\"temperature\": %.2f,", params->temperature);
    if (data_append(request_json, temp_field, 0) != RESULT_OK) {
        RETURN_ERR("Failed to add temperature field to LLM request");
        return RESULT_ERR;
    }
    
    /* Add top_p if specified */
    if (params->top_p > 0.0f && params->top_p <= 1.0f) {
        char top_p_field[64];
        snprintf(top_p_field, sizeof(top_p_field), "\"top_p\": %.2f,", params->top_p);
        if (data_append(request_json, top_p_field, 0) != RESULT_OK) {
            RETURN_ERR("Failed to add top_p field to LLM request");
            return RESULT_ERR;
        }
    }
    
    /* Add frequency_penalty if specified */
    if (params->frequency_penalty != 0.0f) {
        char freq_penalty_field[64];
        snprintf(freq_penalty_field, sizeof(freq_penalty_field), "\"frequency_penalty\": %.2f,", params->frequency_penalty);
        if (data_append(request_json, freq_penalty_field, 0) != RESULT_OK) {
            RETURN_ERR("Failed to add frequency_penalty field to LLM request");
            return RESULT_ERR;
        }
    }
    
    /* Add presence_penalty if specified */
    if (params->presence_penalty != 0.0f) {
        char pres_penalty_field[64];
        snprintf(pres_penalty_field, sizeof(pres_penalty_field), "\"presence_penalty\": %.2f,", params->presence_penalty);
        if (data_append(request_json, pres_penalty_field, 0) != RESULT_OK) {
            RETURN_ERR("Failed to add presence_penalty field to LLM request");
            return RESULT_ERR;
        }
    }
    
    /* Add stop sequences if specified */
    if (params->stop_count > 0) {
        if (data_append(request_json, "\"stop\": [", 9) != RESULT_OK) {
            RETURN_ERR("Failed to add stop array start to LLM request");
            return RESULT_ERR;
        }
        
        for (size_t i = 0; i < params->stop_count && i < 4; i++) {
            if (i > 0) {
                if (data_append(request_json, ",", 1) != RESULT_OK) {
                    RETURN_ERR("Failed to add stop array separator");
                    return RESULT_ERR;
                }
            }
            
            char stop_item[128];
            snprintf(stop_item, sizeof(stop_item), "\"%s\"", params->stop_sequences[i]);
            if (data_append(request_json, stop_item, 0) != RESULT_OK) {
                RETURN_ERR("Failed to add stop sequence to LLM request");
                return RESULT_ERR;
            }
        }
        
        if (data_append(request_json, "],", 2) != RESULT_OK) {
            RETURN_ERR("Failed to close stop array in LLM request");
            return RESULT_ERR;
        }
    }
    
    /* Add stream parameter */
    char stream_field[32];
    snprintf(stream_field, sizeof(stream_field), "\"stream\": %s", params->stream ? "true" : "false");
    if (data_append(request_json, stream_field, 0) != RESULT_OK) {
        RETURN_ERR("Failed to add stream field to LLM request");
        return RESULT_ERR;
    }
    
    /* Close JSON object */
    if (data_append(request_json, "}", 1) != RESULT_OK) {
        RETURN_ERR("Failed to close JSON object for LLM request");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

/**
 * @brief Parse LLM response JSON
 * 
 * @param response_json JSON response from LLM
 * @param llm_response Response structure to populate
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t parse_llm_response(const char* response_json, llm_response_t* llm_response) {
    if (!response_json || !llm_response) {
        RETURN_ERR("Invalid parameters for LLM response parsing");
        return RESULT_ERR;
    }
    
    /* Simple JSON parsing - look for key patterns */
    
    /* Extract text content */
    const char* text_start = strstr(response_json, "\"text\":");
    if (!text_start) {
        /* Try alternative field names */
        text_start = strstr(response_json, "\"content\":");
        if (!text_start) {
            text_start = strstr(response_json, "\"response\":");
        }
    }
    
    if (text_start) {
        /* Find the start of the actual text value */
        const char* value_start = strchr(text_start, ':');
        if (value_start) {
            value_start++;
            /* Skip whitespace */
            while (*value_start && (*value_start == ' ' || *value_start == '\t' || *value_start == '\n')) {
                value_start++;
            }
            
            if (*value_start == '"') {
                value_start++; /* Skip opening quote */
                
                /* Find end quote */
                const char* value_end = value_start;
                while (*value_end && *value_end != '"') {
                    if (*value_end == '\\' && *(value_end + 1)) {
                        value_end += 2; /* Skip escaped character */
                    } else {
                        value_end++;
                    }
                }
                
                if (*value_end == '"') {
                    /* Extract and unescape content */
                    size_t content_len = value_end - value_start;
                    if (content_len > 0) {
                        char* temp_content = malloc(content_len + 1);
                        if (!temp_content) {
                            RETURN_ERR("Failed to allocate memory for response content");
                            return RESULT_ERR;
                        }
                        
                        /* Copy and unescape */
                        size_t out_pos = 0;
                        for (const char* p = value_start; p < value_end; p++) {
                            if (*p == '\\' && p + 1 < value_end) {
                                char next = *(p + 1);
                                if (next == 'n') {
                                    temp_content[out_pos++] = '\n';
                                } else if (next == 'r') {
                                    temp_content[out_pos++] = '\r';
                                } else if (next == 't') {
                                    temp_content[out_pos++] = '\t';
                                } else if (next == '"') {
                                    temp_content[out_pos++] = '"';
                                } else if (next == '\\') {
                                    temp_content[out_pos++] = '\\';
                                } else {
                                    temp_content[out_pos++] = next;
                                }
                                p++; /* Skip escaped character */
                            } else {
                                temp_content[out_pos++] = *p;
                            }
                        }
                        temp_content[out_pos] = '\0';
                        
                        if (data_set(&llm_response->content, temp_content, 0) != RESULT_OK) {
                            free(temp_content);
                            RETURN_ERR("Failed to set response content");
                            return RESULT_ERR;
                        }
                        
                        free(temp_content);
                    }
                }
            }
        }
    }
    
    /* Extract model name */
    const char* model_start = strstr(response_json, "\"model\":");
    if (model_start) {
        const char* value_start = strchr(model_start, ':');
        if (value_start) {
            value_start++;
            while (*value_start && (*value_start == ' ' || *value_start == '\t' || *value_start == '\n')) {
                value_start++;
            }
            
            if (*value_start == '"') {
                value_start++;
                const char* value_end = strchr(value_start, '"');
                if (value_end) {
                    size_t model_len = MIN(value_end - value_start, sizeof(llm_response->model) - 1);
                    strncpy(llm_response->model, value_start, model_len);
                    llm_response->model[model_len] = '\0';
                }
            }
        }
    }
    
    /* Extract token counts */
    const char* usage_start = strstr(response_json, "\"usage\":");
    if (usage_start) {
        /* Look for completion_tokens */
        const char* completion_tokens = strstr(usage_start, "\"completion_tokens\":");
        if (completion_tokens) {
            llm_response->tokens_generated = strtoul(completion_tokens + 20, NULL, 10);
        }
        
        /* Look for prompt_tokens */
        const char* prompt_tokens = strstr(usage_start, "\"prompt_tokens\":");
        if (prompt_tokens) {
            llm_response->tokens_prompt = strtoul(prompt_tokens + 16, NULL, 10);
        }
        
        /* Look for total_tokens */
        const char* total_tokens = strstr(usage_start, "\"total_tokens\":");
        if (total_tokens) {
            llm_response->tokens_total = strtoul(total_tokens + 15, NULL, 10);
        }
    }
    
    /* Calculate total if not provided */
    if (llm_response->tokens_total == 0) {
        llm_response->tokens_total = llm_response->tokens_generated + llm_response->tokens_prompt;
    }
    
    /* Extract finish reason */
    const char* finish_reason_start = strstr(response_json, "\"finish_reason\":");
    if (finish_reason_start) {
        const char* value_start = strchr(finish_reason_start, ':');
        if (value_start) {
            value_start++;
            while (*value_start && (*value_start == ' ' || *value_start == '\t' || *value_start == '\n')) {
                value_start++;
            }
            
            if (*value_start == '"') {
                value_start++;
                const char* value_end = strchr(value_start, '"');
                if (value_end) {
                    size_t reason_len = MIN(value_end - value_start, sizeof(llm_response->finish_reason) - 1);
                    strncpy(llm_response->finish_reason, value_start, reason_len);
                    llm_response->finish_reason[reason_len] = '\0';
                }
            }
        }
    }
    
    /* Generate request ID if not present */
    if (llm_response->request_id[0] == '\0') {
        snprintf(llm_response->request_id, sizeof(llm_response->request_id), 
                "req_%lu_%d", (unsigned long)time(NULL), rand() % 10000);
    }
    
    return RESULT_OK;
}

/**
 * @brief Apply default LLM parameters
 * 
 * @param params Parameters structure to populate with defaults
 */
static void apply_default_params(llm_request_params_t* params) {
    if (!params) return;
    
    if (params->model[0] == '\0') {
        strcpy(params->model, "gpt-3.5-turbo");
    }
    
    if (params->max_tokens == 0) {
        params->max_tokens = 1000;
    }
    
    if (params->temperature < 0.0f || params->temperature > 2.0f) {
        params->temperature = 0.7f;
    }
    
    if (params->top_p <= 0.0f || params->top_p > 1.0f) {
        params->top_p = 1.0f;
    }
    
    if (params->frequency_penalty < -2.0f || params->frequency_penalty > 2.0f) {
        params->frequency_penalty = 0.0f;
    }
    
    if (params->presence_penalty < -2.0f || params->presence_penalty > 2.0f) {
        params->presence_penalty = 0.0f;
    }
    
    /* Default stop sequences can be empty */
    if (params->stop_count > 4) {
        params->stop_count = 4;
    }
}

/** @} */

result_t llm_client_init(llm_client_t* client, const llm_client_config_t* config) {
    if (!client) {
        RETURN_ERR("LLM client pointer is NULL");
        return RESULT_ERR;
    }
    
    /* Initialize structure */
    memset(client, 0, sizeof(llm_client_t));
    
    /* Apply configuration or defaults */
    if (config) {
        client->config = *config;
    } else {
        /* Default configuration */
        strcpy(client->config.base_url, "http://localhost:1234");
        strcpy(client->config.default_model, "gpt-3.5-turbo");
        client->config.request_timeout = 60;
        client->config.connect_timeout = 10;
        client->config.max_retries = 3;
        client->config.enable_caching = false;
        client->config.cache_ttl = 300;
        
        /* Default request parameters */
        strcpy(client->config.default_params.model, "gpt-3.5-turbo");
        client->config.default_params.max_tokens = 1000;
        client->config.default_params.temperature = 0.7f;
        client->config.default_params.top_p = 1.0f;
        client->config.default_params.frequency_penalty = 0.0f;
        client->config.default_params.presence_penalty = 0.0f;
        client->config.default_params.stop_count = 0;
        client->config.default_params.stream = false;
    }
    
    /* Initialize HTTP client */
    http_client_config_t http_config;
    memset(&http_config, 0, sizeof(http_config));
    http_config.connect_timeout = client->config.connect_timeout;
    http_config.request_timeout = client->config.request_timeout;
    http_config.max_retries = client->config.max_retries;
    http_config.retry_delay = 2000;
    http_config.max_response_size = 2 * 1024 * 1024; /* 2MB */
    strcpy(http_config.user_agent, "LKJAgent-LLM/1.0");
    http_config.enable_keepalive = false;
    
    if (http_client_init(&client->http_client, &http_config) != RESULT_OK) {
        RETURN_ERR("Failed to initialize HTTP client for LLM");
        return RESULT_ERR;
    }
    
    /* Initialize data buffers */
    if (data_init(&client->available_models, 1024) != RESULT_OK) {
        http_client_cleanup(&client->http_client);
        RETURN_ERR("Failed to initialize available models buffer");
        return RESULT_ERR;
    }
    
    if (data_init(&client->model_capabilities, 512) != RESULT_OK) {
        http_client_cleanup(&client->http_client);
        data_clear(&client->available_models);
        RETURN_ERR("Failed to initialize model capabilities buffer");
        return RESULT_ERR;
    }
    
    if (data_init(&client->request_cache, 2048) != RESULT_OK) {
        http_client_cleanup(&client->http_client);
        data_clear(&client->available_models);
        data_clear(&client->model_capabilities);
        RETURN_ERR("Failed to initialize request cache buffer");
        return RESULT_ERR;
    }
    
    /* Set API key header if provided */
    if (client->config.api_key[0] != '\0') {
        char auth_header[512];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s\r\n", client->config.api_key);
        if (http_client_set_headers(&client->http_client, auth_header) != RESULT_OK) {
            /* Non-fatal error, continue without API key */
        }
    }
    
    /* Initialize statistics */
    client->stats.requests_sent = 0;
    client->stats.requests_succeeded = 0;
    client->stats.requests_failed = 0;
    client->stats.total_tokens_generated = 0;
    client->stats.total_response_time = 0;
    client->stats.last_request_time = 0;
    
    return RESULT_OK;
}

result_t llm_send_request(llm_client_t* client, const char* prompt, const llm_request_params_t* params, llm_response_t* response) {
    if (!client || !prompt || !response) {
        RETURN_ERR("Invalid parameters for LLM request");
        return RESULT_ERR;
    }
    
    /* Use provided parameters or defaults */
    llm_request_params_t request_params;
    if (params) {
        request_params = *params;
    } else {
        request_params = client->config.default_params;
    }
    
    /* Apply defaults for missing values */
    apply_default_params(&request_params);
    
    /* Update statistics */
    client->stats.requests_sent++;
    client->stats.last_request_time = time(NULL);
    
    /* Build request JSON */
    data_t request_json;
    if (data_init(&request_json, strlen(prompt) + 1024) != RESULT_OK) {
        client->stats.requests_failed++;
        RETURN_ERR("Failed to initialize request JSON buffer");
        return RESULT_ERR;
    }
    
    if (build_llm_request(prompt, &request_params, &request_json) != RESULT_OK) {
        data_clear(&request_json);
        client->stats.requests_failed++;
        RETURN_ERR("Failed to build LLM request JSON");
        return RESULT_ERR;
    }
    
    /* Construct URL */
    char url[512];
    snprintf(url, sizeof(url), "%s/v1/completions", client->config.base_url);
    
    /* Send HTTP request */
    http_response_t http_response;
    if (http_response_init(&http_response, 1024 * 1024) != RESULT_OK) {
        data_clear(&request_json);
        client->stats.requests_failed++;
        RETURN_ERR("Failed to initialize HTTP response");
        return RESULT_ERR;
    }
    
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    result_t http_result = http_client_post(&client->http_client, url, request_json.data, &http_response);
    
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    uint64_t request_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                           (end_time.tv_nsec - start_time.tv_nsec) / 1000000;
    
    data_clear(&request_json);
    
    if (http_result != RESULT_OK) {
        http_response_cleanup(&http_response);
        client->stats.requests_failed++;
        RETURN_ERR("HTTP request to LLM service failed");
        return RESULT_ERR;
    }
    
    /* Process response */
    response->response_time = request_time;
    
    result_t parse_result = llm_receive_response(client, &http_response, response);
    
    http_response_cleanup(&http_response);
    
    if (parse_result != RESULT_OK) {
        client->stats.requests_failed++;
        RETURN_ERR("Failed to parse LLM response");
        return RESULT_ERR;
    }
    
    /* Update statistics */
    client->stats.requests_succeeded++;
    client->stats.total_tokens_generated += response->tokens_generated;
    client->stats.total_response_time += request_time;
    
    return RESULT_OK;
}

result_t llm_receive_response(llm_client_t* client, const http_response_t* http_response, llm_response_t* llm_response) {
    if (!client || !http_response || !llm_response) {
        RETURN_ERR("Invalid parameters for LLM response processing");
        return RESULT_ERR;
    }
    
    /* Check HTTP status */
    if (http_response->status_code != HTTP_STATUS_OK) {
        RETURN_ERR("LLM service returned non-OK status");
        return RESULT_ERR;
    }
    
    /* Check if response has content */
    if (http_response->body.size == 0 || !http_response->body.data) {
        RETURN_ERR("LLM response is empty");
        return RESULT_ERR;
    }
    
    /* Parse JSON response */
    if (parse_llm_response(http_response->body.data, llm_response) != RESULT_OK) {
        RETURN_ERR("Failed to parse LLM JSON response");
        return RESULT_ERR;
    }
    
    /* Validate response has content */
    if (llm_response->content.size == 0) {
        RETURN_ERR("LLM response contains no generated content");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t llm_client_configure(llm_client_t* client, const llm_client_config_t* config) {
    if (!client || !config) {
        RETURN_ERR("Invalid parameters for LLM client configuration");
        return RESULT_ERR;
    }
    
    /* Update configuration */
    client->config = *config;
    
    /* Update HTTP client timeouts */
    if (http_client_set_timeout(&client->http_client, config->connect_timeout, config->request_timeout) != RESULT_OK) {
        RETURN_ERR("Failed to update HTTP client timeouts");
        return RESULT_ERR;
    }
    
    /* Update API key if changed */
    if (config->api_key[0] != '\0') {
        char auth_header[512];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s\r\n", config->api_key);
        if (http_client_set_headers(&client->http_client, auth_header) != RESULT_OK) {
            /* Non-fatal error */
        }
    }
    
    return RESULT_OK;
}

result_t llm_client_test_connection(llm_client_t* client, uint64_t* response_time) {
    if (!client || !response_time) {
        RETURN_ERR("Invalid parameters for connection test");
        return RESULT_ERR;
    }
    
    /* Parse base URL for connectivity test */
    char host[256];
    uint16_t port = 80;
    
    /* Simple URL parsing for host and port */
    const char* url = client->config.base_url;
    if (strncmp(url, "http://", 7) == 0) {
        url += 7;
        const char* port_start = strchr(url, ':');
        const char* path_start = strchr(url, '/');
        
        if (port_start && (!path_start || port_start < path_start)) {
            size_t host_len = port_start - url;
            if (host_len < sizeof(host)) {
                strncpy(host, url, host_len);
                host[host_len] = '\0';
                port = (uint16_t)strtoul(port_start + 1, NULL, 10);
            }
        } else if (path_start) {
            size_t host_len = path_start - url;
            if (host_len < sizeof(host)) {
                strncpy(host, url, host_len);
                host[host_len] = '\0';
            }
        } else {
            strncpy(host, url, sizeof(host) - 1);
            host[sizeof(host) - 1] = '\0';
        }
    } else {
        RETURN_ERR("Unsupported URL format for connection test");
        return RESULT_ERR;
    }
    
    if (port == 0) port = 80;
    
    /* Test connectivity */
    return http_client_test_connectivity(&client->http_client, host, port, response_time);
}

result_t llm_client_get_models(llm_client_t* client, data_t* models_list) {
    if (!client || !models_list) {
        RETURN_ERR("Invalid parameters for getting models list");
        return RESULT_ERR;
    }
    
    /* Construct models endpoint URL */
    char url[512];
    snprintf(url, sizeof(url), "%s/v1/models", client->config.base_url);
    
    /* Send GET request */
    http_response_t response;
    if (http_response_init(&response, 4096) != RESULT_OK) {
        RETURN_ERR("Failed to initialize HTTP response for models request");
        return RESULT_ERR;
    }
    
    result_t result = http_client_get(&client->http_client, url, &response);
    
    if (result == RESULT_OK && response.status_code == HTTP_STATUS_OK) {
        /* Copy response to models list */
        if (data_set(models_list, response.body.data, 0) != RESULT_OK) {
            result = RESULT_ERR;
            RETURN_ERR("Failed to copy models list response");
        } else {
            /* Cache models list in client */
            if (data_set(&client->available_models, response.body.data, 0) != RESULT_OK) {
                /* Non-fatal - continue without caching */
            }
        }
    } else {
        result = RESULT_ERR;
        RETURN_ERR("Failed to get models list from LLM service");
    }
    
    http_response_cleanup(&response);
    return result;
}

result_t llm_client_set_model(llm_client_t* client, const char* model_name) {
    if (!client || !model_name) {
        RETURN_ERR("Invalid parameters for setting model");
        return RESULT_ERR;
    }
    
    /* Update default model in configuration */
    strncpy(client->config.default_model, model_name, sizeof(client->config.default_model) - 1);
    client->config.default_model[sizeof(client->config.default_model) - 1] = '\0';
    
    /* Update default parameters model */
    strncpy(client->config.default_params.model, model_name, sizeof(client->config.default_params.model) - 1);
    client->config.default_params.model[sizeof(client->config.default_params.model) - 1] = '\0';
    
    return RESULT_OK;
}

result_t llm_client_get_stats(llm_client_t* client, data_t* stats_json) {
    if (!client || !stats_json) {
        RETURN_ERR("Invalid parameters for getting client statistics");
        return RESULT_ERR;
    }
    
    /* Build statistics JSON */
    char stats_buffer[1024];
    snprintf(stats_buffer, sizeof(stats_buffer),
             "{"
             "\"requests_sent\": %lu,"
             "\"requests_succeeded\": %lu,"
             "\"requests_failed\": %lu,"
             "\"success_rate\": %.2f,"
             "\"total_tokens_generated\": %lu,"
             "\"average_tokens_per_request\": %.1f,"
             "\"total_response_time_ms\": %lu,"
             "\"average_response_time_ms\": %.1f,"
             "\"last_request_time\": %lu"
             "}",
             (unsigned long)client->stats.requests_sent,
             (unsigned long)client->stats.requests_succeeded,
             (unsigned long)client->stats.requests_failed,
             client->stats.requests_sent > 0 ? 
                 (double)client->stats.requests_succeeded / client->stats.requests_sent * 100.0 : 0.0,
             (unsigned long)client->stats.total_tokens_generated,
             client->stats.requests_succeeded > 0 ? 
                 (double)client->stats.total_tokens_generated / client->stats.requests_succeeded : 0.0,
             (unsigned long)client->stats.total_response_time,
             client->stats.requests_succeeded > 0 ? 
                 (double)client->stats.total_response_time / client->stats.requests_succeeded : 0.0,
             (unsigned long)client->stats.last_request_time);
    
    return data_set(stats_json, stats_buffer, 0);
}

result_t llm_client_cleanup(llm_client_t* client) {
    if (!client) {
        RETURN_ERR("LLM client pointer is NULL");
        return RESULT_ERR;
    }
    
    /* Clean up HTTP client */
    http_client_cleanup(&client->http_client);
    
    /* Clean up data buffers */
    data_clear(&client->available_models);
    data_clear(&client->model_capabilities);
    data_clear(&client->request_cache);
    
    /* Reset structure */
    memset(client, 0, sizeof(llm_client_t));
    
    return RESULT_OK;
}

result_t llm_response_init(llm_response_t* response) {
    if (!response) {
        RETURN_ERR("LLM response pointer is NULL");
        return RESULT_ERR;
    }
    
    /* Initialize structure */
    memset(response, 0, sizeof(llm_response_t));
    
    /* Initialize content buffer */
    if (data_init(&response->content, 8192) != RESULT_OK) {
        RETURN_ERR("Failed to initialize LLM response content buffer");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t llm_response_cleanup(llm_response_t* response) {
    if (!response) {
        RETURN_ERR("LLM response pointer is NULL");
        return RESULT_ERR;
    }
    
    /* Clean up content buffer */
    data_clear(&response->content);
    
    /* Reset structure */
    memset(response, 0, sizeof(llm_response_t));
    
    return RESULT_OK;
}
