/**
 * @file agent_runner.c
 * @brief High-level agent execution and coordination
 * 
 * This file contains the main agent execution loops and coordination logic
 * that ties together all the state modules and components.
 */

#include "../lkjagent.h"

/**
 * @brief Execute a single step of agent operation
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_TASK_COMPLETE when done, RESULT_ERR on failure
 */
result_t agent_step(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    // Check iteration limit
    if (agent->iteration_count >= agent->config.max_iterations) {
        printf("Agent reached maximum iterations (%d)\n", agent->config.max_iterations);
        return RESULT_ERR;
    }

    agent->iteration_count++;
    printf("Agent Step %d (State: %s)\n", agent->iteration_count, agent_state_to_string(agent->state));
    
    // Check if memory paging is needed before any state transition
    if (agent_should_page(agent) && agent->state != AGENT_STATE_PAGING) {
        printf("  Memory usage high, transitioning to paging state\n");
        if (agent_transition_state(agent, AGENT_STATE_PAGING) != RESULT_OK) {
            lkj_log_error(__func__, "Failed to transition to paging state");
            return RESULT_ERR;
        }
        return RESULT_OK; // Let paging happen in next step
    }

    // Execute state-specific operations
    result_t state_result = RESULT_OK;
    switch (agent->state) {
        case AGENT_STATE_THINKING:
            state_result = state_thinking_execute(agent);
            break;
        case AGENT_STATE_EXECUTING:
            state_result = state_executing_execute(agent);
            break;
        case AGENT_STATE_EVALUATING:
            state_result = state_evaluating_execute(agent);
            break;
        case AGENT_STATE_PAGING:
            state_result = state_paging_execute(agent);
            break;
        default:
            lkj_log_error(__func__, "unknown agent state");
            return RESULT_ERR;
    }

    // Check if task completed during state execution
    if (state_result == RESULT_TASK_COMPLETE) {
        return RESULT_TASK_COMPLETE;
    } else if (state_result != RESULT_OK) {
        return RESULT_ERR;
    }

    // Decide on next state transition
    agent_state_t next_state;
    if (agent_decide_next_state(agent, &next_state) == RESULT_OK) {
        if (agent_transition_state(agent, next_state) != RESULT_OK) {
            lkj_log_error(__func__, "Failed state transition");
            return RESULT_ERR;
        }
    }

    return RESULT_OK;
}

/**
 * @brief Enhanced agent step with intelligent state transition decisions
 * @param agent Pointer to agent structure
 * @return RESULT_OK to continue, RESULT_TASK_COMPLETE when done, RESULT_ERR on failure
 */
result_t agent_step_intelligent(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    // Check iteration limit
    if (agent->iteration_count >= agent->config.max_iterations) {
        printf("Agent reached maximum iterations (%d)\n", agent->config.max_iterations);
        return RESULT_ERR;
    }

    agent->iteration_count++;
    printf("Intelligent Step %d (State: %s)\n", agent->iteration_count, agent_state_to_string(agent->state));
    
    // Check for task completion first
    if (agent_is_task_complete(agent)) {
        printf("  Task analysis complete - all objectives achieved\n");
        if (token_append(&agent->memory.scratchpad, "TASK_COMPLETE: All objectives successfully achieved.\n") != RESULT_OK) {
            lkj_log_error(__func__, "Failed to log task completion");
        }
        return RESULT_TASK_COMPLETE;
    }
    
    // Execute the current state with enhanced intelligence
    result_t execution_result = agent_step(agent);
    
    // Use intelligent decision making for next state
    agent_state_t next_state;
    if (agent_decide_next_state(agent, &next_state) == RESULT_OK) {
        if (agent_transition_state(agent, next_state) != RESULT_OK) {
            lkj_log_error(__func__, "Failed intelligent state transition");
            return RESULT_ERR;
        }
    } else {
        // No transition recommended, task might be complete or needs to stay in current state
        if (agent->state == AGENT_STATE_EVALUATING && agent_is_task_complete(agent)) {
            return RESULT_TASK_COMPLETE;
        }
    }
    
    return execution_result;
}

/**
 * @brief AI-driven agent step where AI decides what to process
 * @param agent Pointer to agent structure
 * @return RESULT_OK to continue, RESULT_TASK_COMPLETE when AI decides to stop, RESULT_ERR on failure
 */
