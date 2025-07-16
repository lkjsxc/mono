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

    static char json_buffer[8192];
    static char timestamp_buffer[64];
    token_t json_output;
    
    if (token_init(&json_output, json_buffer, sizeof(json_buffer)) != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize JSON output token");
        return RESULT_ERR;
    }

    // Get current timestamp
    time_t now = time(NULL);
    struct tm* tm_info = gmtime(&now);
    strftime(timestamp_buffer, sizeof(timestamp_buffer), "%Y-%m-%dT%H:%M:%SZ", tm_info);

    // Build JSON structure
    if (token_set(&json_output, "{\n") != RESULT_OK ||
        token_append(&json_output, "  \"metadata\": {\n") != RESULT_OK ||
        token_append(&json_output, "    \"version\": \"1.0\",\n") != RESULT_OK ||
        token_append(&json_output, "    \"last_modified\": \"") != RESULT_OK ||
        token_append(&json_output, timestamp_buffer) != RESULT_OK ||
        token_append(&json_output, "\",\n") != RESULT_OK ||
        token_append(&json_output, "    \"state\": \"") != RESULT_OK ||
        token_append(&json_output, agent_state_to_string(agent->state)) != RESULT_OK ||
        token_append(&json_output, "\",\n") != RESULT_OK ||
        token_append(&json_output, "    \"iterations\": ") != RESULT_OK) {
        lkj_log_error(__func__, "failed to build JSON metadata");
        return RESULT_ERR;
    }

    // Add iteration count
    char iter_str[32];
    snprintf(iter_str, sizeof(iter_str), "%d", agent->iteration_count);
    if (token_append(&json_output, iter_str) != RESULT_OK ||
        token_append(&json_output, "\n  },\n") != RESULT_OK) {
        lkj_log_error(__func__, "failed to add iteration count");
        return RESULT_ERR;
    }

    // Add working memory
    if (token_append(&json_output, "  \"working_memory\": {\n") != RESULT_OK ||
        token_append(&json_output, "    \"current_task\": \"") != RESULT_OK ||
        token_append(&json_output, agent->memory.task_goal.data) != RESULT_OK ||
        token_append(&json_output, "\",\n") != RESULT_OK ||
        token_append(&json_output, "    \"context\": \"") != RESULT_OK ||
        token_append(&json_output, agent->memory.scratchpad.data) != RESULT_OK ||
        token_append(&json_output, "\",\n") != RESULT_OK ||
        token_append(&json_output, "    \"variables\": \"\"\n") != RESULT_OK ||
        token_append(&json_output, "  },\n") != RESULT_OK) {
        lkj_log_error(__func__, "failed to add working memory");
        return RESULT_ERR;
    }

    // Add knowledge base (simplified)
    if (token_append(&json_output, "  \"knowledge_base\": {\n") != RESULT_OK ||
        token_append(&json_output, "    \"facts\": {}\n") != RESULT_OK ||
        token_append(&json_output, "  },\n") != RESULT_OK) {
        lkj_log_error(__func__, "failed to add knowledge base");
        return RESULT_ERR;
    }

    // Add log entry
    if (token_append(&json_output, "  \"log\": [\n") != RESULT_OK ||
        token_append(&json_output, "    {\n") != RESULT_OK ||
        token_append(&json_output, "      \"timestamp\": \"") != RESULT_OK ||
        token_append(&json_output, timestamp_buffer) != RESULT_OK ||
        token_append(&json_output, "\",\n") != RESULT_OK ||
        token_append(&json_output, "      \"state\": \"") != RESULT_OK ||
        token_append(&json_output, agent_state_to_string(agent->state)) != RESULT_OK ||
        token_append(&json_output, "\",\n") != RESULT_OK ||
        token_append(&json_output, "      \"action\": \"memory_save\",\n") != RESULT_OK ||
        token_append(&json_output, "      \"details\": \"") != RESULT_OK ||
        token_append(&json_output, agent->memory.recent_history.data) != RESULT_OK ||
        token_append(&json_output, "\"\n") != RESULT_OK ||
        token_append(&json_output, "    }\n") != RESULT_OK ||
        token_append(&json_output, "  ]\n") != RESULT_OK ||
        token_append(&json_output, "}") != RESULT_OK) {
        lkj_log_error(__func__, "failed to add log entry");
        return RESULT_ERR;
    }

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
