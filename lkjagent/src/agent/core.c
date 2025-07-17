/**
 * @file core.c
 * @brief Agent core functionality implementation
 *
 * This module implements the basic agent lifecycle and management functions.
 * It provides initialization and execution stubs that can be expanded as
 * the agent system develops.
 */

#include "../lkjagent.h"
#include <stdio.h>
#include <string.h>

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
    if (!agent) {
        RETURN_ERR("lkjagent_init: NULL agent parameter");
        return RESULT_ERR;
    }
    
    // Clear error state
    lkj_clear_last_error();
    
    printf("Initializing LKJAgent...\n");
    
    // Initialize agent structure
    memset(agent, 0, sizeof(lkjagent_t));
    
    // For now, just a basic stub implementation
    printf("Agent initialization completed.\n");
    
    return RESULT_OK;
}

/**
 * @brief Run agent main execution loop
 *
 * Executes the agent's main operational cycle. This is a stub
 * implementation that demonstrates the basic API.
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
    
    printf("Starting agent execution...\n");
    
    // Demonstrate token usage
    char buffer1[256];
    char buffer2[256];
    token_t test_token;
    token_t response_token;
    
    if (token_init(&test_token, buffer1, sizeof(buffer1)) != RESULT_OK) {
        printf("Failed to initialize test token: %s\n", lkj_get_last_error());
        return RESULT_ERR;
    }
    
    if (token_init(&response_token, buffer2, sizeof(buffer2)) != RESULT_OK) {
        printf("Failed to initialize response token: %s\n", lkj_get_last_error());
        return RESULT_ERR;
    }
    
    // Test token operations
    if (token_set(&test_token, "Hello, LKJAgent!") != RESULT_OK) {
        printf("Failed to set token: %s\n", lkj_get_last_error());
        return RESULT_ERR;
    }
    
    printf("Token content: %s\n", test_token.data);
    printf("Token size: %zu\n", test_token.size);
    
    // Test token copy
    if (token_copy(&response_token, &test_token) != RESULT_OK) {
        printf("Failed to copy token: %s\n", lkj_get_last_error());
        return RESULT_ERR;
    }
    
    printf("Copied token content: %s\n", response_token.data);
    
    // Test file operations (write and read)
    printf("Testing file operations...\n");
    
    char file_content_buffer[512];
    token_t file_content;
    if (token_init(&file_content, file_content_buffer, sizeof(file_content_buffer)) != RESULT_OK) {
        printf("Failed to initialize file content token: %s\n", lkj_get_last_error());
        return RESULT_ERR;
    }
    
    if (token_set(&file_content, "This is a test file created by LKJAgent.\nIt demonstrates file I/O operations.") != RESULT_OK) {
        printf("Failed to set file content: %s\n", lkj_get_last_error());
        return RESULT_ERR;
    }
    
    const char* test_file_path = "./test_output.txt";
    if (file_write(test_file_path, &file_content) != RESULT_OK) {
        printf("Failed to write test file: %s\n", lkj_get_last_error());
        return RESULT_ERR;
    }
    
    printf("Successfully wrote test file: %s\n", test_file_path);
    
    // Read the file back
    char read_buffer[512];
    token_t read_content;
    if (token_init(&read_content, read_buffer, sizeof(read_buffer)) != RESULT_OK) {
        printf("Failed to initialize read content token: %s\n", lkj_get_last_error());
        return RESULT_ERR;
    }
    
    if (file_read(test_file_path, &read_content) != RESULT_OK) {
        printf("Failed to read test file: %s\n", lkj_get_last_error());
        return RESULT_ERR;
    }
    
    printf("Successfully read file content (%zu bytes):\n%s\n", read_content.size, read_content.data);
    
    // Test JSON operations
    printf("Testing JSON operations...\n");
    
    const char* keys[] = {"name", "version", "status"};
    const char* values[] = {"LKJAgent", "1.0", "active"};
    
    char json_buffer[512];
    token_t json_token;
    if (token_init(&json_token, json_buffer, sizeof(json_buffer)) != RESULT_OK) {
        printf("Failed to initialize JSON token: %s\n", lkj_get_last_error());
        return RESULT_ERR;
    }
    
    if (json_create_object(&json_token, keys, values, 3) != RESULT_OK) {
        printf("Failed to create JSON object: %s\n", lkj_get_last_error());
        return RESULT_ERR;
    }
    
    printf("Created JSON: %s\n", json_token.data);
    
    // Validate the JSON
    if (json_validate(&json_token) != RESULT_OK) {
        printf("JSON validation failed: %s\n", lkj_get_last_error());
        return RESULT_ERR;
    }
    
    printf("JSON validation successful.\n");
    
    // Extract a value from JSON
    char extracted_buffer[128];
    token_t extracted_value;
    if (token_init(&extracted_value, extracted_buffer, sizeof(extracted_buffer)) != RESULT_OK) {
        printf("Failed to initialize extracted value token: %s\n", lkj_get_last_error());
        return RESULT_ERR;
    }
    
    if (json_get_string(&json_token, "name", &extracted_value) != RESULT_OK) {
        printf("Failed to extract 'name' from JSON: %s\n", lkj_get_last_error());
        return RESULT_ERR;
    }
    
    printf("Extracted 'name' value: %s\n", extracted_value.data);
    
    printf("Agent execution completed successfully.\n");
    
    return RESULT_OK;
}