#include "../src/lkjagent.h"
#include "../src/token.c"
#include "../src/json.c"

void test_json_validation() {
    printf("=== Testing JSON Validation ===\n");
    
    char json_buffer[1024];
    token_t json_token;
    
    if (token_init(&json_token, json_buffer, sizeof(json_buffer)) != RESULT_OK) {
        printf("Failed to initialize token\n");
        return;
    }
    
    // Test valid JSON object
    if (token_set(&json_token, "{\"name\":\"test\",\"value\":42}") != RESULT_OK) {
        printf("Failed to set JSON token\n");
        return;
    }
    if (json_validate(&json_token) == RESULT_OK) {
        printf("✓ Valid JSON object passed validation\n");
    } else {
        printf("✗ Valid JSON object failed validation\n");
    }
    
    // Test valid JSON array
    if (token_set(&json_token, "[1,2,3,\"hello\",true,null]") != RESULT_OK) {
        printf("Failed to set JSON token\n");
        return;
    }
    if (json_validate(&json_token) == RESULT_OK) {
        printf("✓ Valid JSON array passed validation\n");
    } else {
        printf("✗ Valid JSON array failed validation\n");
    }
    
    // Test invalid JSON
    if (token_set(&json_token, "{\"name\":\"test\",\"value\":}") != RESULT_OK) {
        printf("Failed to set JSON token\n");
        return;
    }
    if (json_validate(&json_token) == RESULT_ERR) {
        printf("✓ Invalid JSON correctly rejected\n");
    } else {
        printf("✗ Invalid JSON incorrectly accepted\n");
    }
    
    // Test empty string
    if (token_clear(&json_token) != RESULT_OK) {
        printf("Failed to clear JSON token\n");
        return;
    }
    if (json_validate(&json_token) == RESULT_ERR) {
        printf("✓ Empty JSON correctly rejected\n");
    } else {
        printf("✗ Empty JSON incorrectly accepted\n");
    }
    
    printf("\n");
}

void test_json_string_extraction() {
    printf("=== Testing JSON String Extraction ===\n");
    
    char json_buffer[1024];
    char result_buffer[256];
    token_t json_token, result_token;
    
    if (token_init(&json_token, json_buffer, sizeof(json_buffer)) != RESULT_OK ||
        token_init(&result_token, result_buffer, sizeof(result_buffer)) != RESULT_OK) {
        printf("Failed to initialize tokens\n");
        return;
    }
    
    // Test simple string extraction
    if (token_set(&json_token, "{\"model\":\"qwen/qwen3-8b\",\"temperature\":0.7}") != RESULT_OK) {
        printf("Failed to set JSON token\n");
        return;
    }
    
    if (json_get_string(&json_token, "model", &result_token) == RESULT_OK) {
        printf("✓ Extracted model: '%s'\n", result_token.data);
    } else {
        printf("✗ Failed to extract model string\n");
    }
    
    // Test non-existent key
    if (json_get_string(&json_token, "nonexistent", &result_token) == RESULT_ERR) {
        printf("✓ Correctly failed to extract non-existent key\n");
    } else {
        printf("✗ Incorrectly extracted non-existent key\n");
    }
    
    // Test LMStudio-style JSON
    if (token_set(&json_token, "{\"messages\":[{\"role\":\"user\",\"content\":\"Hello\"}],\"model\":\"test-model\"}") != RESULT_OK) {
        printf("Failed to set JSON token\n");
        return;
    }
    
    if (json_get_string(&json_token, "model", &result_token) == RESULT_OK) {
        printf("✓ Extracted from complex JSON: '%s'\n", result_token.data);
    } else {
        printf("✗ Failed to extract from complex JSON\n");
    }
    
    printf("\n");
}

void test_json_number_extraction() {
    printf("=== Testing JSON Number Extraction ===\n");
    
    char json_buffer[1024];
    token_t json_token;
    double result;
    
    if (token_init(&json_token, json_buffer, sizeof(json_buffer)) != RESULT_OK) {
        printf("Failed to initialize token\n");
        return;
    }
    
    // Test number extraction
    if (token_set(&json_token, "{\"temperature\":0.7,\"max_tokens\":1024}") != RESULT_OK) {
        printf("Failed to set JSON token\n");
        return;
    }
    
    if (json_get_number(&json_token, "temperature", &result) == RESULT_OK) {
        printf("✓ Extracted temperature: %.2f\n", result);
    } else {
        printf("✗ Failed to extract temperature\n");
    }
    
    if (json_get_number(&json_token, "max_tokens", &result) == RESULT_OK) {
        printf("✓ Extracted max_tokens: %.0f\n", result);
    } else {
        printf("✗ Failed to extract max_tokens\n");
    }
    
    // Test non-existent number
    if (json_get_number(&json_token, "nonexistent", &result) == RESULT_ERR) {
        printf("✓ Correctly failed to extract non-existent number\n");
    } else {
        printf("✗ Incorrectly extracted non-existent number\n");
    }
    
    printf("\n");
}

