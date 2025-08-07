#ifndef LKJAGENT_AGENT_HTTP_H
#define LKJAGENT_AGENT_HTTP_H

#include "global/const.h"
#include "global/macro.h"
#include "global/std.h"
#include "global/types.h"
#include "utils/string.h"
#include "utils/pool.h"
#include "utils/object.h"
#include "utils/http.h"

// HTTP communication functions for LLM interaction

// High-level HTTP communication wrapper
__attribute__((warn_unused_result)) result_t agent_http_send_receive(pool_t* pool, config_t* config, const string_t* prompt, string_t** response_content);

// Resource management functions

// Create and initialize HTTP request/response resources
__attribute__((warn_unused_result)) result_t agent_http_create_resources(pool_t* pool, string_t** request_string, string_t** response_string, string_t** content_type);

// Clean up HTTP resources
__attribute__((warn_unused_result)) result_t agent_http_cleanup_resources(pool_t* pool, string_t* request_string, string_t* response_string, string_t* content_type);

// LLM response processing functions

// Extract content from LLM API response JSON
__attribute__((warn_unused_result)) result_t agent_http_extract_response_content(pool_t* pool, const string_t* response_json, string_t** content);

// Build endpoint URL from configuration
__attribute__((warn_unused_result)) result_t agent_http_build_endpoint_url(pool_t* pool, config_t* config, string_t** endpoint_url);

// Validate and extract LLM configuration parameters
__attribute__((warn_unused_result)) result_t agent_http_extract_llm_config(pool_t* pool, config_t* config, object_t** llm_config, object_t** endpoint_obj, object_t** model_obj, object_t** temperature_obj);

#endif
