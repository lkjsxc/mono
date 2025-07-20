/**
 * @file test_llm_basic.c
 * @brief Basic test for working LLM integration components
 * 
 * This test focuses on the components that are actually implemented and working.
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
 * @brief Test integration between HTTP and LLM clients
 */
static void test_integration(void) {
    printf("\n=== Testing HTTP and LLM Integration ===\n");
    
    // Test that LLM client properly uses HTTP client
    llm_client_t llm_client;
    llm_client_config_t llm_config = {0};
    strcpy(llm_config.base_url, "http://localhost:1234");
    strcpy(llm_config.default_model, "test-model");
    llm_config.request_timeout = 30;
    llm_config.connect_timeout = 10;
    
    result_t llm_init = llm_client_init(&llm_client, &llm_config);
    TEST_ASSERT(llm_init == RESULT_OK, "LLM client initialization for integration test");
    
    if (llm_init == RESULT_OK) {
        // Test that HTTP client is properly initialized within LLM client
        TEST_ASSERT(llm_client.http_client.config.connect_timeout == 10, "HTTP client timeout configured via LLM client");
        TEST_ASSERT(llm_client.http_client.config.request_timeout == 30, "HTTP client request timeout configured");
        
        // Test connectivity check (doesn't require actual server)
        uint64_t response_time;
        result_t conn_test = llm_client_test_connection(&llm_client, &response_time);
        // This will fail since no server is running, but should handle error gracefully
        TEST_ASSERT(conn_test == RESULT_ERR, "Connection test returns error for non-existent server");
        
        result_t llm_cleanup = llm_client_cleanup(&llm_client);
        TEST_ASSERT(llm_cleanup == RESULT_OK, "LLM client cleanup in integration test");
    }
}

/**
 * @brief Test data structures and initialization
 */
static void test_data_structures(void) {
    printf("\n=== Testing Data Structures ===\n");
    
    // Test LLM response structure
    llm_response_t response;
    result_t response_init = llm_response_init(&response);
    TEST_ASSERT(response_init == RESULT_OK, "LLM response structure initialization");
    
    if (response_init == RESULT_OK) {
        TEST_ASSERT(response.content.capacity > 0, "LLM response content buffer allocated");
        TEST_ASSERT(response.tokens_generated == 0, "Initial token count is zero");
        
        result_t response_cleanup = llm_response_cleanup(&response);
        TEST_ASSERT(response_cleanup == RESULT_OK, "LLM response structure cleanup");
    }
    
    // Test HTTP response structure
    http_response_t http_response;
    result_t http_init_result = http_response_init(&http_response, 1024);
    TEST_ASSERT(http_init_result == RESULT_OK, "HTTP response structure initialization");
    
    if (http_init_result == RESULT_OK) {
        TEST_ASSERT(http_response.body.capacity >= 1024, "HTTP response body buffer allocated");
        TEST_ASSERT(http_response.headers.capacity > 0, "HTTP response headers buffer allocated");
        
        result_t http_cleanup_result = http_response_cleanup(&http_response);
        TEST_ASSERT(http_cleanup_result == RESULT_OK, "HTTP response structure cleanup");
    }
}

/**
 * @brief Main test runner
 */
int main(void) {
    printf("LKJAgent LLM Integration Basic Test Suite\n");
    printf("==========================================\n");
    
    test_http_client();
    test_llm_client();
    test_llm_parser();
    test_integration();
    test_data_structures();
    
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
