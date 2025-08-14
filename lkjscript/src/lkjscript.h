#ifndef LKJSCRIPT_H
#define LKJSCRIPT_H

#include "lkjlib.h"

#define SOURCECODE_PATH "/data/index.txt"

__attribute__((warn_unused_result)) result_t lkjscript_run(pool_t* pool, const char* sourcecode);

#endif