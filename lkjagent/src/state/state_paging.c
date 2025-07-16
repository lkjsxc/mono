/**
 * @file state_paging.c
 * @brief Implementation for PAGING state operations
 * 
 * This file contains all logic specific to the PAGING state,
 * including memory management, disk persistence, and RAM optimization.
 */

#include "../lkjagent.h"

/**
 * @brief Initialize the paging state
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t state_paging_init(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    // Log memory management activity
    if (token_append(&agent->memory.scratchpad, "=== MEMORY PAGING ===\n") != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize paging state");
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * @brief Execute paging state operations
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t state_paging_execute(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    printf("  Managing memory and optimizing storage...\n");
    
    // Save current state to disk
    if (agent_memory_save_to_disk(agent) == RESULT_OK) {
        printf("  Memory successfully paged to disk\n");
    } else {
        printf("  Warning: Memory paging failed\n");
    }
    
    // Clear some working memory to free up space
    if (agent_memory_clear_ram(agent) == RESULT_OK) {
        printf("  RAM cleared for optimization\n");
    }

    // In autonomous mode, log AI memory optimization
    if (agent->loaded_config.agent.autonomous_mode) {
        printf("  AI memory optimization...\n");
        if (agent_memory_save_to_disk(agent) == RESULT_OK) {
            printf("  AI completed memory optimization\n");
        }
    }

    return RESULT_OK;
}

/**
 * @brief Determine next state after paging
 * @param agent Pointer to agent structure
 * @param next_state Pointer to store the next state
 * @return RESULT_OK if transition should occur, RESULT_ERR to stay in paging
 */
result_t state_paging_next(agent_t* agent, agent_state_t* next_state) {
    if (!agent || !next_state) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    // After paging, return to thinking to reassess
    *next_state = AGENT_STATE_THINKING;
    return RESULT_OK;
}
