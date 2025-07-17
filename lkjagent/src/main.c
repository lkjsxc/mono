/**
 * @file main.c
 * @brief Main entry point for the LKJAgent demonstration
 * 
 * This file contains the main function that demonstrates the functionality
 * of the agent system with proper separation of concerns.
 */

#include "lkjagent.h"

/**
 * @brief Test 1: Agent initialization and basic state management
 * @param agent Pointer to store the initialized agent
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t test_agent_initialization(agent_t* agent) {
    printf("Test 1: Agent Initialization and State Management\n");
    
    // Initialize agent using stack allocation
    if (agent_init(agent, "./data/config.json") != RESULT_OK) {
        printf("Failed to initialize agent: %s\n", lkj_get_last_error());
        return RESULT_ERR;
    }

    // Initialize agent memory with proper buffers
    static char memory_buffers[7][2048];
    if (agent_memory_init(&agent->memory, memory_buffers, 7) != RESULT_OK) {
        printf("Failed to initialize agent memory: %s\n", lkj_get_last_error());
        agent_cleanup(agent);
        return RESULT_ERR;
    }
    
    printf("Agent initialized successfully\n");
    printf("Initial state: %s\n", agent_state_to_string(agent->state));
    
    // Set a task for the agent
    const char* task = "Analyze the current system and provide a status report";
    if (agent_set_task(agent, task) != RESULT_OK) {
        printf("Failed to set agent task: %s\n", lkj_get_last_error());
        agent_cleanup(agent);
        return RESULT_ERR;
    }
    
    printf("Task set: %s\n", task);
    printf("Agent state after task setting: %s\n", agent_state_to_string(agent->state));
    
    return RESULT_OK;
}

/**
 * @brief Test 2: Demonstrate state transitions
 * @param agent Pointer to agent
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t test_state_transitions(agent_t* agent) {
    printf("\n=====================================\n");
    printf("Test 2: State Transition Demonstration\n");
    
    // Demonstrate state transitions
    for (int i = 0; i < 5; i++) {
        printf("Step %d - Current state: %s\n", i + 1, agent_state_to_string(agent->state));
        
        // Execute a single step
        if (agent_step(agent) == RESULT_OK) {
            printf("  -> Transitioned to: %s\n", agent_state_to_string(agent->state));
            
            // Break if we've completed the task
            if (agent->state == AGENT_STATE_EVALUATING && i >= 2) {
                printf("  Task evaluation indicates completion!\n");
                break;
            }
        } else {
            printf("  Step failed: %s\n", lkj_get_last_error());
            break;
        }
        
        // Small delay for readability
        usleep(500000); // 500ms
    }
    
    printf("\nFinal agent state: %s\n", agent_state_to_string(agent->state));
    printf("Total iterations: %d\n", agent->iteration_count);
    
    return RESULT_OK;
}

/**
 * @brief Test 3: Intelligent state transition system
 * @param intelligent_agent Pointer to store the initialized intelligent agent
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t test_intelligent_transitions(agent_t* intelligent_agent) {
    printf("\n=====================================\n");
    printf("Test 3: Intelligent State Transition System\n");
    
    // Initialize a new agent for intelligent transitions
    if (agent_init(intelligent_agent, "./data/config.json") != RESULT_OK) {
        printf("Failed to initialize intelligent agent: %s\n", lkj_get_last_error());
        return RESULT_ERR;
    }
    
    static char intelligent_memory_buffers[7][2048];
    if (agent_memory_init(&intelligent_agent->memory, intelligent_memory_buffers, 7) != RESULT_OK) {
        printf("Failed to initialize intelligent agent memory: %s\n", lkj_get_last_error());
        agent_cleanup(intelligent_agent);
        return RESULT_ERR;
    }
    
    if (agent_set_task(intelligent_agent, "Perform comprehensive system analysis with intelligent decision making") != RESULT_OK) {
        printf("Failed to set intelligent agent task: %s\n", lkj_get_last_error());
        agent_cleanup(intelligent_agent);
        return RESULT_ERR;
    }
    
    printf("Intelligent Agent initialized\n");
    printf("Task: Perform comprehensive system analysis with intelligent decision making\n");
    printf("Starting intelligent state transition demonstration...\n\n");
    
    // Run intelligent agent with enhanced decision making
    result_t intelligent_result = RESULT_OK;
    int max_intelligent_steps = 6;
    
    for (int step = 0; step < max_intelligent_steps && intelligent_result == RESULT_OK; step++) {
        intelligent_result = agent_step_intelligent(intelligent_agent);
        
        if (intelligent_result == RESULT_TASK_COMPLETE) {
            printf("\n✓ Intelligent agent successfully completed the task!\n");
            break;
        } else if (intelligent_result == RESULT_ERR) {
            printf("\n✗ Intelligent agent encountered an error\n");
            break;
        }
        
        // Brief pause for readability
        usleep(300000); // 300ms
    }
    
    printf("\nIntelligent Agent Results:\n");
    printf("  Final state: %s\n", agent_state_to_string(intelligent_agent->state));
    printf("  Iterations completed: %d\n", intelligent_agent->iteration_count);
    printf("  Task completion status: %s\n", 
           intelligent_result == RESULT_TASK_COMPLETE ? "COMPLETED" : "IN PROGRESS");
    
    // Save intelligent agent results
    if (agent_memory_save_to_disk(intelligent_agent) == RESULT_OK) {
        printf("  Intelligent agent state saved to disk\n");
    }
    
    return RESULT_OK;
}

/**
 * @brief Test 4: Memory management and persistence
 * @param agent Pointer to agent
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t test_memory_management(agent_t* agent) {
    printf("\n=====================================\n");
    printf("Test 4: Memory Management and Persistence\n");
    
    // Test memory operations
    printf("Testing memory operations...\n");
    if (agent_memory_save_to_disk(agent) == RESULT_OK) {
        printf("  ✓ Agent memory saved to disk\n");
    } else {
        printf("  ✗ Failed to save agent memory\n");
    }
    
    if (agent_memory_load_from_disk(agent) == RESULT_OK) {
        printf("  ✓ Agent memory loaded from disk\n");
    } else {
        printf("  ✗ Failed to load agent memory\n");
    }
    
    return RESULT_OK;
}

/**
 * @brief Test 5: Tool system demonstration
 * @param agent Pointer to agent
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t test_tool_system(agent_t* agent) {
    printf("\n=====================================\n");
    printf("Test 5: Tool System Demonstration\n");
    
    // Test tool execution
    static char tool_result_buffer[1024];
    token_t tool_result;
    if (token_init(&tool_result, tool_result_buffer, sizeof(tool_result_buffer)) == RESULT_OK) {
        printf("Testing agent tools...\n");
        
        if (agent_tool_search(agent, "system information", &tool_result) == RESULT_OK) {
            printf("  ✓ Search tool executed: %s\n", tool_result.data);
        } else {
            printf("  ✗ Search tool failed\n");
        }
        
        if (agent_tool_write(agent, "test_key", "test_value", "demo") == RESULT_OK) {
            printf("  ✓ Write tool executed\n");
        } else {
            printf("  ✗ Write tool failed\n");
        }
        
        if (agent_tool_retrieve(agent, "test_key", &tool_result) == RESULT_OK) {
            printf("  ✓ Retrieve tool executed: %s\n", tool_result.data);
        } else {
            printf("  ✗ Retrieve tool failed\n");
        }
    }
    
    return RESULT_OK;
}

int main() {
    printf("=== LKJAgent Demo - Organized Architecture ===\n\n");
    
    // Clear any previous errors
    lkj_clear_last_error();

    agent_t agent;
    agent_t intelligent_agent;
    result_t test_result = RESULT_OK;
    
    // Run Test 1: Agent Initialization
    test_result = test_agent_initialization(&agent);
    if (test_result != RESULT_OK) {
        return 1;
    }
    
    // Run Test 2: State Transitions
    test_result = test_state_transitions(&agent);
    if (test_result != RESULT_OK) {
        agent_cleanup(&agent);
        return 1;
    }
    
    // Run Test 3: Intelligent Transitions
    test_result = test_intelligent_transitions(&intelligent_agent);
    if (test_result != RESULT_OK) {
        agent_cleanup(&agent);
        return 1;
    }
    
    // Run Test 4: Memory Management
    test_result = test_memory_management(&agent);
    if (test_result != RESULT_OK) {
        agent_cleanup(&agent);
        agent_cleanup(&intelligent_agent);
        return 1;
    }
    
    // Run Test 5: Tool System
    test_result = test_tool_system(&agent);
    if (test_result != RESULT_OK) {
        agent_cleanup(&agent);
        agent_cleanup(&intelligent_agent);
        return 1;
    }
    
    printf("\n=====================================\n");
    printf("Demo completed successfully!\n");
    printf("Architecture separation working properly.\n");
    
    // Clean up
    agent_cleanup(&agent);
    agent_cleanup(&intelligent_agent);
    
    return 0;
}
