/**
 * @file llm_context.h
 * @brief LLM context preparation and management interface
 * 
 * This header provides context preparation capabilities for LLM interactions,
 * including memory integration, context window management, and intelligent
 * context sizing for optimal LLM performance.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#ifndef LKJAGENT_LLM_CONTEXT_H
#define LKJAGENT_LLM_CONTEXT_H

#include "types.h"
#include "data.h"

/**
 * @defgroup LLM_Context LLM Context Management Operations
 * @{
 */

/**
 * @brief Context preparation configuration
 */
typedef struct {
    /** Maximum context window size in tokens */
    size_t max_context_tokens;
    /** Maximum context size in characters (fallback) */
    size_t max_context_chars;
    /** Reserve tokens for response generation */
    size_t response_token_reserve;
    /** Context priority weights */
    struct {
        float system_prompt_weight;
        float recent_memory_weight;
        float important_memory_weight;
        float current_state_weight;
    } priority_weights;
    /** Enable context compression */
    bool enable_compression;
    /** Enable context summarization */
    bool enable_summarization;
} llm_context_config_t;

/**
 * @brief Context component structure
 */
typedef struct {
    /** Component content */
    data_t content;
    /** Component type */
    char component_type[32];
    /** Priority score (0-100) */
    size_t priority;
    /** Token count estimate */
    size_t token_count;
    /** Context key if applicable */
    char context_key[64];
    /** Timestamp */
    time_t timestamp;
} llm_context_component_t;

/**
 * @brief Complete context structure
 */
typedef struct {
    /** System prompt component */
    llm_context_component_t system_prompt;
    /** Memory components */
    llm_context_component_t memory_components[64];
    /** Number of memory components */
    size_t memory_count;
    /** Current state component */
    llm_context_component_t current_state;
    /** Total estimated token count */
    size_t total_tokens;
    /** Context is within limits */
    bool within_limits;
    /** Context preparation timestamp */
    time_t preparation_time;
} llm_context_t;

/**
 * @brief Prepare complete context for LLM request
 * 
 * Assembles a complete context from memory, system prompts, and current
 * state information, optimized for the specified context window limits.
 * 
 * @param memory Memory system instance
 * @param current_state Current agent state
 * @param config Context preparation configuration
 * @param context Context structure to populate
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Automatically selects and prioritizes memory components
 * @note Applies context window management to fit within limits
 * @note Includes state-appropriate system prompt
 * @note Estimates token counts for accurate sizing
 * 
 * @warning memory parameter must not be NULL and must be initialized
 * @warning config parameter must not be NULL
 * @warning context parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * llm_context_t context;
 * llm_context_init(&context);
 * 
 * llm_context_config_t config = {
 *     .max_context_tokens = 4000,
 *     .max_context_chars = 16000,
 *     .response_token_reserve = 1000
 * };
 * 
 * if (llm_context_prepare(memory, STATE_THINKING, &config, &context) == RESULT_OK) {
 *     // Context ready for LLM request
 *     printf("Total tokens: %zu\n", context.total_tokens);
 * }
 * 
 * llm_context_cleanup(&context);
 * @endcode
 */
result_t llm_context_prepare(void* memory, agent_state_t current_state, const llm_context_config_t* config, llm_context_t* context) __attribute__((warn_unused_result)) __attribute__((nonnull(3, 4)));

/**
 * @brief Build complete prompt from context
 * 
 * Constructs the final prompt string by combining all context components
 * in the optimal order for LLM processing.
 * 
 * @param context Prepared context structure
 * @param prompt_buffer Data buffer to store the complete prompt
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Components are ordered for optimal LLM understanding
 * @note System prompt is positioned appropriately for the LLM
 * @note Memory components are integrated seamlessly
 * @note Current state information is included contextually
 * 
 * @warning context parameter must not be NULL and must be prepared
 * @warning prompt_buffer parameter must not be NULL and must be initialized
 */
result_t llm_context_build_prompt(const llm_context_t* context, data_t* prompt_buffer) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Add system prompt to context
 * 
 * Integrates the appropriate system prompt for the current agent state
 * into the context structure.
 * 
 * @param context Context structure to modify
 * @param current_state Current agent state
 * @param system_prompt System prompt text
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note System prompt is formatted for the specific agent state
 * @note Priority is set based on system prompt importance
 * @note Token count is estimated for context sizing
 * 
 * @warning context parameter must not be NULL
 * @warning system_prompt parameter must not be NULL
 */
result_t llm_context_add_system(llm_context_t* context, agent_state_t current_state, const char* system_prompt) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 3)));

/**
 * @brief Add memory context to context structure
 * 
 * Integrates relevant memory content from the memory system into the
 * context structure with appropriate prioritization.
 * 
 * @param context Context structure to modify
 * @param memory Memory system instance
 * @param max_memory_components Maximum number of memory components to add
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Selects most relevant memory components based on current state
 * @note Applies priority weighting for memory component selection
 * @note Includes both working and disk memory as appropriate
 * @note Memory components are validated and filtered
 * 
 * @warning context parameter must not be NULL
 * @warning memory parameter must not be NULL
 */