void test_json_creation() {
    printf("=== Testing JSON Creation ===\n");
    
    char json_buffer[1024];
    token_t json_token;
    
    if (token_init(&json_token, json_buffer, sizeof(json_buffer)) != RESULT_OK) {
        printf("Failed to initialize token\n");
        return;
    }
    
    // Test creating a simple JSON object
    const char* keys[] = {"name", "version", "status"};
    const char* values[] = {"lkjagent", "1.0", "active"};
    
    if (json_create_object(&json_token, keys, values, 3) == RESULT_OK) {
        printf("✓ Created JSON object: %s\n", json_token.data);
        
        // Validate the created JSON
        if (json_validate(&json_token) == RESULT_OK) {
            printf("✓ Created JSON is valid\n");
        } else {
            printf("✗ Created JSON is invalid\n");
        }
    } else {
        printf("✗ Failed to create JSON object\n");
    }
    
    printf("\n");
}

void test_json_formatting() {
    printf("=== Testing JSON Formatting ===\n");
    
    char input_buffer[1024];
    char output_buffer[2048];
    token_t input_token, output_token;
    
    if (token_init(&input_token, input_buffer, sizeof(input_buffer)) != RESULT_OK ||
        token_init(&output_token, output_buffer, sizeof(output_buffer)) != RESULT_OK) {
        printf("Failed to initialize tokens\n");
        return;
    }
    
    // Test formatting compact JSON
    if (token_set(&input_token, "{\"model\":\"test\",\"messages\":[{\"role\":\"user\",\"content\":\"Hello\"}],\"temperature\":0.7}") != RESULT_OK) {
        printf("Failed to set input token\n");
        return;
    }
    
    if (json_format(&input_token, &output_token) == RESULT_OK) {
        printf("✓ Formatted JSON:\n%s\n", output_token.data);
    } else {
        printf("✗ Failed to format JSON\n");
    }
    
    printf("\n");
}

void test_lmstudio_json() {
    printf("=== Testing LMStudio JSON Compatibility ===\n");
    
    char json_buffer[2048];
    char result_buffer[256];
    token_t json_token, result_token;
    double number_result;
    
    if (token_init(&json_token, json_buffer, sizeof(json_buffer)) != RESULT_OK ||
        token_init(&result_token, result_buffer, sizeof(result_buffer)) != RESULT_OK) {
        printf("Failed to initialize tokens\n");
        return;
    }
    
    // Test with actual LMStudio JSON structure
    if (token_set(&json_token, "{\n"
        "    \"model\": \"qwen/qwen3-8b\",\n"
        "    \"messages\": [\n"
        "      { \"role\": \"system\", \"content\": \"Always answer in rhymes. Today is Thursday\" },\n"
        "      { \"role\": \"user\", \"content\": \"What day is it today?\" }\n"
        "    ],\n"
        "    \"temperature\": 0.7,\n"
        "    \"max_tokens\": -1,\n"
        "    \"stream\": false\n"
        "}") != RESULT_OK) {
        printf("Failed to set LMStudio JSON\n");
        return;
    }
    
    printf("Testing LMStudio JSON validation...\n");
    if (json_validate(&json_token) == RESULT_OK) {
        printf("✓ LMStudio JSON is valid\n");
    } else {
        printf("✗ LMStudio JSON validation failed\n");
    }
    
    // Extract model name
    if (json_get_string(&json_token, "model", &result_token) == RESULT_OK) {
        printf("✓ Extracted model: '%s'\n", result_token.data);
    } else {
        printf("✗ Failed to extract model from LMStudio JSON\n");
    }
    
    // Extract temperature
    if (json_get_number(&json_token, "temperature", &number_result) == RESULT_OK) {
        printf("✓ Extracted temperature: %.1f\n", number_result);
    } else {
        printf("✗ Failed to extract temperature from LMStudio JSON\n");
    }
    
    // Extract max_tokens (negative number)
    if (json_get_number(&json_token, "max_tokens", &number_result) == RESULT_OK) {
        printf("✓ Extracted max_tokens: %.0f\n", number_result);
    } else {
        printf("✗ Failed to extract max_tokens from LMStudio JSON\n");
    }
    
    printf("\n");
}

int main() {
    printf("JSON Implementation Test Suite\n");
    printf("==============================\n\n");
    
    test_json_validation();
    test_json_string_extraction();
    test_json_number_extraction();
    test_json_creation();
    test_json_formatting();
    test_lmstudio_json();
    
    printf("Test suite completed.\n");
    return 0;
}
