#ifndef LKJAGENT_H
#define LKJAGENT_H

#include "config.h"
#include "const.h"
#include "fileio.h"
#include "json.h"
#include "lkjstring.h"
#include "macro.h"
#include "pool.h"
#include "types.h"

/**
 * Initialize the lkjagent structure
 * @param lkjagent The lkjagent structure to initialize
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t lkjagent_init(lkjagent_t* lkjagent);

/**
 * Run the lkjagent
 * @param lkjagent The lkjagent structure to run
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t lkjagent_run(lkjagent_t* lkjagent);

#endif
