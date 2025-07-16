/**
 * @file lkjagent.h
 * @brief Main header file for the lkjagent autonomous AI agent system
 * 
 * This header provides all the necessary definitions, structures, and function
 * declarations for the modular lkjagent system organized by functional areas:
 * - Core agent management (core/)
 * - State management (state/)
 * - Memory management (memory/)
 * - API integration (api/)
 * - Tool system (tools/)
 * - Utilities (utils/)
 */

#ifndef _LKJAGENT_H
#define _LKJAGENT_H

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// ============================================================================
// BASIC TYPES AND ENUMS
// ============================================================================

typedef enum {
    RESULT_OK = 0,
    RESULT_ERR = 1,
    RESULT_TASK_COMPLETE = 2,
} result_t;

// Agent states
typedef enum {
    AGENT_STATE_THINKING = 0,
    AGENT_STATE_EXECUTING = 1,
    AGENT_STATE_EVALUATING = 2,
    AGENT_STATE_PAGING = 3
} agent_state_t;

// Tool types
typedef enum {
    TOOL_SEARCH = 0,
    TOOL_RETRIEVE = 1,
    TOOL_WRITE = 2,
    TOOL_EXECUTE_CODE = 3,
    TOOL_FORGET = 4
} tool_type_t;

// ============================================================================
// CORE DATA STRUCTURES
// ============================================================================

typedef struct {
    char* data;
    size_t size;
    size_t capacity;
} token_t;

// Configuration structures for loading from config.json
typedef struct {
    char endpoint[256];
    char model[64];
    int stream;
} lmstudio_config_t;

typedef struct {
    int max_iterations;
    double evaluation_threshold;
    char memory_file[256];
    size_t ram_size;
    size_t max_history;
    int autonomous_mode;
    int continuous_thinking;
    int self_directed;
} agent_config_detailed_t;

typedef struct {
    int timeout_seconds;
    int max_request_size;
    int max_response_size;
    char user_agent[64];
} http_config_t;

typedef struct {
    char role[16];
    char content[512];
} system_prompt_config_t;

typedef struct {
    lmstudio_config_t lmstudio;
    agent_config_detailed_t agent;
    http_config_t http;
    system_prompt_config_t system_prompt;
} full_config_t;

// Memory structure for RAM
typedef struct {
    token_t system_prompt;
    token_t current_state;
    token_t task_goal;
    token_t plan;
    token_t scratchpad;
    token_t recent_history;
    token_t retrieved_from_disk;
} agent_memory_t;

// Agent configuration
typedef struct {
    int max_iterations;
    double evaluation_threshold;
    size_t ram_size;
    char disk_file[256];
    size_t max_history;
} agent_config_t;

// Agent context
typedef struct {
    agent_state_t state;
    agent_memory_t memory;
    agent_config_t config;
    int iteration_count;
    char lmstudio_endpoint[256];
    char model_name[64];
    full_config_t loaded_config;
} agent_t;

// ============================================================================
// ERROR HANDLING AND LOGGING
// ============================================================================

void lkj_log_error(const char* function, const char* message);
void lkj_log_errno(const char* function, const char* operation);
const char* lkj_get_last_error(void);
void lkj_clear_last_error(void);
void lkj_set_error_logging(int enable);
int lkj_is_error_logging_enabled(void);

// ============================================================================
// CORE AGENT FUNCTIONS (core/)
// ============================================================================

// Agent lifecycle management
agent_t* agent_create(const char* config_file);
void agent_destroy(agent_t* agent);
__attribute__((warn_unused_result)) result_t agent_set_task(agent_t* agent, const char* task);
int agent_is_task_complete(const agent_t* agent);

// Configuration management
__attribute__((warn_unused_result)) result_t config_load(const char* config_file, full_config_t* config);
__attribute__((warn_unused_result)) result_t config_apply_to_agent(agent_t* agent, const full_config_t* config);

// Agent execution
__attribute__((warn_unused_result)) result_t agent_step(agent_t* agent);
__attribute__((warn_unused_result)) result_t agent_step_intelligent(agent_t* agent);
__attribute__((warn_unused_result)) result_t agent_step_ai_driven(agent_t* agent);
__attribute__((warn_unused_result)) result_t agent_run(agent_t* agent);
__attribute__((warn_unused_result)) result_t agent_run_autonomous(agent_t* agent);

// ============================================================================
// STATE MANAGEMENT FUNCTIONS (state/)
// ============================================================================

// State management and transitions
const char* agent_state_to_string(agent_state_t state);
__attribute__((warn_unused_result)) result_t agent_transition_state(agent_t* agent, agent_state_t new_state);
int agent_is_valid_transition(agent_state_t current_state, agent_state_t new_state);
int agent_should_page(const agent_t* agent);
const char* agent_get_transition_reason(agent_state_t current_state, agent_state_t new_state);
__attribute__((warn_unused_result)) result_t agent_initialize_state(agent_t* agent, agent_state_t new_state);
__attribute__((warn_unused_result)) result_t agent_decide_next_state(agent_t* agent, agent_state_t* next_state);

// State-specific functions
__attribute__((warn_unused_result)) result_t state_thinking_init(agent_t* agent);
__attribute__((warn_unused_result)) result_t state_thinking_execute(agent_t* agent);
__attribute__((warn_unused_result)) result_t state_thinking_next(agent_t* agent, agent_state_t* next_state);