result_t llm_context_add_memory(llm_context_t* context, void* memory, size_t max_memory_components) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Add current state information to context
 * 
 * Includes current agent state information and relevant operational
 * context in the context structure.
 * 
 * @param context Context structure to modify
 * @param current_state Current agent state
 * @param state_info Additional state information
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note State information is formatted for LLM understanding
 * @note Includes operational context and recent actions
 * @note Priority is set based on current state relevance
 * 
 * @warning context parameter must not be NULL
 * @warning state_info parameter can be NULL for default state info
 */
result_t llm_context_add_state(llm_context_t* context, agent_state_t current_state, const char* state_info) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Trim context to fit within size limits
 * 
 * Reduces context size by removing or summarizing less important components
 * to fit within the specified context window limits.
 * 
 * @param context Context structure to trim
 * @param config Context configuration with size limits
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Removes lowest priority components first
 * @note Summarizes long memory components if possible
 * @note Preserves critical system prompt and state information
 * @note Updates token counts after trimming
 * 
 * @warning context parameter must not be NULL
 * @warning config parameter must not be NULL
 */
result_t llm_context_trim_size(llm_context_t* context, const llm_context_config_t* config) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Prioritize context components for optimal LLM performance
 * 
 * Reorders and prioritizes context components based on relevance to
 * the current task and agent state.
 * 
 * @param context Context structure to prioritize
 * @param current_state Current agent state
 * @param config Context configuration with priority weights
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Applies state-specific prioritization strategies
 * @note Uses configuration weights for component importance
 * @note Optimizes component order for LLM processing
 * @note Updates priority scores for all components
 * 
 * @warning context parameter must not be NULL
 * @warning config parameter must not be NULL
 */
result_t llm_context_prioritize(llm_context_t* context, agent_state_t current_state, const llm_context_config_t* config) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 3)));

/**
 * @brief Calculate context size in tokens
 * 
 * Estimates the total token count for the context structure using
 * heuristics and character-to-token ratios.
 * 
 * @param context Context structure to analyze
 * @param token_count Pointer to store estimated token count
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Uses character-based estimation with model-specific ratios
 * @note Includes overhead for formatting and structure
 * @note Updates individual component token counts
 * @note Provides conservative estimates for safety
 * 
 * @warning context parameter must not be NULL
 * @warning token_count parameter must not be NULL
 */
result_t llm_context_calculate_size(const llm_context_t* context, size_t* token_count) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Fit context within available window
 * 
 * Adjusts context size and composition to fit within the specified
 * context window while maximizing information content.
 * 
 * @param context Context structure to adjust
 * @param available_tokens Available token budget
 * @param config Context configuration
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Optimizes content selection for available space
 * @note Maintains critical information while removing less important content
 * @note Uses intelligent summarization when appropriate
 * @note Ensures context remains coherent after fitting
 * 
 * @warning context parameter must not be NULL
 * @warning config parameter must not be NULL
 */
result_t llm_context_fit_window(llm_context_t* context, size_t available_tokens, const llm_context_config_t* config) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 3)));

/**
 * @brief Summarize older context for compression
 * 
 * Creates summaries of older or less important context components
 * to reduce memory usage while preserving key information.
 * 
 * @param context Context structure with components to summarize
 * @param component_index Index of component to summarize
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Preserves key information while reducing size
 * @note Maintains context coherence and relationships
 * @note Updates component metadata after summarization
 * @note Uses intelligent summarization strategies
 * 
 * @warning context parameter must not be NULL
 */
result_t llm_context_summarize_old(llm_context_t* context, size_t component_index) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Preserve important context during trimming
 * 
 * Ensures that critical context components are preserved during
 * context trimming and size reduction operations.
 * 
 * @param context Context structure to process
 * @param importance_threshold Minimum importance score to preserve
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Protects high-importance components from removal
 * @note Maintains context integrity and coherence
 * @note Uses importance scores for preservation decisions
 * @note Updates preservation flags for components
 * 
 * @warning context parameter must not be NULL
 */
result_t llm_context_preserve_important(llm_context_t* context, size_t importance_threshold) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Manage context overflow situations
 * 
 * Handles situations where context exceeds available window size
 * by implementing intelligent overflow management strategies.
 * 
 * @param context Context structure with overflow
 * @param config Context configuration
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Implements multiple overflow handling strategies
 * @note Prioritizes critical information preservation
 * @note Uses summarization and compression techniques
 * @note Maintains context usability after overflow handling
 * 
 * @warning context parameter must not be NULL
 * @warning config parameter must not be NULL
 */
result_t llm_context_manage_overflow(llm_context_t* context, const llm_context_config_t* config) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Initialize context structure
 * 
 * Initializes all data buffers and components in a context structure
 * with appropriate initial capacities.
 * 
 * @param context Context structure to initialize
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Initializes all component data buffers
 * @note Sets default values for all fields
 * @note Prepares structure for context preparation
 * 
 * @warning context parameter must not be NULL
 */
result_t llm_context_init(llm_context_t* context) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Clean up context structure
 * 
 * Frees all data buffers and resources associated with a context structure.
 * Must be called to prevent memory leaks.
 * 
 * @param context Context structure to clean up
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Frees all component data buffers
 * @note Resets all fields to default values
 * @note Structure becomes invalid after cleanup
 * 
 * @warning context parameter must not be NULL
 */
result_t llm_context_cleanup(llm_context_t* context) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/** @} */

#endif /* LKJAGENT_LLM_CONTEXT_H */
