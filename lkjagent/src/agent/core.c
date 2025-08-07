#include "agent/core.h"

// Main execution function that processes LLM responses and executes agent actions
result_t lkjagent_agent_execute(pool_t* pool, config_t* config, agent_t* agent, const string_t* recv) {
    object_t* response_obj = NULL;
    object_t* agent_response = NULL;
    object_t* action_obj = NULL;

    // Phase 1: Parse the LLM response
    if (agent_actions_parse_response(pool, recv, &response_obj) != RESULT_OK) {
        // If parsing fails completely, force a state reset and continue
        if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
            RETURN_ERR("Failed to reset state after parse failure");
        }
        return RESULT_OK; // Continue execution but skip this cycle
    }

    // Phase 2: Extract agent response
    if (object_provide_str(pool, &agent_response, response_obj, "agent") != RESULT_OK) {
        if (object_destroy(pool, response_obj) != RESULT_OK) {
            RETURN_ERR("Failed to destroy response object after agent extraction failure");
        }
        // If we can't extract agent response, force a state reset to thinking
        if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
            RETURN_ERR("Failed to reset agent state to thinking after extraction failure");
        }
        return RESULT_OK; // Continue execution but skip this cycle
    }

    // Phase 3: Check if this is an action (executing state) or state transition
    if (object_provide_str(pool, &action_obj, agent_response, "action") == RESULT_OK) {
        // This is an action - dispatch it
        if (agent_actions_dispatch(pool, agent, action_obj) != RESULT_OK) {
            // If action fails, reset to thinking state
            if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
                if (object_destroy(pool, response_obj) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy response object after state reset failure");
                }
                RETURN_ERR("Failed to reset to thinking after action failure");
            }
        } else {
            // After executing any action, automatically transition state
            if (agent_state_auto_transition(pool, config, agent) != RESULT_OK) {
                // If auto transition fails, reset to thinking
                if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
                    if (object_destroy(pool, response_obj) != RESULT_OK) {
                        RETURN_ERR("Failed to destroy response object after auto transition failure");
                    }
                    RETURN_ERR("Failed to reset to thinking after auto transition failure");
                }
            }
        }
    } else {
        // This might be a state transition - handle it with unified logic
        // Use simple state update that handles both thinking and evaluating states
        if (agent_state_update_and_log(pool, agent, agent_response) != RESULT_OK) {
            // If state update fails, force reset to thinking
            if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
                if (object_destroy(pool, response_obj) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy response object after state update failure");
                }
                RETURN_ERR("Failed to reset to thinking after state update failure");
            }
        }
    }

    // Phase 4: Synchronize logs to working memory for consistent access
    if (agent_state_sync_logs_to_working_memory(pool, agent) != RESULT_OK) {
        // Don't fail the cycle if sync fails, just continue
    }

    // Phase 5: Save the updated agent memory to file (make this more robust)
    result_t save_result = agent_actions_save_memory(pool, agent);
    (void)save_result; // Don't fail the cycle if save fails

    // Phase 6: Clean up
    if (object_destroy(pool, response_obj) != RESULT_OK) {
        // Don't fail if cleanup fails, just continue
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
