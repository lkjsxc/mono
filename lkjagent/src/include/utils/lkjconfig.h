#ifndef LKJAGENT_LKJCONFIG_H
#define LKJAGENT_LKJCONFIG_H

#include "macro.h"
#include "std.h"
#include "types.h"
#include "utils/lkjfileio.h"
#include "utils/lkjjson.h"
#include "utils/lkjstring.h"
#include "utils/lkjpool.h"

/**
 * Initialize a configuration structure
 * @param pool Memory pool for allocations
 * @param config_path Path to the configuration file
 * @param config Configuration structure to initialize
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t config_init(pool_t* pool, config_t* config);

/**
 * Load configuration from a file
 * @param pool Memory pool for allocations
 * @param config Configuration structure to populate
 * @param buf Buffer to use for file reading
 * @param config_path Path to the configuration file
 * @return RESULT_OK on success, RESULT_ERR on error
 */
__attribute__((warn_unused_result)) result_t config_load(pool_t* pool, config_t* config, const char* config_path);

#endif
