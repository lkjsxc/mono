#ifndef LKJAGENT_UTILS_HTTP_H
#define LKJAGENT_UTILS_HTTP_H

#include "global/const.h"
#include "global/macro.h"
#include "global/std.h"
#include "global/types.h"
#include "utils/pool.h"
#include "utils/string.h"
#include "utils/object.h"

// HTTP client functions
__attribute__((warn_unused_result)) result_t http_get(pool_t* pool, const string_t* url, string_t** response);
__attribute__((warn_unused_result)) result_t http_post(pool_t* pool, const string_t* url, const string_t* content_type, const string_t* body, string_t** response);

#endif
