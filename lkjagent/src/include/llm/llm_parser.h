/**
 * @file llm_parser.h
 * @brief LLM response parser interface for simple tag format processing
 * 
 * This header provides comprehensive parsing capabilities for LLM responses
 * using the simple tag format. It handles extraction of thinking blocks,
 * action blocks, context keys, and paging directives with robust error handling.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#ifndef LKJAGENT_LLM_PARSER_H
#define LKJAGENT_LLM_PARSER_H

#include "types.h"
#include "data.h"

/**
 * @defgroup LLM_Parser LLM Response Parser Operations
 * @{
 */

/**
 * @brief Parsed LLM response structure
 * 
 * Contains all extracted components from an LLM response including
 * structured blocks, context keys, and directives.
 */
typedef struct {
    /** Thinking block content */
    data_t thinking;
    /** Action block content */
    data_t action;
    /** Evaluation block content */
    data_t evaluation;
    /** Paging block content */
    data_t paging;
    /** Extracted context keys */
    char context_keys[32][64]; /* MAX_TAG_SIZE from main header */
    /** Number of context keys found */
    size_t context_key_count;
    /** Paging directives */
    char paging_directives[16][128];
    /** Number of paging directives found */
    size_t directive_count;
    /** Response quality score (0-100) */
    size_t quality_score;
    /** Parse errors and warnings */
    data_t parse_errors;
} llm_parsed_response_t;

/**
 * @brief Context key extraction result structure
 */
typedef struct {
    /** Context key name */
    char key[64]; /* MAX_TAG_SIZE from main header */
    /** Importance score if specified */
    size_t importance;
    /** Source block where key was found */
    char source_block[32];
    /** Position in original text */
    size_t text_position;
} context_key_result_t;

/**
 * @brief Paging directive structure
 */
typedef struct {
    /** Directive type (move, archive, delete, importance) */
    char directive_type[32];
    /** Target context key */
    char target_key[64];
    /** Directive parameters */
    char parameters[256];
    /** Source line in paging block */
    size_t source_line;
} paging_directive_t;

/**
 * @brief Parse complete LLM response with all blocks
 * 
 * Performs comprehensive parsing of an LLM response, extracting all
 * structured blocks, context keys, and directives using the simple tag format.
 * 
 * @param llm_response Complete LLM response text
 * @param parsed_response Structure to store all parsed components
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Processes all supported block types (thinking, action, evaluation, paging)
 * @note Extracts context keys from all blocks using pattern matching
 * @note Validates tag format and handles nested or malformed tags
 * @note Calculates response quality score based on structure and content
 * @note Parse errors and warnings are accumulated in parse_errors field
 * 
 * @warning llm_response parameter must not be NULL
 * @warning parsed_response parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * llm_parsed_response_t parsed;
 * llm_parsed_response_init(&parsed);
 * 
 * const char* response = "<thinking>Need to analyze data</thinking>\n<action>run_analysis</action>";
 * if (llm_parse_response(response, &parsed) == RESULT_OK) {
 *     printf("Thinking: %s\n", parsed.thinking.data);
 *     printf("Action: %s\n", parsed.action.data);
 *     printf("Context keys found: %zu\n", parsed.context_key_count);
 * }
 * 
 * llm_parsed_response_cleanup(&parsed);
 * @endcode
 */
result_t llm_parse_response(const char* llm_response, llm_parsed_response_t* parsed_response) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Parse thinking block from LLM response
 * 
 * Specialized parser for `<thinking>` blocks that contain the agent's
 * internal reasoning process. Handles multiline content and preserves formatting.
 * 
 * @param llm_response Complete LLM response text
 * @param thinking_content Data buffer to store thinking content
 * @return RESULT_OK on success, RESULT_ERR on failure or thinking block not found
 * 
 * @note Thinking blocks can span multiple lines and contain complex reasoning
 * @note Leading and trailing whitespace within blocks is trimmed
 * @note If multiple thinking blocks exist, all are concatenated with separators
 * @note The output buffer is cleared before writing the extracted content
 * 
 * @warning llm_response parameter must not be NULL
 * @warning thinking_content parameter must not be NULL and must be initialized
 */
result_t llm_parse_thinking_block(const char* llm_response, data_t* thinking_content) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Parse action block from LLM response
 * 
 * Specialized parser for `<action>` blocks that contain the agent's
 * intended actions or commands to execute.
 * 
 * @param llm_response Complete LLM response text
 * @param action_content Data buffer to store action content
 * @return RESULT_OK on success, RESULT_ERR on failure or action block not found
 * 
 * @note Action blocks typically contain structured commands or instructions
 * @note Multiple action blocks are concatenated with newline separators
 * @note Leading and trailing whitespace within blocks is trimmed
 * @note The output buffer is cleared before writing the extracted content
 * 
 * @warning llm_response parameter must not be NULL
 * @warning action_content parameter must not be NULL and must be initialized
 */
