# API Reference Documentation

## Overview

This document provides comprehensive API reference for all public functions and data structures in LKJAgent. The API is organized into functional modules with consistent error handling and memory management patterns.

## Common Patterns

### Function Return Type
All functions return `result_t`:
```c
typedef enum {
    RESULT_OK = 0,   // Success
    RESULT_ERR = 1,  // Error occurred
} result_t;
```

### Error Handling Pattern
```c
if (function_call(args) != RESULT_OK) {
    // Handle error, cleanup resources
    RETURN_ERR("Descriptive error message");
}
```

### Memory Management Pattern
- All allocations go through memory pool
- Resources must be explicitly freed
- RAII-style cleanup on error paths

## Core Agent API

### lkjagent_agent()
Main agent processing function that orchestrates one complete agent cycle.

```c
__attribute__((warn_unused_result)) 
result_t lkjagent_agent(pool_t* pool, config_t* config, agent_t* agent);
```

**Parameters:**
- `pool`: Memory pool for allocations
- `config`: Configuration object with LLM and agent settings
- `agent`: Agent state including working memory and storage

**Returns:**
- `RESULT_OK`: Agent cycle completed successfully
- `RESULT_ERR`: Error occurred during processing

**Description:**
Executes a complete agent processing cycle:
1. Extract configuration and memory context
2. Generate prompt based on current state
3. Send HTTP request to LLM endpoint
4. Parse XML response from LLM
5. Execute actions based on response
6. Update agent state and memory
7. Persist changes to storage

**Example:**
```c
lkjagent_t lkjagent;
if (lkjagent_init(&lkjagent) != RESULT_OK) {
    return RESULT_ERR;
}

for (int i = 0; i < 5; i++) {
    if (lkjagent_agent(&lkjagent.pool, &lkjagent.config, &lkjagent.agent) != RESULT_OK) {
        return RESULT_ERR;
    }
}
```

## Memory Pool API

### pool_init()
Initialize memory pool with all sub-pools and freelists.

```c
__attribute__((warn_unused_result)) 
result_t pool_init(pool_t* pool);
```

**Parameters:**
- `pool`: Uninitialized pool structure

**Returns:**
- `RESULT_OK`: Pool initialized successfully
- `RESULT_ERR`: Initialization failed

**Description:**
Sets up all memory pools and initializes freelists for efficient allocation.

### String Pool Functions

#### pool_string_alloc()
Allocate string from appropriate pool based on capacity requirement.

```c
__attribute__((warn_unused_result)) 
result_t pool_string_alloc(pool_t* pool, string_t** string, uint64_t capacity);
```

**Parameters:**
- `pool`: Memory pool instance
- `string`: Output pointer for allocated string
- `capacity`: Required string capacity

**Returns:**
- `RESULT_OK`: String allocated successfully
- `RESULT_ERR`: Allocation failed (pool exhausted)

#### pool_string_free()
Return string to appropriate freelist.

```c
__attribute__((warn_unused_result)) 
result_t pool_string_free(pool_t* pool, string_t* string);
```

**Parameters:**
- `pool`: Memory pool instance
- `string`: String to deallocate

**Returns:**
- `RESULT_OK`: String freed successfully
- `RESULT_ERR`: Deallocation failed

#### pool_string_realloc()
Resize string, potentially moving to different pool.

```c
__attribute__((warn_unused_result)) 
result_t pool_string_realloc(pool_t* pool, string_t** string, uint64_t capacity);
```

**Parameters:**
- `pool`: Memory pool instance
- `string`: Pointer to string pointer (may be updated)
- `capacity`: New required capacity

**Returns:**
- `RESULT_OK`: String resized successfully
- `RESULT_ERR`: Reallocation failed

#### Specific Pool Allocators

```c
result_t pool_string16_alloc(pool_t* pool, string_t** string);
result_t pool_string256_alloc(pool_t* pool, string_t** string);
result_t pool_string4096_alloc(pool_t* pool, string_t** string);
result_t pool_string65536_alloc(pool_t* pool, string_t** string);
result_t pool_string1048576_alloc(pool_t* pool, string_t** string);
```

### Object Pool Functions

#### pool_object_alloc()
Allocate object from object pool.

```c
__attribute__((warn_unused_result)) 
result_t pool_object_alloc(pool_t* pool, object_t** object);
```

#### pool_object_free()
Return object to object pool freelist.

```c
__attribute__((warn_unused_result)) 
result_t pool_object_free(pool_t* pool, object_t* object);
```

## String API

### String Creation and Destruction

#### string_create()
Create new empty string.

```c
__attribute__((warn_unused_result)) 
result_t string_create(pool_t* pool, string_t** string);
```

#### string_destroy()
Destroy string and return to pool.

```c
__attribute__((warn_unused_result)) 
result_t string_destroy(pool_t* pool, string_t* string);
```

