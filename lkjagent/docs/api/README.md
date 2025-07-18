# API Reference

Complete API documentation for LKJAgent modules.

## Core Types

### Basic Types

```c
typedef enum {
    RESULT_OK = 0,    /**< Operation completed successfully */
    RESULT_ERR = 1    /**< General error */
} result_t;

typedef struct {
    char* data;       /**< Pointer to character buffer */
    size_t capacity;  /**< Maximum buffer size including null terminator */
    size_t size;      /**< Current string length (excluding null terminator) */
} data_t;
```

## Module APIs

### Data Token Management (`src/utils/data.c`)

Safe string handling with bounded operations:

```c
result_t data_init(data_t* token, char* buffer, size_t capacity);
result_t data_set(data_t* token, const char* str);
result_t data_append(data_t* token, const char* str);
result_t data_find(const data_t* token, const char* substr, size_t* position);
result_t data_substring(const data_t* source, size_t start, size_t length, data_t* dest);
int data_compare(const data_t* a, const data_t* b);
```

### Tagged Memory (`src/memory/tagged_memory.c`)

Advanced memory system with multi-tag support:

```c
result_t tagged_memory_init(tagged_memory_t* memory);
result_t tagged_memory_add_entry(tagged_memory_t* memory, const char* content, const char** tags, size_t tag_count);
result_t tagged_memory_query(const tagged_memory_t* memory, const char** tags, size_t tag_count, memory_entry_t** results, size_t* result_count);
result_t tagged_memory_cleanup(tagged_memory_t* memory, double threshold);
```

### Configuration Management (`src/config/config.c`)

Configuration loading and validation:

```c
result_t config_init(config_t* config);
result_t config_load_from_file(config_t* config, const char* filename);
result_t config_save_to_file(const config_t* config, const char* filename);
result_t config_validate(const config_t* config);
```

### LLM Integration (`src/llm/llm_client.c`)

LLM communication interface:

```c
result_t llm_client_init(llm_client_t* client, const config_t* config);
result_t llm_send_request(const llm_client_t* client, const data_t* prompt, data_t* response);
result_t llm_parse_response(const data_t* response, llm_response_t* parsed);
```

### State Machine (`src/agent/core.c`)

Agent core functionality:

```c
result_t agent_init(lkjagent_t* agent, const config_t* config);
result_t agent_run_cycle(lkjagent_t* agent);
result_t agent_transition_state(lkjagent_t* agent, agent_state_t new_state);
```

## Error Handling

All functions return `result_t` and use the `RETURN_ERR` macro for error reporting:

```c
#define RETURN_ERR(error_message) \
    do { \
        /* Concrete error reporting implementation */ \
        write(STDERR_FILENO, error_message, strlen(error_message)); \
    } while(0)
```

## Memory Safety Guidelines

- All functions validate input parameters
- Buffer operations respect capacity limits
- No dynamic memory allocation
- Null termination guaranteed
- Use `__attribute__((warn_unused_result))` for error-prone functions