result_t llm_parse_action_block(const char* llm_response, data_t* action_content) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Parse evaluation block from LLM response
 * 
 * Specialized parser for `<evaluation>` blocks that contain the agent's
 * assessment and evaluation of progress or results.
 * 
 * @param llm_response Complete LLM response text
 * @param evaluation_content Data buffer to store evaluation content
 * @return RESULT_OK on success, RESULT_ERR on failure or evaluation block not found
 * 
 * @note Evaluation blocks contain progress assessments and quality metrics
 * @note Content includes reasoning about effectiveness and next steps
 * @note Multiple evaluation blocks are concatenated with separators
 * @note The output buffer is cleared before writing the extracted content
 * 
 * @warning llm_response parameter must not be NULL
 * @warning evaluation_content parameter must not be NULL and must be initialized
 */
result_t llm_parse_evaluation_block(const char* llm_response, data_t* evaluation_content) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Parse paging block from LLM response
 * 
 * Specialized parser for `<paging>` blocks that contain LLM-directed
 * memory management directives and instructions.
 * 
 * @param llm_response Complete LLM response text
 * @param paging_content Data buffer to store paging content
 * @return RESULT_OK on success, RESULT_ERR on failure or paging block not found
 * 
 * @note Paging blocks contain memory management directives
 * @note Directives include move, archive, delete, and importance commands
 * @note Each directive is typically on a separate line with clear syntax
 * @note The output buffer is cleared before writing the extracted content
 * 
 * @warning llm_response parameter must not be NULL
 * @warning paging_content parameter must not be NULL and must be initialized
 */
result_t llm_parse_paging_block(const char* llm_response, data_t* paging_content) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Validate tag format and structure
 * 
 * Validates that LLM response contains properly formatted tags with
 * correct opening and closing syntax. Detects common formatting errors.
 * 
 * @param llm_response LLM response text to validate
 * @param validation_errors Buffer to store validation error messages
 * @return RESULT_OK if format is valid, RESULT_ERR if errors found
 * 
 * @note Checks for proper tag opening and closing syntax
 * @note Detects nested tags and malformed tag structures
 * @note Reports specific errors with line numbers when possible
 * @note Validation errors are accumulated in the errors buffer
 * 
 * @warning llm_response parameter must not be NULL
 * @warning validation_errors parameter must not be NULL and must be initialized
 */
result_t llm_validate_tag_format(const char* llm_response, data_t* validation_errors) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Extract context keys from LLM response
 * 
 * Performs comprehensive context key extraction from all parts of the LLM
 * response using pattern matching and heuristics to identify key references.
 * 
 * @param llm_response Complete LLM response text
 * @param context_keys Array to store extracted context keys
 * @param max_keys Maximum number of keys to extract
 * @param keys_found Pointer to store actual number of keys found
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Uses multiple extraction strategies: explicit tags, patterns, references
 * @note Deduplicates extracted keys and validates key format
 * @note Keys are sorted by relevance and importance
 * @note Extraction includes keys from all block types
 * 
 * @warning llm_response parameter must not be NULL
 * @warning context_keys parameter must not be NULL
 * @warning keys_found parameter must not be NULL
 */
result_t llm_extract_context_keys(const char* llm_response, context_key_result_t* context_keys, size_t max_keys, size_t* keys_found) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 4)));

/**
 * @brief Parse paging directives from paging block
 * 
 * Extracts and parses individual paging directives from a paging block,
 * validating directive syntax and parameters.
 * 
 * @param paging_content Content of paging block
 * @param directives Array to store parsed directives
 * @param max_directives Maximum number of directives to parse
 * @param directives_found Pointer to store actual number of directives found
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Parses directive types: move, archive, delete, importance
 * @note Validates directive syntax and required parameters
 * @note Handles multi-line directives and parameter lists
 * @note Reports parsing errors for malformed directives
 * 
 * @warning paging_content parameter must not be NULL
 * @warning directives parameter must not be NULL
 * @warning directives_found parameter must not be NULL
 */
result_t llm_parse_directives(const char* paging_content, paging_directive_t* directives, size_t max_directives, size_t* directives_found) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 4)));

/**
 * @brief Calculate response quality score
 * 
 * Analyzes LLM response structure and content to calculate a quality
 * score based on completeness, formatting, and content richness.
 * 
 * @param llm_response Complete LLM response text
 * @param parsed_response Parsed response structure (optional, can be NULL)
 * @param quality_score Pointer to store calculated quality score (0-100)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Quality factors: tag format, content length, structure completeness
 * @note Higher scores for well-structured responses with rich content
 * @note Penalizes malformed tags, empty blocks, or incomplete responses
 * @note Score helps identify response quality issues for optimization
 * 
 * @warning llm_response parameter must not be NULL
 * @warning quality_score parameter must not be NULL
 */
