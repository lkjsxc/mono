/**
 * @file tag_parser.h
 * @brief Simple tag format parser interface for LKJAgent
 * 
 * This header provides the interface for parsing simple XML-like tags from
 * LLM responses. The parser handles basic `<tag>content</tag>` format with
 * robust error handling for malformed input.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#ifndef LKJAGENT_TAG_PARSER_H
#define LKJAGENT_TAG_PARSER_H

#include "types.h"
#include "data.h"

/**
 * @defgroup Tag_Parser Simple Tag Format Parser
 * @{
 */

/**
 * @brief Parse simple tag format and extract content
 * 
 * Extracts content from between matching opening and closing tags in the
 * format `<tag>content</tag>`. Handles nested tags correctly and validates
 * tag matching.
 * 
 * @param input Input text containing tagged content
 * @param tag_name Name of the tag to extract (without angle brackets)
 * @param output Data buffer to store extracted content
 * @param allow_nested Whether to allow nested tags of the same name
 * @return RESULT_OK on success, RESULT_ERR on failure or tag not found
 * 
 * @note If multiple instances of the tag exist, only the first is extracted
 * @note The output buffer is cleared before writing the extracted content
 * @note Tag names are case-sensitive
 * @note Whitespace around tag names is ignored
 * 
 * @warning input parameter must not be NULL
 * @warning tag_name parameter must not be NULL and must be non-empty
 * @warning output parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t result;
 * data_init(&result, 256);
 * const char* text = "Some text <thinking>I need to analyze this</thinking> more text";
 * if (tag_parse_simple(text, "thinking", &result, false) == RESULT_OK) {
 *     printf("Thinking content: %s\n", result.data); // "I need to analyze this"
 * }
 * data_destroy(&result);
 * @endcode
 */
result_t tag_parse_simple(const char* input, const char* tag_name, data_t* output, bool allow_nested) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Extract content between specific opening and closing tags
 * 
 * Low-level function to extract content between manually specified opening
 * and closing tag strings. Provides more control than tag_parse_simple.
 * 
 * @param input Input text to search
 * @param opening_tag Complete opening tag (e.g., "<thinking>")
 * @param closing_tag Complete closing tag (e.g., "</thinking>")
 * @param output Data buffer to store extracted content
 * @param start_offset Offset in input to start searching from
 * @return RESULT_OK on success, RESULT_ERR on failure or tags not found
 * 
 * @note The search starts from start_offset in the input string
 * @note The output buffer is cleared before writing the extracted content
 * @note Tag matching is exact and case-sensitive
 * @note Returns the content between the first matching tag pair found
 * 
 * @warning input parameter must not be NULL
 * @warning opening_tag parameter must not be NULL and must be non-empty
 * @warning closing_tag parameter must not be NULL and must be non-empty
 * @warning output parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t result;
 * data_init(&result, 256);
 * const char* text = "Before <custom>content here</custom> after";
 * if (tag_extract_content(text, "<custom>", "</custom>", &result, 0) == RESULT_OK) {
 *     printf("Content: %s\n", result.data); // "content here"
 * }
 * data_destroy(&result);
 * @endcode
 */
result_t tag_extract_content(const char* input, const char* opening_tag, const char* closing_tag, data_t* output, size_t start_offset) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3, 4)));

/**
 * @brief Parse thinking block from LLM response
 * 
 * Specialized parser for `<thinking>` blocks that contain the agent's
 * internal reasoning process. Handles multiline content and preserves
 * formatting.
 * 
 * @param llm_response Complete LLM response text
 * @param thinking_content Data buffer to store thinking content
 * @return RESULT_OK on success, RESULT_ERR on failure or thinking block not found
 * 
 * @note Thinking blocks can span multiple lines and contain complex reasoning
 * @note Leading and trailing whitespace is preserved within the block
 * @note If multiple thinking blocks exist, all are concatenated with separators
 * @note The output buffer is cleared before writing the extracted content
 * 
 * @warning llm_response parameter must not be NULL
 * @warning thinking_content parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t thinking;
 * data_init(&thinking, 1024);
 * const char* response = "<thinking>\nI need to consider multiple factors:\n1. User intent\n2. Available data\n</thinking>";
 * if (tag_parse_thinking(response, &thinking) == RESULT_OK) {
 *     printf("Agent thinking: %s\n", thinking.data);
 * }
 * data_destroy(&thinking);
 * @endcode
 */
result_t tag_parse_thinking(const char* llm_response, data_t* thinking_content) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

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
 * 
 * Example usage:
 * @code
 * data_t action;
 * data_init(&action, 512);
 * const char* response = "Analysis complete. <action>execute_command: ls -la</action>";
 * if (tag_parse_action(response, &action) == RESULT_OK) {
 *     printf("Action to execute: %s\n", action.data);
 * }
 * data_destroy(&action);
 * @endcode
 */
