/**
 * @file config_loader.h
 * @brief Configuration loading and management interface for LKJAgent
 * 
 * This header provides configuration management capabilities including
 * loading from files, validation, defaults, and state-specific prompts.
 * All operations include comprehensive error handling and validation.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#ifndef LKJAGENT_CONFIG_LOADER_H
#define LKJAGENT_CONFIG_LOADER_H

#include "types.h"
#include "data.h"

/**
 * @defgroup Config_Management Configuration Management
 * @{
 */

/**
 * @brief Load configuration from file
 * 
 * Loads configuration from a JSON file and populates the config structure.
 * Validates all parameters and applies defaults for missing values.
 * 
 * @param filename Path to configuration file
 * @param config Configuration structure to populate
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note If file doesn't exist, loads defaults and marks as invalid
 * @note All configuration values are validated for sanity and security
 * @note Missing optional fields are filled with appropriate defaults
 * @note The config structure is always left in a consistent state
 * 
 * @warning filename parameter must not be NULL
 * @warning config parameter must not be NULL
 * 
 * Example usage:
 * @code
 * config_t config;
 * if (config_load("/path/to/config.json", &config) == RESULT_OK) {
 *     printf("Configuration loaded successfully\n");
 *     if (config.is_valid) {
 *         printf("Configuration is valid and ready for use\n");
 *     }
 * }
 * @endcode
 */
result_t config_load(const char* filename, config_t* config) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Load default configuration values
 * 
 * Populates the config structure with safe default values for all fields.
 * This ensures the system can operate even without a configuration file.
 * 
 * @param config Configuration structure to populate with defaults
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note All defaults are safe values that allow the system to start
 * @note Default prompts are included for all agent states
 * @note LLM settings use conservative defaults suitable for most setups
 * @note Memory settings are optimized for typical usage patterns
 * 
 * @warning config parameter must not be NULL
 * 
 * Example usage:
 * @code
 * config_t config;
 * if (config_load_defaults(&config) == RESULT_OK) {
 *     printf("Default configuration loaded\n");
 *     // System can now operate with safe defaults
 * }
 * @endcode
 */
result_t config_load_defaults(config_t* config) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Validate configuration parameters
 * 
 * Performs comprehensive validation of all configuration parameters,
 * checking for valid values, security constraints, and consistency.
 * 
 * @param config Configuration structure to validate
 * @return RESULT_OK if valid, RESULT_ERR if invalid
 * 
 * @note Checks all string fields for appropriate length and content
 * @note Validates numeric ranges for memory and timeout settings
 * @note Ensures LLM endpoint URLs are properly formatted
 * @note Verifies state prompts are present and reasonable
 * @note Updates the is_valid flag in the config structure
 * 
 * @warning config parameter must not be NULL
 * 
 * Example usage:
 * @code
 * config_t config;
 * config_load("/path/to/config.json", &config);
 * 
 * if (config_validate(&config) == RESULT_OK) {
 *     printf("Configuration is valid\n");
 * } else {
 *     printf("Configuration validation failed\n");
 *     // Check specific fields or load defaults
 * }
 * @endcode
 */
result_t config_validate(config_t* config) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Get state-specific system prompt
 * 
 * Retrieves the system prompt for the specified agent state from the
 * configuration. Handles fallbacks for missing prompts.
 * 
 * @param config Configuration structure containing prompts
 * @param state Agent state for which to get the prompt
 * @param prompt Data buffer to store the prompt content
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Each agent state has a specific system prompt optimized for that phase
 * @note If the specific prompt is missing, a generic fallback is provided
 * @note Prompts are loaded from the configuration and validated
 * @note The output buffer is cleared before storing the prompt
 * 
 * @warning config parameter must not be NULL
 * @warning prompt parameter must not be NULL and must be initialized
 * @warning state must be a valid agent_state_t value
 * 
 * Example usage:
 * @code
 * config_t config;
 * config_load("/path/to/config.json", &config);
 * 
 * data_t thinking_prompt;
 * data_init(&thinking_prompt, 1024);
 * 
 * if (config_get_state_prompt(&config, STATE_THINKING, &thinking_prompt) == RESULT_OK) {
 *     printf("Thinking prompt: %s\n", thinking_prompt.data);
 * }
 * data_destroy(&thinking_prompt);
 * @endcode
 */
result_t config_get_state_prompt(const config_t* config, agent_state_t state, data_t* prompt) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 3)));

