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
__attribute__((warn_unused_result)) result_t agent_http_create_resources(pool_t* pool, string_t** send_string, string_t** content_type, string_t** recv_http_string);

__attribute__((warn_unused_result)) result_t agent_http_extract_response_content(pool_t* pool, object_t* recv_http_object, object_t** recv_content_object);

__attribute__((warn_unused_result)) result_t agent_http_cleanup_resources(pool_t* pool, string_t* send_string, string_t* content_type, string_t* recv_http_string, object_t* recv_http_object);

__attribute__((warn_unused_result)) result_t agent_http_send_receive(pool_t* pool, config_t* config, string_t* prompt, string_t** response_content);

#endif
