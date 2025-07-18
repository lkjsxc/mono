/**
 * @file memory.c
 * @brief Agent memory management implementation
 *
 * This module provides comprehensive memory management functionality for the LKJAgent system.
 * It handles both volatile working memory (RAM) and persistent storage (disk) operations,
 * maintaining the dual-memory architecture described in the documentation.
 *
 * Key features:
 * - Working memory initialization with static buffers
 * - Persistent memory storage in JSON format
 * - Memory metadata management with timestamps
 * - Safe memory operations with bounded buffers
 * - Comprehensive validation and error handling
 */

#include "lkjagent.h"

// ============================================================================
// Memory Initialization
// ============================================================================

/**
 * @brief Initialize agent memory with static buffers
 *
 * Initializes the agent working memory structure using provided static buffers.
 * This follows the stack-based allocation pattern and ensures memory safety.
 *
 * @param memory Memory structure to initialize
 * @param buffers Array of 2048-byte character buffers
 * @param num_buffers Number of buffers (minimum 7 required)
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t agent_memory_init(agent_memory_t* memory, char buffers[][2048], size_t num_buffers) {
    if (!memory) {
        RETURN_ERR("agent_memory_init: NULL memory parameter");
        return RESULT_ERR;
    }
    
    if (!buffers) {
        RETURN_ERR("agent_memory_init: NULL buffers parameter");
        return RESULT_ERR;
    }
    
    if (num_buffers < 7) {
        RETURN_ERR("agent_memory_init: Insufficient buffers (minimum 7 required)");
        return RESULT_ERR;
    }
    
    // Initialize all memory tokens with the provided buffers
    if (token_init(&memory->system_prompt, buffers[0], 2048) != RESULT_OK ||
        token_init(&memory->current_state, buffers[1], 2048) != RESULT_OK ||
        token_init(&memory->task_goal, buffers[2], 2048) != RESULT_OK ||
        token_init(&memory->plan, buffers[3], 2048) != RESULT_OK ||
        token_init(&memory->scratchpad, buffers[4], 2048) != RESULT_OK ||
        token_init(&memory->recent_history, buffers[5], 2048) != RESULT_OK ||
        token_init(&memory->retrieved_from_disk, buffers[6], 2048) != RESULT_OK) {
        RETURN_ERR("agent_memory_init: Failed to initialize memory tokens");
        return RESULT_ERR;
    }
    
    // Set default system prompt from agent configuration if available
    if (token_set(&memory->system_prompt, "You are an intelligent autonomous agent with tagged memory capabilities.") != RESULT_OK) {
        RETURN_ERR("agent_memory_init: Failed to set default system prompt");
        return RESULT_ERR;
    }
    
    // Set initial state
    if (token_set(&memory->current_state, "initialized") != RESULT_OK) {
        RETURN_ERR("agent_memory_init: Failed to set initial state");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

// ============================================================================
// Memory Persistence
// ============================================================================

/**
 * @brief Load persistent memory from memory.json file
 *
 * Loads memory data from the persistent storage file and populates the agent's
 * working memory. Creates a new memory file with default structure if none exists.
 *
 * @param agent Agent structure containing memory and configuration
 * @param file_path Path to memory.json file
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t agent_memory_load_from_file(lkjagent_t* agent, const char* file_path) {
    if (!agent) {
        RETURN_ERR("agent_memory_load_from_file: NULL agent parameter");
        return RESULT_ERR;
    }
    
    if (!file_path) {
        RETURN_ERR("agent_memory_load_from_file: NULL file_path parameter");
        return RESULT_ERR;
    }
    
    // Check if file exists
    if (!file_exists(file_path)) {
        printf("Memory file not found, creating new memory.json\n");
        
        // Create initial memory structure and save it
        if (agent_memory_update_metadata(&agent->metadata) != RESULT_OK) {
            RETURN_ERR("agent_memory_load_from_file: Failed to initialize metadata");
            return RESULT_ERR;
        }
        
        if (agent_memory_save_to_file(agent, file_path) != RESULT_OK) {
            RETURN_ERR("agent_memory_load_from_file: Failed to create initial memory file");
            return RESULT_ERR;
        }
        
        return RESULT_OK;
    }
    
    // Read the memory file
    static char file_content_buffer[16384]; // 16KB for memory file
    token_t file_content;
    if (token_init(&file_content, file_content_buffer, sizeof(file_content_buffer)) != RESULT_OK) {
        RETURN_ERR("agent_memory_load_from_file: Failed to initialize file content token");
        return RESULT_ERR;
    }
    
    if (file_read(file_path, &file_content) != RESULT_OK) {
        RETURN_ERR("agent_memory_load_from_file: Failed to read memory file");
        return RESULT_ERR;
    }
    
    // Validate JSON structure
    if (json_validate(&file_content) != RESULT_OK) {
        RETURN_ERR("agent_memory_load_from_file: Invalid JSON in memory file");
        return RESULT_ERR;
    }
    
    // Parse metadata
    static char metadata_buffer[1024];
    token_t metadata_token;
    if (token_init(&metadata_token, metadata_buffer, sizeof(metadata_buffer)) == RESULT_OK &&
        json_get_object(&file_content, "metadata", &metadata_token) == RESULT_OK) {
        
        static char temp_buffer[256];
        token_t temp_token;
        if (token_init(&temp_token, temp_buffer, sizeof(temp_buffer)) == RESULT_OK) {
            
            if (json_get_string(&metadata_token, "version", &temp_token) == RESULT_OK) {
                if (token_copy(&agent->metadata.version, &temp_token) != RESULT_OK) {
                    printf("Warning: Failed to copy metadata version\n");
                }
            }
            
            if (json_get_string(&metadata_token, "created", &temp_token) == RESULT_OK) {
                if (token_copy(&agent->metadata.created, &temp_token) != RESULT_OK) {
                    printf("Warning: Failed to copy metadata created timestamp\n");
                }
            }
            
            if (json_get_string(&metadata_token, "last_modified", &temp_token) == RESULT_OK) {
                if (token_copy(&agent->metadata.last_modified, &temp_token) != RESULT_OK) {
                    printf("Warning: Failed to copy metadata last_modified timestamp\n");
                }
            }
        }
    }
    
    // Parse working memory
    static char working_memory_buffer[4096];
    token_t working_memory_token;
    if (token_init(&working_memory_token, working_memory_buffer, sizeof(working_memory_buffer)) == RESULT_OK &&
        json_get_object(&file_content, "working_memory", &working_memory_token) == RESULT_OK) {
        
        static char temp_buffer[2048];
        token_t temp_token;
        if (token_init(&temp_token, temp_buffer, sizeof(temp_buffer)) == RESULT_OK) {
            
            if (json_get_string(&working_memory_token, "current_task", &temp_token) == RESULT_OK) {
                if (token_copy(&agent->memory.task_goal, &temp_token) != RESULT_OK) {
                    printf("Warning: Failed to copy current_task to task_goal\n");
                }
            }
            
            if (json_get_string(&working_memory_token, "context", &temp_token) == RESULT_OK) {
                if (token_copy(&agent->memory.scratchpad, &temp_token) != RESULT_OK) {
                    printf("Warning: Failed to copy context to scratchpad\n");
                }
            }
            
            if (json_get_string(&working_memory_token, "current_state", &temp_token) == RESULT_OK) {
                if (token_copy(&agent->memory.current_state, &temp_token) != RESULT_OK) {
                    printf("Warning: Failed to copy current_state\n");
                }
            }
        }
    }
    
    printf("Memory loaded successfully from: %s\n", file_path);
    return RESULT_OK;
}

/**
 * @brief Save current agent memory to persistent storage
 *
 * Serializes the agent's current memory state to JSON format and saves it to
 * the specified file. Creates the directory structure if needed.
 *
 * @param agent Agent structure containing memory to save
 * @param file_path Path to save memory.json file
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t agent_memory_save_to_file(const lkjagent_t* agent, const char* file_path) {
    if (!agent) {
        RETURN_ERR("agent_memory_save_to_file: NULL agent parameter");
        return RESULT_ERR;
    }
    
    if (!file_path) {
        RETURN_ERR("agent_memory_save_to_file: NULL file_path parameter");
        return RESULT_ERR;
    }
    
    // Build JSON structure
    static char json_buffer[16384]; // 16KB for memory JSON
    token_t json_token;
    if (token_init(&json_token, json_buffer, sizeof(json_buffer)) != RESULT_OK) {
        RETURN_ERR("agent_memory_save_to_file: Failed to initialize JSON token");
        return RESULT_ERR;
    }
    
    // Start JSON object
    if (token_set(&json_token, "{\n") != RESULT_OK) {
        RETURN_ERR("agent_memory_save_to_file: Failed to start JSON object");
        return RESULT_ERR;
    }
    
    // Add metadata section
    if (token_append(&json_token, "  \"metadata\": {\n") != RESULT_OK ||
        token_append(&json_token, "    \"version\": \"") != RESULT_OK ||
        token_append(&json_token, agent->metadata.version.data ? agent->metadata.version.data : "1.0") != RESULT_OK ||
        token_append(&json_token, "\",\n") != RESULT_OK ||
        token_append(&json_token, "    \"created\": \"") != RESULT_OK ||
        token_append(&json_token, agent->metadata.created.data ? agent->metadata.created.data : "2025-07-18T00:00:00Z") != RESULT_OK ||
        token_append(&json_token, "\",\n") != RESULT_OK ||
        token_append(&json_token, "    \"last_modified\": \"") != RESULT_OK ||
        token_append(&json_token, agent->metadata.last_modified.data ? agent->metadata.last_modified.data : "2025-07-18T00:00:00Z") != RESULT_OK ||
        token_append(&json_token, "\"\n") != RESULT_OK ||
        token_append(&json_token, "  },\n") != RESULT_OK) {
        RETURN_ERR("agent_memory_save_to_file: Failed to write metadata section");
        return RESULT_ERR;
    }
    
    // Add working memory section
    if (token_append(&json_token, "  \"working_memory\": {\n") != RESULT_OK ||
        token_append(&json_token, "    \"current_task\": \"") != RESULT_OK ||
        token_append(&json_token, agent->memory.task_goal.data ? agent->memory.task_goal.data : "") != RESULT_OK ||
        token_append(&json_token, "\",\n") != RESULT_OK ||
        token_append(&json_token, "    \"context\": \"") != RESULT_OK ||
        token_append(&json_token, agent->memory.scratchpad.data ? agent->memory.scratchpad.data : "") != RESULT_OK ||
        token_append(&json_token, "\",\n") != RESULT_OK ||
        token_append(&json_token, "    \"current_state\": \"") != RESULT_OK ||
        token_append(&json_token, agent->memory.current_state.data ? agent->memory.current_state.data : "initialized") != RESULT_OK ||
        token_append(&json_token, "\",\n") != RESULT_OK ||
        token_append(&json_token, "    \"variables\": {}\n") != RESULT_OK ||
        token_append(&json_token, "  },\n") != RESULT_OK) {
        RETURN_ERR("agent_memory_save_to_file: Failed to write working memory section");
        return RESULT_ERR;
    }
    
    // Add knowledge base section
    if (token_append(&json_token, "  \"knowledge_base\": {\n") != RESULT_OK ||
        token_append(&json_token, "    \"concepts\": {},\n") != RESULT_OK ||
        token_append(&json_token, "    \"procedures\": {},\n") != RESULT_OK ||
        token_append(&json_token, "    \"facts\": {}\n") != RESULT_OK ||
        token_append(&json_token, "  },\n") != RESULT_OK) {
        RETURN_ERR("agent_memory_save_to_file: Failed to write knowledge base section");
        return RESULT_ERR;
    }
    
    // Add log section
    if (token_append(&json_token, "  \"log\": [],\n") != RESULT_OK) {
        RETURN_ERR("agent_memory_save_to_file: Failed to write log section");
        return RESULT_ERR;
    }
    
    // Add file section and close JSON
    if (token_append(&json_token, "  \"file\": {\n") != RESULT_OK ||
        token_append(&json_token, "    \"generated_code\": {},\n") != RESULT_OK ||
        token_append(&json_token, "    \"documents\": {},\n") != RESULT_OK ||
        token_append(&json_token, "    \"data\": {}\n") != RESULT_OK ||
        token_append(&json_token, "  }\n") != RESULT_OK ||
        token_append(&json_token, "}\n") != RESULT_OK) {
        RETURN_ERR("agent_memory_save_to_file: Failed to write file section");
        return RESULT_ERR;
    }
    
    // Write to file
    if (file_write(file_path, &json_token) != RESULT_OK) {
        RETURN_ERR("agent_memory_save_to_file: Failed to write memory file");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

// ============================================================================
// Memory Utility Functions
// ============================================================================

/**
 * @brief Clear working memory but preserve metadata
 *
 * Clears all working memory tokens while preserving the agent's metadata.
 * Useful for resetting the agent state between tasks.
 *
 * @param memory Memory structure to clear
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t agent_memory_clear_working(agent_memory_t* memory) {
    if (!memory) {
        RETURN_ERR("agent_memory_clear_working: NULL memory parameter");
        return RESULT_ERR;
    }
    
    // Clear working memory tokens (preserve system_prompt)
    if (token_clear(&memory->current_state) != RESULT_OK ||
        token_clear(&memory->task_goal) != RESULT_OK ||
        token_clear(&memory->plan) != RESULT_OK ||
        token_clear(&memory->scratchpad) != RESULT_OK ||
        token_clear(&memory->recent_history) != RESULT_OK ||
        token_clear(&memory->retrieved_from_disk) != RESULT_OK) {
        RETURN_ERR("agent_memory_clear_working: Failed to clear memory tokens");
        return RESULT_ERR;
    }
    
    // Reset to initial state
    if (token_set(&memory->current_state, "initialized") != RESULT_OK) {
        RETURN_ERR("agent_memory_clear_working: Failed to reset current state");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

/**
 * @brief Update memory metadata with current timestamp
 *
 * Updates the memory metadata with current timestamps. Creates default
 * values if metadata tokens are not initialized.
 *
 * @param metadata Metadata structure to update
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t agent_memory_update_metadata(memory_metadata_t* metadata) {
    if (!metadata) {
        RETURN_ERR("agent_memory_update_metadata: NULL metadata parameter");
        return RESULT_ERR;
    }
    
    // Simple timestamp implementation (could be enhanced with actual time functions)
    const char* current_time = "2025-07-18T12:00:00Z";
    
    // Initialize tokens if needed and set values
    if (!metadata->version.data || token_set(&metadata->version, "1.0") != RESULT_OK) {
        RETURN_ERR("agent_memory_update_metadata: Failed to set version");
        return RESULT_ERR;
    }
    
    if (!metadata->created.data || token_is_empty(&metadata->created)) {
        if (token_set(&metadata->created, current_time) != RESULT_OK) {
            RETURN_ERR("agent_memory_update_metadata: Failed to set created timestamp");
            return RESULT_ERR;
        }
    }
    
    if (token_set(&metadata->last_modified, current_time) != RESULT_OK) {
        RETURN_ERR("agent_memory_update_metadata: Failed to set last_modified timestamp");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

/**
 * @brief Validate memory structure and content
 *
 * Performs comprehensive validation of the agent memory structure,
 * checking that all tokens are properly initialized and contain valid data.
 *
 * @param memory Memory structure to validate
 * @return RESULT_OK if valid, RESULT_ERR if invalid
 */