result_t llm_response_quality_score(const char* llm_response, const llm_parsed_response_t* parsed_response, size_t* quality_score) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 3)));

/**
 * @brief Extract context keys from thinking responses
 * 
 * Specialized context key extraction for thinking blocks, using patterns
 * specific to reasoning and analysis content.
 * 
 * @param thinking_content Content of thinking block
 * @param context_keys Array to store extracted keys
 * @param max_keys Maximum number of keys to extract
 * @param keys_found Pointer to store actual number of keys found
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Uses thinking-specific patterns for key detection
 * @note Looks for analytical references and data mentions
 * @note Prioritizes keys mentioned in reasoning context
 * 
 * @warning thinking_content parameter must not be NULL
 * @warning context_keys parameter must not be NULL
 * @warning keys_found parameter must not be NULL
 */
result_t llm_extract_context_keys_thinking(const char* thinking_content, char context_keys[][64], size_t max_keys, size_t* keys_found) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 4)));

/**
 * @brief Extract context keys from action responses
 * 
 * Specialized context key extraction for action blocks, focusing on
 * operational and execution-related key references.
 * 
 * @param action_content Content of action block
 * @param context_keys Array to store extracted keys
 * @param max_keys Maximum number of keys to extract
 * @param keys_found Pointer to store actual number of keys found
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Uses action-specific patterns for key detection
 * @note Looks for operational references and resource mentions
 * @note Prioritizes keys related to execution context
 * 
 * @warning action_content parameter must not be NULL
 * @warning context_keys parameter must not be NULL
 * @warning keys_found parameter must not be NULL
 */
result_t llm_extract_context_keys_action(const char* action_content, char context_keys[][64], size_t max_keys, size_t* keys_found) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 4)));

/**
 * @brief Extract context keys from evaluation responses
 * 
 * Specialized context key extraction for evaluation blocks, focusing on
 * assessment and quality-related key references.
 * 
 * @param evaluation_content Content of evaluation block
 * @param context_keys Array to store extracted keys
 * @param max_keys Maximum number of keys to extract
 * @param keys_found Pointer to store actual number of keys found
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Uses evaluation-specific patterns for key detection
 * @note Looks for performance and quality references
 * @note Prioritizes keys mentioned in assessment context
 * 
 * @warning evaluation_content parameter must not be NULL
 * @warning context_keys parameter must not be NULL
 * @warning keys_found parameter must not be NULL
 */
result_t llm_extract_context_keys_evaluation(const char* evaluation_content, char context_keys[][64], size_t max_keys, size_t* keys_found) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 4)));

/**
 * @brief Extract context keys from paging responses
 * 
 * Specialized context key extraction for paging blocks, focusing on
 * memory management and organization-related key references.
 * 
 * @param paging_content Content of paging block
 * @param context_keys Array to store extracted keys
 * @param max_keys Maximum number of keys to extract
 * @param keys_found Pointer to store actual number of keys found
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Uses paging-specific patterns for key detection
 * @note Looks for memory organization and context references
 * @note Prioritizes keys mentioned in paging directives
 * 
 * @warning paging_content parameter must not be NULL
 * @warning context_keys parameter must not be NULL
 * @warning keys_found parameter must not be NULL
 */
result_t llm_extract_context_keys_paging(const char* paging_content, char context_keys[][64], size_t max_keys, size_t* keys_found) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 4)));

/**
 * @brief Initialize parsed response structure
 * 
 * Initializes all data buffers and arrays in a parsed response structure
 * with appropriate initial capacities.
 * 
 * @param parsed_response Parsed response structure to initialize
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Initializes all content data buffers
 * @note Sets counters and arrays to empty state
 * @note Structure is ready for parsing after successful initialization
 * 
 * @warning parsed_response parameter must not be NULL
 */
result_t llm_parsed_response_init(llm_parsed_response_t* parsed_response) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Clean up parsed response structure
 * 
 * Frees all data buffers and resets arrays in a parsed response structure.
 * Must be called to prevent memory leaks.
 * 
 * @param parsed_response Parsed response structure to clean up
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Frees all content data buffers
 * @note Resets all counters and arrays to empty state
 * @note Structure becomes invalid after cleanup
 * 
 * @warning parsed_response parameter must not be NULL
 */
result_t llm_parsed_response_cleanup(llm_parsed_response_t* parsed_response) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/** @} */

#endif /* LKJAGENT_LLM_PARSER_H */