#### string_create_str()
Create string initialized with C string content.

```c
__attribute__((warn_unused_result)) 
result_t string_create_str(pool_t* pool, string_t** string, const char* src);
```

#### string_create_string()
Create string initialized with another string's content.

```c
__attribute__((warn_unused_result)) 
result_t string_create_string(pool_t* pool, string_t** string, const string_t* src);
```

### String Content Manipulation

#### string_set_str()
Set string content from C string.

```c
__attribute__((warn_unused_result)) 
result_t string_set_str(string_t* string, const char* src);
```

#### string_append_str()
Append C string to existing string.

```c
__attribute__((warn_unused_result)) 
result_t string_append_str(pool_t* pool, string_t** string, const char* src);
```

#### string_append_string()
Append another string to existing string.

```c
__attribute__((warn_unused_result)) 
result_t string_append_string(pool_t* pool, string_t** string, const string_t* src);
```

#### string_resize()
Resize string capacity.

```c
__attribute__((warn_unused_result)) 
result_t string_resize(pool_t* pool, string_t** string, uint64_t new_capacity);
```

## Object API

### Object Creation and Destruction

#### object_create()
Create new empty object.

```c
__attribute__((warn_unused_result)) 
result_t object_create(pool_t* pool, object_t** dst);
```

#### object_destroy()
Recursively destroy object and all children.

```c
__attribute__((warn_unused_result)) 
result_t object_destroy(pool_t* pool, object_t* object);
```

### JSON/XML Processing

#### object_parse_json()
Parse JSON string into object hierarchy.

```c
__attribute__((warn_unused_result)) 
result_t object_parse_json(pool_t* pool, object_t** dst, const string_t* src);
```

#### object_parse_xml()
Parse XML string into object hierarchy.

```c
__attribute__((warn_unused_result)) 
result_t object_parse_xml(pool_t* pool, object_t** dst, const string_t* src);
```

#### object_tostring_json()
Serialize object hierarchy to JSON string.

```c
__attribute__((warn_unused_result)) 
result_t object_tostring_json(pool_t* pool, string_t** dst, const object_t* src);
```

#### object_tostring_xml()
Serialize object hierarchy to XML string.

```c
__attribute__((warn_unused_result)) 
result_t object_tostring_xml(pool_t* pool, string_t** dst, const object_t* src);
```

### Object Navigation and Manipulation

#### object_set()
Set object value at specified path.

```c
__attribute__((warn_unused_result)) 
result_t object_set(pool_t* pool, object_t* object, const string_t* path, object_t* value);
```

#### object_set_string()
Set string value at specified path.

```c
__attribute__((warn_unused_result)) 
result_t object_set_string(pool_t* pool, object_t* object, const string_t* path, const string_t* value);
```

#### object_get()
Get object at specified path.

```c
__attribute__((warn_unused_result)) 
result_t object_get(object_t** dst, const object_t* object, const string_t* path);
```

#### object_provide_string()
Get object at path, creating path if it doesn't exist.

```c
__attribute__((warn_unused_result)) 
result_t object_provide_string(object_t** dst, const object_t* object, const string_t* path);
```

#### object_provide_str()
Convenience function for C string paths.

```c
__attribute__((warn_unused_result)) 
result_t object_provide_str(pool_t* pool, object_t** dst, const object_t* object, const char* path);
```

**Path Format:**
- Dot notation: `"parent.child.grandchild"`
- Array access: `"array.0.property"`
- Root access: `"property"`

## File I/O API

### file_read()
Read entire file content into string.

```c
__attribute__((warn_unused_result)) 
result_t file_read(pool_t* pool, const char* path, string_t** content);
```

**Parameters:**
- `pool`: Memory pool for string allocation
- `path`: File path to read
- `content`: Output string with file content

### file_write()
Write string content to file.

```c
__attribute__((warn_unused_result)) 
result_t file_write(const char* path, const string_t* content);
```

**Parameters:**
- `path`: File path to write
- `content`: String content to write

## HTTP Client API

### http_get()
Perform HTTP GET request.

```c
__attribute__((warn_unused_result)) 
result_t http_get(pool_t* pool, const string_t* url, string_t** response);
```

**Parameters:**
- `pool`: Memory pool for response allocation
- `url`: URL to request
- `response`: Output string with response body

### http_post()
Perform HTTP POST request.

```c
__attribute__((warn_unused_result)) 
result_t http_post(pool_t* pool, const string_t* url, const string_t* content_type, 
                   const string_t* body, string_t** response);
```

**Parameters:**
- `pool`: Memory pool for response allocation
- `url`: URL to request
- `content_type`: Content-Type header value
- `body`: Request body content
- `response`: Output string with response body

