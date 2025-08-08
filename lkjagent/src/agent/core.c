#include "agent/core.h"

// Main execution function that processes LLM responses and executes agent actions
result_t lkjagent_agent_execute(pool_t* pool, config_t* config, agent_t* agent, const string_t* recv) {
    object_t* response_obj = NULL;
    object_t* agent_response = NULL;
    object_t* action_obj = NULL;

    if (agent_actions_parse_response(pool, recv, &response_obj) != RESULT_OK) {
        if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
            RETURN_ERR("Failed to reset state after parse failure");
        }
        return RESULT_OK;
    }

    if (object_provide_str(pool, &agent_response, response_obj, "agent") != RESULT_OK) {
        if (object_destroy(pool, response_obj) != RESULT_OK) {
            printf("Warning: Failed to destroy response_obj after agent extraction failure\n");
        }
        if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
            RETURN_ERR("Failed to reset agent state to thinking after extraction failure");
        }
        return RESULT_OK;
    }

    if (object_provide_str(pool, &action_obj, agent_response, "action") == RESULT_OK) {
        if (agent_actions_dispatch(pool, config, agent, action_obj) != RESULT_OK) {
            if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
                if (object_destroy(pool, response_obj) != RESULT_OK) {
                    printf("Warning: Failed to destroy response_obj after action failure\n");
                }
                RETURN_ERR("Failed to reset to thinking after action failure");
            }
        } else {
            if (agent_state_auto_transition(pool, config, agent) != RESULT_OK) {
                if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
                    if (object_destroy(pool, response_obj) != RESULT_OK) {
                        printf("Warning: Failed to destroy response_obj after auto transition failure\n");
                    }
                    RETURN_ERR("Failed to reset to thinking after auto transition failure");
                }
            }
        }
    } else {
        object_t* current_state_obj = NULL;
        if (object_provide_str(pool, &current_state_obj, agent->data, "state") == RESULT_OK &&
            current_state_obj != NULL && current_state_obj->string != NULL) {
            if (string_equal_str(current_state_obj->string, "evaluating")) {
                if (agent_state_handle_evaluation_transition(pool, config, agent, agent_response) != RESULT_OK) {
                    if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
                        if (object_destroy(pool, response_obj) != RESULT_OK) {
                            printf("Warning: Failed to destroy response_obj after evaluation transition failure\n");
                        }
                        RETURN_ERR("Failed to reset to thinking after evaluation transition failure");
                    }
                }
            } else {
                if (agent_state_update_and_log(pool, config, agent, agent_response) != RESULT_OK) {
                    if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
                        if (object_destroy(pool, response_obj) != RESULT_OK) {
                            printf("Warning: Failed to destroy response_obj after state update failure\n");
                        }
                        RETURN_ERR("Failed to reset to thinking after state update failure");
                    }
                }
            }
        } else {
            if (agent_state_update_and_log(pool, config, agent, agent_response) != RESULT_OK) {
                if (agent_state_update_state(pool, agent, "thinking") != RESULT_OK) {
                    if (object_destroy(pool, response_obj) != RESULT_OK) {
                        printf("Warning: Failed to destroy response_obj after state update failure (no state)\n");
                    }
                    RETURN_ERR("Failed to reset to thinking after state update failure");
                }
            }
        }
    }

    if (agent_state_sync_logs_to_working_memory(pool, agent) != RESULT_OK) {
        printf("Warning: Failed to sync logs to working memory\n");
    }
    if (agent_actions_save_memory(pool, agent) != RESULT_OK) {
        printf("Warning: Failed to save memory\n");
    }
    if (object_destroy(pool, response_obj) != RESULT_OK) {
        printf("Warning: Failed to destroy response_obj at end of execute\n");
    }

    return RESULT_OK;
}

// Core agent processing function - orchestrates the full LLM interaction cycle
result_t lkjagent_agent(pool_t* pool, config_t* config, agent_t* agent) {
    string_t* prompt = NULL;
    string_t* response_content = NULL;

    if (agent_prompt_generate(pool, config, agent, &prompt) != RESULT_OK) {
        RETURN_ERR("Failed to create prompt for agent");
    }

    if (agent_http_send_receive(pool, config, prompt, &response_content) != RESULT_OK) {
        if (string_destroy(pool, prompt) != RESULT_OK) {
            RETURN_ERR("Failed to destroy prompt after HTTP communication failure");
        }
        RETURN_ERR("Failed to communicate with LLM");
    }

    if (lkjagent_agent_execute(pool, config, agent, response_content) != RESULT_OK) {
        if (string_destroy(pool, prompt) != RESULT_OK || string_destroy(pool, response_content) != RESULT_OK) {
            RETURN_ERR("Failed to destroy resources after agent execution failure");
        }
        RETURN_ERR("Failed to execute agent with received content");
    }

    if (string_destroy(pool, prompt) != RESULT_OK) {
        RETURN_ERR("Failed to destroy prompt");
    }

    if (string_destroy(pool, response_content) != RESULT_OK) {
        RETURN_ERR("Failed to destroy response content");
    }

    return RESULT_OK;
}
