#include "lkjagent.h"

static __attribute__((warn_unused_result)) result_t lkjagent_init(lkjagent_t* lkjagent) {
    if (pool_init(&lkjagent->pool) != RESULT_OK) {
        RETURN_ERR("Failed to initialize memory pool");
    }

    return RESULT_OK;
}

static __attribute__((warn_unused_result)) result_t lkjagent_run(lkjagent_t* lkjagent) {

    string_t* config_string;
    object_t* config_object;

    string_t* test_string;
    string_t* test_path;
    object_t* test_object;

    // LM Studio integration variables
    string_t* endpoint_string;
    string_t* model_string;
    string_t* user_message;
    object_t* llm_config;
    object_t* chat_request;
    object_t* chat_response;

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

    if(object_parse_json(&lkjagent->pool, &config_object, config_string) != RESULT_OK) {
        RETURN_ERR("Failed to parse config JSON");
    }

    if(object_get(&test_object, config_object, test_path) != RESULT_OK) {
        RETURN_ERR("Failed to get object from config");
    }

    // Destroy and recreate the test_string to ensure it's clean
    if(string_destroy(&lkjagent->pool, test_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy test string");
    }
    
    if(string_create(&lkjagent->pool, &test_string) != RESULT_OK) {
        RETURN_ERR("Failed to recreate test string");
    }

    if(object_tostring_json(&lkjagent->pool, &test_string, test_object) != RESULT_OK) {
        RETURN_ERR("Failed to convert object to JSON string");
    }

    // Now demonstrate LM Studio integration
    printf("\n=== LM Studio Integration Test ===\n");

    // Get LM Studio configuration
    string_t* llm_path;
    if(string_create_str(&lkjagent->pool, &llm_path, "llm") != RESULT_OK) {
        RETURN_ERR("Failed to create llm path string");
    }

    if(object_get(&llm_config, config_object, llm_path) != RESULT_OK) {
        RETURN_ERR("Failed to get LLM config from config");
    }

    // Extract endpoint and model
    string_t* endpoint_path;
    string_t* model_path;
    object_t* endpoint_obj;
    object_t* model_obj;
    
    if(string_create_str(&lkjagent->pool, &endpoint_path, "endpoint") != RESULT_OK) {
        RETURN_ERR("Failed to create endpoint path string");
    }
    
    if(string_create_str(&lkjagent->pool, &model_path, "model") != RESULT_OK) {
        RETURN_ERR("Failed to create model path string");
    }

    if(object_get(&endpoint_obj, llm_config, endpoint_path) != RESULT_OK) {
        RETURN_ERR("Failed to get endpoint from LLM config");
    }

    if(object_get(&model_obj, llm_config, model_path) != RESULT_OK) {
        RETURN_ERR("Failed to get model from LLM config");
    }

    // Create strings from the config values
    if(string_create_string(&lkjagent->pool, &endpoint_string, endpoint_obj->string) != RESULT_OK) {
        RETURN_ERR("Failed to create endpoint string");
    }

    if(string_create_string(&lkjagent->pool, &model_string, model_obj->string) != RESULT_OK) {
        RETURN_ERR("Failed to create model string");
    }

    // Create a test message
    if(string_create_str(&lkjagent->pool, &user_message, "Hello! Please respond with a simple greeting.") != RESULT_OK) {
        RETURN_ERR("Failed to create user message");
    }

    printf("Endpoint: %.*s\n", (int)endpoint_string->size, endpoint_string->data);
    printf("Model: %.*s\n", (int)model_string->size, model_string->data);
    printf("User message: %.*s\n", (int)user_message->size, user_message->data);

    // Create chat completion request
    if(lmstudio_create_chat_request(&lkjagent->pool, model_string, user_message, 0.7, &chat_request) != RESULT_OK) {
        RETURN_ERR("Failed to create chat request");
    }

    printf("\n--- Sending request to LM Studio ---\n");

    // Send chat completion request to LM Studio
    if(lmstudio_chat_completion(&lkjagent->pool, endpoint_string, chat_request, &chat_response) != RESULT_OK) {
        printf("Failed to send chat completion request to LM Studio. This is expected if LM Studio is not running.\n");
    } else {
        printf("--- LM Studio Response ---\n");
        
        // Convert response to JSON and print
        string_t* response_json;
        if(object_tostring_json(&lkjagent->pool, &response_json, chat_response) != RESULT_OK) {
            RETURN_ERR("Failed to convert response to JSON");
        }
        
        printf("Response JSON: %.*s\n", (int)response_json->size, response_json->data);
        
        // Try to extract the message content
        string_t* choices_path;
        string_t* content_path;
        object_t* choices_obj;
        object_t* content_obj;
        
        if(string_create_str(&lkjagent->pool, &choices_path, "choices") == RESULT_OK &&
           object_get(&choices_obj, chat_response, choices_path) == RESULT_OK &&
           choices_obj->child != NULL) {
            
            if(string_create_str(&lkjagent->pool, &content_path, "message.content") == RESULT_OK &&
               object_get(&content_obj, choices_obj->child, content_path) == RESULT_OK) {
                printf("AI Response: %.*s\n", (int)content_obj->string->size, content_obj->string->data);
            }
        }
    }

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
