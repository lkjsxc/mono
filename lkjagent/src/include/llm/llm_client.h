/**
 * @file llm_client.h
 * @brief LLM client interface for LMStudio communication
 * 
 * This header provides the high-level LLM client interface for communicating
 * with LMStudio and other compatible LLM services. It handles request construction,
 * response processing, and model management with comprehensive error handling.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#ifndef LKJAGENT_LLM_CLIENT_H
#define LKJAGENT_LLM_CLIENT_H

#include "types.h"
#include "data.h"
#include "http_client.h"

/**
 * @defgroup LLM_Client LLM Client Operations
 * @{
 */

/**
 * @brief LLM request parameters structure
 */
typedef struct {
    /** Model name to use for request */
    char model[128];
    /** Maximum tokens to generate */
    uint32_t max_tokens;
    /** Temperature for sampling (0.0 to 2.0) */
    float temperature;
    /** Top-p sampling parameter (0.0 to 1.0) */
    float top_p;
    /** Top-k sampling parameter */
    uint32_t top_k;
    /** Frequency penalty (-2.0 to 2.0) */
    float frequency_penalty;
    /** Presence penalty (-2.0 to 2.0) */
    float presence_penalty;
    /** Stop sequences (array of strings) */
    char stop_sequences[4][64];
    /** Number of stop sequences */
    size_t stop_count;
    /** Enable streaming response */
    bool stream;
} llm_request_params_t;

/**
 * @brief LLM client configuration structure
 */
typedef struct {
    /** Base URL for LMStudio API */
    char base_url[256];
    /** API key if required */
    char api_key[256];
    /** Default model name */
    char default_model[128];
    /** Default request parameters */
    llm_request_params_t default_params;
    /** Request timeout in seconds */
    uint32_t request_timeout;
    /** Connection timeout in seconds */
    uint32_t connect_timeout;
    /** Maximum retries for failed requests */
    uint32_t max_retries;
    /** Enable request caching */
    bool enable_caching;
    /** Cache TTL in seconds */
    uint32_t cache_ttl;
} llm_client_config_t;

/**
 * @brief LLM response structure
 */
typedef struct {
    /** Generated text content */
    data_t content;
    /** Model used for generation */
    char model[128];
    /** Number of tokens generated */
    uint32_t tokens_generated;
    /** Number of tokens in prompt */
    uint32_t tokens_prompt;
    /** Total tokens used */
    uint32_t tokens_total;
    /** Finish reason (completed, length, stop) */
    char finish_reason[32];
    /** Response time in milliseconds */
    uint64_t response_time;
    /** Request ID for tracking */
    char request_id[64];
} llm_response_t;

/**
 * @brief LLM client instance structure
 */
typedef struct {
    /** Client configuration */
    llm_client_config_t config;
    /** HTTP client for communication */
    http_client_t http_client;
    /** Available models list */
    data_t available_models;
    /** Current model capabilities */
    data_t model_capabilities;
    /** Request cache */
    data_t request_cache;
    /** Client statistics */
    struct {
        uint64_t requests_sent;
        uint64_t requests_succeeded;
        uint64_t requests_failed;
        uint64_t total_tokens_generated;
        uint64_t total_response_time;
        time_t last_request_time;
    } stats;
} llm_client_t;

/**
 * @brief Initialize LLM client with configuration
 * 
 * Creates and initializes an LLM client instance with the specified
 * configuration. Sets up HTTP client, validates connection, and prepares
 * for LLM interactions.
 * 
 * @param client Pointer to LLM client structure to initialize
 * @param config Configuration parameters for the client
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note The client structure is fully initialized and ready for use
 * @note Default configuration values are applied if config is NULL
 * @note Initial connectivity test is performed during initialization
 * @note Model list is fetched and cached during initialization
 * 
 * @warning client parameter must not be NULL
 * 
 * Example usage:
 * @code
 * llm_client_t client;
 * llm_client_config_t config = {0};
 * strcpy(config.base_url, "http://localhost:1234");
 * strcpy(config.default_model, "gpt-3.5-turbo");
 * config.default_params.max_tokens = 1000;
 * config.default_params.temperature = 0.7f;
 * 
 * if (llm_client_init(&client, &config) == RESULT_OK) {
 *     // Client ready for LLM requests
 *     llm_client_cleanup(&client);
 * }
 * @endcode
 */
result_t llm_client_init(llm_client_t* client, const llm_client_config_t* config) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Send request to LLM and receive response
 * 
 * Sends a complete prompt request to the LLM service and handles the
 * response with comprehensive error handling and retry logic.
 * 
 * @param client LLM client instance
 * @param prompt Complete prompt text to send to LLM
 * @param params Request parameters (or NULL for defaults)
 * @param response Response structure to store the result
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Automatically constructs JSON request payload
 * @note Handles HTTP communication and response parsing
 * @note Implements retry logic for transient failures
 * @note Updates client statistics and performance metrics
 * @note Response content is stored in response->content data buffer
 * 
 * @warning client parameter must not be NULL and must be initialized
 * @warning prompt parameter must not be NULL
 * @warning response parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * llm_response_t response;
 * llm_response_init(&response);
 * 
 * llm_request_params_t params = {0};
 * strcpy(params.model, "gpt-3.5-turbo");
 * params.max_tokens = 500;
 * params.temperature = 0.8f;
 * 
 * const char* prompt = "Analyze the following data and provide insights: ...";
 * if (llm_send_request(&client, prompt, &params, &response) == RESULT_OK) {
 *     printf("LLM Response: %s\n", response.content.data);
 *     printf("Tokens generated: %u\n", response.tokens_generated);
 * }
 * 
 * llm_response_cleanup(&response);
 * @endcode
 */