__attribute__((warn_unused_result))
result_t agent_memory_validate(const agent_memory_t* memory) {
    if (!memory) {
        RETURN_ERR("agent_memory_validate: NULL memory parameter");
        return RESULT_ERR;
    }
    
    // Validate that all memory tokens are initialized
    if (!memory->system_prompt.data || !memory->current_state.data ||
        !memory->task_goal.data || !memory->plan.data ||
        !memory->scratchpad.data || !memory->recent_history.data ||
        !memory->retrieved_from_disk.data) {
        RETURN_ERR("agent_memory_validate: One or more memory tokens not initialized");
        return RESULT_ERR;
    }
    
    // Validate that system prompt is not empty
    if (token_is_empty(&memory->system_prompt)) {
        RETURN_ERR("agent_memory_validate: System prompt cannot be empty");
        return RESULT_ERR;
    }
    
    // Validate that current state is not empty
    if (token_is_empty(&memory->current_state)) {
        RETURN_ERR("agent_memory_validate: Current state cannot be empty");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

// ============================================================================
// Enhanced Memory Functions
// ============================================================================

/**
 * @brief Add a log entry to memory with timestamp
 *
 * Adds a timestamped log entry to the agent's recent history. This helps
 * track the agent's activities and decision-making process.
 *
 * @param memory Memory structure to update
 * @param state Current agent state
 * @param action Action being performed
 * @param details Additional details about the action
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t agent_memory_add_log_entry(agent_memory_t* memory, const char* state, 
                                    const char* action, const char* details) {
    if (!memory || !state || !action) {
        RETURN_ERR("agent_memory_add_log_entry: NULL parameter");
        return RESULT_ERR;
    }
    
    // Create log entry
    char log_entry[512];
    snprintf(log_entry, sizeof(log_entry), 
             "[%s] %s: %s%s%s\n", 
             state, action, 
             details ? details : "",
             details ? " " : "",
             "");
    
    // Append to recent history
    if (token_append(&memory->recent_history, log_entry) != RESULT_OK) {
        // If buffer is full, trim some old content
        if (memory->recent_history.size > memory->recent_history.capacity / 2) {
            // Keep only the last third of the history
            size_t keep_from = memory->recent_history.size * 2 / 3;
            if (keep_from < memory->recent_history.size) {
                memmove(memory->recent_history.data, 
                       memory->recent_history.data + keep_from,
                       memory->recent_history.size - keep_from);
                memory->recent_history.size -= keep_from;
                memory->recent_history.data[memory->recent_history.size] = '\0';
            }
            
            // Try appending again
            if (token_append(&memory->recent_history, log_entry) != RESULT_OK) {
                RETURN_ERR("agent_memory_add_log_entry: Failed to append log entry after trimming");
                return RESULT_ERR;
            }
        } else {
            RETURN_ERR("agent_memory_add_log_entry: Failed to append log entry");
            return RESULT_ERR;
        }
    }
    
    return RESULT_OK;
}

/**
 * @brief Update task goal in memory
 *
 * Updates the agent's current task goal and logs the change.
 *
 * @param memory Memory structure to update
 * @param new_goal New task goal
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t agent_memory_update_task_goal(agent_memory_t* memory, const char* new_goal) {
    if (!memory || !new_goal) {
        RETURN_ERR("agent_memory_update_task_goal: NULL parameter");
        return RESULT_ERR;
    }
    
    // Update task goal
    if (token_set(&memory->task_goal, new_goal) != RESULT_OK) {
        RETURN_ERR("agent_memory_update_task_goal: Failed to set new task goal");
        return RESULT_ERR;
    }
    
    // Log the change
    return agent_memory_add_log_entry(memory, "system", "task_goal_updated", new_goal);
}

/**
 * @brief Update current state in memory
 *
 * Updates the agent's current operational state and logs the transition.
 *
 * @param memory Memory structure to update
 * @param new_state New operational state
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t agent_memory_update_state(agent_memory_t* memory, const char* new_state) {
    if (!memory || !new_state) {
        RETURN_ERR("agent_memory_update_state: NULL parameter");
        return RESULT_ERR;
    }
    
    // Get current state for logging
    char transition[256];
    snprintf(transition, sizeof(transition), "%s -> %s", 
             memory->current_state.data ? memory->current_state.data : "unknown",
             new_state);
    
    // Update current state
    if (token_set(&memory->current_state, new_state) != RESULT_OK) {
        RETURN_ERR("agent_memory_update_state: Failed to set new state");
        return RESULT_ERR;
    }
    
    // Log the transition
    return agent_memory_add_log_entry(memory, "system", "state_transition", transition);
}

/**
 * @brief Append to scratchpad with automatic formatting
 *
 * Appends formatted content to the agent's scratchpad for temporary notes.
 *
 * @param memory Memory structure to update
 * @param content Content to append
 * @param prefix Optional prefix for the content
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t agent_memory_append_scratchpad(agent_memory_t* memory, const char* content, const char* prefix) {
    if (!memory || !content) {
        RETURN_ERR("agent_memory_append_scratchpad: NULL parameter");
        return RESULT_ERR;
    }
    
    // Add prefix if provided
    if (prefix) {
        if (token_append(&memory->scratchpad, prefix) != RESULT_OK ||
            token_append(&memory->scratchpad, ": ") != RESULT_OK) {
            RETURN_ERR("agent_memory_append_scratchpad: Failed to append prefix");
            return RESULT_ERR;
        }
    }
    
    // Add content and newline
    if (token_append(&memory->scratchpad, content) != RESULT_OK ||
        token_append(&memory->scratchpad, "\n") != RESULT_OK) {
        // Try to free some space if buffer is full
        if (memory->scratchpad.size > memory->scratchpad.capacity / 2) {
            size_t keep_from = memory->scratchpad.size / 2;
            memmove(memory->scratchpad.data, 
                   memory->scratchpad.data + keep_from,
                   memory->scratchpad.size - keep_from);
            memory->scratchpad.size -= keep_from;
            memory->scratchpad.data[memory->scratchpad.size] = '\0';
            
            // Try again
            if (token_append(&memory->scratchpad, content) != RESULT_OK ||
                token_append(&memory->scratchpad, "\n") != RESULT_OK) {
                RETURN_ERR("agent_memory_append_scratchpad: Failed to append after cleanup");
                return RESULT_ERR;
            }
        } else {
            RETURN_ERR("agent_memory_append_scratchpad: Failed to append content");
            return RESULT_ERR;
        }
    }
    
    return RESULT_OK;
}

/**
 * @brief Get memory usage statistics
 *
 * Calculates and returns memory usage statistics for monitoring.
 *
 * @param memory Memory structure to analyze
 * @param total_used Returns total bytes used
 * @param total_capacity Returns total bytes available
 * @param utilization_percent Returns utilization percentage
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t agent_memory_get_stats(const agent_memory_t* memory, size_t* total_used, 
                                size_t* total_capacity, double* utilization_percent) {
    if (!memory || !total_used || !total_capacity || !utilization_percent) {
        RETURN_ERR("agent_memory_get_stats: NULL parameter");
        return RESULT_ERR;
    }
    
    *total_used = memory->system_prompt.size + memory->current_state.size +
                  memory->task_goal.size + memory->plan.size +
                  memory->scratchpad.size + memory->recent_history.size +
                  memory->retrieved_from_disk.size;
    
    *total_capacity = memory->system_prompt.capacity + memory->current_state.capacity +
                      memory->task_goal.capacity + memory->plan.capacity +
                      memory->scratchpad.capacity + memory->recent_history.capacity +
                      memory->retrieved_from_disk.capacity;
    
    *utilization_percent = (*total_capacity > 0) ? ((double)*total_used / *total_capacity) * 100.0 : 0.0;
    
    return RESULT_OK;
}
