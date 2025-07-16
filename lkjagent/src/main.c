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

    // Create a proper LMStudio API request body with config.json data
    agent_t test_agent;
    if (agent_init(&test_agent, "./data/config.json") != RESULT_OK) {
        printf("Failed to load config: %s - using defaults\n", lkj_get_last_error());
        // Create a simple test request body
        if (token_set(&body, "{\"model\":\"qwen/qwen3-8b\",\"messages\":[{\"role\":\"user\",\"content\":\"Hello, how are you?\"}],\"temperature\":0.7,\"max_tokens\":-1,\"stream\":false}") != RESULT_OK) {
            printf("Failed to set default body: %s\n", lkj_get_last_error());
            return 1;
        }
    } else {
        // Create request body using loaded configuration
        char body_json[1024];
        snprintf(body_json, sizeof(body_json),
            "{"
            "\"model\":\"%s\","
            "\"messages\":["
                "{\"role\":\"%s\",\"content\":\"%s\"},"
                "{\"role\":\"user\",\"content\":\"Hello, how are you today?\"}"
            "],"
            "\"temperature\":%.2f,"
            "\"max_tokens\":%d,"
            "\"stream\":%s"
            "}",
            test_agent.loaded_config.lmstudio.model,
            test_agent.loaded_config.system_prompt.role,
            test_agent.loaded_config.system_prompt.content,
            test_agent.loaded_config.lmstudio.temperature,
            test_agent.loaded_config.lmstudio.max_tokens,
            test_agent.loaded_config.lmstudio.stream ? "true" : "false"
        );
        
        if (token_set(&body, body_json) != RESULT_OK) {
            printf("Failed to set config-based body: %s\n", lkj_get_last_error());
            return 1;
        }
        
        printf("Using configuration from config.json:\n");
        printf("  Model: %s\n", test_agent.loaded_config.lmstudio.model);
        printf("  Endpoint: %s\n", test_agent.loaded_config.lmstudio.endpoint);
        printf("  Temperature: %.2f\n", test_agent.loaded_config.lmstudio.temperature);
        
        // Update URL to use config endpoint
        if (token_set(&url, test_agent.loaded_config.lmstudio.endpoint) != RESULT_OK) {
            printf("Failed to set config URL: %s\n", lkj_get_last_error());
            return 1;
        }
    }

    // Make HTTP request to LMStudio
    if (http_request(&method, &url, &body, &response) != RESULT_OK) {
        printf("HTTP request failed: %s - LMStudio may not be running\n", lkj_get_last_error());
        printf("Continuing with offline agent demo...\n");
    } else {
        printf("LMStudio Response received (%zu bytes):\n", response.size);
        printf("%.*s\n", (int)(response.size > 200 ? 200 : response.size), response.data);
        if (response.size > 200) printf("...(truncated)\n");
    }

    printf("\n=====================================\n");

    // Test 2: Agent functionality
    printf("Test 2: Agent Management System\n");
    
    // Create agent and memory buffers
    agent_t agent;
    static char memory_buffers[7][2048];  // 7 buffers for agent memory
    
    // Initialize agent
    if (agent_init(&agent, "./data/config.json") != RESULT_OK) {
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
    
    // Check if autonomous mode is enabled in config
    if (agent.loaded_config.agent.autonomous_mode && agent.loaded_config.agent.continuous_thinking) {
        printf("🤖 Autonomous mode enabled - AI will decide its own tasks!\n");
        printf("Starting autonomous continuous thinking mode...\n\n");
        
        // Run in fully autonomous mode
        if (agent_run_autonomous(&agent) == RESULT_OK) {
            printf("Autonomous session completed successfully\n");
        } else {
            printf("Autonomous session encountered issues\n");
        }
    } else {
        // Set a specific task for the agent
        const char* task = "Analyze the current system and provide a status report";
        if (agent_set_task(&agent, task) != RESULT_OK) {
            printf("Failed to set agent task: %s\n", lkj_get_last_error());
            return 1;
        }
        
        printf("Task set: %s\n", task);
        printf("Agent state after task setting: %s\n", agent_state_to_string(agent.state));
        
        // Demonstrate autonomous agent execution
        printf("\nDemonstrating autonomous agent execution:\n");
        
        // Run the agent autonomously - it will handle its own state transitions
        if (agent_run(&agent) == RESULT_OK) {
            printf("Agent completed task autonomously\n");
        } else {
            printf("Agent encountered issues during execution\n");
        }
    }
    
    // If agent_run didn't work, demonstrate manual state transitions
    if (agent.iteration_count == 0) {
        printf("\nFalling back to manual state transition demonstration:\n");
        
        for (int i = 0; i < 100; i++) {
            printf("Step %d - Current state: %s\n", i + 1, agent_state_to_string(agent.state));
            
            // Execute a single step
            if (agent_step(&agent) == RESULT_OK) {
                printf("  -> Transitioned to: %s\n", agent_state_to_string(agent.state));
                
                // Break if we've completed the task
                if (agent.state == AGENT_STATE_EVALUATING && i >= 2) {
                    printf("  Task evaluation indicates completion!\n");
                    break;
                }
            } else {
                printf("  Step failed: %s\n", lkj_get_last_error());
                break;
            }
            
            // Small delay for readability
            // usleep(500000); // 500ms
        }
    }
    
    printf("\nFinal agent state: %s\n", agent_state_to_string(agent.state));
    printf("Total iterations: %d\n", agent.iteration_count);
    
    printf("\n=====================================\n");
    printf("Test 3: Intelligent State Transition System\n");
    
    // Reset agent for intelligent state transition demo
    agent_t intelligent_agent;
    static char intelligent_memory_buffers[7][2048];
    
    if (agent_init(&intelligent_agent, "./data/config.json") != RESULT_OK) {
        printf("Failed to initialize intelligent agent: %s\n", lkj_get_last_error());
        return 1;
    }
    
    if (agent_memory_init(&intelligent_agent.memory, intelligent_memory_buffers, 7) != RESULT_OK) {
        printf("Failed to initialize intelligent agent memory: %s\n", lkj_get_last_error());
        return 1;
    }
    
    if (agent_set_task(&intelligent_agent, "Perform comprehensive system analysis with intelligent decision making") != RESULT_OK) {
        printf("Failed to set intelligent agent task: %s\n", lkj_get_last_error());
        return 1;
    }
    
    printf("Intelligent Agent initialized\n");
    printf("Task: Perform comprehensive system analysis with intelligent decision making\n");
    printf("Starting intelligent state transition demonstration...\n\n");
    
    // Run intelligent agent with enhanced decision making
    result_t intelligent_result = RESULT_OK;
    int max_intelligent_steps = 6;
    
    for (int step = 0; step < max_intelligent_steps && intelligent_result == RESULT_OK; step++) {
        intelligent_result = agent_step_intelligent(&intelligent_agent);
        
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
    printf("  Final state: %s\n", agent_state_to_string(intelligent_agent.state));
    printf("  Iterations completed: %d\n", intelligent_agent.iteration_count);
    printf("  Task completion status: %s\n", 
           intelligent_result == RESULT_TASK_COMPLETE ? "COMPLETED" : "IN PROGRESS");
    
    // Save intelligent agent results
    if (agent_memory_save_to_disk(&intelligent_agent) == RESULT_OK) {
        printf("  Intelligent agent state saved to disk\n");
    }
    
    printf("\n=====================================\n");
    
    printf("Test 4: AI-Driven Autonomous Thinking (No Stop Condition)\n");
    
    // Create AI-driven agent that thinks autonomously
    agent_t ai_agent;
    static char ai_memory_buffers[7][2048];
    
    if (agent_init(&ai_agent, "./data/config.json") != RESULT_OK) {
        printf("Failed to initialize AI-driven agent: %s\n", lkj_get_last_error());
        return 1;
    }
    
    if (agent_memory_init(&ai_agent.memory, ai_memory_buffers, 7) != RESULT_OK) {
        printf("Failed to initialize AI-driven agent memory: %s\n", lkj_get_last_error());
        return 1;
    }
    
    if (agent_set_task(&ai_agent, "Think freely and explore whatever interests you") != RESULT_OK) {
        printf("Failed to set AI-driven agent task: %s\n", lkj_get_last_error());
        return 1;
    }
    
    printf("AI-Driven Agent initialized\n");
    printf("Task: Think freely and explore whatever interests you\n");
    printf("Note: This AI will decide what to process and when to stop\n");
    printf("Starting AI-driven autonomous thinking...\n\n");
    
    // Let the AI think autonomously - it will decide when to stop
    result_t ai_result = RESULT_OK;
    int max_ai_steps = 10; // Safety limit, but AI can choose to stop earlier
    
    for (int step = 0; step < max_ai_steps && ai_result == RESULT_OK; step++) {
        ai_result = agent_step_ai_driven(&ai_agent);
        
        if (ai_result == RESULT_TASK_COMPLETE) {
            printf("\n🤖 AI chose to conclude its autonomous thinking session!\n");
            break;
        } else if (ai_result == RESULT_ERR) {
            printf("\n❌ AI-driven agent encountered an error\n");
            break;
        }
        
        // Longer pause to show AI thinking process
        usleep(500000); // 500ms
        
        // Show progress
        printf("  -> AI continues autonomous exploration...\n");
    }
    
    if (ai_result == RESULT_OK && ai_agent.iteration_count >= max_ai_steps) {
        printf("\n⏱️  AI reached safety iteration limit (%d steps) but could continue thinking\n", max_ai_steps);
        printf("     (In unlimited mode, AI would continue indefinitely until it chooses to stop)\n");
    }
    
    printf("\nAI-Driven Agent Results:\n");
    printf("  Final state: %s\n", agent_state_to_string(ai_agent.state));
    printf("  Autonomous iterations: %d\n", ai_agent.iteration_count);
    printf("  AI decision status: %s\n", 
           ai_result == RESULT_TASK_COMPLETE ? "AI CHOSE TO STOP" : 
           ai_result == RESULT_ERR ? "ERROR OCCURRED" : "COULD CONTINUE THINKING");
    
    // Save AI-driven results
    if (agent_memory_save_to_disk(&ai_agent) == RESULT_OK) {
        printf("  AI autonomous thinking session saved to disk\n");
    }
    
    printf("\n=====================================\n");
    
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