result_t llm_send_request(llm_client_t* client, const char* prompt, const llm_request_params_t* params, llm_response_t* response) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 4)));

/**
 * @brief Receive and validate LLM response
 * 
 * Processes the raw HTTP response from the LLM service, validates the format,
 * extracts the generated content, and populates the response structure.
 * 
 * @param client LLM client instance
 * @param http_response Raw HTTP response from LLM service
 * @param llm_response LLM response structure to populate
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Validates JSON response format and structure
 * @note Extracts content, token counts, and metadata
 * @note Handles various LLM response formats and edge cases
 * @note Updates client statistics based on response
 * 
 * @warning client parameter must not be NULL and must be initialized
 * @warning http_response parameter must not be NULL
 * @warning llm_response parameter must not be NULL and must be initialized
 */
result_t llm_receive_response(llm_client_t* client, const http_response_t* http_response, llm_response_t* llm_response) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Configure LLM client at runtime
 * 
 * Updates LLM client configuration parameters during runtime without
 * requiring reinitialization. Changes take effect for subsequent requests.
 * 
 * @param client LLM client instance
 * @param config New configuration parameters
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Configuration changes are validated before application
 * @note HTTP client configuration is updated if needed
 * @note Model list is refreshed if base URL changes
 * 
 * @warning client parameter must not be NULL and must be initialized
 * @warning config parameter must not be NULL
 */
result_t llm_client_configure(llm_client_t* client, const llm_client_config_t* config) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Test connection to LLM service
 * 
 * Performs a connectivity test to verify that the LLM service is reachable
 * and responsive. Used for health checks and configuration validation.
 * 
 * @param client LLM client instance
 * @param response_time Pointer to store response time in milliseconds
 * @return RESULT_OK if service is available, RESULT_ERR if not
 * 
 * @note Performs actual request to LLM service endpoint
 * @note Measures response time for performance monitoring
 * @note Does not consume significant resources or tokens
 * 
 * @warning client parameter must not be NULL and must be initialized
 * @warning response_time parameter must not be NULL
 */
result_t llm_client_test_connection(llm_client_t* client, uint64_t* response_time) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Get available models from LLM service
 * 
 * Retrieves the list of available models from the LLM service and updates
 * the client's model cache. Used for model discovery and validation.
 * 
 * @param client LLM client instance
 * @param models_list Data buffer to store models list (JSON format)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Models list is cached in the client instance
 * @note Response format depends on LLM service API
 * @note Model capabilities may be included in response
 * 
 * @warning client parameter must not be NULL and must be initialized
 * @warning models_list parameter must not be NULL and must be initialized
 */
result_t llm_client_get_models(llm_client_t* client, data_t* models_list) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Set active model for subsequent requests
 * 
 * Configures the LLM client to use the specified model for all subsequent
 * requests. Validates model availability and updates client configuration.
 * 
 * @param client LLM client instance
 * @param model_name Name of the model to use
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Model name is validated against available models
 * @note Model capabilities are fetched and cached if available
 * @note Default request parameters may be adjusted for model
 * 
 * @warning client parameter must not be NULL and must be initialized
 * @warning model_name parameter must not be NULL
 */
result_t llm_client_set_model(llm_client_t* client, const char* model_name) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Get client statistics and performance metrics
 * 
 * Retrieves comprehensive statistics about LLM client usage, performance,
 * and resource consumption for monitoring and optimization.
 * 
 * @param client LLM client instance
 * @param stats_json Data buffer to store statistics in JSON format
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Statistics include request counts, token usage, and timing
 * @note Performance metrics help optimize client configuration
 * @note Statistics are updated in real-time during operation
 * 
 * @warning client parameter must not be NULL and must be initialized
 * @warning stats_json parameter must not be NULL and must be initialized
 */
result_t llm_client_get_stats(llm_client_t* client, data_t* stats_json) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Clean up LLM client resources
 * 
 * Closes connections, frees all resources, and resets the LLM client
 * instance. Must be called before program termination.
 * 
 * @param client LLM client instance to clean up
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Closes HTTP client connections gracefully
 * @note Frees all cached data and internal buffers
 * @note Client instance becomes invalid after cleanup
 * 
 * @warning client parameter must not be NULL
 */
result_t llm_client_cleanup(llm_client_t* client) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Initialize LLM response structure
 * 
 * Initializes all data buffers in an LLM response structure with
 * appropriate initial capacities for content and metadata.
 * 
 * @param response LLM response structure to initialize
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Initializes content data buffer with reasonable capacity
 * @note Sets all counters and timestamps to default values
 * @note Response is ready for use after successful initialization
 * 
 * @warning response parameter must not be NULL
 */
result_t llm_response_init(llm_response_t* response) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Clean up LLM response structure
 * 
 * Frees all data buffers associated with an LLM response structure.
 * Must be called to prevent memory leaks.
 * 
 * @param response LLM response structure to clean up
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Frees content data buffer and resets all fields
 * @note Response structure becomes invalid after cleanup
 * 
 * @warning response parameter must not be NULL
 */
result_t llm_response_cleanup(llm_response_t* response) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/** @} */

#endif /* LKJAGENT_LLM_CLIENT_H */
