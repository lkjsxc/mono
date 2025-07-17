/**
 * @file agent_core.c
 * @brief Core agent management and coordination
 * 
 * This file contains the main agent structure management,
 * initialization, cleanup, and high-level coordination functions.
 */

#include "../lkjagent.h"

/**
 * @brief Initialize an agent structure with configuration
 * @param agent Pointer to pre-allocated agent structure
 * @param config_file Path to configuration file
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_init(agent_t* agent, const char* config_file) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }
    if (!config_file) {
        lkj_log_error(__func__, "config_file is NULL");
        return RESULT_ERR;
    }

    // Initialize to zero
    memset(agent, 0, sizeof(agent_t));

    // Load configuration
    if (config_load(config_file, &agent->loaded_config) != RESULT_OK) {
        lkj_log_error(__func__, "failed to load configuration");
        return RESULT_ERR;
    }

    // Apply configuration to agent
    if (config_apply_to_agent(agent, &agent->loaded_config) != RESULT_OK) {
        lkj_log_error(__func__, "failed to apply configuration");
        return RESULT_ERR;
    }

    // Initialize state to thinking
    agent->state = AGENT_STATE_THINKING;
    agent->iteration_count = 0;

    printf("Agent initialized successfully\n");
    return RESULT_OK;
}

/**
 * @brief Create and initialize a new agent (deprecated - use agent_init instead)
 * @param config_file Path to configuration file
 * @return Pointer to agent or NULL on failure
 */
agent_t* agent_create(const char* config_file) {
    if (!config_file) {
        lkj_log_error(__func__, "config_file is NULL");
        return NULL;
    }

    // This function is now deprecated - users should use stack allocation with agent_init
    lkj_log_error(__func__, "agent_create is deprecated - use stack allocation with agent_init instead");
    return NULL;
}

/**
 * @brief Clean up agent resources
 * @param agent Pointer to agent to clean up
 */
void agent_cleanup(agent_t* agent) {
    if (!agent) {
        return;
    }

    // Only save memory if both agent and memory are properly initialized
    if (agent->memory.scratchpad.data && 
        agent->config.disk_file[0] != '\0' && 
        agent->memory.scratchpad.size > 0) {
        // Attempt to save, but continue cleanup even if save fails
        result_t save_result = agent_memory_save_to_disk(agent);
        if (save_result != RESULT_OK) {
            // Silently continue - cleanup should not fail due to save errors
        }
    }

    printf("Agent cleaned up\n");
}

/**
 * @brief Destroy agent and free resources (deprecated - use agent_cleanup instead)
 * @param agent Pointer to agent to destroy
 */
void agent_destroy(agent_t* agent) {
    if (!agent) {
        return;
    }

    // This function is now deprecated since we no longer use malloc
    // Just perform cleanup without freeing memory
    agent_cleanup(agent);
    printf("Agent destroyed\n");
}

/**
 * @brief Set a task for the agent
 * @param agent Pointer to agent structure
 * @param task Task description string
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_set_task(agent_t* agent, const char* task) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }
    if (!task) {
        lkj_log_error(__func__, "task parameter is NULL");
        return RESULT_ERR;
    }

    if (token_set(&agent->memory.task_goal, task) != RESULT_OK) {
        lkj_log_error(__func__, "failed to set task goal in memory");
        return RESULT_ERR;
    }

    // Reset agent state when new task is set
    agent->state = AGENT_STATE_THINKING;
    agent->iteration_count = 0;
    
    // Clear previous plan and scratchpad
    if (token_clear(&agent->memory.plan) != RESULT_OK) {
        lkj_log_error(__func__, "failed to clear previous plan");
        return RESULT_ERR;
    }
    if (token_clear(&agent->memory.scratchpad) != RESULT_OK) {
        lkj_log_error(__func__, "failed to clear scratchpad");
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * @brief Check if the agent's current task is complete
 * @param agent Pointer to agent structure
 * @return 1 if task is complete, 0 if more work needed
 */
int agent_is_task_complete(const agent_t* agent) {
    if (!agent) {
        return 0;
    }
    
    // Check for explicit completion markers in scratchpad
    int has_completion_marker = 0;
    if (agent->memory.scratchpad.data) {
        has_completion_marker = (strstr(agent->memory.scratchpad.data, "TASK_EXPLICITLY_COMPLETE") != NULL ||
                               strstr(agent->memory.scratchpad.data, "FINAL_CONCLUSION_REACHED") != NULL);
    }
    
    // Only complete if we have explicit markers AND have done significant work
    if (agent->iteration_count >= 10 && 
        agent->state == AGENT_STATE_EVALUATING &&
        has_completion_marker &&
        agent->memory.scratchpad.size > 500) { // Substantial evidence of work
        return 1;
    }
    
    return 0;
}
