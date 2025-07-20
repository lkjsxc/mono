#ifndef LKJAGENT_H
#define LKJAGENT_H

#include "const.h"
#include "types.h"
#include "macro.h"
#include "lkjstring.h"

__attribute__((warn_unused_result)) result_t lkjagent_init(lkjagent_t* lkjagent);
__attribute__((warn_unused_result)) result_t lkjagent_loadconfig(lkjagent_t* lkjagent);
__attribute__((warn_unused_result)) result_t lkjagent_run(lkjagent_t* lkjagent);

#endif
