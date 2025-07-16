/**
 * @file state_manager.c
 * @brief Central state management and transition coordination
 * 
 * This file contains the core state machine logic, transition validation,
 * and coordination between different state implementations.
 */

#include "../lkjagent.h"

/**
 * @brief Convert agent state enum to string representation
 * @param state The agent state to convert
 * @return String representation of the state
 */
const char* agent_state_to_string(agent_state_t state) {
    switch (state) {
        case AGENT_STATE_THINKING:   return "thinking";
        case AGENT_STATE_EXECUTING:  return "executing";
        case AGENT_STATE_EVALUATING: return "evaluating";
        case AGENT_STATE_PAGING:     return "paging";
        default:                     return "unknown";
    }
}

/**
 * @brief Check if agent should transition to paging state
 * @param agent Pointer to agent structure
 * @return 1 if paging needed, 0 otherwise
 */
int agent_should_page(const agent_t* agent) {
    if (!agent) {
        return 0;
    }
    
    // Check if scratchpad is getting full (over 80% capacity)
    int scratchpad_usage = (int)((agent->memory.scratchpad.size * 100) / agent->memory.scratchpad.capacity);
    
    // Check if recent history is getting full
    int history_usage = (int)((agent->memory.recent_history.size * 100) / agent->memory.recent_history.capacity);
    
    // Trigger paging if any memory area is over 80% full
    return (scratchpad_usage > 80 || history_usage > 80);
}

/**
 * @brief Check if a state transition is valid
 * @param current_state Current agent state
 * @param new_state Proposed new state
 * @return 1 if valid, 0 if invalid
 */
int agent_is_valid_transition(agent_state_t current_state, agent_state_t new_state) {
    // No transition needed if already in target state
    if (current_state == new_state) {
        return 0; // Invalid - no self-transitions allowed
    }
    
    // Define valid state transitions based on agent lifecycle
    switch (current_state) {
        case AGENT_STATE_THINKING:
            // From thinking, can go to executing or paging
            return (new_state == AGENT_STATE_EXECUTING || new_state == AGENT_STATE_PAGING);
            
        case AGENT_STATE_EXECUTING:
            // From executing, can go to evaluating or paging
            return (new_state == AGENT_STATE_EVALUATING || new_state == AGENT_STATE_PAGING);
            
        case AGENT_STATE_EVALUATING:
            // From evaluating, can go to thinking (continue) or paging
            return (new_state == AGENT_STATE_THINKING || new_state == AGENT_STATE_PAGING);
            
        case AGENT_STATE_PAGING:
            // From paging, can return to any operational state
            return (new_state == AGENT_STATE_THINKING || 
                   new_state == AGENT_STATE_EXECUTING || 
                   new_state == AGENT_STATE_EVALUATING);
            
        default:
            return 0; // Unknown state - invalid
    }
}

/**
 * @brief Get descriptive reason for state transition
 * @param current_state Current agent state
 * @param new_state New state to transition to
 * @return String describing the reason for transition
 */
const char* agent_get_transition_reason(agent_state_t current_state, agent_state_t new_state) {
    switch (current_state) {
        case AGENT_STATE_THINKING:
            if (new_state == AGENT_STATE_EXECUTING) {
                return "plan complete, beginning execution";
            } else if (new_state == AGENT_STATE_PAGING) {
                return "memory full during planning";
            }
            break;
            
        case AGENT_STATE_EXECUTING:
            if (new_state == AGENT_STATE_EVALUATING) {
                return "actions completed, evaluating results";
            } else if (new_state == AGENT_STATE_PAGING) {
                return "memory full during execution";
            }
            break;
            
        case AGENT_STATE_EVALUATING:
            if (new_state == AGENT_STATE_THINKING) {
                return "more work needed, replanning";
            } else if (new_state == AGENT_STATE_PAGING) {
                return "memory management required";
            }
            break;
            
        case AGENT_STATE_PAGING:
            if (new_state == AGENT_STATE_THINKING) {
                return "memory optimized, resuming planning";
            } else if (new_state == AGENT_STATE_EXECUTING) {
                return "memory optimized, resuming execution";
            } else if (new_state == AGENT_STATE_EVALUATING) {
                return "memory optimized, resuming evaluation";
            }
            break;
    }
    
    return "state machine transition";
}

