#include "../src/lkjagent.h"
#include "../src/token.c"
#include "../src/http.c"

void test_token_functions() {
    printf("=== Testing Token Functions ===\n");
    
    // Test basic token operations
    char buffer1[100], buffer2[100], buffer3[100];
    token_t token1, token2, token3;
    
    token_init(&token1, buffer1, sizeof(buffer1));
    token_init(&token2, buffer2, sizeof(buffer2));
    token_init(&token3, buffer3, sizeof(buffer3));
    
    // Test set and append
    token_set(&token1, "Hello");
    token_append(&token1, " World");
    printf("Token1: '%s' (size: %zu)\n", token1.data, token1.size);
    
    // Test copy
    token_copy(&token2, &token1);
    printf("Token2 (copy): '%s'\n", token2.data);
    
    // Test equals
    printf("Token1 equals Token2: %s\n", token_equals(&token1, &token2) ? "true" : "false");
    printf("Token1 equals 'Hello World': %s\n", token_equals_str(&token1, "Hello World") ? "true" : "false");
    
    // Test find
    size_t position;
    if (token_find(&token1, "World", &position) == RESULT_OK) {
        printf("Found 'World' at position: %zu\n", position);
    }
    
    // Test substring
    if (token_substring(&token1, 0, 5, &token3) == RESULT_OK) {
        printf("Substring (0, 5): '%s'\n", token3.data);
    }
    
    // Test trim
    token_set(&token1, "  \t  Hello World  \n  ");
    printf("Before trim: '%s'\n", token1.data);
    token_trim(&token1);
    printf("After trim: '%s'\n", token1.data);
    
    printf("Available space in token1: %d bytes\n\n", token_available_space(&token1));
}

void test_http_functions() {
    printf("=== Testing HTTP Functions ===\n");
    
    char url_data[256];
    char response_data[4096];
    char body_data[1024];
    
    token_t url, response, body;
    
    // Initialize tokens
    token_init(&url, url_data, sizeof(url_data));
    token_init(&response, response_data, sizeof(response_data));
    token_init(&body, body_data, sizeof(body_data));
    
    // Test GET request using convenience function
    printf("--- GET Request Test ---\n");
    token_set(&url, "http://httpbin.org/get");
    
    if (http_get(&url, &response) == RESULT_OK) {
        printf("GET Response (%zu bytes):\n", response.size);
        // Print first 200 characters of response
        size_t print_len = response.size < 200 ? response.size : 200;
        for (size_t i = 0; i < print_len; i++) {
            printf("%c", response.data[i]);
        }
        if (response.size > 200) {
            printf("...[truncated]");
        }
        printf("\n\n");
    } else {
        printf("GET request failed\n\n");
    }
    
    // Test POST request using convenience function
    printf("--- POST Request Test ---\n");
    token_set(&url, "http://httpbin.org/post");
    token_set(&body, "{\"test\":\"lkjagent\",\"features\":[\"tokens\",\"http\"]}");
    
    if (http_post(&url, &body, &response) == RESULT_OK) {
        printf("POST Response (%zu bytes):\n", response.size);
        // Print first 300 characters of response
        size_t print_len = response.size < 300 ? response.size : 300;
        for (size_t i = 0; i < print_len; i++) {
            printf("%c", response.data[i]);
        }
        if (response.size > 300) {
            printf("...[truncated]");
        }
        printf("\n\n");
    } else {
        printf("POST request failed\n\n");
    }
}

int main() {
    printf("lkjagent - Comprehensive Test Suite\n");
    printf("====================================\n\n");
    
    test_token_functions();
    test_http_functions();
    
    printf("All tests completed!\n");
    return 0;
}
