#ifndef LKJAGENT_H
#define LKJAGENT_H

#include "types.h"

__attribute__((warn_unused_result)) result_t lkjagent_init(lkjagent_t* lkjagent);
__attribute__((warn_unused_result)) result_t lkjagent_loadconfig(lkjagent_t* lkjagent);
__attribute__((warn_unused_result)) result_t lkjagent_run(lkjagent_t* lkjagent);

#endif
