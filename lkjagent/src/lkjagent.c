#include "lkjagent.h"

static __attribute__((warn_unused_result)) result_t lkjagent_init(lkjagent_t* lkjagent) {
    if (pool_init(&lkjagent->pool) != RESULT_OK) {
        RETURN_ERR("Failed to initialize memory pool");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_run(lkjagent_t* lkjagent) {

    string_t* config_string;
    json_value_t* config_json;

    string_t* test_string;
    string_t* test_path;
    json_value_t* test_json;

    if(string_create(&lkjagent->pool, &config_string) != RESULT_OK) {
        RETURN_ERR("Failed to create config string");
    }
    if(string_create(&lkjagent->pool, &test_string) != RESULT_OK) {
        RETURN_ERR("Failed to create test string");
    }
    if(string_create_str(&lkjagent->pool, &test_path, "agent.prompt.base") != RESULT_OK) {
        RETURN_ERR("Failed to create test path string");
    }

    if(file_read(&lkjagent->pool, CONFIG_PATH, &config_string) != RESULT_OK) {
        RETURN_ERR("Failed to read config file");
    }

    if(json_parse(&lkjagent->pool, &config_json, config_string) != RESULT_OK) {
        RETURN_ERR("Failed to parse config JSON");
    }

    if(json_object_get(&lkjagent->pool, &test_json, config_json, test_path) != RESULT_OK) {
        RETURN_ERR("Failed to get JSON object from config");
    }

    // Destroy and recreate the test_string to ensure it's clean
    if(string_destroy(&lkjagent->pool, test_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy test string");
    }
    
    if(string_create(&lkjagent->pool, &test_string) != RESULT_OK) {
        RETURN_ERR("Failed to recreate test string");
    }

    if(json_to_string(&lkjagent->pool, &test_string, test_json) != RESULT_OK) {
        RETURN_ERR("Failed to convert JSON object to string");
    }

    printf("Test JSON: %.*s\n", (int)test_string->size, test_string->data);
    printf("String size: %llu\n", (unsigned long long)test_string->size);
    printf("String capacity: %llu\n", (unsigned long long)test_string->capacity);
    
    // Debug: print first few bytes in hex
    printf("First 10 bytes (hex): ");
    for (int i = 0; i < 10 && i < (int)test_string->size; i++) {
        printf("%02x ", (unsigned char)test_string->data[i]);
    }
    printf("\n");
    
    // Debug: print first few characters
    printf("First 10 chars: ");
    for (int i = 0; i < 10 && i < (int)test_string->size; i++) {
        char c = test_string->data[i];
        if (c >= 32 && c <= 126) {
            printf("'%c' ", c);
        } else {
            printf("\\x%02x ", (unsigned char)c);
        }
    }
    printf("\n");

    return RESULT_OK;
}

int main() {
    lkjagent_t* lkjagent = malloc(sizeof(lkjagent_t));

    if (!lkjagent) {
        RETURN_ERR("Failed to allocate memory for lkjagent");
    }

    if (lkjagent_init(lkjagent) != RESULT_OK) {
        RETURN_ERR("Failed to initialize lkjagent");
    }

    if (lkjagent_run(lkjagent) != RESULT_OK) {
        RETURN_ERR("Failed to run lkjagent");
    }

    return RESULT_OK;
}