/**
 * @brief Get LLM communication settings
 * 
 * Extracts LLM-related configuration parameters in a convenient structure
 * for use by the LLM integration components.
 * 
 * @param config Configuration structure containing LLM settings
 * @param endpoint Data buffer to store LLM endpoint URL
 * @param model Data buffer to store LLM model name
 * @param api_key Data buffer to store API key (if any)
 * @param max_context Pointer to store maximum context size
 * @param timeout Pointer to store request timeout
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note All string parameters are validated for format and security
 * @note Numeric parameters are checked for reasonable ranges
 * @note API keys are handled securely (not logged or exposed)
 * @note Missing or invalid values are replaced with safe defaults
 * 
 * @warning config parameter must not be NULL
 * @warning All data buffer parameters must not be NULL and must be initialized
 * @warning All numeric pointer parameters must not be NULL
 * 
 * Example usage:
 * @code
 * config_t config;
 * config_load("/path/to/config.json", &config);
 * 
 * data_t endpoint, model, api_key;
 * data_init(&endpoint, 256);
 * data_init(&model, 128);
 * data_init(&api_key, 256);
 * 
 * size_t max_context;
 * int timeout;
 * 
 * if (config_get_llm_settings(&config, &endpoint, &model, &api_key, &max_context, &timeout) == RESULT_OK) {
 *     printf("LLM endpoint: %s\n", endpoint.data);
 *     printf("Model: %s\n", model.data);
 *     printf("Max context: %zu\n", max_context);
 * }
 * 
 * data_destroy(&endpoint);
 * data_destroy(&model);
 * data_destroy(&api_key);
 * @endcode
 */
result_t config_get_llm_settings(const config_t* config, data_t* endpoint, data_t* model, data_t* api_key, size_t* max_context, int* timeout) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3, 4, 5, 6)));

/**
 * @brief Get memory management settings
 * 
 * Extracts memory-related configuration parameters for use by the
 * memory management system.
 * 
 * @param config Configuration structure containing memory settings
 * @param max_working_size Pointer to store maximum working memory size
 * @param max_disk_size Pointer to store maximum disk memory size
 * @param cleanup_threshold Pointer to store cleanup threshold
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note All values are validated for reasonable ranges
 * @note Memory sizes are in bytes and checked against system limits
 * @note Cleanup threshold is a percentage value (0-100)
 * @note Invalid values are replaced with safe defaults
 * 
 * @warning config parameter must not be NULL
 * @warning All pointer parameters must not be NULL
 * 
 * Example usage:
 * @code
 * config_t config;
 * config_load("/path/to/config.json", &config);
 * 
 * size_t max_working, max_disk, cleanup_threshold;
 * 
 * if (config_get_memory_settings(&config, &max_working, &max_disk, &cleanup_threshold) == RESULT_OK) {
 *     printf("Working memory limit: %zu bytes\n", max_working);
 *     printf("Disk memory limit: %zu bytes\n", max_disk);
 *     printf("Cleanup threshold: %zu%%\n", cleanup_threshold);
 * }
 * @endcode
 */
result_t config_get_memory_settings(const config_t* config, size_t* max_working_size, size_t* max_disk_size, size_t* cleanup_threshold) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3, 4)));

/**
 * @brief Save configuration to file
 * 
 * Serializes the configuration structure to a JSON file with proper
 * formatting and validation.
 * 
 * @param config Configuration structure to save
 * @param filename Path to configuration file
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note The file is written atomically to prevent corruption
 * @note Sensitive information (API keys) may be excluded or masked
 * @note The file is formatted for human readability
 * @note Existing files are backed up before overwriting
 * 
 * @warning config parameter must not be NULL
 * @warning filename parameter must not be NULL
 * 
 * Example usage:
 * @code
 * config_t config;
 * config_load_defaults(&config);
 * // ... modify configuration ...
 * 
 * if (config_save(&config, "/path/to/config.json") == RESULT_OK) {
 *     printf("Configuration saved successfully\n");
 * }
 * @endcode
 */
result_t config_save(const config_t* config, const char* filename) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Check if configuration file has been modified
 * 
 * Checks whether the configuration file has been modified since it was
 * last loaded. Useful for detecting external configuration changes.
 * 
 * @param config Configuration structure with stored modification time
 * @param filename Path to configuration file
 * @param has_changed Pointer to store result (true if file changed)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Compares file modification time with stored timestamp in config
 * @note Returns false if file doesn't exist or cannot be accessed
 * @note Useful for implementing configuration reloading
 * 
 * @warning config parameter must not be NULL
 * @warning filename parameter must not be NULL
 * @warning has_changed parameter must not be NULL
 * 
 * Example usage:
 * @code
 * config_t config;
 * config_load("/path/to/config.json", &config);
 * 
 * // ... later in the program ...
 * bool changed;
 * if (config_has_changed(&config, "/path/to/config.json", &changed) == RESULT_OK) {
 *     if (changed) {
 *         printf("Configuration file has been modified\n");
 *         config_load("/path/to/config.json", &config); // Reload
 *     }
 * }
 * @endcode
 */
result_t config_has_changed(const config_t* config, const char* filename, bool* has_changed) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Free all memory associated with configuration
 * 
 * Releases all allocated memory within the configuration structure
 * and resets all fields to safe values.
 * 
 * @param config Configuration structure to destroy
 * 
 * @note All data_t fields are properly destroyed
 * @note String fields are cleared and set to safe values
 * @note Numeric fields are reset to zero
 * @note The structure can be safely reused after this call
 * 
 * @warning config parameter must not be NULL
 * 
 * Example usage:
 * @code
 * config_t config;
 * config_load("/path/to/config.json", &config);
 * 
 * // ... use configuration ...
 * 
 * config_destroy(&config); // Clean up all memory
 * // config is now ready for reuse or disposal
 * @endcode
 */
void config_destroy(config_t* config) __attribute__((nonnull(1)));

/** @} */

#endif /* LKJAGENT_CONFIG_LOADER_H */
