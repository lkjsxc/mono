# LKJAgent API Reference

## Table of Contents

1. [Agent Management API](#agent-management-api)
2. [Memory Management API](#memory-management-api)
3. [Token Management API](#token-management-api)
4. [HTTP Client API](#http-client-api)
5. [JSON Processing API](#json-processing-api)
6. [Tool System API](#tool-system-api)
7. [State Management API](#state-management-api)
8. [Configuration API](#configuration-api)
9. [Error Handling API](#error-handling-api)
10. [File Operations API](#file-operations-api)

---

## Agent Management API

### Core Agent Functions

#### `agent_init`
```c
__attribute__((warn_unused_result)) 
result_t agent_init(agent_t* agent, const char* config_file);
```

**Description:** Initialize an agent structure with configuration from file.

**Parameters:**
- `agent`: Pointer to pre-allocated agent structure
- `config_file`: Path to JSON configuration file

**Returns:**
- `RESULT_OK`: Success
- `RESULT_ERR`: Initialization failed

**Example:**
```c
agent_t agent;
if (agent_init(&agent, "./data/config.json") != RESULT_OK) {
    fprintf(stderr, "Failed to initialize agent: %s\n", lkj_get_last_error());
    return 1;
}
```

**Error Conditions:**
- NULL agent pointer
- NULL or invalid config_file path
- Configuration loading failure
- Memory initialization failure

---

#### `agent_cleanup`
```c
void agent_cleanup(agent_t* agent);
```

**Description:** Clean up agent resources and save state to disk.

**Parameters:**
- `agent`: Pointer to agent to clean up

**Example:**
```c
// Always clean up agent resources
agent_cleanup(&agent);
```

**Note:** This function automatically attempts to save agent state to disk before cleanup.

---

#### `agent_set_task`
```c
__attribute__((warn_unused_result))
result_t agent_set_task(agent_t* agent, const char* task);
```

**Description:** Set the primary task for the agent to execute.

**Parameters:**
- `agent`: Target agent
- `task`: Task description string

**Returns:**
- `RESULT_OK`: Task set successfully
- `RESULT_ERR`: Failed to set task

**Example:**
```c
const char* task = "Analyze system logs and provide security recommendations";
if (agent_set_task(&agent, task) != RESULT_OK) {
    fprintf(stderr, "Failed to set task: %s\n", lkj_get_last_error());
}
```

---

### Agent Execution Functions

#### `agent_step`
```c
__attribute__((warn_unused_result))
result_t agent_step(agent_t* agent);
```

**Description:** Execute a single agent step with basic state transitions.

**Parameters:**
- `agent`: Agent to execute step for

**Returns:**
- `RESULT_OK`: Step completed successfully
- `RESULT_TASK_COMPLETE`: Task finished
- `RESULT_ERR`: Error occurred

**Example:**
```c
result_t step_result = agent_step(&agent);
switch (step_result) {
    case RESULT_OK:
        printf("Step completed, state: %s\n", agent_state_to_string(agent.state));
        break;
    case RESULT_TASK_COMPLETE:
        printf("Task completed successfully!\n");
        break;
    case RESULT_ERR:
        printf("Step failed: %s\n", lkj_get_last_error());
        break;
}
```

---

#### `agent_step_intelligent`
```c
__attribute__((warn_unused_result))
result_t agent_step_intelligent(agent_t* agent);
```

**Description:** Execute a step with enhanced AI-driven decision making.

**Parameters:**
- `agent`: Agent to execute intelligent step for

**Returns:** Same as `agent_step`

**Example:**
```c
// Run with AI-enhanced decision making
while (agent_step_intelligent(&agent) == RESULT_OK) {
    printf("Intelligent step completed, iteration: %d\n", agent.iteration_count);
    if (agent.iteration_count >= 10) break; // Safety limit
}
```

---

#### `agent_run`
```c
__attribute__((warn_unused_result))
result_t agent_run(agent_t* agent);
```

**Description:** Run the agent until task completion or maximum iterations.

**Parameters:**
- `agent`: Agent to run

**Returns:**
- `RESULT_OK`: Agent completed execution
- `RESULT_TASK_COMPLETE`: Task completed successfully
- `RESULT_ERR`: Error occurred

**Example:**
```c
printf("Starting agent execution...\n");
result_t run_result = agent_run(&agent);
printf("Agent execution finished with result: %s\n", 
       run_result == RESULT_TASK_COMPLETE ? "SUCCESS" : "ERROR");
```

---

## Memory Management API

### Memory Initialization

#### `agent_memory_init`
```c
__attribute__((warn_unused_result))
result_t agent_memory_init(agent_memory_t* memory, char buffers[][2048], size_t num_buffers);
```

**Description:** Initialize agent memory with static buffers.

**Parameters:**
- `memory`: Memory structure to initialize
- `buffers`: Array of 2048-byte character buffers
- `num_buffers`: Number of buffers (minimum 7 required)

**Returns:**
- `RESULT_OK`: Memory initialized successfully
- `RESULT_ERR`: Initialization failed

**Example:**
```c
// Allocate memory buffers on stack
static char memory_buffers[7][2048];
agent_memory_t memory;

if (agent_memory_init(&memory, memory_buffers, 7) != RESULT_OK) {
    fprintf(stderr, "Memory initialization failed: %s\n", lkj_get_last_error());
    return 1;
}
```

**Buffer Allocation:**
- Index 0: `system_prompt`
- Index 1: `current_state`
- Index 2: `task_goal`
- Index 3: `plan`
- Index 4: `scratchpad`
- Index 5: `recent_history`
- Index 6: `retrieved_from_disk`

---

### Persistent Storage

#### `agent_memory_save_to_disk`
```c
__attribute__((warn_unused_result))
result_t agent_memory_save_to_disk(const agent_t* agent);
```

**Description:** Save current agent state to persistent JSON storage.

**Parameters:**
- `agent`: Agent whose state to save

**Returns:**
- `RESULT_OK`: State saved successfully
- `RESULT_ERR`: Save operation failed

**Example:**
```c
// Save agent state before shutdown
if (agent_memory_save_to_disk(&agent) != RESULT_OK) {
    fprintf(stderr, "Warning: Failed to save agent state: %s\n", lkj_get_last_error());
}
```

---

#### `agent_memory_load_from_disk`
```c
__attribute__((warn_unused_result))
result_t agent_memory_load_from_disk(agent_t* agent);
```

**Description:** Load agent state from persistent JSON storage.

**Parameters:**
- `agent`: Agent to load state into

**Returns:**
- `RESULT_OK`: State loaded successfully
- `RESULT_ERR`: Load operation failed

**Example:**
```c
// Restore agent state from disk
if (agent_memory_load_from_disk(&agent) != RESULT_OK) {
    printf("No previous state found, starting fresh\n");
}
```

---

#### `agent_memory_clear_ram`
```c
__attribute__((warn_unused_result))
result_t agent_memory_clear_ram(agent_t* agent);
```

**Description:** Clear volatile RAM memory while preserving persistent data.

**Parameters:**
- `agent`: Agent whose RAM to clear

**Returns:**
- `RESULT_OK`: RAM cleared successfully
- `RESULT_ERR`: Clear operation failed

---

## Token Management API

### Token Initialization

#### `token_init`
```c
__attribute__((warn_unused_result))
result_t token_init(token_t* token, char* buffer, size_t capacity);
```

**Description:** Initialize a token with a static buffer.

**Parameters:**
- `token`: Token structure to initialize
- `buffer`: Character buffer to use
- `capacity`: Maximum buffer size (minimum 2 bytes)

**Returns:**
- `RESULT_OK`: Token initialized successfully
- `RESULT_ERR`: Initialization failed

**Example:**
```c
char buffer[256];
token_t token;

if (token_init(&token, buffer, sizeof(buffer)) != RESULT_OK) {
    fprintf(stderr, "Token initialization failed\n");
    return 1;
}
```

**Safety Features:**
- Validates all parameters
- Clears buffer on initialization
- Ensures null termination capability

---

### Token Content Management

#### `token_set`
```c
__attribute__((warn_unused_result))
result_t token_set(token_t* token, const char* str);
```

**Description:** Set token content from a C string.

**Parameters:**
- `token`: Target token
- `str`: Source string (null-terminated)

**Returns:**
- `RESULT_OK`: Content set successfully
- `RESULT_ERR`: Operation failed

**Example:**
```c
if (token_set(&token, "Hello, World!") != RESULT_OK) {
    fprintf(stderr, "Failed to set token content\n");
}
```

---

#### `token_set_length`
```c
__attribute__((warn_unused_result))
result_t token_set_length(token_t* token, const char* buffer, size_t length);
```

**Description:** Set token content from buffer with specified length.

**Parameters:**
- `token`: Target token
- `buffer`: Source buffer
- `length`: Number of bytes to copy

**Returns:**
- `RESULT_OK`: Content set successfully
- `RESULT_ERR`: Operation failed

**Example:**
```c
const char* data = "Binary\0Data\0Here";
if (token_set_length(&token, data, 15) != RESULT_OK) {
    fprintf(stderr, "Failed to set token with binary data\n");
}
```

---

#### `token_append`
```c
__attribute__((warn_unused_result))
result_t token_append(token_t* token, const char* str);
```

**Description:** Append string to existing token content.

**Parameters:**
- `token`: Target token
- `str`: String to append

**Returns:**
- `RESULT_OK`: Content appended successfully
- `RESULT_ERR`: Operation failed (insufficient space)

**Example:**
```c
token_set(&token, "Hello");
if (token_append(&token, ", World!") != RESULT_OK) {
    fprintf(stderr, "Not enough space to append\n");
}
printf("Result: %s\n", token.data); // "Hello, World!"
```

---

#### `token_copy`
```c
__attribute__((warn_unused_result))
result_t token_copy(token_t* dest, const token_t* src);
```

**Description:** Copy content from source token to destination token.

**Parameters:**
- `dest`: Destination token
- `src`: Source token

**Returns:**
- `RESULT_OK`: Content copied successfully
- `RESULT_ERR`: Operation failed

**Example:**
```c
token_t source, destination;
// ... initialize both tokens ...

token_set(&source, "Data to copy");
if (token_copy(&destination, &source) != RESULT_OK) {
    fprintf(stderr, "Copy failed\n");
}
```

---

### Token Utility Functions

#### `token_equals`
```c
int token_equals(const token_t* token1, const token_t* token2);
```

**Description:** Compare two tokens for equality.

**Parameters:**
- `token1`: First token
- `token2`: Second token

**Returns:**
- `1`: Tokens are equal
- `0`: Tokens are not equal or error

**Example:**
```c
if (token_equals(&token1, &token2)) {
    printf("Tokens contain identical content\n");
}
```

---

#### `token_equals_str`
```c
int token_equals_str(const token_t* token, const char* str);
```

**Description:** Compare token with C string for equality.

**Parameters:**
- `token`: Token to compare
- `str`: String to compare with

**Returns:**
- `1`: Token equals string
- `0`: Not equal or error

**Example:**
```c
if (token_equals_str(&token, "expected_value")) {
    printf("Token contains expected value\n");
}
```

---

#### `token_find`
```c
__attribute__((warn_unused_result))
result_t token_find(const token_t* token, const char* needle, size_t* position);
```

**Description:** Find substring in token content.

**Parameters:**
- `token`: Token to search in
- `needle`: String to find
- `position`: Pointer to store found position

**Returns:**
- `RESULT_OK`: Substring found
- `RESULT_ERR`: Not found or error

**Example:**
```c
size_t pos;
if (token_find(&token, "World", &pos) == RESULT_OK) {
    printf("Found 'World' at position %zu\n", pos);
}
```

---

#### `token_substring`
```c
__attribute__((warn_unused_result))
result_t token_substring(const token_t* token, size_t start, size_t length, token_t* dest);
```

**Description:** Extract substring from token.

**Parameters:**
- `token`: Source token
- `start`: Starting position
- `length`: Number of characters
- `dest`: Destination token

**Returns:**
- `RESULT_OK`: Substring extracted
- `RESULT_ERR`: Invalid range or error

**Example:**
```c
token_t substring;
char sub_buffer[100];
token_init(&substring, sub_buffer, sizeof(sub_buffer));

// Extract first 5 characters
if (token_substring(&source, 0, 5, &substring) == RESULT_OK) {
    printf("First 5 chars: %s\n", substring.data);
}
```

---

## HTTP Client API

### HTTP Request Functions

#### `http_get`
```c
__attribute__((warn_unused_result))
result_t http_get(token_t* url, token_t* response);
```

**Description:** Perform HTTP GET request.

**Parameters:**
- `url`: URL token (must contain complete HTTP URL)
- `response`: Token to store response body

**Returns:**
- `RESULT_OK`: Request successful
- `RESULT_ERR`: Request failed

**Example:**
```c
char url_buffer[256], response_buffer[4096];
token_t url, response;

token_init(&url, url_buffer, sizeof(url_buffer));
token_init(&response, response_buffer, sizeof(response_buffer));

token_set(&url, "http://httpbin.org/get");
if (http_get(&url, &response) == RESULT_OK) {
    printf("Response (%zu bytes): %s\n", response.size, response.data);
} else {
    printf("GET request failed: %s\n", lkj_get_last_error());
}
```

---

#### `http_post`
```c
__attribute__((warn_unused_result))
result_t http_post(token_t* url, const token_t* body, token_t* response);
```

**Description:** Perform HTTP POST request with body.

**Parameters:**
- `url`: URL token
- `body`: Request body token
- `response`: Token to store response

**Returns:**
- `RESULT_OK`: Request successful
- `RESULT_ERR`: Request failed

**Example:**
```c
char body_buffer[1024];
token_t body;

token_init(&body, body_buffer, sizeof(body_buffer));
token_set(&url, "http://httpbin.org/post");
token_set(&body, "{\"name\":\"lkjagent\",\"version\":\"1.0\"}");

if (http_post(&url, &body, &response) == RESULT_OK) {
    printf("POST successful\n");
}
```

---

#### `http_request`
```c
__attribute__((warn_unused_result))
result_t http_request(token_t* method, token_t* url, const token_t* body, token_t* response);
```

**Description:** Generic HTTP request function.

**Parameters:**
- `method`: HTTP method (GET, POST, PUT, etc.)
- `url`: URL token
- `body`: Request body (can be NULL for GET)
- `response`: Response token

**Returns:**
- `RESULT_OK`: Request successful
- `RESULT_ERR`: Request failed

**Example:**
```c
char method_buffer[16];
token_t method;

token_init(&method, method_buffer, sizeof(method_buffer));
token_set(&method, "PUT");

if (http_request(&method, &url, &body, &response) == RESULT_OK) {
    printf("PUT request successful\n");
}
```

---

## JSON Processing API

### JSON Validation

#### `json_validate`
```c
__attribute__((warn_unused_result))
result_t json_validate(const token_t* json_token);
```

**Description:** Validate JSON structure and syntax.

**Parameters:**
- `json_token`: Token containing JSON string

**Returns:**
- `RESULT_OK`: JSON is valid
- `RESULT_ERR`: JSON is invalid

**Example:**
```c
char json_buffer[512];
token_t json;

token_init(&json, json_buffer, sizeof(json_buffer));
token_set(&json, "{\"name\":\"test\",\"value\":123}");

if (json_validate(&json) == RESULT_OK) {
    printf("JSON is valid\n");
} else {
    printf("Invalid JSON: %s\n", lkj_get_last_error());
}
```

---

### JSON Value Extraction

#### `json_get_string`
```c
__attribute__((warn_unused_result))
result_t json_get_string(const token_t* json_token, const char* key_path, token_t* result);
```

**Description:** Extract string value from JSON using key path.

**Parameters:**
- `json_token`: JSON token
- `key_path`: Dot-separated path to value (e.g., "user.name")
- `result`: Token to store extracted string

**Returns:**
- `RESULT_OK`: Value extracted successfully
- `RESULT_ERR`: Key not found or not a string

**Example:**
```c
char result_buffer[256];
token_t result;

token_init(&result, result_buffer, sizeof(result_buffer));
token_set(&json, "{\"user\":{\"name\":\"John\",\"age\":30}}");

if (json_get_string(&json, "user.name", &result) == RESULT_OK) {
    printf("User name: %s\n", result.data);
}
```

---

#### `json_get_number`
```c
__attribute__((warn_unused_result))
result_t json_get_number(const token_t* json_token, const char* key_path, double* result);
```

**Description:** Extract numeric value from JSON.

**Parameters:**
- `json_token`: JSON token
- `key_path`: Dot-separated path to value
- `result`: Pointer to store numeric value

**Returns:**
- `RESULT_OK`: Value extracted successfully
- `RESULT_ERR`: Key not found or not a number

**Example:**
```c
double age;
if (json_get_number(&json, "user.age", &age) == RESULT_OK) {
    printf("User age: %.0f\n", age);
}
```

---

### JSON Creation

#### `json_create_object`
```c
__attribute__((warn_unused_result))
result_t json_create_object(token_t* result, const char* keys[], const char* values[], size_t count);
```

**Description:** Create JSON object from key-value pairs.

**Parameters:**
- `result`: Token to store created JSON
- `keys`: Array of key strings
- `values`: Array of value strings
- `count`: Number of key-value pairs

**Returns:**
- `RESULT_OK`: JSON created successfully
- `RESULT_ERR`: Creation failed

**Example:**
```c
const char* keys[] = {"name", "version", "status"};
const char* values[] = {"lkjagent", "1.0", "active"};

char json_buffer[512];
token_t json_result;

token_init(&json_result, json_buffer, sizeof(json_buffer));

if (json_create_object(&json_result, keys, values, 3) == RESULT_OK) {
    printf("Created JSON: %s\n", json_result.data);
}
```

---

## Tool System API

### Tool Execution

#### `agent_execute_tool`
```c
__attribute__((warn_unused_result))
result_t agent_execute_tool(agent_t* agent, tool_type_t tool, const char* args, token_t* result);
```

**Description:** Execute any agent tool with arguments.

**Parameters:**
- `agent`: Agent context
- `tool`: Tool type to execute
- `args`: Tool arguments string
- `result`: Token to store tool output

**Returns:**
- `RESULT_OK`: Tool executed successfully
- `RESULT_ERR`: Tool execution failed

**Example:**
```c
char result_buffer[1024];
token_t result;

token_init(&result, result_buffer, sizeof(result_buffer));

// Execute search tool
if (agent_execute_tool(&agent, TOOL_SEARCH, "system status", &result) == RESULT_OK) {
    printf("Search result: %s\n", result.data);
}
```

**Tool Types:**
```c
typedef enum {
    TOOL_SEARCH = 0,      // Query persistent storage
    TOOL_RETRIEVE = 1,    // Read specific data
    TOOL_WRITE = 2,       // Save information
    TOOL_EXECUTE_CODE = 3,// Run code snippets
    TOOL_FORGET = 4       // Delete information
} tool_type_t;
```

---

### Individual Tool Functions

#### `agent_tool_search`
```c
__attribute__((warn_unused_result))
result_t agent_tool_search(agent_t* agent, const char* query, token_t* result);
```

**Description:** Search persistent storage for relevant information.

**Parameters:**
- `agent`: Agent context
- `query`: Search query string
- `result`: Token to store search results

**Example:**
```c
if (agent_tool_search(&agent, "security events", &result) == RESULT_OK) {
    printf("Found: %s\n", result.data);
}
```

---

#### `agent_tool_retrieve`
```c
__attribute__((warn_unused_result))
result_t agent_tool_retrieve(agent_t* agent, const char* key, token_t* result);
```

**Description:** Retrieve specific data by key.

**Parameters:**
- `agent`: Agent context
- `key`: Data key to retrieve
- `result`: Token to store retrieved data

**Example:**
```c
if (agent_tool_retrieve(&agent, "user_preferences", &result) == RESULT_OK) {
    printf("Preferences: %s\n", result.data);
}
```

---

#### `agent_tool_write`
```c
__attribute__((warn_unused_result))
result_t agent_tool_write(agent_t* agent, const char* key, const char* value, const char* tags);
```

**Description:** Save information to persistent storage with optional tags.

**Parameters:**
- `agent`: Agent context
- `key`: Storage key
- `value`: Data to store
- `tags`: Optional tags (comma-separated)

**Example:**
```c
if (agent_tool_write(&agent, "analysis_result", 
                     "System performance is optimal", 
                     "analysis,performance") == RESULT_OK) {
    printf("Analysis saved\n");
}
```

---

#### `agent_tool_execute_code`
```c
__attribute__((warn_unused_result))
result_t agent_tool_execute_code(agent_t* agent, const char* code, token_t* result);
```

**Description:** Execute code snippet and capture output.

**Parameters:**
- `agent`: Agent context
- `code`: Code to execute
- `result`: Token to store execution output

**Example:**
```c
const char* script = "echo 'System check: '; uptime";
if (agent_tool_execute_code(&agent, script, &result) == RESULT_OK) {
    printf("Script output: %s\n", result.data);
}
```

---

#### `agent_tool_forget`
```c
__attribute__((warn_unused_result))
result_t agent_tool_forget(agent_t* agent, const char* key);
```

**Description:** Delete information from persistent storage.

**Parameters:**
- `agent`: Agent context
- `key`: Key to delete

**Example:**
```c
if (agent_tool_forget(&agent, "temporary_data") == RESULT_OK) {
    printf("Temporary data removed\n");
}
```

---

## State Management API

### State Information

#### `agent_state_to_string`
```c
const char* agent_state_to_string(agent_state_t state);
```

**Description:** Convert agent state enum to string representation.

**Parameters:**
- `state`: Agent state enum

**Returns:** String representation of state

**Example:**
```c
printf("Current agent state: %s\n", agent_state_to_string(agent.state));
```

**State Values:**
- `AGENT_STATE_THINKING`: "thinking"
- `AGENT_STATE_EXECUTING`: "executing"  
- `AGENT_STATE_EVALUATING`: "evaluating"
- `AGENT_STATE_PAGING`: "paging"

---

### State Transitions

#### `agent_transition_state`
```c
__attribute__((warn_unused_result))
result_t agent_transition_state(agent_t* agent, agent_state_t new_state);
```

**Description:** Perform validated state transition.

**Parameters:**
- `agent`: Agent to transition
- `new_state`: Target state

**Returns:**
- `RESULT_OK`: Transition successful
- `RESULT_ERR`: Invalid transition

**Example:**
```c
if (agent_transition_state(&agent, AGENT_STATE_EXECUTING) == RESULT_OK) {
    printf("Transitioned to executing state\n");
} else {
    printf("Invalid state transition\n");
}
```

---

#### `agent_is_valid_transition`
```c
int agent_is_valid_transition(agent_state_t current_state, agent_state_t new_state);
```

**Description:** Check if state transition is valid.

**Parameters:**
- `current_state`: Current agent state
- `new_state`: Proposed new state

**Returns:**
- `1`: Transition is valid
- `0`: Transition is invalid

**Valid Transitions:**
- `THINKING` → `EXECUTING`, `PAGING`
- `EXECUTING` → `EVALUATING`, `PAGING`
- `EVALUATING` → `THINKING`, `PAGING`
- `PAGING` → Any state

---

## Configuration API

### Configuration Loading

#### `config_load`
```c
__attribute__((warn_unused_result))
result_t config_load(const char* config_file, full_config_t* config);
```

**Description:** Load configuration from JSON file.

**Parameters:**
- `config_file`: Path to configuration file
- `config`: Configuration structure to populate

**Example:**
```c
full_config_t config;
if (config_load("./data/config.json", &config) != RESULT_OK) {
    fprintf(stderr, "Failed to load configuration\n");
    return 1;
}
```

---

#### `config_apply_to_agent`
```c
__attribute__((warn_unused_result))
result_t config_apply_to_agent(agent_t* agent, const full_config_t* config);
```

**Description:** Apply loaded configuration to agent.

**Parameters:**
- `agent`: Agent to configure
- `config`: Configuration to apply

---

## Error Handling API

### Error Logging

#### `lkj_log_error`
```c
void lkj_log_error(const char* function, const char* message);
```

**Description:** Log error with function context.

**Parameters:**
- `function`: Function name (usually `__func__`)
- `message`: Error message

**Example:**
```c
if (some_operation() != RESULT_OK) {
    lkj_log_error(__func__, "operation failed due to invalid parameters");
    return RESULT_ERR;
}
```

---

#### `lkj_get_last_error`
```c
const char* lkj_get_last_error(void);
```

**Description:** Retrieve the last logged error message.

**Returns:** String containing last error message

**Example:**
```c
if (operation() != RESULT_OK) {
    printf("Operation failed: %s\n", lkj_get_last_error());
}
```

---

#### `lkj_clear_last_error`
```c
void lkj_clear_last_error(void);
```

**Description:** Clear the error state.

**Example:**
```c
// Clear errors before critical operation
lkj_clear_last_error();
if (critical_operation() != RESULT_OK) {
    // Fresh error state
}
```

---

## File Operations API

### File I/O

#### `file_read`
```c
__attribute__((warn_unused_result))
result_t file_read(const char* path, token_t* content);
```

**Description:** Read file content into token.

**Parameters:**
- `path`: File path to read
- `content`: Token to store file content

**Example:**
```c
char content_buffer[4096];
token_t content;

token_init(&content, content_buffer, sizeof(content_buffer));

if (file_read("./data/sample.txt", &content) == RESULT_OK) {
    printf("File content: %s\n", content.data);
}
```

---

#### `file_write`
```c
__attribute__((warn_unused_result))
result_t file_write(const char* path, const token_t* content);
```

**Description:** Write token content to file.

**Parameters:**
- `path`: File path to write
- `content`: Token containing data to write

**Example:**
```c
token_set(&content, "Data to save to file");
if (file_write("./output.txt", &content) == RESULT_OK) {
    printf("File written successfully\n");
}
```

---

## Usage Patterns

### Complete Agent Example

```c
#include "lkjagent.h"

int main() {
    // Initialize agent
    agent_t agent;
    if (agent_init(&agent, "./data/config.json") != RESULT_OK) {
        fprintf(stderr, "Agent init failed: %s\n", lkj_get_last_error());
        return 1;
    }
    
    // Initialize memory
    static char memory_buffers[7][2048];
    if (agent_memory_init(&agent.memory, memory_buffers, 7) != RESULT_OK) {
        fprintf(stderr, "Memory init failed: %s\n", lkj_get_last_error());
        agent_cleanup(&agent);
        return 1;
    }
    
    // Set task
    if (agent_set_task(&agent, "Analyze system and provide recommendations") != RESULT_OK) {
        fprintf(stderr, "Task setting failed: %s\n", lkj_get_last_error());
        agent_cleanup(&agent);
        return 1;
    }
    
    // Run agent
    result_t result = agent_run(&agent);
    printf("Agent completed with result: %s\n", 
           result == RESULT_TASK_COMPLETE ? "SUCCESS" : "ERROR");
    
    // Cleanup
    agent_cleanup(&agent);
    return 0;
}
```

### HTTP Client Example

```c
void demo_http_client() {
    char url_buffer[256], response_buffer[4096], body_buffer[1024];
    token_t url, response, body;
    
    // Initialize tokens
    token_init(&url, url_buffer, sizeof(url_buffer));
    token_init(&response, response_buffer, sizeof(response_buffer));
    token_init(&body, body_buffer, sizeof(body_buffer));
    
    // GET request
    token_set(&url, "http://httpbin.org/get");
    if (http_get(&url, &response) == RESULT_OK) {
        printf("GET Response: %s\n", response.data);
    }
    
    // POST request
    token_set(&url, "http://httpbin.org/post");
    token_set(&body, "{\"message\":\"Hello from lkjagent\"}");
    if (http_post(&url, &body, &response) == RESULT_OK) {
        printf("POST Response: %s\n", response.data);
    }
}
```

### Tool Usage Example

```c
void demo_tools(agent_t* agent) {
    char result_buffer[1024];
    token_t result;
    token_init(&result, result_buffer, sizeof(result_buffer));
    
    // Search for information
    if (agent_tool_search(agent, "configuration", &result) == RESULT_OK) {
        printf("Search result: %s\n", result.data);
    }
    
    // Write information
    if (agent_tool_write(agent, "status", "system_healthy", "monitoring") == RESULT_OK) {
        printf("Status saved\n");
    }
    
    // Retrieve information
    if (agent_tool_retrieve(agent, "status", &result) == RESULT_OK) {
        printf("Retrieved status: %s\n", result.data);
    }
    
    // Execute code
    if (agent_tool_execute_code(agent, "echo 'System check complete'", &result) == RESULT_OK) {
        printf("Code output: %s\n", result.data);
    }
}
```

---

*This API reference provides complete function signatures and usage examples for all public functions in the LKJAgent system.*
