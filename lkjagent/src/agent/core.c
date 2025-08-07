#include "agent/core.h"

// Main execution function that processes LLM responses and executes agent actions
result_t lkjagent_agent_execute(pool_t* pool, config_t* config, agent_t* agent, const string_t* recv) {
    object_t* response_obj = NULL;
    object_t* agent_response = NULL;
    object_t* action_obj = NULL;

    // Phase 1: Parse the LLM response
    if (agent_actions_parse_response(pool, recv, &response_obj) != RESULT_OK) {
        RETURN_ERR("Failed to parse LLM response");
    }

    // Phase 2: Extract agent response
    if (object_provide_str(pool, &agent_response, response_obj, "agent") != RESULT_OK) {
        if (object_destroy(pool, response_obj) != RESULT_OK) {
            RETURN_ERR("Failed to destroy response object after agent extraction failure");
        }
        RETURN_ERR("Failed to get agent object from LLM response");
    }

    // Phase 3: Check if this is an action (executing state) or state transition
    if (object_provide_str(pool, &action_obj, agent_response, "action") == RESULT_OK) {
        // This is an action - dispatch it
        if (agent_actions_dispatch(pool, agent, action_obj) != RESULT_OK) {
            if (object_destroy(pool, response_obj) != RESULT_OK) {
                RETURN_ERR("Failed to destroy response object after action dispatch failure");
            }
            RETURN_ERR("Failed to execute agent action");
        }

        // After executing any action, automatically transition state
        if (agent_state_auto_transition(pool, config, agent) != RESULT_OK) {
            if (object_destroy(pool, response_obj) != RESULT_OK) {
                RETURN_ERR("Failed to destroy response object after auto transition failure");
            }
            RETURN_ERR("Failed to automatically transition state after action execution");
        }
    } else {
        // This might be a state transition - handle it
        // Check if we're in evaluating state and need special transition logic
        object_t* current_state;
        if (object_provide_str(pool, &current_state, agent->data, "state") == RESULT_OK) {
            if (strncmp(current_state->string->data, "evaluating", current_state->string->size) == 0) {
                // Special handling for evaluating state transitions (memory-aware)
                if (agent_state_handle_evaluation_transition(pool, config, agent, response_obj) != RESULT_OK) {
                    if (object_destroy(pool, response_obj) != RESULT_OK) {
                        RETURN_ERR("Failed to destroy response object after evaluation transition failure");
                    }
                    RETURN_ERR("Failed to handle evaluation state transition");
                }
            } else {
                // Regular state update for thinking and other states
                if (agent_state_update_and_log(pool, agent, response_obj) != RESULT_OK) {
                    if (object_destroy(pool, response_obj) != RESULT_OK) {
                        RETURN_ERR("Failed to destroy response object after state update failure");
                    }
                    RETURN_ERR("Failed to update agent state");
                }
            }
        } else {
            // Fallback to regular state update if we can't determine current state
            if (agent_state_update_and_log(pool, agent, response_obj) != RESULT_OK) {
                if (object_destroy(pool, response_obj) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy response object after state update failure");
                }
                RETURN_ERR("Failed to update agent state");
            }
        }
    }

    // Phase 4: Save the updated agent memory to file
    if (agent_actions_save_memory(pool, agent) != RESULT_OK) {
        if (object_destroy(pool, response_obj) != RESULT_OK) {
            RETURN_ERR("Failed to destroy response object after memory save failure");
        }
        RETURN_ERR("Failed to save agent memory");
    }

    // Phase 5: Clean up
    if (object_destroy(pool, response_obj) != RESULT_OK) {
        RETURN_ERR("Failed to destroy response object");
    }

    return RESULT_OK;
}

// Core agent processing function - orchestrates the full LLM interaction cycle
result_t lkjagent_agent(pool_t* pool, config_t* config, agent_t* agent) {
    string_t* prompt = NULL;
    string_t* response_content = NULL;

    // Phase 1: Generate prompt based on current agent state and configuration
    if (agent_prompt_generate(pool, config, agent, &prompt) != RESULT_OK) {
        RETURN_ERR("Failed to create prompt for agent");
    }

    // Phase 2: Send HTTP request to LLM and receive response
    if (agent_http_send_receive(pool, config, prompt, &response_content) != RESULT_OK) {
        if (string_destroy(pool, prompt) != RESULT_OK) {
            RETURN_ERR("Failed to destroy prompt after HTTP communication failure");
        }
        RETURN_ERR("Failed to communicate with LLM");
    }

    // Debug
    printf("Received response content: %.*s\n", (int)response_content->size, response_content->data);
    fflush(stdout);

    // Phase 3: Execute agent actions based on LLM response
    if (lkjagent_agent_execute(pool, config, agent, response_content) != RESULT_OK) {
        if (string_destroy(pool, prompt) != RESULT_OK || string_destroy(pool, response_content) != RESULT_OK) {
            RETURN_ERR("Failed to destroy resources after agent execution failure");
        }
        RETURN_ERR("Failed to execute agent with received content");
    }

    // Phase 4: Clean up allocated resources
    if (string_destroy(pool, prompt) != RESULT_OK) {
        RETURN_ERR("Failed to destroy prompt");
    }

    if (string_destroy(pool, response_content) != RESULT_OK) {
        RETURN_ERR("Failed to destroy response content");
    }

    return RESULT_OK;
}
