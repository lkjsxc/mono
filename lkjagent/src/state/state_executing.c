/**
 * @file state_executing.c
 * @brief Implementation for EXECUTING state operations
 * 
 * This file contains all logic specific to the EXECUTING state,
 * including action execution, tool usage, and implementation.
 */

#include "../lkjagent.h"

/**
 * @brief Initialize the executing state
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t state_executing_init(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    // Mark the beginning of execution phase
    if (token_append(&agent->memory.scratchpad, "=== EXECUTION PHASE ===\n") != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize executing state");
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * @brief Execute executing state operations
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t state_executing_execute(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    printf("  Executing planned actions...\n");
    
    // Simulate tool execution and data gathering
    if (token_append(&agent->memory.scratchpad, "EXECUTING: Running system analysis tools.\n") != RESULT_OK) {
        lkj_log_error(__func__, "Failed to update scratchpad");
        return RESULT_ERR;
    }
    
    // Execute some tools to gather information
    static char tool_result_buffer[512];
    token_t tool_result;
    if (token_init(&tool_result, tool_result_buffer, sizeof(tool_result_buffer)) == RESULT_OK) {
        // Search for system information
        if (agent_tool_search(agent, "system status", &tool_result) == RESULT_OK) {
            if (token_append(&agent->memory.scratchpad, "TOOL_RESULT: ") != RESULT_OK ||
                token_append(&agent->memory.scratchpad, tool_result.data) != RESULT_OK ||
                token_append(&agent->memory.scratchpad, "\n") != RESULT_OK) {
                lkj_log_error(__func__, "Failed to log tool results");
            }
        }
    }

    // In autonomous mode, let AI direct execution
    if (agent->loaded_config.agent.autonomous_mode) {
        printf("  AI-directed execution and investigation...\n");
        if (token_append(&agent->memory.scratchpad, "AI_DIRECTED_EXECUTION: Following autonomous research plan.\n") != RESULT_OK) {
            lkj_log_error(__func__, "Failed to update scratchpad");
            return RESULT_ERR;
        }
        
        // Let AI explore through tools
        static char tool_buffer[512];
        token_t ai_tool_result;
        if (token_init(&ai_tool_result, tool_buffer, sizeof(tool_buffer)) == RESULT_OK) {
            char search_query[256];
            snprintf(search_query, sizeof(search_query), "autonomous investigation topic %d", agent->iteration_count);
            
            if (agent_tool_search(agent, search_query, &ai_tool_result) == RESULT_OK) {
                if (token_append(&agent->memory.scratchpad, "AI_TOOL_EXPLORATION: ") != RESULT_OK ||
                    token_append(&agent->memory.scratchpad, ai_tool_result.data) != RESULT_OK ||
                    token_append(&agent->memory.scratchpad, "\n") != RESULT_OK) {
                    lkj_log_error(__func__, "Failed to log tool exploration");
                }
            }
        }
    }

    return RESULT_OK;
}

/**
 * @brief Determine next state after executing
 * @param agent Pointer to agent structure
 * @param next_state Pointer to store the next state
 * @return RESULT_OK if transition should occur, RESULT_ERR to stay in executing
 */
result_t state_executing_next(agent_t* agent, agent_state_t* next_state) {
    if (!agent || !next_state) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    // Check if memory is getting full
    if (agent_should_page(agent)) {
        *next_state = AGENT_STATE_PAGING;
        return RESULT_OK;
    }

    // Normal flow: executing -> evaluating
    *next_state = AGENT_STATE_EVALUATING;
    return RESULT_OK;
}
