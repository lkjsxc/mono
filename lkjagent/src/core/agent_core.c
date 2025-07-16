/**
 * @file agent_core.c
 * @brief Core agent management and coordination
 * 
 * This file contains the main agent structure management,
 * initialization, cleanup, and high-level coordination functions.
 */

#include "../lkjagent.h"

/**
 * @brief Create and initialize a new agent
 * @param config_file Path to configuration file
 * @return Pointer to agent or NULL on failure
 */
agent_t* agent_create(const char* config_file) {
    if (!config_file) {
        lkj_log_error(__func__, "config_file is NULL");
        return NULL;
    }

    // Allocate agent structure
    agent_t* agent = malloc(sizeof(agent_t));
    if (!agent) {
        lkj_log_error(__func__, "failed to allocate agent");
        return NULL;
    }

    // Initialize to zero
    memset(agent, 0, sizeof(agent_t));

    // Load configuration
    if (config_load(config_file, &agent->loaded_config) != RESULT_OK) {
        lkj_log_error(__func__, "failed to load configuration");
        free(agent);
        return NULL;
    }

    // Apply configuration to agent
    if (config_apply_to_agent(agent, &agent->loaded_config) != RESULT_OK) {
        lkj_log_error(__func__, "failed to apply configuration");
        free(agent);
        return NULL;
    }

    // Initialize state to thinking
    agent->state = AGENT_STATE_THINKING;
    agent->iteration_count = 0;

    printf("Agent created successfully\n");
    return agent;
}

/**
 * @brief Destroy agent and free resources
 * @param agent Pointer to agent to destroy
 */
void agent_destroy(agent_t* agent) {
    if (!agent) {
        return;
    }

    // Save memory to disk before destroying
    if (agent->memory.scratchpad.data) {
        if (agent_memory_save_to_disk(agent) != RESULT_OK) {
            printf("Warning: Failed to save agent memory before destruction\n");
        }
    }

    free(agent);
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
