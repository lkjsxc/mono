/**
 * @file state_thinking.c
 * @brief Implementation for THINKING state operations
 * 
 * This file contains all logic specific to the THINKING state,
 * including planning, analysis, and decision making.
 */

#include "../lkjagent.h"

/**
 * @brief Initialize the thinking state
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t state_thinking_init(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    // Clear any previous planning notes and start fresh
    if (token_append(&agent->memory.scratchpad, "=== THINKING PHASE ===\n") != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize thinking state");
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * @brief Execute thinking state operations
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t state_thinking_execute(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    printf("  Analyzing task and formulating plan...\n");
    
    // Add task analysis to scratchpad
    if (token_append(&agent->memory.scratchpad, "THINKING: Analyzing system requirements.\n") != RESULT_OK) {
        lkj_log_error(__func__, "Failed to update scratchpad");
        return RESULT_ERR;
    }

    // In autonomous mode, let AI decide what to think about
    if (agent->loaded_config.agent.autonomous_mode) {
        printf("  AI deep thinking and exploration...\n");
        if (token_append(&agent->memory.scratchpad, "DEEP_AI_THINKING: Exploring new dimensions and possibilities.\n") != RESULT_OK) {
            lkj_log_error(__func__, "Failed to update scratchpad");
            return RESULT_ERR;
        }
    }

    return RESULT_OK;
}

/**
 * @brief Determine next state after thinking
 * @param agent Pointer to agent structure
 * @param next_state Pointer to store the next state
 * @return RESULT_OK if transition should occur, RESULT_ERR to stay in thinking
 */
result_t state_thinking_next(agent_t* agent, agent_state_t* next_state) {
    if (!agent || !next_state) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    // Check if memory is getting full
    if (agent_should_page(agent)) {
        *next_state = AGENT_STATE_PAGING;
        return RESULT_OK;
    }

    // Normal flow: thinking -> executing
    *next_state = AGENT_STATE_EXECUTING;
    return RESULT_OK;
}