__attribute__((warn_unused_result)) result_t state_executing_init(agent_t* agent);
__attribute__((warn_unused_result)) result_t state_executing_execute(agent_t* agent);
__attribute__((warn_unused_result)) result_t state_executing_next(agent_t* agent, agent_state_t* next_state);

__attribute__((warn_unused_result)) result_t state_evaluating_init(agent_t* agent);
__attribute__((warn_unused_result)) result_t state_evaluating_execute(agent_t* agent);
__attribute__((warn_unused_result)) result_t state_evaluating_next(agent_t* agent, agent_state_t* next_state);

__attribute__((warn_unused_result)) result_t state_paging_init(agent_t* agent);
__attribute__((warn_unused_result)) result_t state_paging_execute(agent_t* agent);
__attribute__((warn_unused_result)) result_t state_paging_next(agent_t* agent, agent_state_t* next_state);

// ============================================================================
// MEMORY MANAGEMENT FUNCTIONS (memory/)
// ============================================================================

__attribute__((warn_unused_result)) result_t agent_memory_init(agent_memory_t* memory, char buffers[][2048], size_t num_buffers);
__attribute__((warn_unused_result)) result_t agent_memory_save_to_disk(const agent_t* agent);
__attribute__((warn_unused_result)) result_t agent_memory_load_from_disk(agent_t* agent);
__attribute__((warn_unused_result)) result_t agent_memory_clear_ram(agent_t* agent);

// ============================================================================
// API INTEGRATION FUNCTIONS (api/)
// ============================================================================

__attribute__((warn_unused_result)) result_t agent_build_prompt(const agent_t* agent, token_t* prompt);
__attribute__((warn_unused_result)) result_t agent_call_lmstudio(agent_t* agent, const token_t* prompt, token_t* response);
__attribute__((warn_unused_result)) result_t agent_parse_response(agent_t* agent, const token_t* response);
__attribute__((warn_unused_result)) result_t agent_ai_decide_next_action(agent_t* agent, token_t* next_action);

// ============================================================================
// TOOL SYSTEM FUNCTIONS (tools/)
// ============================================================================

__attribute__((warn_unused_result)) result_t agent_execute_tool(agent_t* agent, tool_type_t tool, const char* args, token_t* result);
__attribute__((warn_unused_result)) result_t agent_tool_search(agent_t* agent, const char* query, token_t* result);
__attribute__((warn_unused_result)) result_t agent_tool_retrieve(agent_t* agent, const char* key, token_t* result);
__attribute__((warn_unused_result)) result_t agent_tool_write(agent_t* agent, const char* key, const char* value, const char* tags);
__attribute__((warn_unused_result)) result_t agent_tool_execute_code(agent_t* agent, const char* code, token_t* result);
__attribute__((warn_unused_result)) result_t agent_tool_forget(agent_t* agent, const char* key);

// ============================================================================
// UTILITY FUNCTIONS (utils/)
// ============================================================================

// File management functions
__attribute__((warn_unused_result)) result_t file_read(const char* path, token_t* content);
__attribute__((warn_unused_result)) result_t file_write(const char* path, const token_t* content);

// Token management functions
__attribute__((warn_unused_result)) result_t token_init(token_t* token, char* buffer, size_t capacity);
__attribute__((warn_unused_result)) result_t token_clear(token_t* token);
__attribute__((warn_unused_result)) result_t token_set(token_t* token, const char* str);
__attribute__((warn_unused_result)) result_t token_set_length(token_t* token, const char* buffer, size_t length);
__attribute__((warn_unused_result)) result_t token_append(token_t* token, const char* str);
__attribute__((warn_unused_result)) result_t token_append_length(token_t* token, const char* buffer, size_t length);
__attribute__((warn_unused_result)) result_t token_copy(token_t* dest, const token_t* src);
__attribute__((warn_unused_result)) result_t token_validate(const token_t* token);
int token_equals(const token_t* token1, const token_t* token2);
int token_equals_str(const token_t* token, const char* str);
int token_is_empty(const token_t* token);
int token_available_space(const token_t* token);

// Additional utility functions
__attribute__((warn_unused_result)) result_t token_find(const token_t* token, const char* needle, size_t* position);
__attribute__((warn_unused_result)) result_t token_substring(const token_t* token, size_t start, size_t length, token_t* dest);
__attribute__((warn_unused_result)) result_t token_trim(token_t* token);

// HTTP utility functions
__attribute__((warn_unused_result)) result_t http_request(token_t* method, token_t* url, token_t* body, token_t* response);
__attribute__((warn_unused_result)) result_t http_get(token_t* url, token_t* response);
__attribute__((warn_unused_result)) result_t http_post(token_t* url, token_t* body, token_t* response);

// JSON utility functions
__attribute__((warn_unused_result)) result_t json_validate(const token_t* json_token);
__attribute__((warn_unused_result)) result_t json_get_string(const token_t* json_token, const char* key_path, token_t* result);
__attribute__((warn_unused_result)) result_t json_get_number(const token_t* json_token, const char* key_path, double* result);
__attribute__((warn_unused_result)) result_t json_create_object(token_t* result, const char* keys[], const char* values[], size_t count);
__attribute__((warn_unused_result)) result_t json_format(const token_t* input, token_t* output);

#endif /* _LKJAGENT_H */
