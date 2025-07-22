#ifndef LKJAGENT_H
#define LKJAGENT_H

#include "const.h"
#include "macro.h"
#include "types.h"
#include "utils/config.h"
#include "utils/fileio.h"
#include "utils/http.h"
#include "utils/json.h"
#include "utils/lkjstring.h"
#include "utils/pool.h"

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
