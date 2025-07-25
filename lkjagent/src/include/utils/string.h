#ifndef LKJAGENT_UTILS_STRING_H
#define LKJAGENT_UTILS_STRING_H

#include "global/const.h"
#include "global/macro.h"
#include "global/std.h"
#include "global/types.h"
#include "utils/pool.h"

__attribute__((warn_unused_result)) result_t string_create(pool_t* pool, string_t** string);

__attribute__((warn_unused_result)) result_t string_copy_str(pool_t* pool, string_t** string, const char* str);

#endif
