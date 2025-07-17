/**
 * @file memory_manager.c
 * @brief Memory management system for agent RAM and persistent storage
 * 
 * This file contains all memory-related functionality including:
 * - RAM memory initialization and management
 * - Persistent disk storage operations
 * - Memory optimization and paging
 */

#include "../lkjagent.h"

/**
 * @brief Initialize agent memory with static buffers
 * @param memory Pointer to memory structure to initialize
 * @param buffers Array of static character buffers
 * @param num_buffers Number of buffers available
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_memory_init(agent_memory_t* memory, char buffers[][2048], size_t num_buffers) {
    if (!memory) {
        lkj_log_error(__func__, "memory parameter is NULL");
        return RESULT_ERR;
    }
    if (!buffers) {
        lkj_log_error(__func__, "buffers parameter is NULL");
        return RESULT_ERR;
    }
    if (num_buffers < 7) {
        lkj_log_error(__func__, "insufficient buffers (need at least 7)");
        return RESULT_ERR;
    }

    // Initialize each memory component with its own buffer
    if (token_init(&memory->system_prompt, buffers[0], 2048) != RESULT_OK ||
        token_init(&memory->current_state, buffers[1], 2048) != RESULT_OK ||
        token_init(&memory->task_goal, buffers[2], 2048) != RESULT_OK ||
        token_init(&memory->plan, buffers[3], 2048) != RESULT_OK ||
        token_init(&memory->scratchpad, buffers[4], 2048) != RESULT_OK ||
        token_init(&memory->recent_history, buffers[5], 2048) != RESULT_OK ||
        token_init(&memory->retrieved_from_disk, buffers[6], 2048) != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize memory tokens");
        return RESULT_ERR;
    }

    // Set default system prompt
    if (token_set(&memory->system_prompt, 
                  "You are an autonomous AI agent designed to complete tasks through structured reasoning.\n"
                  "You operate in four states: thinking, executing, evaluating, and paging.\n"
                  "Available tools: search, retrieve, write, execute_code, forget.\n"
                  "Always respond with valid JSON containing your next action and state transition.") != RESULT_OK) {
        lkj_log_error(__func__, "failed to set default system prompt");
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * @brief Save agent memory to disk in JSON format
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_memory_save_to_disk(const agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    static char json_buffer[2048];  // Reduced size for simpler JSON
    static char timestamp_buffer[32];
    token_t json_output;
    
    if (token_init(&json_output, json_buffer, sizeof(json_buffer)) != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize JSON output token");
        return RESULT_ERR;
    }

    // Get current timestamp (simplified format)
    time_t now = time(NULL);
    struct tm* tm_info = gmtime(&now);
    strftime(timestamp_buffer, sizeof(timestamp_buffer), "%Y-%m-%d %H:%M:%S", tm_info);

    // Create a very minimal JSON structure using snprintf for better control
    int json_len = snprintf(json_buffer, sizeof(json_buffer),
        "{\n"
        "  \"version\": \"1.0\",\n"
        "  \"timestamp\": \"%s\",\n"
        "  \"state\": \"%s\",\n"
        "  \"iterations\": %d,\n"
        "  \"status\": \"saved\"\n"
        "}",
        timestamp_buffer,
        agent_state_to_string(agent->state),
        agent->iteration_count
    );

    // Check if JSON was truncated
    if (json_len >= (int)sizeof(json_buffer)) {
        lkj_log_error(__func__, "JSON output truncated");
        return RESULT_ERR;
    }

    // Set the token size manually since we used snprintf
    json_output.size = (size_t)json_len;

    // Write to disk
    if (file_write(agent->config.disk_file, &json_output) != RESULT_OK) {
        lkj_log_error(__func__, "failed to write memory to disk");
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * @brief Load agent memory from disk
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_memory_load_from_disk(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    static char json_buffer[8192];
    token_t json_input;
    
    if (token_init(&json_input, json_buffer, sizeof(json_buffer)) != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize JSON input token");
        return RESULT_ERR;
    }

    // Try to read from disk
    if (file_read(agent->config.disk_file, &json_input) != RESULT_OK) {
        // File doesn't exist or can't be read - this is okay for new agents
        return RESULT_OK;
    }

    // Validate JSON
    if (json_validate(&json_input) != RESULT_OK) {
        lkj_log_error(__func__, "invalid JSON in memory file");
        return RESULT_ERR;
    }

    // Extract basic information
    static char string_buffer[512];
    token_t string_result;
    double number_result;

    if (token_init(&string_result, string_buffer, sizeof(string_buffer)) != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize string result token");
        return RESULT_ERR;
    }

    // Load current task if available
    if (json_get_string(&json_input, "working_memory.current_task", &string_result) == RESULT_OK) {
        if (token_copy(&agent->memory.task_goal, &string_result) != RESULT_OK) {
            lkj_log_error(__func__, "failed to load current task");
        }
    }

    // Load iteration count
    if (json_get_number(&json_input, "metadata.iterations", &number_result) == RESULT_OK) {
        agent->iteration_count = (int)number_result;
    }

    return RESULT_OK;
}

/**
 * @brief Clear RAM memory to free up space
 * @param agent Pointer to agent structure
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_memory_clear_ram(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }

    // Clear non-essential memory components
    if (token_clear(&agent->memory.recent_history) != RESULT_OK ||
        token_clear(&agent->memory.retrieved_from_disk) != RESULT_OK) {
        lkj_log_error(__func__, "failed to clear RAM memory");
        return RESULT_ERR;
    }

    // Optionally clear part of scratchpad if it's getting too full
    if (agent->memory.scratchpad.size > (agent->memory.scratchpad.capacity * 3 / 4)) {
        // Keep only the last portion of scratchpad
        size_t keep_size = agent->memory.scratchpad.capacity / 2;
        size_t start_offset = agent->memory.scratchpad.size - keep_size;
        
        memmove(agent->memory.scratchpad.data, 
               agent->memory.scratchpad.data + start_offset, 
               keep_size);
        agent->memory.scratchpad.size = keep_size;
        agent->memory.scratchpad.data[keep_size] = '\0';
    }

    return RESULT_OK;
}
