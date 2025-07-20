/**
 * @file llm_prompt.h
 * @brief LLM prompt construction and management interface
 * 
 * This header provides state-specific prompt construction capabilities
 * for optimal LLM interaction in each agent state. It includes prompt
 * templates, dynamic prompt generation, and prompt optimization.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#ifndef LKJAGENT_LLM_PROMPT_H
#define LKJAGENT_LLM_PROMPT_H

#include "types.h"
#include "data.h"
#include "llm/llm_context.h"

/**
 * @defgroup LLM_Prompt LLM Prompt Construction Operations
 * @{
 */

/**
 * @brief Prompt template structure
 */
typedef struct {
    /** Template name */
    char name[64];
    /** Template content with placeholders */
    data_t template_content;
    /** Template variables */
    char variables[16][32];
    /** Number of variables */
    size_t variable_count;
    /** Template priority */
    size_t priority;
    /** Template effectiveness score */
    float effectiveness_score;
} llm_prompt_template_t;

/**
 * @brief Prompt variable substitution
 */
typedef struct {
    /** Variable name */
    char name[32];
    /** Variable value */
    data_t value;
} llm_prompt_variable_t;

/**
 * @brief Complete prompt structure
 */
typedef struct {
    /** System prompt section */
    data_t system_prompt;
    /** Context section */
    data_t context_section;
    /** Instructions section */
    data_t instructions;
    /** Memory section */
    data_t memory_section;
    /** State-specific section */
    data_t state_section;
    /** Final assembled prompt */
    data_t final_prompt;
    /** Prompt metadata */
    struct {
        agent_state_t target_state;
        size_t estimated_tokens;
        time_t creation_time;
        char template_used[64];
    } metadata;
} llm_prompt_t;

/**
 * @brief Build thinking state prompt
 * 
 * Constructs an optimized prompt for the thinking state that encourages
 * deep analysis, planning, and structured reasoning.
 * 
 * @param context Prepared context for the prompt
 * @param instructions Specific thinking instructions
 * @param prompt Prompt structure to populate
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Includes thinking-specific system prompt
 * @note Encourages structured analysis and planning
 * @note Promotes use of thinking tags for internal reasoning
 * @note Optimizes for deep reasoning and insight generation
 * 
 * @warning context parameter must not be NULL and must be prepared
 * @warning prompt parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * llm_prompt_t prompt;
 * llm_prompt_init(&prompt);
 * 
 * const char* instructions = "Analyze the current situation and plan next steps";
 * if (llm_prompt_build_thinking(&context, instructions, &prompt) == RESULT_OK) {
 *     printf("Thinking prompt: %s\n", prompt.final_prompt.data);
 * }
 * 
 * llm_prompt_cleanup(&prompt);
 * @endcode
 */
result_t llm_prompt_build_thinking(const llm_context_t* context, const char* instructions, llm_prompt_t* prompt) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 3)));

/**
 * @brief Build executing state prompt
 * 
 * Constructs an optimized prompt for the executing state that focuses
 * on action planning, resource management, and execution strategies.
 * 
 * @param context Prepared context for the prompt
 * @param instructions Specific execution instructions
 * @param prompt Prompt structure to populate
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Includes execution-specific system prompt
 * @note Focuses on actionable steps and implementation
 * @note Promotes use of action tags for command specification
 * @note Optimizes for clear action generation and resource usage
 * 
 * @warning context parameter must not be NULL and must be prepared
 * @warning prompt parameter must not be NULL and must be initialized
 */
result_t llm_prompt_build_executing(const llm_context_t* context, const char* instructions, llm_prompt_t* prompt) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 3)));

/**
 * @brief Build evaluating state prompt
 * 
 * Constructs an optimized prompt for the evaluating state that emphasizes
 * assessment, quality measurement, and progress evaluation.
 * 
 * @param context Prepared context for the prompt
 * @param instructions Specific evaluation instructions
 * @param prompt Prompt structure to populate
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Includes evaluation-specific system prompt
 * @note Emphasizes quality assessment and progress measurement
 * @note Promotes use of evaluation tags for structured assessment
 * @note Optimizes for objective evaluation and improvement recommendations
 * 
 * @warning context parameter must not be NULL and must be prepared
 * @warning prompt parameter must not be NULL and must be initialized
 */
result_t llm_prompt_build_evaluating(const llm_context_t* context, const char* instructions, llm_prompt_t* prompt) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 3)));