/**
 * @brief Initialize state-specific context when entering a new state
 * @param agent Pointer to agent structure
 * @param new_state State being entered
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_initialize_state(agent_t* agent, agent_state_t new_state) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }
    
    // Delegate to state-specific initialization functions
    switch (new_state) {
        case AGENT_STATE_THINKING:
            return state_thinking_init(agent);
            
        case AGENT_STATE_EXECUTING:
            return state_executing_init(agent);
            
        case AGENT_STATE_EVALUATING:
            return state_evaluating_init(agent);
            
        case AGENT_STATE_PAGING:
            return state_paging_init(agent);
            
        default:
            return RESULT_ERR;
    }
}

/**
 * @brief Transition agent to a new state
 * @param agent Pointer to agent structure
 * @param new_state New state to transition to
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_transition_state(agent_t* agent, agent_state_t new_state) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    const char* old_state_str = agent_state_to_string(agent->state);
    const char* new_state_str = agent_state_to_string(new_state);

    // Prevent self-transitions
    if (agent->state == new_state) {
        lkj_log_error(__func__, "attempted self-transition to same state");
        return RESULT_ERR;
    }

    // Validate state transition
    if (!agent_is_valid_transition(agent->state, new_state)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), 
                "invalid state transition: %s -> %s", old_state_str, new_state_str);
        lkj_log_error(__func__, error_msg);
        return RESULT_ERR;
    }

    // Store previous state for potential rollback
    agent_state_t previous_state = agent->state;
    agent->state = new_state;
    
    // Update current state in memory
    if (token_set(&agent->memory.current_state, new_state_str) != RESULT_OK) {
        lkj_log_error(__func__, "failed to update current state in memory");
        // Rollback state change
        agent->state = previous_state;
        return RESULT_ERR;
    }

    // Perform state-specific initialization
    result_t init_result = agent_initialize_state(agent, new_state);
    if (init_result != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize new state");
        // Rollback state change
        agent->state = previous_state;
        if (token_set(&agent->memory.current_state, old_state_str) != RESULT_OK) {
            lkj_log_error(__func__, "failed to rollback state in memory");
        }
        return RESULT_ERR;
    }

    printf("  State transition: %s -> %s (%s)\n", 
           old_state_str, new_state_str, 
           agent_get_transition_reason(previous_state, new_state));
    return RESULT_OK;
}

/**
 * @brief Make an intelligent decision about the next state transition
 * @param agent Pointer to agent structure
 * @param next_state Pointer to store the recommended next state
 * @return RESULT_OK if a transition is recommended, RESULT_ERR if should stay in current state
 */
result_t agent_decide_next_state(agent_t* agent, agent_state_t* next_state) {
    if (!agent || !next_state) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }
    
    // Check for memory pressure first
    if (agent_should_page(agent) && agent->state != AGENT_STATE_PAGING) {
        *next_state = AGENT_STATE_PAGING;
        return RESULT_OK;
    }
    
    // Delegate to state-specific next state logic
    switch (agent->state) {
        case AGENT_STATE_THINKING:
            return state_thinking_next(agent, next_state);
            
        case AGENT_STATE_EXECUTING:
            return state_executing_next(agent, next_state);
            
        case AGENT_STATE_EVALUATING:
            return state_evaluating_next(agent, next_state);
            
        case AGENT_STATE_PAGING:
            return state_paging_next(agent, next_state);
            
        default:
            lkj_log_error(__func__, "unknown current state");
            return RESULT_ERR;
    }
}
