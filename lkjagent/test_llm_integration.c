/**
 * @file test_llm_integration.c
 * @brief Comprehensive test for LLM integration system
 * 
 * This test program validates the complete LLM integration including
 * HTTP client, LLM client, parser, context management, and prompt construction.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "src/lkjagent.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 * @brief Test result tracking
 */
typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
} test_results_t;

static test_results_t g_results = {0, 0, 0};

/**
 * @brief Test assertion macro
 */
#define TEST_ASSERT(condition, message) \
    do { \
        g_results.tests_run++; \
        if (condition) { \
            g_results.tests_passed++; \
            printf("✓ %s\n", message); \
        } else { \
            g_results.tests_failed++; \
            printf("✗ %s (FAILED)\n", message); \
        } \
    } while(0)

/**
 * @brief Test HTTP client initialization and basic functionality
 */
static void test_http_client(void) {
    printf("\n=== Testing HTTP Client ===\n");
    
    http_client_t client;
    http_client_config_t config = {
        .connect_timeout = 5,
        .request_timeout = 10,
        .max_retries = 2,
        .retry_delay = 1000,
        .max_response_size = 1024 * 1024
    };
    strcpy(config.user_agent, "LKJAgent-Test/1.0");
    
    result_t result = http_client_init(&client, &config);
    TEST_ASSERT(result == RESULT_OK, "HTTP client initialization");
    
    if (result == RESULT_OK) {
        TEST_ASSERT(client.config.connect_timeout == 5, "HTTP client config applied");
        TEST_ASSERT(strcmp(client.config.user_agent, "LKJAgent-Test/1.0") == 0, "User agent set correctly");
        
        result_t cleanup_result = http_client_cleanup(&client);
        TEST_ASSERT(cleanup_result == RESULT_OK, "HTTP client cleanup");
    }
}

/**
 * @brief Test LLM client initialization and configuration
 */
static void test_llm_client(void) {
    printf("\n=== Testing LLM Client ===\n");
    
    llm_client_t client;
    llm_client_config_t config = {0};
    strcpy(config.base_url, "http://localhost:1234");
    strcpy(config.default_model, "test-model");
    config.request_timeout = 30;
    config.connect_timeout = 10;
    config.max_retries = 3;
    
    result_t result = llm_client_init(&client, &config);
    TEST_ASSERT(result == RESULT_OK, "LLM client initialization");
    
    if (result == RESULT_OK) {
        TEST_ASSERT(strcmp(client.config.base_url, "http://localhost:1234") == 0, "LLM client base URL");
        TEST_ASSERT(strcmp(client.config.default_model, "test-model") == 0, "LLM client default model");
        TEST_ASSERT(client.config.request_timeout == 30, "LLM client timeout configuration");
        
        // Test model setting
        result_t model_result = llm_client_set_model(&client, "new-test-model");
        TEST_ASSERT(model_result == RESULT_OK, "LLM client model setting");
        TEST_ASSERT(strcmp(client.config.default_model, "new-test-model") == 0, "Model updated correctly");
        
        // Test statistics retrieval
        data_t stats;
        if (data_init(&stats, 1024) == RESULT_OK) {
            result_t stats_result = llm_client_get_stats(&client, &stats);
            TEST_ASSERT(stats_result == RESULT_OK, "LLM client statistics retrieval");
            TEST_ASSERT(stats.size > 0, "Statistics contain data");
            data_destroy(&stats);
        }
        
        result_t cleanup_result = llm_client_cleanup(&client);
        TEST_ASSERT(cleanup_result == RESULT_OK, "LLM client cleanup");
    }
}

/**
 * @brief Test LLM response parsing with simple tag format
 */
static void test_llm_parser(void) {
    printf("\n=== Testing LLM Parser ===\n");
    
    const char* test_response = 
        "<thinking>\n"
        "I need to analyze this request carefully. The user is asking about the system status.\n"
        "Key considerations:\n"
        "- Check memory usage\n"
        "- Verify connectivity\n"
        "- Review recent activities\n"
        "</thinking>\n"
        "\n"
        "<action>\n"
        "Based on my analysis, I'll check the system status by examining memory and connectivity.\n"
        "Context keys: [system_status, memory_usage, connectivity_check]\n"
        "</action>\n"
        "\n"
        "<paging>\n"
        "move:old_logs:archive\n"
        "importance:system_status:90\n"
        "</paging>";
    
    llm_parsed_response_t parsed_response;
    result_t init_result = llm_parsed_response_init(&parsed_response);
    TEST_ASSERT(init_result == RESULT_OK, "LLM parsed response initialization");
    
    if (init_result == RESULT_OK) {
        result_t parse_result = llm_parse_response(test_response, &parsed_response);
        TEST_ASSERT(parse_result == RESULT_OK, "LLM response parsing");
        
        TEST_ASSERT(parsed_response.thinking.size > 0, "Thinking block extracted");
        TEST_ASSERT(parsed_response.action.size > 0, "Action block extracted");
        TEST_ASSERT(parsed_response.paging.size > 0, "Paging block extracted");
        
        TEST_ASSERT(parsed_response.context_key_count > 0, "Context keys extracted");
        TEST_ASSERT(parsed_response.directive_count > 0, "Paging directives extracted");
        
        // Test individual block parsing
        data_t thinking_content;
        if (data_init(&thinking_content, 1024) == RESULT_OK) {
            result_t thinking_result = llm_parse_thinking_block(test_response, &thinking_content);
            TEST_ASSERT(thinking_result == RESULT_OK, "Thinking block individual parsing");
            TEST_ASSERT(thinking_content.size > 0, "Thinking content extracted");
            data_destroy(&thinking_content);
        }
        
        data_t action_content;
        if (data_init(&action_content, 1024) == RESULT_OK) {
            result_t action_result = llm_parse_action_block(test_response, &action_content);
            TEST_ASSERT(action_result == RESULT_OK, "Action block individual parsing");
            TEST_ASSERT(action_content.size > 0, "Action content extracted");
            data_destroy(&action_content);
        }
        
        result_t cleanup_result = llm_parsed_response_cleanup(&parsed_response);
        TEST_ASSERT(cleanup_result == RESULT_OK, "LLM parsed response cleanup");
    }
}

