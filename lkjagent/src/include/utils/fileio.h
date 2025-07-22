#ifndef LKJAGENT_FILEIO_H
#define LKJAGENT_FILEIO_H

#include "macro.h"
#include "std.h"
#include "types.h"
#include "utils/lkjstring.h"

__attribute__((warn_unused_result)) result_t file_read(const char* path, string_t* string);
__attribute__((warn_unused_result)) result_t file_write(const char* path, const string_t* string);

#endif