result_t agent_step_ai_driven(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    // No iteration limit - let the AI think as long as it wants!
    agent->iteration_count++;
    
    printf("AI-Driven Step %d (State: %s)\n", agent->iteration_count, agent_state_to_string(agent->state));
    
    // Let the AI decide what it wants to work on next
    static char ai_decision_buffer[1024];
    token_t ai_decision;
    if (token_init(&ai_decision, ai_decision_buffer, sizeof(ai_decision_buffer)) != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize AI decision token");
        return RESULT_ERR;
    }
    
    // Get AI's decision about what to process next
    if (agent_ai_decide_next_action(agent, &ai_decision) == RESULT_OK) {
        printf("  AI Decision: %s\n", ai_decision.data);
        
        // Log the AI's autonomous decision
        if (token_append(&agent->memory.scratchpad, "AI_AUTONOMOUS_DECISION: ") != RESULT_OK ||
            token_append(&agent->memory.scratchpad, ai_decision.data) != RESULT_OK ||
            token_append(&agent->memory.scratchpad, "\n") != RESULT_OK) {
            lkj_log_error(__func__, "Failed to log AI decision");
        }
        
        // Check if AI wants to stop thinking
        if (strstr(ai_decision.data, "stop") != NULL || 
            strstr(ai_decision.data, "complete") != NULL ||
            strstr(ai_decision.data, "finished") != NULL ||
            strstr(ai_decision.data, "done") != NULL) {
            printf("  AI has decided to conclude its work\n");
            if (token_append(&agent->memory.scratchpad, "AI_CHOSE_TO_STOP_THINKING\n") != RESULT_OK) {
                lkj_log_error(__func__, "Failed to log AI completion decision");
            }
            return RESULT_TASK_COMPLETE;
        }
    }

    // Execute the current state
    result_t state_result = agent_step(agent);
    
    // Use AI-driven state transitions
    agent_state_t next_state;
    if (agent_decide_next_state(agent, &next_state) == RESULT_OK) {
        if (agent_transition_state(agent, next_state) != RESULT_OK) {
            lkj_log_error(__func__, "Failed AI-driven state transition");
            return RESULT_ERR;
        }
    }
    
    return state_result;
}

/**
 * @brief Run agent until task completion or max iterations
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_run(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    printf("Starting autonomous agent execution...\n");
    printf("Task: %s\n", agent->memory.task_goal.data);
    printf("Initial state: %s\n", agent_state_to_string(agent->state));

    result_t step_result = RESULT_OK;
    int unlimited = (agent->config.max_iterations == -1);
    
    while ((unlimited || agent->iteration_count < agent->config.max_iterations) && step_result == RESULT_OK) {
        step_result = agent_step(agent);
        
        if (step_result == RESULT_TASK_COMPLETE) {
            printf("✅ Task completed successfully after %d iterations\n", agent->iteration_count);
            break;
        } else if (step_result == RESULT_ERR) {
            printf("❌ Agent encountered an error during execution\n");
            break;
        }
        
        // Brief pause between steps for readability
        usleep(200000); // 200ms
    }
    
    if (step_result == RESULT_OK && agent->iteration_count >= agent->config.max_iterations) {
        printf("⏱️  Agent reached maximum iterations (%d) without completion\n", agent->config.max_iterations);
    }
    
    // Save final state
    if (agent_memory_save_to_disk(agent) == RESULT_OK) {
        printf("💾 Final agent state saved to disk\n");
    }
    
    return step_result;
}

/**
 * @brief Run agent in fully autonomous mode where AI decides everything
 * @param agent Pointer to agent structure
 * @return RESULT_OK on completion, RESULT_ERR on error
 */
result_t agent_run_autonomous(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    printf("🤖 Starting fully autonomous AI agent...\n");
    printf("The AI will decide its own tasks and when to stop\n\n");
    
    static char current_task_buffer[512];
    token_t current_task;
    if (token_init(&current_task, current_task_buffer, sizeof(current_task_buffer)) != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize current task token");
        return RESULT_ERR;
    }

    // Set initial autonomous task
    if (token_set(&current_task, "Explore and analyze whatever seems interesting and valuable") != RESULT_OK ||
        agent_set_task(agent, current_task.data) != RESULT_OK) {
        lkj_log_error(__func__, "failed to set initial autonomous task");
        return RESULT_ERR;
    }

    int max_tasks = 5; // Safety limit for autonomous task switching
    
    for (int task_num = 0; task_num < max_tasks; task_num++) {
        printf("🎯 Autonomous Task %d: %s\n", task_num + 1, current_task.data);
        
        result_t step_result = RESULT_OK;
        int task_iterations = 0;
        
        // Let AI work on current task
        while (step_result == RESULT_OK && task_iterations < 20) {
            step_result = agent_step_ai_driven(agent);
            task_iterations++;
            
            if (step_result == RESULT_TASK_COMPLETE) {
                printf("✅ AI completed task after %d iterations\n", task_iterations);
                break;
            } else if (step_result == RESULT_ERR) {
                printf("❌ Error in AI task execution\n");
                return RESULT_ERR;
            }
            
            // Brief pause between steps
            usleep(100000); // 100ms
        }
        
        // Save progress
        if (agent_memory_save_to_disk(agent) == RESULT_OK) {
            printf("💾 Progress saved to disk\n");
        }
        
        // Check if AI wants to continue with a new task
        printf("🤔 AI deciding on next autonomous task...\n");
        
        // In a real implementation, this would ask the AI what to do next
        // For now, we'll simulate the AI choosing to stop after exploring
        if (task_num >= 2) {
            printf("🎯 AI has decided to conclude autonomous session\n");
            break;
        }
        
        // Generate next autonomous task
        char next_task[256];
        snprintf(next_task, sizeof(next_task), 
                "Continue autonomous exploration #%d: investigate new patterns", task_num + 2);
        
        if (token_set(&current_task, next_task) != RESULT_OK ||
            agent_set_task(agent, current_task.data) != RESULT_OK) {
            printf("Failed to set new autonomous task, stopping\n");
            break;
        }
    }
    
    printf("\n🏁 Autonomous session completed\n");
    printf("Total autonomous iterations: %d\n", agent->iteration_count);
    
    return RESULT_OK;
}