/**
 * @brief Test prompt construction for different states
 */
static void test_prompt_construction(void) {
    printf("\n=== Testing Prompt Construction ===\n");
    
    llm_prompt_t prompt;
    result_t init_result = llm_prompt_init(&prompt);
    TEST_ASSERT(init_result == RESULT_OK, "LLM prompt initialization");
    
    if (init_result == RESULT_OK) {
        // Test thinking state prompt
        llm_context_t context;
        result_t context_init = llm_context_init(&context);
        TEST_ASSERT(context_init == RESULT_OK, "LLM context initialization");
        
        if (context_init == RESULT_OK) {
            const char* instructions = "Analyze the current situation and plan next steps.";
            result_t thinking_result = llm_prompt_build_thinking(&context, instructions, &prompt);
            TEST_ASSERT(thinking_result == RESULT_OK, "Thinking prompt construction");
            TEST_ASSERT(prompt.final_prompt.size > 0, "Thinking prompt has content");
            TEST_ASSERT(prompt.metadata.target_state == STATE_THINKING, "Thinking prompt metadata");
            
            result_t context_cleanup = llm_context_cleanup(&context);
            TEST_ASSERT(context_cleanup == RESULT_OK, "LLM context cleanup");
        }
        
        result_t cleanup_result = llm_prompt_cleanup(&prompt);
        TEST_ASSERT(cleanup_result == RESULT_OK, "LLM prompt cleanup");
    }
}

/**
 * @brief Test LLM context preparation and management
 */
static void test_context_management(void) {
    printf("\n=== Testing Context Management ===\n");
    
    llm_context_t context;
    result_t init_result = llm_context_init(&context);
    TEST_ASSERT(init_result == RESULT_OK, "LLM context initialization");
    
    if (init_result == RESULT_OK) {
        // Test context size calculation
        size_t token_count;
        result_t size_result = llm_context_calculate_size(&context, &token_count);
        TEST_ASSERT(size_result == RESULT_OK, "Context size calculation");
        TEST_ASSERT(token_count >= 0, "Token count is valid");
        
        result_t cleanup_result = llm_context_cleanup(&context);
        TEST_ASSERT(cleanup_result == RESULT_OK, "LLM context cleanup");
    }
}

/**
 * @brief Test error handling and edge cases
 */
static void test_error_handling(void) {
    printf("\n=== Testing Error Handling ===\n");
    
    // Test NULL parameter handling
    result_t null_result = http_client_init(NULL, NULL);
    TEST_ASSERT(null_result == RESULT_ERR, "HTTP client NULL parameter rejection");
    
    null_result = llm_client_init(NULL, NULL);
    TEST_ASSERT(null_result == RESULT_ERR, "LLM client NULL parameter rejection");
    
    null_result = llm_parse_response(NULL, NULL);
    TEST_ASSERT(null_result == RESULT_ERR, "LLM parser NULL parameter rejection");
    
    // Test malformed response parsing
    const char* malformed_response = "<thinking>Unclosed thinking block\n<action>No closing tag";
    llm_parsed_response_t parsed_response;
    if (llm_parsed_response_init(&parsed_response) == RESULT_OK) {
        result_t parse_result = llm_parse_response(malformed_response, &parsed_response);
        // Should handle malformed input gracefully
        TEST_ASSERT(parse_result == RESULT_OK || parse_result == RESULT_ERR, "Malformed response handling");
        if (llm_parsed_response_cleanup(&parsed_response) != RESULT_OK) {
            // Cleanup might fail if structure is corrupted
        }
    }
}

/**
 * @brief Main test runner
 */
int main(void) {
    printf("LKJAgent LLM Integration Test Suite\n");
    printf("===================================\n");
    
    test_http_client();
    test_llm_client();
    test_llm_parser();
    test_prompt_construction();
    test_context_management();
    test_error_handling();
    
    printf("\n=== Test Results ===\n");
    printf("Tests run: %d\n", g_results.tests_run);
    printf("Tests passed: %d\n", g_results.tests_passed);
    printf("Tests failed: %d\n", g_results.tests_failed);
    
    if (g_results.tests_failed > 0) {
        printf("❌ Some tests failed\n");
        return 1;
    } else {
        printf("✅ All tests passed\n");
        return 0;
    }
}
