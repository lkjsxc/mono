#include "lkjagent.h"
#include <stdbool.h>

// Helper macro for cleanup that ignores return values (suppresses warnings)
#define CLEANUP_IGNORE_RESULT(call) do { \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wunused-result\"") \
    call; \
    _Pragma("GCC diagnostic pop") \
} while(0)

// Helper function to create a string key-value pair
static result_t create_kv_pair(pool_t* pool, const char* key, const char* value, object_t** pair) {
    if (pool_object_alloc(pool, pair) != RESULT_OK) {
        return RESULT_ERR;
    }
    if (string_create_str(pool, &(*pair)->string, key) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    object_t* val_obj;
    if (pool_object_alloc(pool, &val_obj) != RESULT_OK) {
        return RESULT_ERR;
    }
    if (string_create_str(pool, &val_obj->string, value) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    (*pair)->child = val_obj;
    (*pair)->next = NULL;
    return RESULT_OK;
}

// Helper function to create a string_t value key-value pair
static result_t create_kv_string_pair(pool_t* pool, const char* key, const string_t* value, object_t** pair) {
    if (pool_object_alloc(pool, pair) != RESULT_OK) {
        return RESULT_ERR;
    }
    if (string_create_str(pool, &(*pair)->string, key) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    object_t* val_obj;
    if (pool_object_alloc(pool, &val_obj) != RESULT_OK) {
        return RESULT_ERR;
    }
    if (string_create_string(pool, &val_obj->string, value) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    (*pair)->child = val_obj;
    (*pair)->next = NULL;
    return RESULT_OK;
}

// Helper function to create a boolean key-value pair
static result_t create_kv_bool_pair(pool_t* pool, const char* key, bool value, object_t** pair) {
    if (pool_object_alloc(pool, pair) != RESULT_OK) {
        return RESULT_ERR;
    }
    if (string_create_str(pool, &(*pair)->string, key) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    object_t* val_obj;
    if (pool_object_alloc(pool, &val_obj) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    const char* val_str = value ? "true" : "false";
    if (string_create_str(pool, &val_obj->string, val_str) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    (*pair)->child = val_obj;
    (*pair)->next = NULL;
    return RESULT_OK;
}

// Helper function to create a numeric key-value pair
static result_t create_kv_number_pair(pool_t* pool, const char* key, double value, object_t** pair) {
    if (pool_object_alloc(pool, pair) != RESULT_OK) {
        return RESULT_ERR;
    }
    if (string_create_str(pool, &(*pair)->string, key) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    object_t* val_obj;
    if (pool_object_alloc(pool, &val_obj) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    char val_str[32];
    if (value == (int)value) {
        snprintf(val_str, sizeof(val_str), "%d", (int)value);
    } else {
        snprintf(val_str, sizeof(val_str), "%.2f", value);
    }
    if (string_create_str(pool, &val_obj->string, val_str) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    (*pair)->child = val_obj;
    (*pair)->next = NULL;
    return RESULT_OK;
}

// LM Studio specific function to create chat request
static result_t lmstudio_create_chat_request(pool_t* pool, const string_t* model, const string_t* message, double temperature, object_t** request_data) {
    // Create the main request object
    if (object_create(pool, request_data) != RESULT_OK) {
        RETURN_ERR("Failed to create request object");
    }
    
    // Create model pair
    object_t* model_pair;
    if (create_kv_string_pair(pool, "model", model, &model_pair) != RESULT_OK) {
        RETURN_ERR("Failed to create model pair");
    }
    
    // Create messages array
    object_t* messages_pair;
    if (pool_object_alloc(pool, &messages_pair) != RESULT_OK) {
        RETURN_ERR("Failed to allocate messages pair");
    }
    if (string_create_str(pool, &messages_pair->string, "messages") != RESULT_OK) {
        RETURN_ERR("Failed to create messages key");
    }
    
    // Create messages array container
    object_t* messages_array;
    if (pool_object_alloc(pool, &messages_array) != RESULT_OK) {
        RETURN_ERR("Failed to allocate messages array");
    }
    messages_array->string = NULL; // Array has no string key
    
    // Create user message object
    object_t* user_message_obj;
    if (pool_object_alloc(pool, &user_message_obj) != RESULT_OK) {
        RETURN_ERR("Failed to allocate user message object");
    }
    user_message_obj->string = NULL; // Object in array has no key
    
    // Create role and content pairs for user message
    object_t* role_pair;
    object_t* content_pair;
    if (create_kv_pair(pool, "role", "user", &role_pair) != RESULT_OK) {
        RETURN_ERR("Failed to create role pair");
    }
    if (create_kv_string_pair(pool, "content", message, &content_pair) != RESULT_OK) {
        RETURN_ERR("Failed to create content pair");
    }
    
    // Link user message structure
    role_pair->next = content_pair;
    user_message_obj->child = role_pair;
    
    // Set up messages array
    messages_array->child = user_message_obj;
    messages_pair->child = messages_array;
    
    // Create temperature pair
    object_t* temp_pair;
    if (create_kv_number_pair(pool, "temperature", temperature, &temp_pair) != RESULT_OK) {
        RETURN_ERR("Failed to create temperature pair");
    }
    
    // Create max_tokens pair (use -1 as in the example)
    object_t* max_tokens_pair;
    if (create_kv_number_pair(pool, "max_tokens", -1, &max_tokens_pair) != RESULT_OK) {
        RETURN_ERR("Failed to create max_tokens pair");
    }
    
    // Create stream pair (boolean false)
    object_t* stream_pair;
    if (create_kv_bool_pair(pool, "stream", false, &stream_pair) != RESULT_OK) {
        RETURN_ERR("Failed to create stream pair");
    }
    
    // Link all pairs together
    model_pair->next = messages_pair;
    messages_pair->next = temp_pair;
    temp_pair->next = max_tokens_pair;
    max_tokens_pair->next = stream_pair;
    
    // Set the root object children
    (*request_data)->child = model_pair;
    
    return RESULT_OK;
}

// LM Studio specific function to handle chat completion
static result_t lmstudio_chat_completion(pool_t* pool, const string_t* endpoint, const object_t* request_data, object_t** response_data) {
    string_t* json_request;
    string_t* content_type;
    http_response_t* http_response;
    
    // Convert request data to JSON
    if (object_tostring_json(pool, &json_request, request_data) != RESULT_OK) {
        RETURN_ERR("Failed to convert request to JSON");
    }
    
    // Set content type
    if (string_create_str(pool, &content_type, "application/json") != RESULT_OK) {
        CLEANUP_IGNORE_RESULT(string_destroy(pool, json_request));
        RETURN_ERR("Failed to create content type string");
    }
    
    // Send HTTP POST request
    if (http_post(pool, endpoint, content_type, json_request, &http_response) != RESULT_OK) {
        CLEANUP_IGNORE_RESULT(string_destroy(pool, json_request));
        CLEANUP_IGNORE_RESULT(string_destroy(pool, content_type));
        RETURN_ERR("Failed to send HTTP POST request");
    }
    
    // Check HTTP status
    if (http_response->status_code != 200) {
        printf("HTTP Error: Status code %d\n", http_response->status_code);
        if (http_response->body) {
            printf("Response body: %.*s\n", (int)http_response->body->size, http_response->body->data);
        }
        CLEANUP_IGNORE_RESULT(http_response_destroy(pool, http_response));
        CLEANUP_IGNORE_RESULT(string_destroy(pool, json_request));
        CLEANUP_IGNORE_RESULT(string_destroy(pool, content_type));
        RETURN_ERR("HTTP request failed with non-200 status");
    }
    
    // Parse JSON response
    if (!http_response->body || http_response->body->size == 0) {
        CLEANUP_IGNORE_RESULT(http_response_destroy(pool, http_response));
        CLEANUP_IGNORE_RESULT(string_destroy(pool, json_request));
        CLEANUP_IGNORE_RESULT(string_destroy(pool, content_type));
        RETURN_ERR("Empty response body");
    }
    
    if (object_parse_json(pool, response_data, http_response->body) != RESULT_OK) {
        CLEANUP_IGNORE_RESULT(http_response_destroy(pool, http_response));
        CLEANUP_IGNORE_RESULT(string_destroy(pool, json_request));
        CLEANUP_IGNORE_RESULT(string_destroy(pool, content_type));
        RETURN_ERR("Failed to parse JSON response");
    }
    
    // Cleanup
    CLEANUP_IGNORE_RESULT(http_response_destroy(pool, http_response));
    CLEANUP_IGNORE_RESULT(string_destroy(pool, json_request));
    CLEANUP_IGNORE_RESULT(string_destroy(pool, content_type));
    
    return RESULT_OK;
}

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
