# LKJAgent API Reference

This document provides a comprehensive reference for all public APIs, data structures, and functions in the LKJAgent system.

## Table of Contents

- [Core Types](#core-types)
- [Result Types](#result-types)
- [Memory Pool API](#memory-pool-api)
- [String Utilities API](#string-utilities-api)
- [Object/JSON API](#objectjson-api)
- [File I/O API](#file-io-api)
- [HTTP Client API](#http-client-api)
- [Agent Core API](#agent-core-api)
- [Agent Actions API](#agent-actions-api)
- [Agent State API](#agent-state-api)
- [Error Handling](#error-handling)
- [Constants and Macros](#constants-and-macros)

## Core Types

### `result_t`
```c
typedef enum {
    RESULT_OK = 0,     // Operation completed successfully
    RESULT_ERR = 1,    // Operation failed
} result_t;
```

Standard result type for all API functions. All functions that can fail return this type.

### `string_t`
```c
typedef struct string_s {
    char* data;           // Pointer to string data
    uint64_t capacity;    // Total allocated capacity
    uint64_t size;        // Current string length
} string_t;
```

Dynamic string structure with explicit capacity management.

### `object_t`
```c
typedef struct object_t {
    string_t* string;           // String value (for leaf nodes)
    struct object_t* child;     // First child object (for containers)
    struct object_t* next;      // Next sibling object
} object_t;
```

JSON object representation using a tree structure. Can represent:
- **Leaf nodes**: Objects with string values
- **Container nodes**: Objects with child objects
- **Arrays**: Linked lists of sibling objects

### `pool_t`
```c
typedef struct {
    // String pools of different sizes
    char string16_data[POOL_STRING16_MAXCOUNT * 16];
    string_t string16[POOL_STRING16_MAXCOUNT];
    string_t* string16_freelist_data[POOL_STRING16_MAXCOUNT];
    uint64_t string16_freelist_count;
    
    // Similar structures for string256, string4096, string65536, string1048576
    
    // Object pool
    object_t object_data[POOL_OBJECT_MAXCOUNT];
    object_t* object_freelist_data[POOL_OBJECT_MAXCOUNT];
    uint64_t object_freelist_count;
} pool_t;
```

Memory pool managing pre-allocated strings and objects of various sizes.

### `config_t`
```c
typedef struct {
    object_t* data;    // Configuration data as JSON object tree
} config_t;
```

Configuration container holding all agent settings.

### `agent_t`
```c
typedef struct {
    object_t* data;    // Agent state and memory as JSON object tree
} agent_t;
```

Agent instance containing state, working memory, and storage.

### `lkjagent_t`
```c
typedef struct {
    pool_t pool;       // Memory pool for this agent instance
    config_t config;   // Agent configuration
    agent_t agent;     // Agent state and memory
} lkjagent_t;
```

Main LKJAgent instance containing all components.

## Result Types

All API functions return `result_t` to indicate success or failure:

- **`RESULT_OK`**: Operation completed successfully
- **`RESULT_ERR`**: Operation failed (see error logging for details)

## Memory Pool API

### Pool Initialization

#### `pool_init(pool_t* pool)`
```c
__attribute__((warn_unused_result)) result_t pool_init(pool_t* pool);
```

**Description:** Initializes memory pool structures and freelists.

**Parameters:**
- `pool`: Pointer to pool structure to initialize

**Returns:**
- `RESULT_OK`: Pool initialized successfully
- `RESULT_ERR`: Initialization failed

**Usage:**
```c
pool_t pool;
if (pool_init(&pool) != RESULT_OK) {
    // Handle initialization error
}
```

### String Allocation

#### `pool_string_alloc(pool_t* pool, string_t** string, uint64_t capacity)`
```c
__attribute__((warn_unused_result)) result_t pool_string_alloc(
    pool_t* pool, 
    string_t** string, 
    uint64_t capacity
);
```

**Description:** Allocates a string from the appropriate pool based on capacity.

**Parameters:**
- `pool`: Memory pool to allocate from
- `string`: Output pointer to allocated string
- `capacity`: Required string capacity

**Returns:**
- `RESULT_OK`: String allocated successfully
- `RESULT_ERR`: Allocation failed (pool exhausted)

**Size-Specific Allocation Functions:**
```c
result_t pool_string16_alloc(pool_t* pool, string_t** string);      // 16 bytes
result_t pool_string256_alloc(pool_t* pool, string_t** string);     // 256 bytes
result_t pool_string4096_alloc(pool_t* pool, string_t** string);    // 4KB
result_t pool_string65536_alloc(pool_t* pool, string_t** string);   // 64KB
result_t pool_string1048576_alloc(pool_t* pool, string_t** string); // 1MB
```

### String Deallocation

#### `pool_string_free(pool_t* pool, string_t* string)`
```c
__attribute__((warn_unused_result)) result_t pool_string_free(
    pool_t* pool, 
    string_t* string
);
```

**Description:** Returns a string to the appropriate pool.

**Parameters:**
- `pool`: Memory pool to return string to
- `string`: String to deallocate

**Returns:**
- `RESULT_OK`: String freed successfully
- `RESULT_ERR`: Free operation failed

### Object Allocation

#### `pool_object_alloc(pool_t* pool, object_t** object)`
```c
__attribute__((warn_unused_result)) result_t pool_object_alloc(
    pool_t* pool, 
    object_t** object
);
```

**Description:** Allocates an object from the object pool.

**Parameters:**
- `pool`: Memory pool to allocate from
- `object`: Output pointer to allocated object

**Returns:**
- `RESULT_OK`: Object allocated successfully
- `RESULT_ERR`: Allocation failed

#### `pool_object_free(pool_t* pool, object_t* object)`
```c
__attribute__((warn_unused_result)) result_t pool_object_free(
    pool_t* pool, 
    object_t* object
);
```

**Description:** Returns an object to the object pool.

**Parameters:**
- `pool`: Memory pool to return object to
- `object`: Object to deallocate

**Returns:**
- `RESULT_OK`: Object freed successfully
- `RESULT_ERR`: Free operation failed

## String Utilities API

### String Creation and Destruction

#### `string_create(pool_t* pool, string_t** string)`
```c
__attribute__((warn_unused_result)) result_t string_create(
    pool_t* pool, 
    string_t** string
);
```

**Description:** Creates a new empty string with default capacity.

#### `string_destroy(pool_t* pool, string_t* string)`
```c
__attribute__((warn_unused_result)) result_t string_destroy(
    pool_t* pool, 
    string_t* string
);
```

**Description:** Destroys a string and returns memory to pool.

### String Manipulation

#### `string_assign_str(string_t* string, const char* str)`
```c
__attribute__((warn_unused_result)) result_t string_assign_str(
    string_t* string, 
    const char* str
);
```

**Description:** Assigns C string content to a string_t.

#### `string_append_str(string_t* string, const char* str)`
```c
__attribute__((warn_unused_result)) result_t string_append_str(
    string_t* string, 
    const char* str
);
```

**Description:** Appends C string content to a string_t.

#### `string_append_char(string_t* string, char c)`
```c
__attribute__((warn_unused_result)) result_t string_append_char(
    string_t* string, 
    char c
);
```

**Description:** Appends a single character to a string_t.

### String Comparison

#### `string_equal_str(const string_t* string, const char* str)`
```c
uint64_t string_equal_str(const string_t* string, const char* str);
```

**Description:** Compares string_t with C string for equality.

**Returns:**
- `1`: Strings are equal
- `0`: Strings are different

#### `string_starts_with_str(const string_t* string, const char* prefix)`
```c
uint64_t string_starts_with_str(const string_t* string, const char* prefix);
```

**Description:** Checks if string_t starts with given prefix.

## Object/JSON API

### Object Creation and Destruction

#### `object_create(pool_t* pool, object_t** object)`
```c
__attribute__((warn_unused_result)) result_t object_create(
    pool_t* pool, 
    object_t** object
);
```

**Description:** Creates a new empty object.

#### `object_destroy(pool_t* pool, object_t* object)`
```c
__attribute__((warn_unused_result)) result_t object_destroy(
    pool_t* pool, 
    object_t* object
);
```

**Description:** Recursively destroys an object and all its children.

### JSON Parsing

#### `object_parse_json(pool_t* pool, object_t** object, const string_t* json)`
```c
__attribute__((warn_unused_result)) result_t object_parse_json(
    pool_t* pool, 
    object_t** object, 
    const string_t* json
);
```

**Description:** Parses JSON string into object tree structure.

**Parameters:**
- `pool`: Memory pool for allocations
- `object`: Output pointer to parsed object tree
- `json`: JSON string to parse

**Returns:**
- `RESULT_OK`: JSON parsed successfully
- `RESULT_ERR`: Parse error (invalid JSON)

### Object Navigation

#### `object_provide_str(pool_t* pool, object_t** result, object_t* obj, const char* path)`
```c
__attribute__((warn_unused_result)) result_t object_provide_str(
    pool_t* pool, 
    object_t** result, 
    object_t* obj, 
    const char* path
);
```

**Description:** Navigates object tree using dot-separated path.

**Parameters:**
- `pool`: Memory pool (may be needed for intermediate operations)
- `result`: Output pointer to found object
- `obj`: Root object to search from
- `path`: Dot-separated path (e.g., "agent.state.thinking")

**Example:**
```c
object_t* state_obj;
if (object_provide_str(pool, &state_obj, config_obj, "agent.state") == RESULT_OK) {
    // state_obj now points to the state configuration
}
```

#### `object_provide_string(object_t** result, object_t* obj, const string_t* key)`
```c
__attribute__((warn_unused_result)) result_t object_provide_string(
    object_t** result, 
    object_t* obj, 
    const string_t* key
);
```

**Description:** Finds child object by string_t key.

### Object Modification

#### `object_set_str(pool_t* pool, object_t* obj, const char* key, const char* value)`
```c
__attribute__((warn_unused_result)) result_t object_set_str(
    pool_t* pool, 
    object_t* obj, 
    const char* key, 
    const char* value
);
```

**Description:** Sets string value for given key in object.

#### `object_set_string(pool_t* pool, object_t* obj, const string_t* key, const string_t* value)`
```c
__attribute__((warn_unused_result)) result_t object_set_string(
    pool_t* pool, 
    object_t* obj, 
    const string_t* key, 
    const string_t* value
);
```

**Description:** Sets string_t value for string_t key in object.

## File I/O API

### File Reading

#### `file_read(pool_t* pool, const char* filename, string_t** content)`
```c
__attribute__((warn_unused_result)) result_t file_read(
    pool_t* pool, 
    const char* filename, 
    string_t** content
);
```

**Description:** Reads entire file content into a string_t.

**Parameters:**
- `pool`: Memory pool for string allocation
- `filename`: Path to file to read
- `content`: Output pointer to string containing file content

**Returns:**
- `RESULT_OK`: File read successfully
- `RESULT_ERR`: File not found, permission denied, or read error

### File Writing

#### `file_write(const char* filename, const string_t* content)`
```c
__attribute__((warn_unused_result)) result_t file_write(
    const char* filename, 
    const string_t* content
);
```

**Description:** Writes string_t content to file.

**Parameters:**
- `filename`: Path to file to write
- `content`: String content to write

**Returns:**
- `RESULT_OK`: File written successfully
- `RESULT_ERR`: Permission denied, disk full, or write error

## HTTP Client API

### HTTP Communication

#### `agent_http_send_receive(pool_t* pool, config_t* config, const string_t* prompt, string_t** response)`
```c
__attribute__((warn_unused_result)) result_t agent_http_send_receive(
    pool_t* pool, 
    config_t* config, 
    const string_t* prompt, 
    string_t** response
);
```

**Description:** Sends prompt to LLM endpoint and receives response.

**Parameters:**
- `pool`: Memory pool for response allocation
- `config`: Configuration containing LLM endpoint details
- `prompt`: Prompt to send to LLM
- `response`: Output pointer to LLM response

**Returns:**
- `RESULT_OK`: Communication successful
- `RESULT_ERR`: Network error, HTTP error, or timeout

**Configuration Requirements:**
The config object must contain:
- `llm.endpoint`: HTTP endpoint URL
- `llm.model`: Model name to use
- `llm.temperature`: Temperature parameter (optional)

## Agent Core API

### Main Agent Functions

#### `lkjagent_init(lkjagent_t* lkjagent)`
```c
__attribute__((warn_unused_result)) result_t lkjagent_init(lkjagent_t* lkjagent);
```

**Description:** Initializes complete LKJAgent system including pools, config, and agent state.

#### `lkjagent_agent(pool_t* pool, config_t* config, agent_t* agent)`
```c
__attribute__((warn_unused_result)) result_t lkjagent_agent(
    pool_t* pool, 
    config_t* config, 
    agent_t* agent
);
```

**Description:** Executes one complete agent cycle:
1. Generates prompt based on current state
2. Sends prompt to LLM
3. Processes LLM response
4. Updates agent state and memory
5. Executes any actions

**Parameters:**
- `pool`: Memory pool for allocations
- `config`: Agent configuration
- `agent`: Agent instance with current state

### Agent Command Processing

#### `lkjagent_agent_command(pool_t* pool, config_t* config, agent_t* agent, const string_t* response)`
```c
__attribute__((warn_unused_result)) result_t lkjagent_agent_command(
    pool_t* pool, 
    config_t* config, 
    agent_t* agent, 
    const string_t* response
);
```

**Description:** Processes LLM response and executes contained commands/actions.

## Agent Actions API

### Action Dispatch

#### `agent_actions_dispatch(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj)`
```c
__attribute__((warn_unused_result)) result_t agent_actions_dispatch(
    pool_t* pool, 
    config_t* config, 
    agent_t* agent, 
    object_t* action_obj
);
```

**Description:** Routes action to appropriate handler based on action type.

**Action Object Structure:**
```json
{
  "type": "action_type",
  "tags": ["tag1", "tag2"],
  "value": "action_value"
}
```

### Working Memory Actions

#### `agent_actions_command_working_memory_add(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj)`
```c
__attribute__((warn_unused_result)) result_t agent_actions_command_working_memory_add(
    pool_t* pool, 
    config_t* config, 
    agent_t* agent, 
    object_t* action_obj
);
```

**Description:** Adds information to agent's working memory.

#### `agent_actions_command_working_memory_remove(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj)`
```c
__attribute__((warn_unused_result)) result_t agent_actions_command_working_memory_remove(
    pool_t* pool, 
    config_t* config, 
    agent_t* agent, 
    object_t* action_obj
);
```

**Description:** Removes information from agent's working memory based on tags.

### Storage Actions

#### `agent_actions_command_storage_save(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj)`
```c
__attribute__((warn_unused_result)) result_t agent_actions_command_storage_save(
    pool_t* pool, 
    config_t* config, 
    agent_t* agent, 
    object_t* action_obj
);
```

**Description:** Saves information to persistent storage with tags.

#### `agent_actions_command_storage_load(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj)`
```c
__attribute__((warn_unused_result)) result_t agent_actions_command_storage_load(
    pool_t* pool, 
    config_t* config, 
    agent_t* agent, 
    object_t* action_obj
);
```

**Description:** Loads information from persistent storage based on tags.

#### `agent_actions_command_storage_search(pool_t* pool, config_t* config, agent_t* agent, object_t* action_obj)`
```c
__attribute__((warn_unused_result)) result_t agent_actions_command_storage_search(
    pool_t* pool, 
    config_t* config, 
    agent_t* agent, 
    object_t* action_obj
);
```

**Description:** Searches persistent storage for information matching tags.

### Memory Management

#### `agent_actions_save_memory(pool_t* pool, agent_t* agent)`
```c
__attribute__((warn_unused_result)) result_t agent_actions_save_memory(
    pool_t* pool, 
    agent_t* agent
);
```

**Description:** Persists current agent memory to disk.

### Response Processing

#### `agent_actions_parse_response(pool_t* pool, const string_t* response_content, object_t** response_obj)`
```c
__attribute__((warn_unused_result)) result_t agent_actions_parse_response(
    pool_t* pool, 
    const string_t* response_content, 
    object_t** response_obj
);
```

**Description:** Parses LLM response into structured object for processing.

## Agent State API

### State Management

#### `agent_state_update_state(pool_t* pool, agent_t* agent, const char* new_state)`
```c
__attribute__((warn_unused_result)) result_t agent_state_update_state(
    pool_t* pool, 
    agent_t* agent, 
    const char* new_state
);
```

**Description:** Updates agent's current state.

**Valid States:**
- `"thinking"`: Analysis and planning phase
- `"evaluating"`: Progress assessment phase  
- `"commanding"`: Action execution phase

#### `agent_state_auto_transition(pool_t* pool, config_t* config, agent_t* agent)`
```c
__attribute__((warn_unused_result)) result_t agent_state_auto_transition(
    pool_t* pool, 
    config_t* config, 
    agent_t* agent
);
```

**Description:** Automatically transitions from commanding to evaluating state.

### Log Management

#### `agent_state_update_and_log(pool_t* pool, config_t* config, agent_t* agent, object_t* response_obj)`
```c
__attribute__((warn_unused_result)) result_t agent_state_update_and_log(
    pool_t* pool, 
    config_t* config, 
    agent_t* agent, 
    object_t* response_obj
);
```

**Description:** Updates agent state and manages log entries with rotation.

#### `agent_state_sync_logs_to_working_memory(pool_t* pool, agent_t* agent)`
```c
__attribute__((warn_unused_result)) result_t agent_state_sync_logs_to_working_memory(
    pool_t* pool, 
    agent_t* agent
);
```

**Description:** Synchronizes all logs with working memory for consistent access.

### Memory Estimation

#### `agent_state_estimate_tokens(pool_t* pool, agent_t* agent, uint64_t* token_count)`
```c
__attribute__((warn_unused_result)) result_t agent_state_estimate_tokens(
    pool_t* pool, 
    agent_t* agent, 
    uint64_t* token_count
);
```

**Description:** Estimates token count for current agent memory to manage LLM context limits.

## Error Handling

### Error Reporting Macro

#### `RETURN_ERR(message)`
```c
#define RETURN_ERR(message) do { \
    printf("ERROR: %s\n", message); \
    return RESULT_ERR; \
} while(0)
```

**Description:** Standard error reporting and return macro used throughout the codebase.

### Error Handling Best Practices

1. **Always check return values** from API functions
2. **Use descriptive error messages** with RETURN_ERR
3. **Clean up resources** on error paths
4. **Propagate errors** up the call stack appropriately

**Example:**
```c
result_t example_function(pool_t* pool) {
    string_t* temp_string;
    
    if (string_create(pool, &temp_string) != RESULT_OK) {
        RETURN_ERR("Failed to create temporary string");
    }
    
    // Use temp_string...
    
    if (string_destroy(pool, temp_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy temporary string");
    }
    
    return RESULT_OK;
}
```

## Constants and Macros

### Memory Pool Constants

```c
#define BUF_CAPACITY (1024 * 1024 * 4)      // 4MB buffer capacity
#define POOL_STRING16_MAXCOUNT (1048576)     // 1M small strings
#define POOL_STRING256_MAXCOUNT (65536)      // 64K medium strings  
#define POOL_STRING4096_MAXCOUNT (4096)      // 4K large strings
#define POOL_STRING65536_MAXCOUNT (256)      // 256 very large strings
#define POOL_STRING1048576_MAXCOUNT (16)     // 16 huge strings
#define POOL_OBJECT_MAXCOUNT (65536)         // 64K objects
```

### File Paths

```c
#define CONFIG_PATH "data/config.json"       // Configuration file path
#define MEMORY_PATH "data/memory.json"       // Agent memory file path
```

### Function Attributes

```c
__attribute__((warn_unused_result))         // Compiler warning if return value ignored
```

All functions that can fail use this attribute to ensure error checking.

---

This API reference provides complete documentation for integrating with and extending the LKJAgent system. For implementation examples, see the source code and main documentation.
