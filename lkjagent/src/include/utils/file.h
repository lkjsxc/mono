#ifndef LKJAGENT_UTILS_FILE_H
#define LKJAGENT_UTILS_FILE_H

#include "global/const.h"
#include "global/macro.h"
#include "global/std.h"
#include "global/types.h"
#include "utils/string.h"
#include "utils/pool.h"

__attribute__((warn_unused_result)) result_t file_read(pool_t* pool, const char* path, string_t** string);
__attribute__((warn_unused_result)) result_t file_write(const char* path, const string_t* string);

#endif