/**
 * @brief Build paging state prompt
 * 
 * Constructs an optimized prompt for the paging state that focuses
 * on memory management, context organization, and storage optimization.
 * 
 * @param context Prepared context for the prompt
 * @param instructions Specific paging instructions
 * @param prompt Prompt structure to populate
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Includes paging-specific system prompt
 * @note Focuses on memory organization and context management
 * @note Promotes use of paging tags for memory directives
 * @note Optimizes for intelligent memory management decisions
 * 
 * @warning context parameter must not be NULL and must be prepared
 * @warning prompt parameter must not be NULL and must be initialized
 */
result_t llm_prompt_build_paging(const llm_context_t* context, const char* instructions, llm_prompt_t* prompt) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 3)));

/**
 * @brief Add context information to prompt
 * 
 * Integrates context information from the context structure into
 * the appropriate sections of the prompt.
 * 
 * @param prompt Prompt structure to modify
 * @param context Context information to integrate
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Context is formatted for optimal LLM understanding
 * @note Memory components are integrated seamlessly
 * @note Context relationships are preserved
 * @note Context sizing is considered for prompt optimization
 * 
 * @warning prompt parameter must not be NULL
 * @warning context parameter must not be NULL
 */
result_t llm_prompt_add_context(llm_prompt_t* prompt, const llm_context_t* context) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Add instructions to prompt
 * 
 * Integrates specific instructions into the prompt structure with
 * appropriate formatting and emphasis.
 * 
 * @param prompt Prompt structure to modify
 * @param instructions Instruction text to add
 * @param instruction_type Type of instructions (task, format, behavior)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Instructions are formatted for maximum clarity
 * @note Instruction type affects placement and emphasis
 * @note Multiple instruction sets can be integrated
 * @note Instructions are optimized for LLM compliance
 * 
 * @warning prompt parameter must not be NULL
 * @warning instructions parameter must not be NULL
 * @warning instruction_type parameter must not be NULL
 */
result_t llm_prompt_add_instructions(llm_prompt_t* prompt, const char* instructions, const char* instruction_type) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Validate prompt format and structure
 * 
 * Validates that the prompt structure is properly formatted and
 * optimized for LLM processing.
 * 
 * @param prompt Prompt structure to validate
 * @param validation_errors Buffer to store validation error messages
 * @return RESULT_OK if valid, RESULT_ERR if errors found
 * 
 * @note Checks prompt structure and completeness
 * @note Validates system prompt presence and format
 * @note Ensures context integration is proper
 * @note Reports specific formatting issues
 * 
 * @warning prompt parameter must not be NULL
 * @warning validation_errors parameter must not be NULL and must be initialized
 */
result_t llm_prompt_validate_format(const llm_prompt_t* prompt, data_t* validation_errors) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Load prompt template from configuration
 * 
 * Loads a prompt template for the specified state from the configuration
 * system and prepares it for use.
 * 
 * @param state Agent state for template selection
 * @param template_name Name of template to load
 * @param template Template structure to populate
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Templates are loaded from configuration files
 * @note Template variables are identified and cataloged
 * @note Template effectiveness scores are loaded if available
 * @note Multiple templates per state are supported
 * 
 * @warning template parameter must not be NULL
 */
result_t llm_prompt_load_template(agent_state_t state, const char* template_name, llm_prompt_template_t* template) __attribute__((warn_unused_result)) __attribute__((nonnull(3)));

/**
 * @brief Apply template with variable substitution
 * 
 * Applies a prompt template by substituting variables with actual
 * values and generating the final prompt structure.
 * 
 * @param template Template to apply
 * @param variables Array of variable substitutions
 * @param variable_count Number of variables to substitute
 * @param prompt Prompt structure to populate
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note All template variables must have substitution values
 * @note Variable substitution preserves prompt formatting
 * @note Template metadata is transferred to prompt
 * @note Substitution errors are reported clearly
 * 
 * @warning template parameter must not be NULL
 * @warning variables parameter must not be NULL if variable_count > 0
 * @warning prompt parameter must not be NULL and must be initialized
 */
result_t llm_prompt_apply_template(const llm_prompt_template_t* template, const llm_prompt_variable_t* variables, size_t variable_count, llm_prompt_t* prompt) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 4)));

/**
 * @brief Optimize prompt for LLM performance
 * 
 * Optimizes prompt structure, content, and formatting for maximum
 * LLM performance and response quality.
 * 
 * @param prompt Prompt structure to optimize
 * @param target_state Target agent state for optimization
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Optimization considers state-specific requirements
 * @note Content is reorganized for optimal LLM processing
 * @note Redundant information is removed or consolidated
 * @note Prompt length is optimized for context window
 * 
 * @warning prompt parameter must not be NULL
 */
