#include "agent.c"
#include "error.c"
#include "file.c"
#include "http.c"
#include "json.c"
#include "lkjagent.h"
#include "token.c"

int main() {
    printf("=== LKJAgent Demo ===\n\n");
    
    // Clear any previous errors
    lkj_clear_last_error();

    // Test 1: Basic HTTP functionality (original)
    printf("Test 1: Basic HTTP functionality\n");
    printf("Attempting to connect to LMStudio...\n");
    
    static char method_data[16];
    static char url_data[256];
    static char body_data[1024];
    static char response_data[4096];

    token_t method;
    token_t url;
    token_t body;
    token_t response;

    // Initialize tokens with their respective buffers
    if (token_init(&method, method_data, sizeof(method_data)) != RESULT_OK ||
        token_init(&url, url_data, sizeof(url_data)) != RESULT_OK ||
        token_init(&body, body_data, sizeof(body_data)) != RESULT_OK ||
        token_init(&response, response_data, sizeof(response_data)) != RESULT_OK) {
        printf("Failed to initialize tokens: %s\n", lkj_get_last_error());
        return 1;
    }

    // Set method and URL for LMStudio
    if (token_set(&method, "POST") != RESULT_OK) {
        printf("Failed to set method: %s\n", lkj_get_last_error());
        return 1;
    }

    if (token_set(&url, "http://host.docker.internal:1234/v1/chat/completions") != RESULT_OK) {
        printf("Failed to set URL: %s\n", lkj_get_last_error());
        return 1;
    }

    if (file_read("./data/system.txt", &body) != RESULT_OK) {
        printf("Failed to read body from file: %s - continuing with agent demo\n", lkj_get_last_error());
        // Continue with agent demo even if file read fails
    } else {
        // Make HTTP request to LMStudio
        if (http_request(&method, &url, &body, &response) != RESULT_OK) {
            printf("HTTP request failed: %s - LMStudio may not be running\n", lkj_get_last_error());
            printf("Continuing with offline agent demo...\n");
        } else {
            printf("LMStudio Response received (%zu bytes):\n", response.size);
            printf("%.*s\n", (int)(response.size > 200 ? 200 : response.size), response.data);
            if (response.size > 200) printf("...(truncated)\n");
        }
    }

    printf("\n=====================================\n");

    // Test 2: Agent functionality
    printf("Test 2: Agent Management System\n");
    
    // Create agent and memory buffers
    agent_t agent;
    static char memory_buffers[7][2048];  // 7 buffers for agent memory
    
    // Initialize agent
    if (agent_init(&agent, NULL) != RESULT_OK) {
        printf("Failed to initialize agent: %s\n", lkj_get_last_error());
        return 1;
    }
    
    // Initialize agent memory
    if (agent_memory_init(&agent.memory, memory_buffers, 7) != RESULT_OK) {
        printf("Failed to initialize agent memory: %s\n", lkj_get_last_error());
        return 1;
    }
    
    printf("Agent initialized successfully\n");
    printf("Initial state: %s\n", agent_state_to_string(agent.state));
    
    // Set a task for the agent
    const char* task = "Analyze the current system and provide a status report";
    if (agent_set_task(&agent, task) != RESULT_OK) {
        printf("Failed to set agent task: %s\n", lkj_get_last_error());
        return 1;
    }
    
    printf("Task set: %s\n", task);
    printf("Agent state after task setting: %s\n", agent_state_to_string(agent.state));
    
    // Demonstrate state transitions
    printf("\nDemonstrating state transitions:\n");
    
    for (int i = 0; i < 3; i++) {
        printf("Iteration %d - Current state: %s\n", i + 1, agent_state_to_string(agent.state));
        
        // Simulate state transitions
        switch (agent.state) {
            case AGENT_STATE_THINKING:
                printf("  Agent is thinking about the task...\n");
                if (agent_transition_state(&agent, AGENT_STATE_EXECUTING) != RESULT_OK) {
                    printf("  Warning: failed to transition to executing state: %s\n", lkj_get_last_error());
                }
                break;
            case AGENT_STATE_EXECUTING:
                printf("  Agent is executing actions...\n");
                if (agent_transition_state(&agent, AGENT_STATE_EVALUATING) != RESULT_OK) {
                    printf("  Warning: failed to transition to evaluating state: %s\n", lkj_get_last_error());
                }
                break;
            case AGENT_STATE_EVALUATING:
                printf("  Agent is evaluating results...\n");
                if (i < 2) {
                    if (agent_transition_state(&agent, AGENT_STATE_THINKING) != RESULT_OK) {
                        printf("  Warning: failed to transition to thinking state: %s\n", lkj_get_last_error());
                    }
                } else {
                    printf("  Task evaluation complete!\n");
                }
                break;
            case AGENT_STATE_PAGING:
                printf("  Agent is managing memory...\n");
                if (agent_transition_state(&agent, AGENT_STATE_THINKING) != RESULT_OK) {
                    printf("  Warning: failed to transition to thinking state: %s\n", lkj_get_last_error());
                }
                break;
        }
    }
    
    printf("\nFinal agent state: %s\n", agent_state_to_string(agent.state));
    printf("Total iterations: %d\n", agent.iteration_count);
    
    // Test tool system
    printf("\nTesting tool system:\n");
    static char tool_result_buffer[1024];
    token_t tool_result;
    if (token_init(&tool_result, tool_result_buffer, sizeof(tool_result_buffer)) != RESULT_OK) {
        printf("Failed to initialize tool result token: %s\n", lkj_get_last_error());
        return 1;
    }
    
    // Test search tool
    if (agent_tool_search(&agent, "system status", &tool_result) == RESULT_OK) {
        printf("Search tool result: %s\n", tool_result.data);
    } else {
        printf("Search tool failed: %s\n", lkj_get_last_error());
    }
    
    // Test retrieve tool
    if (token_clear(&tool_result) != RESULT_OK) {
        printf("Warning: failed to clear tool result token: %s\n", lkj_get_last_error());
    }
    if (agent_tool_retrieve(&agent, "config", &tool_result) == RESULT_OK) {
        printf("Retrieve tool result: %s\n", tool_result.data);
    } else {
        printf("Retrieve tool failed: %s\n", lkj_get_last_error());
    }
    
    // Test memory save
    printf("\nTesting memory persistence:\n");
    if (agent_memory_save_to_disk(&agent) == RESULT_OK) {
        printf("Agent memory saved to disk successfully\n");
    } else {
        printf("Failed to save agent memory to disk\n");
    }
    
    // Test memory load
    printf("\nTesting memory loading:\n");
    if (agent_memory_load_from_disk(&agent) == RESULT_OK) {
        printf("Agent memory loaded from disk successfully\n");
        printf("Retrieved from disk: %s\n", agent.memory.retrieved_from_disk.data);
    } else {
        printf("Failed to load agent memory from disk\n");
    }
    
    printf("\n=== Demo Complete ===\n");
    return 0;
}