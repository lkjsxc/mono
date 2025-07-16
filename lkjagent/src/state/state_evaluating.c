/**
 * @file state_evaluating.c
 * @brief Implementation for EVALUATING state operations
 * 
 * This file contains all logic specific to the EVALUATING state,
 * including result analysis, progress assessment, and completion detection.
 */

#include "../lkjagent.h"

/**
 * @brief Initialize the evaluating state
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t state_evaluating_init(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    // Mark the beginning of evaluation phase
    if (token_append(&agent->memory.scratchpad, "=== EVALUATION PHASE ===\n") != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize evaluating state");
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * @brief Execute evaluating state operations
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t state_evaluating_execute(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    printf("  Evaluating results and determining next action...\n");
    
    // Add evaluation to scratchpad
    if (token_append(&agent->memory.scratchpad, "EVALUATING: Assessing gathered data and task progress.\n") != RESULT_OK) {
        lkj_log_error(__func__, "Failed to update scratchpad");
        return RESULT_ERR;
    }

    // In autonomous mode, let AI reflect
    if (agent->loaded_config.agent.autonomous_mode) {
        printf("  AI autonomous evaluation and reflection...\n");
        if (token_append(&agent->memory.scratchpad, "AI_REFLECTION: Analyzing progress and considering new directions.\n") != RESULT_OK) {
            lkj_log_error(__func__, "Failed to update scratchpad");
            return RESULT_ERR;
        }
    }

    // Check if task is complete
    if (agent_is_task_complete(agent)) {
        printf("  Task explicitly marked as complete after %d iterations\n", agent->iteration_count);
        if (token_append(&agent->memory.scratchpad, "TASK_COMPLETE: All objectives successfully achieved.\n") != RESULT_OK) {
            lkj_log_error(__func__, "Failed to log task completion");
        }
        return RESULT_TASK_COMPLETE;
    }

    return RESULT_OK;
}

/**
 * @brief Determine next state after evaluating
 * @param agent Pointer to agent structure
 * @param next_state Pointer to store the next state
 * @return RESULT_OK if transition should occur, RESULT_ERR to stay in evaluating
 */
result_t state_evaluating_next(agent_t* agent, agent_state_t* next_state) {
    if (!agent || !next_state) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    // Check if memory is getting full
    if (agent_should_page(agent)) {
        *next_state = AGENT_STATE_PAGING;
        return RESULT_OK;
    }

    // Check if task is complete
    if (agent_is_task_complete(agent)) {
        // Stay in evaluating state - task is done
        return RESULT_ERR;
    }

    // Continue the thinking cycle - there's always more to explore!
    *next_state = AGENT_STATE_THINKING;
    return RESULT_OK;
}