**Example:**
```c
string_t* url;
string_t* content_type;
string_t* body;
string_t* response;

string_create_str(pool, &url, "http://localhost:1234/v1/chat/completions");
string_create_str(pool, &content_type, "application/json");
string_create_str(pool, &body, "{\"model\":\"gpt-3.5-turbo\",\"messages\":[...]}");

if (http_post(pool, url, content_type, body, &response) == RESULT_OK) {
    printf("Response: %.*s\n", (int)response->size, response->data);
}

// Cleanup all strings...
```

## Data Structures

### pool_t
Memory pool containing all allocators and freelists.

```c
typedef struct {
    // String pools for different sizes
    char string16_data[POOL_STRING16_MAXCOUNT * 16];
    string_t string16[POOL_STRING16_MAXCOUNT];
    string_t* string16_freelist_data[POOL_STRING16_MAXCOUNT];
    uint64_t string16_freelist_count;
    
    // ... similar for other string sizes
    
    // Object pool
    object_t object_data[POOL_OBJECT_MAXCOUNT];
    object_t* object_freelist_data[POOL_OBJECT_MAXCOUNT];
    uint64_t object_freelist_count;
} pool_t;
```

### string_t
Dynamic string with capacity management.

```c
typedef struct string_s {
    char* data;        // Character buffer
    uint64_t capacity; // Maximum size
    uint64_t size;     // Current used size
} string_t;
```

### object_t
Hierarchical object for JSON/XML representation.

```c
typedef struct object_t {
    string_t* string;           // Value (for leaf nodes)
    struct object_t* child;     // First child (for containers)
    struct object_t* next;      // Next sibling
} object_t;
```

### config_t
Configuration container.

```c
typedef struct {
    object_t* data;  // Configuration object hierarchy
} config_t;
```

### agent_t
Agent state container.

```c
typedef struct {
    object_t* data;  // Agent memory and state
} agent_t;
```

### lkjagent_t
Main application structure.

```c
typedef struct {
    pool_t pool;      // Memory pool
    config_t config;  // Configuration
    agent_t agent;    // Agent state
} lkjagent_t;
```

## Constants

### Memory Pool Limits
```c
#define POOL_STRING16_MAXCOUNT    (1048576)  // 1M entries
#define POOL_STRING256_MAXCOUNT   (65536)    // 64K entries
#define POOL_STRING4096_MAXCOUNT  (4096)     // 4K entries
#define POOL_STRING65536_MAXCOUNT (256)      // 256 entries
#define POOL_STRING1048576_MAXCOUNT (16)     // 16 entries
#define POOL_OBJECT_MAXCOUNT      (65536)    // 64K entries
```

### File Paths
```c
#define CONFIG_PATH "data/config.json"
#define MEMORY_PATH "data/memory.json"
```

### Buffer Sizes
```c
#define BUF_CAPACITY (1024 * 1024 * 4)  // 4MB buffer
```

## Error Handling

### RETURN_ERR Macro
Standardized error reporting macro that logs error and returns.

```c
#define RETURN_ERR(msg) do { \
    fprintf(stderr, "ERROR: %s\n", msg); \
    return RESULT_ERR; \
} while(0)
```

### Common Error Patterns

1. **Resource Allocation Failure**
```c
if (string_create(pool, &str) != RESULT_OK) {
    RETURN_ERR("Failed to allocate string");
}
```

2. **Resource Cleanup on Error**
```c
if (operation_that_might_fail() != RESULT_OK) {
    if (cleanup_resource(resource) != RESULT_OK) {
        RETURN_ERR("Failed to cleanup after operation failure");
    }
    RETURN_ERR("Operation failed");
}
```

3. **Validation Failure**
```c
if (validate_input(input) != RESULT_OK) {
    RETURN_ERR("Invalid input parameters");
}
```

## Memory Safety Guidelines

### Resource Lifecycle
1. Allocate resources through pool functions
2. Use resources only while valid
3. Free resources in reverse order of allocation
4. Never access freed resources

### Error Path Cleanup
Always clean up partially allocated resources:

```c
string_t* str1 = NULL;
string_t* str2 = NULL;
object_t* obj = NULL;

if (string_create(pool, &str1) != RESULT_OK) {
    RETURN_ERR("Failed to create str1");
}

if (string_create(pool, &str2) != RESULT_OK) {
    string_destroy(pool, str1);
    RETURN_ERR("Failed to create str2");
}

if (object_create(pool, &obj) != RESULT_OK) {
    string_destroy(pool, str2);
    string_destroy(pool, str1);
    RETURN_ERR("Failed to create object");
}

// Use resources...

// Cleanup in reverse order
object_destroy(pool, obj);
string_destroy(pool, str2);
string_destroy(pool, str1);
```

### Thread Safety
LKJAgent is single-threaded by design. Do not access any API functions from multiple threads simultaneously.