result_t llm_prompt_optimize(llm_prompt_t* prompt, agent_state_t target_state) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Assemble final prompt from components
 * 
 * Combines all prompt components into the final prompt string
 * with optimal formatting and structure.
 * 
 * @param prompt Prompt structure with components to assemble
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Components are ordered for optimal LLM understanding
 * @note Formatting is applied for maximum clarity
 * @note Final prompt is stored in prompt->final_prompt
 * @note Metadata is updated with assembly information
 * 
 * @warning prompt parameter must not be NULL
 */
result_t llm_prompt_assemble(llm_prompt_t* prompt) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Estimate prompt token count
 * 
 * Estimates the token count for the prompt using character-based
 * heuristics and model-specific conversion ratios.
 * 
 * @param prompt Prompt structure to analyze
 * @param token_count Pointer to store estimated token count
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Uses conservative estimation for safety
 * @note Includes overhead for formatting and structure
 * @note Model-specific ratios can be configured
 * @note Estimation is updated in prompt metadata
 * 
 * @warning prompt parameter must not be NULL
 * @warning token_count parameter must not be NULL
 */
result_t llm_prompt_estimate_tokens(const llm_prompt_t* prompt, size_t* token_count) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Get default system prompt for state
 * 
 * Retrieves the default system prompt for the specified agent state
 * from the configuration system.
 * 
 * @param state Agent state for prompt selection
 * @param system_prompt Buffer to store system prompt
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Default prompts are optimized for each state
 * @note System prompts include state-specific instructions
 * @note Prompts are loaded from configuration
 * @note Fallback prompts are provided if configuration is missing
 * 
 * @warning system_prompt parameter must not be NULL and must be initialized
 */
result_t llm_prompt_get_system_default(agent_state_t state, data_t* system_prompt) __attribute__((warn_unused_result)) __attribute__((nonnull(2)));

/**
 * @brief Update prompt effectiveness score
 * 
 * Updates the effectiveness score for a prompt template based on
 * response quality and performance metrics.
 * 
 * @param template_name Name of template to update
 * @param state Agent state associated with template
 * @param effectiveness_score New effectiveness score (0.0-1.0)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Effectiveness scores are used for template selection
 * @note Scores are persisted in configuration
 * @note Historical effectiveness data is maintained
 * @note Scores influence future prompt optimization
 * 
 * @warning template_name parameter must not be NULL
 */
result_t llm_prompt_update_effectiveness(const char* template_name, agent_state_t state, float effectiveness_score) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Initialize prompt structure
 * 
 * Initializes all data buffers and components in a prompt structure
 * with appropriate initial capacities.
 * 
 * @param prompt Prompt structure to initialize
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Initializes all section data buffers
 * @note Sets default values for metadata fields
 * @note Prepares structure for prompt construction
 * 
 * @warning prompt parameter must not be NULL
 */
result_t llm_prompt_init(llm_prompt_t* prompt) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Clean up prompt structure
 * 
 * Frees all data buffers and resources associated with a prompt structure.
 * Must be called to prevent memory leaks.
 * 
 * @param prompt Prompt structure to clean up
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Frees all section data buffers
 * @note Resets all metadata to default values
 * @note Structure becomes invalid after cleanup
 * 
 * @warning prompt parameter must not be NULL
 */
result_t llm_prompt_cleanup(llm_prompt_t* prompt) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Initialize prompt template structure
 * 
 * Initializes a prompt template structure with appropriate buffers
 * and default values.
 * 
 * @param template Template structure to initialize
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Initializes template content buffer
 * @note Sets default values for all fields
 * @note Prepares structure for template loading
 * 
 * @warning template parameter must not be NULL
 */
result_t llm_prompt_template_init(llm_prompt_template_t* template) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Clean up prompt template structure
 * 
 * Frees all resources associated with a prompt template structure.
 * Must be called to prevent memory leaks.
 * 
 * @param template Template structure to clean up
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Frees template content buffer
 * @note Resets all fields to default values
 * @note Structure becomes invalid after cleanup
 * 
 * @warning template parameter must not be NULL
 */
result_t llm_prompt_template_cleanup(llm_prompt_template_t* template) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/** @} */

#endif /* LKJAGENT_LLM_PROMPT_H */