result_t tag_parse_action(const char* llm_response, data_t* action_content) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Parse evaluation block from LLM response
 * 
 * Specialized parser for `<evaluation>` blocks that contain the agent's
 * assessment of results and next steps.
 * 
 * @param llm_response Complete LLM response text
 * @param evaluation_content Data buffer to store evaluation content
 * @return RESULT_OK on success, RESULT_ERR on failure or evaluation block not found
 * 
 * @note Evaluation blocks contain assessment of previous actions and outcomes
 * @note Multiple evaluation blocks are concatenated with separators
 * @note Content formatting and structure is preserved
 * @note The output buffer is cleared before writing the extracted content
 * 
 * @warning llm_response parameter must not be NULL
 * @warning evaluation_content parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t evaluation;
 * data_init(&evaluation, 512);
 * const char* response = "<evaluation>Command executed successfully. Output shows 5 files.</evaluation>";
 * if (tag_parse_evaluation(response, &evaluation) == RESULT_OK) {
 *     printf("Evaluation: %s\n", evaluation.data);
 * }
 * data_destroy(&evaluation);
 * @endcode
 */
result_t tag_parse_evaluation(const char* llm_response, data_t* evaluation_content) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Parse paging block from LLM response
 * 
 * Specialized parser for `<paging>` blocks that contain memory management
 * directives from the LLM about context keys and memory operations.
 * 
 * @param llm_response Complete LLM response text
 * @param paging_content Data buffer to store paging directives
 * @return RESULT_OK on success, RESULT_ERR on failure or paging block not found
 * 
 * @note Paging blocks contain structured directives for memory management
 * @note Content typically includes context key operations and priorities
 * @note Multiple paging blocks are processed in sequence
 * @note The output buffer is cleared before writing the extracted content
 * 
 * @warning llm_response parameter must not be NULL
 * @warning paging_content parameter must not be NULL and must be initialized
 * 
 * Example usage:
 * @code
 * data_t paging;
 * data_init(&paging, 256);
 * const char* response = "<paging>load_context: user_preferences, archive_context: old_logs</paging>";
 * if (tag_parse_paging(response, &paging) == RESULT_OK) {
 *     printf("Paging directives: %s\n", paging.data);
 * }
 * data_destroy(&paging);
 * @endcode
 */
result_t tag_parse_paging(const char* llm_response, data_t* paging_content) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Parse context keys from LLM paging directives
 * 
 * Extracts context key operations from paging content, parsing commands
 * like "load_context: key1", "archive_context: key2", etc.
 * 
 * @param paging_content Content from paging blocks
 * @param context_keys Array to store parsed context keys
 * @param max_keys Maximum number of context keys to parse
 * @param parsed_count Pointer to store number of keys actually parsed
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Supported operations: load_context, archive_context, prioritize_context
 * @note Each operation can specify multiple comma-separated keys
 * @note Keys are validated for format and length constraints
 * @note The context_keys array is filled with valid context_key_t structures
 * 
 * @warning paging_content parameter must not be NULL
 * @warning context_keys parameter must not be NULL
 * @warning parsed_count parameter must not be NULL
 * @warning max_keys must be greater than 0
 * 
 * Example usage:
 * @code
 * context_key_t keys[10];
 * size_t count;
 * const char* paging = "load_context: user_data, session_info; archive_context: old_logs";
 * if (tag_parse_context_keys(paging, keys, 10, &count) == RESULT_OK) {
 *     printf("Parsed %zu context keys\n", count);
 * }
 * @endcode
 */
result_t tag_parse_context_keys(const char* paging_content, context_key_t* context_keys, size_t max_keys, size_t* parsed_count) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 4)));

/**
 * @brief Validate simple tag format structure
 * 
 * Validates that the input contains properly formatted tags with matching
 * opening and closing tags. Useful for validating LLM responses before parsing.
 * 
 * @param input Input text to validate
 * @param tag_name Specific tag name to validate (NULL to validate all tags)
 * @return RESULT_OK if format is valid, RESULT_ERR if malformed
 * 
 * @note If tag_name is NULL, validates all tag pairs in the input
 * @note If tag_name is specified, validates only that specific tag type
 * @note Checks for proper nesting, matching pairs, and valid tag names
 * @note Does not validate tag content, only structural correctness
 * 
 * @warning input parameter must not be NULL
 * 
 * Example usage:
 * @code
 * const char* text = "<thinking>Good content</thinking>";
 * if (tag_validate_format(text, "thinking") == RESULT_OK) {
 *     printf("Tag format is valid\n");
 * }
 * 
 * if (tag_validate_format(text, NULL) == RESULT_OK) {
 *     printf("All tags in text are properly formatted\n");
 * }
 * @endcode
 */
result_t tag_validate_format(const char* input, const char* tag_name) __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Parse paging directives into structured commands
 * 
 * Parses LLM paging directives into structured command format for
 * memory management operations. Handles complex directive syntax.
 * 
 * @param paging_content Raw paging directive content
 * @param commands Array to store parsed commands
 * @param max_commands Maximum number of commands to parse
 * @param parsed_count Pointer to store number of commands actually parsed
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Commands include operation type, target keys, and parameters
 * @note Supports operations: load, archive, prioritize, cleanup, merge
 * @note Each command can affect multiple context keys
 * @note Commands are executed in the order they appear in the directive
 * 
 * @warning paging_content parameter must not be NULL
 * @warning commands parameter must not be NULL
 * @warning parsed_count parameter must not be NULL
 * @warning max_commands must be greater than 0
 * 
 * Example usage:
 * @code
 * // This function would require a command structure definition
 * // Implementation depends on the specific command format needed
 * @endcode
 */
result_t tag_parse_paging_directives(const char* paging_content, void* commands, size_t max_commands, size_t* parsed_count) __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 4)));

/** @} */

#endif /* LKJAGENT_TAG_PARSER_H */
