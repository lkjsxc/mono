# API Specifications

## Core Type System

### Basic Types

```c
typedef enum {
    RESULT_OK = 0,    /**< Operation completed successfully */
    RESULT_ERR = 1    /**< General error */
} result_t;

typedef struct {
    char* data;       /**< Pointer to character buffer */
    size_t size;      /**< Current string length (excluding null terminator) */
    size_t capacity;  /**< Maximum buffer capacity */
} data_t;
```

## Data Token API

### Core Functions

```c
/**
 * @brief Initialize a data token with a buffer
 */
__attribute__((warn_unused_result))
result_t data_init(data_t* token, char* buffer, size_t capacity);

/**
 * @brief Set the content of a data token
 */
__attribute__((warn_unused_result))
result_t data_set(data_t* token, const char* value);

/**
 * @brief Append content to a data token
 */
__attribute__((warn_unused_result))
result_t data_append(data_t* token, const char* value);
```

## Memory System API

### Tagged Memory

```c
/**
 * @brief Initialize the memory system
 */
__attribute__((warn_unused_result))
result_t memory_init(memory_system_t* system);

/**
 * @brief Add an entry with multiple tags
 */
__attribute__((warn_unused_result))
result_t memory_add_entry(memory_system_t* system, const char* content, 
                         const char** tags, size_t tag_count);

/**
 * @brief Query entries matching all specified tags (AND operation)
 */
__attribute__((warn_unused_result))
result_t memory_query_and(memory_system_t* system, const char** tags, 
                         size_t tag_count, memory_result_t* result);
```

## Configuration API

```c
/**
 * @brief Load configuration from file
 */
__attribute__((warn_unused_result))
result_t config_load_from_file(config_t* config, const char* filepath);

/**
 * @brief Save configuration to file
 */
__attribute__((warn_unused_result))
result_t config_save_to_file(const config_t* config, const char* filepath);
```

## LLM Integration API

```c
/**
 * @brief Send a request to the LLM
 */
__attribute__((warn_unused_result))
result_t llm_send_request(llm_client_t* client, const char* prompt, 
                         data_t* response);

/**
 * @brief Parse LLM response using simple tag format
 */
__attribute__((warn_unused_result))
result_t tag_parse_thinking(const char* response, thinking_data_t* thinking);
```

## Error Handling

### RETURN_ERR Macro

```c
#define RETURN_ERR3(n) #n
#define RETURN_ERR2(n) RETURN_ERR3(n)
#define RETURN_ERR(error_message)                                                   \
    do {                                                                            \
        const char* msg = "[ERROR:" __FILE__ ":" RETURN_ERR2(__LINE__) "] " error_message "\n"; \
        write(STDERR_FILENO, msg, strlen(msg));                                     \
    } while(0)
```

## Constants

```c
#define MAX_BUFFER_SIZE 4096
#define MAX_TAGS_PER_ENTRY 16
#define MAX_MEMORY_ENTRIES 1000
#define MAX_PATH_LENGTH 512
#define MAX_CONTEXT_KEYS 128
```

## File Formats

### memory.json Structure

```json
{
    "working_memory": [
        {
            "id": "unique_id",
            "content": "entry content",
            "tags": ["tag1", "tag2"],
            "timestamp": "2025-01-18T12:00:00Z",
            "context_key": "optional_key"
        }
    ],
    "disk_memory": [
        {
            "id": "unique_id",
            "content": "archived content",
            "tags": ["tag1", "tag2"],
            "timestamp": "2025-01-18T12:00:00Z",
            "context_key": "key"
        }
    ]
}
```

### config.json Structure

```json
{
    "agent": {
        "max_iterations": -1,
        "state_timeout_seconds": 300
    },
    "llm": {
        "endpoint": "http://localhost:1234/v1/chat/completions",
        "model": "local-model",
        "max_tokens": 2048
    },
    "memory": {
        "max_entries": 1000,
        "max_tags_per_entry": 16
    },
    "prompts": {
        "thinking_state": "system prompt for thinking...",
        "executing_state": "system prompt for executing...",
        "evaluating_state": "system prompt for evaluating...",
        "paging_state": "system prompt for paging..."
    }
}
```
