/**
 * @file core.c
 * @brief Agent core functionality implementation
 *
 * This module implements the basic agent lifecycle and management functions.
 * It provides initialization and execution stubs that can be expanded as
 * the agent system develops.
 */

#include "../lkjagent.h"

/**
 * @brief Initialize agent structure
 *
 * Sets up the agent with default configuration and initializes
 * all necessary subsystems.
 *
 * @param agent Pointer to agent structure to initialize
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t lkjagent_init(lkjagent_t* agent) {
    
    printf("Initializing LKJAgent...\n");
    
    if (!agent) {
        RETURN_ERR("lkjagent_init: NULL agent parameter");
        return RESULT_ERR;
    }
    
    // Initialize agent structure
    memset(agent, 0, sizeof(lkjagent_t));
    
    // Initialize configuration path token
    static char config_path_buffer[512];
    if (token_init(&agent->config_path, config_path_buffer, sizeof(config_path_buffer)) != RESULT_OK) {
        RETURN_ERR("Failed to initialize config path token");
        return RESULT_ERR;
    }
    
    // Initialize memory path token
    static char memory_path_buffer[512];
    if (token_init(&agent->memory_path, memory_path_buffer, sizeof(memory_path_buffer)) != RESULT_OK) {
        RETURN_ERR("Failed to initialize memory path token");
        return RESULT_ERR;
    }
    
    // Set default paths
    if (token_set(&agent->config_path, "data/config.json") != RESULT_OK) {
        RETURN_ERR("Failed to set default config path");
        return RESULT_ERR;
    }
    
    if (token_set(&agent->memory_path, "data/memory.json") != RESULT_OK) {
        RETURN_ERR("Failed to set default memory path");
        return RESULT_ERR;
    }
    
    // Initialize configuration with defaults
    if (config_init(&agent->config) != RESULT_OK) {
        RETURN_ERR("Failed to initialize configuration with defaults");
        return RESULT_ERR;
    }
    
    // Try to load configuration from file
    if (file_exists(agent->config_path.data)) {
        printf("Loading configuration from: %s\n", agent->config_path.data);
        if (config_load_from_file(&agent->config, agent->config_path.data) != RESULT_OK) {
            printf("Warning: Failed to load configuration file, using defaults\n");
        } else {
            printf("Configuration loaded successfully\n");
        }
    } else {
        printf("Configuration file not found, using defaults\n");
    }
    
    // Validate configuration
    if (config_validate(&agent->config) != RESULT_OK) {
        RETURN_ERR("Configuration validation failed");
        return RESULT_ERR;
    }
    
    // Initialize memory buffers and metadata tokens
    static char memory_buffers[7][2048];
    static char metadata_version_buffer[64];
    static char metadata_created_buffer[64];
    static char metadata_modified_buffer[64];
    
    if (token_init(&agent->metadata.version, metadata_version_buffer, sizeof(metadata_version_buffer)) != RESULT_OK ||
        token_init(&agent->metadata.created, metadata_created_buffer, sizeof(metadata_created_buffer)) != RESULT_OK ||
        token_init(&agent->metadata.last_modified, metadata_modified_buffer, sizeof(metadata_modified_buffer)) != RESULT_OK) {
        RETURN_ERR("Failed to initialize metadata tokens");
        return RESULT_ERR;
    }
    
    // Initialize agent working memory
    if (agent_memory_init(&agent->memory, memory_buffers, 7) != RESULT_OK) {
        RETURN_ERR("Failed to initialize agent memory");
        return RESULT_ERR;
    }
    
    // Load persistent memory from file
    printf("Loading memory from: %s\n", agent->memory_path.data);
    if (agent_memory_load_from_file(agent, agent->memory_path.data) != RESULT_OK) {
        printf("Warning: Failed to load memory file, using defaults\n");
    }
    
    // Validate memory structure
    if (agent_memory_validate(&agent->memory) != RESULT_OK) {
        RETURN_ERR("Memory validation failed");
        return RESULT_ERR;
    }
    
    printf("Agent initialization completed.\n");
    printf("LMStudio URL: %s\n", agent->config.lmstudio.base_url.data);
    printf("Model: %s\n", agent->config.lmstudio.model.data);
    printf("Max iterations: %d\n", agent->config.agent.max_iterations);
    
    return RESULT_OK;
}

/**
 * @brief Run agent main execution loop
 *
 * Executes the agent's main operational cycle. This demonstrates
 * the configuration system and basic API functionality.
 *
 * @param agent Pointer to initialized agent
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t lkjagent_run(lkjagent_t* agent) {
    if (!agent) {
        RETURN_ERR("lkjagent_run: NULL agent parameter");
        return RESULT_ERR;
    }
    
    printf("\n=== LKJAgent Configuration Demo ===\n");
    
    // Display loaded configuration
    printf("Configuration Summary:\n");
    printf("  LMStudio:\n");
    printf("    URL: %s\n", agent->config.lmstudio.base_url.data);
    printf("    Model: %s\n", agent->config.lmstudio.model.data);
    printf("    Temperature: %.1f\n", agent->config.lmstudio.temperature);
    printf("    Max Tokens: %d\n", agent->config.lmstudio.max_tokens);
    printf("    Timeout: %d ms\n", agent->config.lmstudio.timeout_ms);
    
    printf("  Agent:\n");
    printf("    Max Iterations: %d\n", agent->config.agent.max_iterations);
    printf("    Self-directed: %s\n", agent->config.agent.self_directed ? "Yes" : "No");
    printf("    System Prompt: %.50s...\n", agent->config.agent.system_prompt.data);
    
    printf("  Tagged Memory:\n");
    printf("    Max Entries: %d\n", agent->config.agent.tagged_memory.max_entries);
    printf("    Max Tags per Entry: %d\n", agent->config.agent.tagged_memory.max_tags_per_entry);
    printf("    Auto Cleanup Threshold: %.1f\n", agent->config.agent.tagged_memory.auto_cleanup_threshold);
    printf("    Tag Similarity Threshold: %.1f\n", agent->config.agent.tagged_memory.tag_similarity_threshold);
    
    printf("  LLM Decisions:\n");
    printf("    Confidence Threshold: %.1f\n", agent->config.agent.llm_decisions.confidence_threshold);
    printf("    Decision Timeout: %d ms\n", agent->config.agent.llm_decisions.decision_timeout_ms);
    printf("    Fallback Enabled: %s\n", agent->config.agent.llm_decisions.fallback_enabled ? "Yes" : "No");
    printf("    Context Window Size: %d\n", agent->config.agent.llm_decisions.context_window_size);
    
    printf("  Enhanced Tools:\n");
    printf("    Tool Chaining: %s\n", agent->config.agent.enhanced_tools.tool_chaining_enabled ? "Yes" : "No");
    printf("    Max Chain Length: %d\n", agent->config.agent.enhanced_tools.max_tool_chain_length);
    printf("    Parallel Execution: %s\n", agent->config.agent.enhanced_tools.parallel_tool_execution ? "Yes" : "No");
    
    printf("  HTTP:\n");
    printf("    Timeout: %d seconds\n", agent->config.http.timeout_seconds);
    printf("    Max Redirects: %d\n", agent->config.http.max_redirects);
    printf("    User Agent: %s\n", agent->config.http.user_agent.data);
    
    // Test memory system functionality
    printf("\n=== Testing Memory System ===\n");
    
    // Test memory statistics
    size_t total_used, total_capacity;
    double utilization_percent;
    if (agent_memory_get_stats(&agent->memory, &total_used, &total_capacity, &utilization_percent) == RESULT_OK) {
        printf("Memory Statistics:\n");
        printf("  Total Used: %zu bytes\n", total_used);
        printf("  Total Capacity: %zu bytes\n", total_capacity);
        printf("  Utilization: %.1f%%\n", utilization_percent);
    }
    
    // Test memory operations
    printf("\nTesting memory operations...\n");
    
    // Update task goal
    if (agent_memory_update_task_goal(&agent->memory, "Demonstrate memory system functionality") == RESULT_OK) {
        printf("✓ Task goal updated\n");
    }
    
    // Update state
    if (agent_memory_update_state(&agent->memory, "demonstrating") == RESULT_OK) {
        printf("✓ State updated to 'demonstrating'\n");
    }
    
    // Add some scratchpad notes
    if (agent_memory_append_scratchpad(&agent->memory, "Memory system is working correctly", "STATUS") == RESULT_OK &&
        agent_memory_append_scratchpad(&agent->memory, "Configuration loaded successfully", "INFO") == RESULT_OK &&
        agent_memory_append_scratchpad(&agent->memory, "JSON parsing operational", "DEBUG") == RESULT_OK) {
        printf("✓ Scratchpad entries added\n");
    }
    
    // Show current memory state
    printf("\nCurrent Memory State:\n");
    printf("  Task Goal: %s\n", agent->memory.task_goal.data);
    printf("  Current State: %s\n", agent->memory.current_state.data);
    printf("  Recent History: %s", agent->memory.recent_history.data);
    printf("  Scratchpad:\n%s", agent->memory.scratchpad.data);
    
    // Save memory to disk
    printf("\nSaving memory to disk...\n");
    if (agent_memory_update_metadata(&agent->metadata) == RESULT_OK &&
        agent_memory_save_to_file(agent, agent->memory_path.data) == RESULT_OK) {
        printf("✓ Memory saved to: %s\n", agent->memory_path.data);
    } else {
        printf("✗ Failed to save memory\n");
    }
    
    // Update memory stats after operations
    if (agent_memory_get_stats(&agent->memory, &total_used, &total_capacity, &utilization_percent) == RESULT_OK) {
        printf("\nUpdated Memory Statistics:\n");
        printf("  Total Used: %zu bytes\n", total_used);
        printf("  Total Capacity: %zu bytes\n", total_capacity);
        printf("  Utilization: %.1f%%\n", utilization_percent);
    }
    
    // Test configuration serialization
    printf("\n=== Testing Configuration Serialization ===\n");
    static char json_buffer[4096];
    token_t json_token;
    if (token_init(&json_token, json_buffer, sizeof(json_buffer)) != RESULT_OK) {
        RETURN_ERR("Failed to initialize JSON token for serialization test");
        return RESULT_ERR;
    }
    
    if (config_to_json(&agent->config, &json_token) == RESULT_OK) {
        printf("Configuration serialized to JSON:\n%s\n", json_token.data);
    } else {
        printf("Failed to serialize configuration to JSON\n");
    }
    
    printf("=== Agent Demo Complete ===\n");
    
    return RESULT_OK;
}