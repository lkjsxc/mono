#include "src/lkjagent.h"
#include "src/token.c"
#include "src/json.c"

void test_json_edge_cases() {
    printf("=== Testing JSON Edge Cases ===\n");
    
    char json_buffer[1024];
    char result_buffer[256];
    token_t json_token, result_token;
    double number_result;
    
    if (token_init(&json_token, json_buffer, sizeof(json_buffer)) != RESULT_OK ||
        token_init(&result_token, result_buffer, sizeof(result_buffer)) != RESULT_OK) {
        printf("Failed to initialize tokens\n");
        return;
    }
    
    // Test 1: Empty JSON object
    if (token_set(&json_token, "{}") != RESULT_OK) {
        printf("Failed to set empty JSON\n");
        return;
    }
    
    if (json_validate(&json_token) == RESULT_OK) {
        printf("✓ Empty JSON object is valid\n");
    } else {
        printf("✗ Empty JSON object validation failed\n");
    }
    
    // Test 2: JSON with whitespace
    if (token_set(&json_token, "  \n\t  {  \"key\"  :  \"value\"  }  \n\t  ") != RESULT_OK) {
        printf("Failed to set whitespace JSON\n");
        return;
    }
    
    if (json_validate(&json_token) == RESULT_OK) {
        printf("✓ JSON with whitespace is valid\n");
    } else {
        printf("✗ JSON with whitespace validation failed\n");
    }
    
    // Test 3: Nested JSON object
    if (token_set(&json_token, "{\"outer\":{\"inner\":\"value\"},\"number\":42}") != RESULT_OK) {
        printf("Failed to set nested JSON\n");
        return;
    }
    
    if (json_validate(&json_token) == RESULT_OK) {
        printf("✓ Nested JSON object is valid\n");
    } else {
        printf("✗ Nested JSON object validation failed\n");
    }
    
    // Test 4: Array with mixed types
    if (token_set(&json_token, "[1, \"hello\", true, null, {\"key\":\"value\"}]") != RESULT_OK) {
        printf("Failed to set mixed array JSON\n");
        return;
    }
    
    if (json_validate(&json_token) == RESULT_OK) {
        printf("✓ Mixed array JSON is valid\n");
    } else {
        printf("✗ Mixed array JSON validation failed\n");
    }
    
    // Test 5: Escaped strings
    if (token_set(&json_token, "{\"escaped\":\"Hello\\nWorld\\\"!\"}") != RESULT_OK) {
        printf("Failed to set escaped string JSON\n");
        return;
    }
    
    if (json_validate(&json_token) == RESULT_OK) {
        printf("✓ JSON with escaped strings is valid\n");
    } else {
        printf("✗ JSON with escaped strings validation failed\n");
    }
    
    // Test 6: Invalid JSON - missing closing brace
    if (token_set(&json_token, "{\"key\":\"value\"") != RESULT_OK) {
        printf("Failed to set invalid JSON\n");
        return;
    }
    
    if (json_validate(&json_token) == RESULT_ERR) {
        printf("✓ Invalid JSON (missing closing brace) correctly rejected\n");
    } else {
        printf("✗ Invalid JSON (missing closing brace) incorrectly accepted\n");
    }
    
    // Test 7: Invalid JSON - trailing comma
    if (token_set(&json_token, "{\"key\":\"value\",}") != RESULT_OK) {
        printf("Failed to set trailing comma JSON\n");
        return;
    }
    
    if (json_validate(&json_token) == RESULT_ERR) {
        printf("✓ Invalid JSON (trailing comma) correctly rejected\n");
    } else {
        printf("✗ Invalid JSON (trailing comma) incorrectly accepted\n");
    }
    
    // Test 8: Very large number
    if (token_set(&json_token, "{\"big_number\":1234567890.123456789}") != RESULT_OK) {
        printf("Failed to set big number JSON\n");
        return;
    }
    
    if (json_get_number(&json_token, "big_number", &number_result) == RESULT_OK) {
        printf("✓ Extracted big number: %.6f\n", number_result);
    } else {
        printf("✗ Failed to extract big number\n");
    }
    
    // Test 9: Negative number
    if (token_set(&json_token, "{\"negative\":-42.5}") != RESULT_OK) {
        printf("Failed to set negative number JSON\n");
        return;
    }
    
    if (json_get_number(&json_token, "negative", &number_result) == RESULT_OK) {
        printf("✓ Extracted negative number: %.1f\n", number_result);
    } else {
        printf("✗ Failed to extract negative number\n");
    }
    
    // Test 10: Empty string value
    if (token_set(&json_token, "{\"empty\":\"\"}") != RESULT_OK) {
        printf("Failed to set empty string JSON\n");
        return;
    }
    
    if (json_get_string(&json_token, "empty", &result_token) == RESULT_OK) {
        printf("✓ Extracted empty string (length: %zu)\n", result_token.size);
    } else {
        printf("✗ Failed to extract empty string\n");
    }
    
    printf("\n");
}

int main() {
    printf("JSON Edge Cases Test Suite\n");
    printf("==========================\n\n");
    
    test_json_edge_cases();
    
    printf("Edge cases test completed.\n");
    return 0;
}
