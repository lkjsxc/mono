/**
 * @file agent_tools.c
 * @brief Agent tool system implementation
 * 
 * This file contains implementations for all agent tools:
 * - search: Query disk storage for relevant information
 * - retrieve: Read specific data from persistent storage
 * - write: Save information to disk with optional tags
 * - execute_code: Run code snippets and capture results
 * - forget: Delete unnecessary information for memory optimization
 */

#include "../lkjagent.h"

/**
 * @brief Execute a tool with given parameters
 * @param agent Pointer to agent structure
 * @param tool Tool type to execute
 * @param args Arguments for the tool
 * @param result Pointer to token to store the result
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_execute_tool(agent_t* agent, tool_type_t tool, const char* args, token_t* result) {
    if (!agent || !args || !result) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    switch (tool) {
        case TOOL_SEARCH:
            return agent_tool_search(agent, args, result);
        case TOOL_RETRIEVE:
            return agent_tool_retrieve(agent, args, result);
        case TOOL_WRITE:
            // Parse args for key, value, and tags
            // This is a simplified implementation
            return agent_tool_write(agent, args, args, "");
        case TOOL_EXECUTE_CODE:
            return agent_tool_execute_code(agent, args, result);
        case TOOL_FORGET:
            return agent_tool_forget(agent, args);
        default:
            lkj_log_error(__func__, "unknown tool type");
            return RESULT_ERR;
    }
}

/**
 * @brief Search tool - queries disk storage for relevant information
 * @param agent Pointer to agent structure
 * @param query Search query string
 * @param result Pointer to token to store search results
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_tool_search(agent_t* agent, const char* query, token_t* result) {
    if (!agent || !query || !result) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    // Clear result token
    if (token_clear(result) != RESULT_OK) {
        lkj_log_error(__func__, "failed to clear result token");
        return RESULT_ERR;
    }

    // Simulate search operation by looking in memory first
    if (strstr(agent->memory.scratchpad.data, query) != NULL) {
        if (token_set(result, "Found relevant information in working memory: ") != RESULT_OK ||
            token_append(result, query) != RESULT_OK) {
            lkj_log_error(__func__, "failed to set search result");
            return RESULT_ERR;
        }
    } else {
        // Simulate disk search
        if (token_set(result, "Searched disk storage for '") != RESULT_OK ||
            token_append(result, query) != RESULT_OK ||
            token_append(result, "' - found related system information") != RESULT_OK) {
            lkj_log_error(__func__, "failed to set search result");
            return RESULT_ERR;
        }
    }

    return RESULT_OK;
}

/**
 * @brief Retrieve tool - reads specific data from persistent storage
 * @param agent Pointer to agent structure
 * @param key Key to retrieve
 * @param result Pointer to token to store retrieved data
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_tool_retrieve(agent_t* agent, const char* key, token_t* result) {
    if (!agent || !key || !result) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    // Clear result token
    if (token_clear(result) != RESULT_OK) {
        lkj_log_error(__func__, "failed to clear result token");
        return RESULT_ERR;
    }

    // Try to load from disk file
    static char file_buffer[2048];
    token_t file_content;
    
    if (token_init(&file_content, file_buffer, sizeof(file_buffer)) == RESULT_OK &&
        file_read(agent->config.disk_file, &file_content) == RESULT_OK) {
        
        // Search for the key in the JSON structure
        static char search_path[256];
        snprintf(search_path, sizeof(search_path), "working_memory.%s", key);
        
        static char value_buffer[512];
        token_t value;
        
        if (token_init(&value, value_buffer, sizeof(value_buffer)) == RESULT_OK &&
            json_get_string(&file_content, search_path, &value) == RESULT_OK) {
            if (token_copy(result, &value) != RESULT_OK) {
                lkj_log_error(__func__, "failed to copy retrieved value");
                return RESULT_ERR;
            }
        } else {
            // Key not found
            if (token_set(result, "Key '") != RESULT_OK ||
                token_append(result, key) != RESULT_OK ||
                token_append(result, "' not found in storage") != RESULT_OK) {
                lkj_log_error(__func__, "failed to set not found message");
                return RESULT_ERR;
            }
        }
    } else {
        // File doesn't exist or can't be read
        if (token_set(result, "Storage file not accessible") != RESULT_OK) {
            lkj_log_error(__func__, "failed to set error message");
            return RESULT_ERR;
        }
    }

    return RESULT_OK;
}

/**
 * @brief Write tool - saves information to disk with optional tags
 * @param agent Pointer to agent structure
 * @param key Key to store data under
 * @param value Value to store
 * @param tags Optional tags for categorization
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_tool_write(agent_t* agent, const char* key, const char* value, const char* tags) {
    if (!agent || !key || !value) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    // For now, just add to scratchpad as a record
    if (token_append(&agent->memory.scratchpad, "WRITE_OPERATION: ") != RESULT_OK ||
        token_append(&agent->memory.scratchpad, key) != RESULT_OK ||
        token_append(&agent->memory.scratchpad, " = ") != RESULT_OK ||
        token_append(&agent->memory.scratchpad, value) != RESULT_OK) {
        lkj_log_error(__func__, "failed to record write operation");
        return RESULT_ERR;
    }

    if (tags && strlen(tags) > 0) {
        if (token_append(&agent->memory.scratchpad, " [tags: ") != RESULT_OK ||
            token_append(&agent->memory.scratchpad, tags) != RESULT_OK ||
            token_append(&agent->memory.scratchpad, "]") != RESULT_OK) {
            lkj_log_error(__func__, "failed to record tags");
            return RESULT_ERR;
        }
    }

    if (token_append(&agent->memory.scratchpad, "\n") != RESULT_OK) {
        lkj_log_error(__func__, "failed to add newline");
        return RESULT_ERR;
    }

    // Save to disk immediately
    return agent_memory_save_to_disk(agent);
}

/**
 * @brief Execute code tool - runs code snippets and captures results
 * @param agent Pointer to agent structure
 * @param code Code to execute
 * @param result Pointer to token to store execution results
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_tool_execute_code(agent_t* agent, const char* code, token_t* result) {
    if (!agent || !code || !result) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    // Clear result token
    if (token_clear(result) != RESULT_OK) {
        lkj_log_error(__func__, "failed to clear result token");
        return RESULT_ERR;
    }

    // For security reasons, we simulate code execution rather than actually running arbitrary code
    if (token_set(result, "Simulated execution of code: ") != RESULT_OK ||
        token_append(result, code) != RESULT_OK ||
        token_append(result, " -> Success (simulated)") != RESULT_OK) {
        lkj_log_error(__func__, "failed to set execution result");
        return RESULT_ERR;
    }

    // Record the execution in scratchpad
    if (token_append(&agent->memory.scratchpad, "CODE_EXECUTION: ") != RESULT_OK ||
        token_append(&agent->memory.scratchpad, result->data) != RESULT_OK ||
        token_append(&agent->memory.scratchpad, "\n") != RESULT_OK) {
        lkj_log_error(__func__, "failed to record code execution");
        return RESULT_ERR;
    }

    return RESULT_OK;
}

/**
 * @brief Forget tool - deletes unnecessary information for memory optimization
 * @param agent Pointer to agent structure
 * @param key Key or pattern to forget
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_tool_forget(agent_t* agent, const char* key) {
    if (!agent || !key) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    // Simple implementation: remove lines containing the key from scratchpad
    static char new_scratchpad[2048];
    token_t new_content;
    
    if (token_init(&new_content, new_scratchpad, sizeof(new_scratchpad)) != RESULT_OK) {
        lkj_log_error(__func__, "failed to initialize new content token");
        return RESULT_ERR;
    }

    // Parse scratchpad line by line and exclude lines containing the key
    char* line_start = agent->memory.scratchpad.data;
    char* line_end;
    
    while ((line_end = strchr(line_start, '\n')) != NULL) {
        // Check if this line contains the key to forget
        *line_end = '\0'; // Temporarily null-terminate
        
        if (strstr(line_start, key) == NULL) {
            // This line doesn't contain the key, keep it
            if (token_append(&new_content, line_start) != RESULT_OK ||
                token_append(&new_content, "\n") != RESULT_OK) {
                *line_end = '\n'; // Restore newline
                lkj_log_error(__func__, "failed to rebuild scratchpad");
                return RESULT_ERR;
            }
        }
        
        *line_end = '\n'; // Restore newline
        line_start = line_end + 1;
    }

    // Copy the filtered content back to scratchpad
    if (token_copy(&agent->memory.scratchpad, &new_content) != RESULT_OK) {
        lkj_log_error(__func__, "failed to update scratchpad");
        return RESULT_ERR;
    }

    // Record the forget operation
    if (token_append(&agent->memory.scratchpad, "FORGET_OPERATION: Removed references to '") != RESULT_OK ||
        token_append(&agent->memory.scratchpad, key) != RESULT_OK ||
        token_append(&agent->memory.scratchpad, "'\n") != RESULT_OK) {
        lkj_log_error(__func__, "failed to record forget operation");
        return RESULT_ERR;
    }

    return RESULT_OK;
}
