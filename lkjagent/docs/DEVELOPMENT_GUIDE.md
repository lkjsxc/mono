# LKJAgent Development Guide

## Table of Contents

1. [Getting Started](#getting-started)
2. [Development Environment](#development-environment)
3. [Code Organization](#code-organization)
4. [Building and Testing](#building-and-testing)
5. [Extending the System](#extending-the-system)
6. [Best Practices](#best-practices)
7. [Debugging Guide](#debugging-guide)
8. [Performance Optimization](#performance-optimization)
9. [Security Considerations](#security-considerations)
10. [Contributing](#contributing)

---

## Getting Started

### Prerequisites

- **C11 Compatible Compiler**: GCC 4.9+ or Clang 3.1+
- **Standard C Library**: POSIX-compliant system
- **Socket Support**: Standard on Linux platforms
- **Make**: For build system
- **Optional**: Valgrind for memory debugging

### Quick Start

1. **Clone and Build**:
```bash
cd lkjagent
make
```

2. **Run Demo**:
```bash
./build/lkjagent
```

3. **Run Tests**:
```bash
make test
```

### Project Structure Understanding

```
lkjagent/
├── src/                    # Source code
│   ├── main.c             # Entry point
│   ├── lkjagent.h         # Main header
│   ├── core/              # Agent lifecycle
│   ├── state/             # State machine
│   ├── memory/            # Memory management
│   ├── tools/             # Tool system
│   ├── api/               # API integration
│   └── utils/             # Utilities
├── data/                  # Configuration and data
├── test/                  # Test suite
└── build/                 # Build artifacts
```

---

## Development Environment

### Recommended Setup

#### VS Code Configuration
```json
{
    "C_Cpp.default.cStandard": "c11",
    "C_Cpp.default.compilerPath": "/usr/bin/gcc",
    "C_Cpp.default.includePath": ["${workspaceFolder}/src"],
    "files.associations": {
        "*.h": "c",
        "*.c": "c"
    }
}
```

#### .clang-format Configuration
```yaml
BasedOnStyle: LLVM
IndentWidth: 4
TabWidth: 4
UseTab: Never
ColumnLimit: 100
AllowShortFunctionsOnASingleLine: None
AllowShortIfStatementsOnASingleLine: false
```

### Development Tools

#### Static Analysis
```bash
# Using cppcheck
cppcheck --enable=all --std=c11 src/

# Using clang static analyzer
scan-build make
```

#### Memory Debugging
```bash
# Compile with debug symbols
make clean
gcc -g -DDEBUG -Wall -Wextra -std=c11 -o build/lkjagent src/main.c src/**/*.c

# Run with valgrind
valgrind --leak-check=full --show-leak-kinds=all ./build/lkjagent
```

#### Code Formatting
```bash
# Format all source files
find src/ -name "*.c" -o -name "*.h" | xargs clang-format -i
```

---

## Code Organization

### Header Organization

The main header `lkjagent.h` contains all public declarations organized by category:

```c
// Types and enums
typedef enum { ... } result_t;
typedef struct { ... } token_t;

// Function declarations grouped by module
// Agent management
__attribute__((warn_unused_result)) result_t agent_init(...);

// Memory management  
__attribute__((warn_unused_result)) result_t agent_memory_init(...);

// Token operations
__attribute__((warn_unused_result)) result_t token_init(...);
```

### Module Organization

Each module follows a consistent pattern:

```c
/**
 * @file module_name.c
 * @brief Brief description of module purpose
 * 
 * Detailed description of what this module does,
 * its responsibilities, and how it fits into the system.
 */

#include "../lkjagent.h"

// Static/private functions first
static result_t internal_function(...) {
    // Implementation
}

// Public functions second
result_t public_function(...) {
    // Parameter validation
    if (!param) {
        lkj_log_error(__func__, "param is NULL");
        return RESULT_ERR;
    }
    
    // Implementation
    return RESULT_OK;
}
```

### Error Handling Pattern

Consistent error handling throughout the codebase:

```c
result_t example_function(param_t* param) {
    // 1. Validate parameters
    if (!param) {
        lkj_log_error(__func__, "param is NULL");
        return RESULT_ERR;
    }
    
    // 2. Perform operation
    if (some_operation(param) != RESULT_OK) {
        lkj_log_error(__func__, "operation failed");
        return RESULT_ERR;
    }
    
    // 3. Return success
    return RESULT_OK;
}
```

---

## Building and Testing

### Build System Details

The Makefile is organized by functional groups:

```makefile
# Source organization
CORE_SOURCES = $(SRCDIR)/core/agent_core.c $(SRCDIR)/core/agent_runner.c
STATE_SOURCES = $(SRCDIR)/state/*.c
MEMORY_SOURCES = $(SRCDIR)/memory/memory_manager.c
TOOL_SOURCES = $(SRCDIR)/tools/agent_tools.c
API_SOURCES = $(SRCDIR)/api/lmstudio_api.c
UTIL_SOURCES = $(SRCDIR)/utils/*.c

# Combined sources
LIB_SOURCES = $(CORE_SOURCES) $(STATE_SOURCES) $(MEMORY_SOURCES) \
              $(TOOL_SOURCES) $(API_SOURCES) $(UTIL_SOURCES)
```

### Build Variants

#### Development Build
```bash
# With debug symbols and verbose warnings
gcc -g -DDEBUG -Wall -Wextra -Wpedantic -std=c11 -o build/lkjagent src/main.c $(LIB_SOURCES)
```

#### Production Build
```bash
# Optimized for performance
gcc -O3 -DNDEBUG -Wall -Wextra -std=c11 -o build/lkjagent src/main.c $(LIB_SOURCES)
```

#### Test Build
```bash
# With additional test coverage
gcc -g -DDEBUG -DTEST_MODE --coverage -Wall -Wextra -std=c11 -o build/test_comprehensive test/test_comprehensive.c $(LIB_SOURCES)
```

### Testing Framework

#### Test Structure
```c
// test/test_example.c
#include "../src/lkjagent.h"

void test_token_operations() {
    printf("=== Testing Token Operations ===\n");
    
    char buffer[100];
    token_t token;
    
    // Test initialization
    assert(token_init(&token, buffer, sizeof(buffer)) == RESULT_OK);
    
    // Test content operations
    assert(token_set(&token, "test") == RESULT_OK);
    assert(token_equals_str(&token, "test") == 1);
    
    printf("Token operations: PASSED\n");
}

int main() {
    test_token_operations();
    // ... other tests
    printf("All tests passed!\n");
    return 0;
}
```

#### Running Tests
```bash
# Run all tests
make test

# Run specific test
make test-json

# Build test without running
make build/test_comprehensive
./build/test_comprehensive
```

---

## Extending the System

### Adding a New Tool

#### Step 1: Define Tool Type
```c
// In lkjagent.h, add to tool_type_t enum
typedef enum {
    TOOL_SEARCH = 0,
    TOOL_RETRIEVE = 1,
    TOOL_WRITE = 2,
    TOOL_EXECUTE_CODE = 3,
    TOOL_FORGET = 4,
    TOOL_NEW_ANALYSIS = 5  // New tool
} tool_type_t;
```

#### Step 2: Declare Function
```c
// In lkjagent.h, add function declaration
__attribute__((warn_unused_result)) 
result_t agent_tool_analysis(agent_t* agent, const char* target, token_t* result);
```

#### Step 3: Implement Tool
```c
// In tools/agent_tools.c
result_t agent_tool_analysis(agent_t* agent, const char* target, token_t* result) {
    if (!agent || !target || !result) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }
    
    // Clear result
    if (token_clear(result) != RESULT_OK) {
        lkj_log_error(__func__, "failed to clear result token");
        return RESULT_ERR;
    }
    
    // Perform analysis
    if (token_set(result, "Analysis of ") != RESULT_OK ||
        token_append(result, target) != RESULT_OK ||
        token_append(result, " completed successfully") != RESULT_OK) {
        lkj_log_error(__func__, "failed to set result");
        return RESULT_ERR;
    }
    
    // Log tool execution
    if (token_append(&agent->memory.recent_history, "analysis_tool_executed;") != RESULT_OK) {
        // Non-fatal error - continue
    }
    
    return RESULT_OK;
}
```

#### Step 4: Add to Tool Dispatcher
```c
// In agent_execute_tool function
result_t agent_execute_tool(agent_t* agent, tool_type_t tool, const char* args, token_t* result) {
    // ... existing code ...
    
    switch (tool) {
        case TOOL_SEARCH:
            return agent_tool_search(agent, args, result);
        // ... existing cases ...
        case TOOL_NEW_ANALYSIS:
            return agent_tool_analysis(agent, args, result);
        default:
            lkj_log_error(__func__, "unknown tool type");
            return RESULT_ERR;
    }
}
```

#### Step 5: Add Tests
```c
// In test file
void test_new_analysis_tool() {
    printf("=== Testing New Analysis Tool ===\n");
    
    agent_t agent;
    // ... initialize agent ...
    
    char result_buffer[256];
    token_t result;
    token_init(&result, result_buffer, sizeof(result_buffer));
    
    // Test analysis tool
    if (agent_tool_analysis(&agent, "system_logs", &result) == RESULT_OK) {
        printf("Analysis result: %s\n", result.data);
        assert(strstr(result.data, "system_logs") != NULL);
    }
    
    printf("Analysis tool: PASSED\n");
}
```

### Adding a New State

#### Step 1: Define State
```c
// In lkjagent.h
typedef enum {
    AGENT_STATE_THINKING = 0,
    AGENT_STATE_EXECUTING = 1,
    AGENT_STATE_EVALUATING = 2,
    AGENT_STATE_PAGING = 3,
    AGENT_STATE_MONITORING = 4  // New state
} agent_state_t;
```

#### Step 2: Create State File
```c
// src/state/state_monitoring.c
/**
 * @file state_monitoring.c
 * @brief MONITORING state implementation
 * 
 * This state continuously monitors system status and responds
 * to changes or alerts that require attention.
 */

#include "../lkjagent.h"

result_t state_monitoring_init(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }
    
    // Set current state in memory
    if (token_set(&agent->memory.current_state, "monitoring") != RESULT_OK) {
        lkj_log_error(__func__, "failed to set current state");
        return RESULT_ERR;
    }
    
    // Initialize monitoring parameters
    if (token_set(&agent->memory.plan, "Monitor system metrics and respond to alerts") != RESULT_OK) {
        lkj_log_error(__func__, "failed to set monitoring plan");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t state_monitoring_execute(agent_t* agent) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }
    
    // Perform monitoring operations
    char search_buffer[256];
    token_t search_result;
    token_init(&search_result, search_buffer, sizeof(search_buffer));
    
    // Check for alerts
    if (agent_tool_search(agent, "alerts", &search_result) == RESULT_OK) {
        // Process any found alerts
        if (token_append(&agent->memory.scratchpad, "Monitoring: ") == RESULT_OK &&
            token_append(&agent->memory.scratchpad, search_result.data) == RESULT_OK) {
            // Alert information added to scratchpad
        }
    }
    
    return RESULT_OK;
}

result_t state_monitoring_next(agent_t* agent, agent_state_t* next_state) {
    if (!agent || !next_state) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }
    
    // Check if we need to respond to alerts
    if (strstr(agent->memory.scratchpad.data, "critical") != NULL) {
        *next_state = AGENT_STATE_EXECUTING;  // Handle critical alert
    } else if (agent_should_page(agent)) {
        *next_state = AGENT_STATE_PAGING;     // Memory management
    } else {
        *next_state = AGENT_STATE_MONITORING; // Continue monitoring
    }
    
    return RESULT_OK;
}
```

#### Step 3: Update State Manager
```c
// In state_manager.c

// Update agent_state_to_string
const char* agent_state_to_string(agent_state_t state) {
    switch (state) {
        case AGENT_STATE_THINKING:   return "thinking";
        case AGENT_STATE_EXECUTING:  return "executing";
        case AGENT_STATE_EVALUATING: return "evaluating";
        case AGENT_STATE_PAGING:     return "paging";
        case AGENT_STATE_MONITORING: return "monitoring";  // New state
        default:                     return "unknown";
    }
}

// Update transition validation
int agent_is_valid_transition(agent_state_t current_state, agent_state_t new_state) {
    // ... existing transitions ...
    
    switch (current_state) {
        // ... existing cases ...
        case AGENT_STATE_MONITORING:
            // From monitoring, can go to executing, paging, or continue monitoring
            return (new_state == AGENT_STATE_EXECUTING || 
                   new_state == AGENT_STATE_PAGING ||
                   new_state == AGENT_STATE_MONITORING);
        // ... rest of cases ...
    }
}
```

#### Step 4: Update State Initialization
```c
// In state_manager.c or appropriate location
result_t agent_initialize_state(agent_t* agent, agent_state_t new_state) {
    // ... existing cases ...
    
    switch (new_state) {
        case AGENT_STATE_THINKING:
            return state_thinking_init(agent);
        case AGENT_STATE_EXECUTING:
            return state_executing_init(agent);
        case AGENT_STATE_EVALUATING:
            return state_evaluating_init(agent);
        case AGENT_STATE_PAGING:
            return state_paging_init(agent);
        case AGENT_STATE_MONITORING:
            return state_monitoring_init(agent);  // New state init
        default:
            lkj_log_error(__func__, "unknown state");
            return RESULT_ERR;
    }
}
```

#### Step 5: Update Makefile
```makefile
# Add new state source to STATE_SOURCES
STATE_SOURCES = $(SRCDIR)/state/state_manager.c \
                $(SRCDIR)/state/state_thinking.c \
                $(SRCDIR)/state/state_executing.c \
                $(SRCDIR)/state/state_evaluating.c \
                $(SRCDIR)/state/state_paging.c \
                $(SRCDIR)/state/state_monitoring.c
```

### Adding Configuration Options

#### Step 1: Update Configuration Structure
```c
// In lkjagent.h
typedef struct {
    int max_iterations;
    double evaluation_threshold;
    char memory_file[256];
    size_t ram_size;
    size_t max_history;
    int autonomous_mode;
    int continuous_thinking;
    int self_directed;
    int monitoring_enabled;    // New option
    int monitoring_interval;   // New option
} agent_config_detailed_t;
```

#### Step 2: Update Configuration Loading
```c
// In config.c
result_t config_load(const char* config_file, full_config_t* config) {
    // ... existing loading code ...
    
    // Load new monitoring options
    double monitoring_enabled;
    if (json_get_number(&json_content, "agent.monitoring_enabled", &monitoring_enabled) == RESULT_OK) {
        config->agent.monitoring_enabled = (int)monitoring_enabled;
    } else {
        config->agent.monitoring_enabled = 0; // Default disabled
    }
    
    double monitoring_interval;
    if (json_get_number(&json_content, "agent.monitoring_interval", &monitoring_interval) == RESULT_OK) {
        config->agent.monitoring_interval = (int)monitoring_interval;
    } else {
        config->agent.monitoring_interval = 60; // Default 60 seconds
    }
    
    return RESULT_OK;
}
```

#### Step 3: Update Configuration File
```json
{
    "agent": {
        "max_iterations": -1,
        "evaluation_threshold": 0.8,
        "memory_file": "data/memory.json",
        "ram_size": 2048,
        "max_history": 100,
        "autonomous_mode": true,
        "continuous_thinking": true,
        "self_directed": true,
        "monitoring_enabled": true,
        "monitoring_interval": 30
    }
}
```

---

## Best Practices

### Memory Safety

#### Always Validate Parameters
```c
result_t safe_function(token_t* token, const char* input) {
    // Validate all parameters first
    if (!token) {
        lkj_log_error(__func__, "token parameter is NULL");
        return RESULT_ERR;
    }
    if (!input) {
        lkj_log_error(__func__, "input parameter is NULL");
        return RESULT_ERR;
    }
    if (token_validate(token) != RESULT_OK) {
        lkj_log_error(__func__, "invalid token provided");
        return RESULT_ERR;
    }
    
    // Proceed with function logic
    return RESULT_OK;
}
```

#### Use Bounded Operations
```c
// Good: Bounded buffer operations
char buffer[256];
token_t token;
token_init(&token, buffer, sizeof(buffer));

// Check available space before operations
if (token_available_space(&token) < strlen(new_data)) {
    lkj_log_error(__func__, "insufficient buffer space");
    return RESULT_ERR;
}

// Bad: Unbounded operations (never do this)
strcpy(buffer, unknown_size_string);  // DANGEROUS!
```

#### Initialize All Variables
```c
// Good: Always initialize
char buffer[256] = {0};
token_t token;
memset(&token, 0, sizeof(token));

// Or use designated initializers
token_t token = {0};
```

### Error Handling

#### Consistent Error Patterns
```c
result_t multi_step_operation() {
    // Step 1: Resource allocation/initialization
    if (init_resources() != RESULT_OK) {
        lkj_log_error(__func__, "resource initialization failed");
        return RESULT_ERR;
    }
    
    // Step 2: Main operation
    if (main_operation() != RESULT_OK) {
        lkj_log_error(__func__, "main operation failed");
        cleanup_resources();  // Clean up on error
        return RESULT_ERR;
    }
    
    // Step 3: Success cleanup
    cleanup_resources();
    return RESULT_OK;
}
```

#### Error Context
```c
// Provide specific error context
if (token_set(&token, long_string) != RESULT_OK) {
    lkj_log_error(__func__, "failed to set token - string too long for buffer");
    return RESULT_ERR;
}

// Chain error information
if (some_operation() != RESULT_OK) {
    lkj_log_error(__func__, "operation failed - check configuration file");
    return RESULT_ERR;
}
```

### Performance

#### Minimize Allocations
```c
// Good: Stack allocation
char buffer[1024];
token_t token;
token_init(&token, buffer, sizeof(buffer));

// Reuse tokens
token_clear(&token);  // Reset for reuse
token_set(&token, new_value);
```

#### Efficient String Operations
```c
// Good: Single append operation
if (token_set(&token, "prefix_") == RESULT_OK &&
    token_append(&token, variable_part) == RESULT_OK &&
    token_append(&token, "_suffix") == RESULT_OK) {
    // All operations succeeded
}

// Bad: Multiple intermediate operations
token_set(&token, "prefix_");
// ... other code that might fail ...
token_append(&token, variable_part);
// ... more code ...
token_append(&token, "_suffix");
```

### Code Style

#### Function Naming
```c
// Module prefix + verb + object
result_t token_init(...);
result_t token_set(...);
result_t agent_execute_tool(...);
result_t http_get(...);

// State functions follow pattern
result_t state_thinking_init(...);
result_t state_thinking_execute(...);
result_t state_thinking_next(...);
```

#### Documentation
```c
/**
 * @brief Brief description of function purpose
 * @param param1 Description of first parameter
 * @param param2 Description of second parameter
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * Detailed description of function behavior, side effects,
 * and any special considerations.
 */
result_t well_documented_function(param_type param1, param_type param2);
```

---

## Debugging Guide

### Debugging Techniques

#### Enable Debug Logging
```c
// In main function or initialization
lkj_set_error_logging(1);

// Use throughout code
if (operation() != RESULT_OK) {
    printf("DEBUG: Operation failed: %s\n", lkj_get_last_error());
}
```

#### Token State Inspection
```c
void debug_token_state(const token_t* token, const char* context) {
    printf("DEBUG [%s]: Token state:\n", context);
    printf("  data: '%s'\n", token->data ? token->data : "NULL");
    printf("  size: %zu\n", token->size);
    printf("  capacity: %zu\n", token->capacity);
    printf("  available: %d\n", token_available_space(token));
    printf("  valid: %s\n", token_validate(token) == RESULT_OK ? "YES" : "NO");
}
```

#### Memory State Inspection
```c
void debug_agent_memory(const agent_t* agent) {
    printf("DEBUG: Agent Memory State:\n");
    printf("  State: %s\n", agent_state_to_string(agent->state));
    printf("  Iterations: %d\n", agent->iteration_count);
    printf("  Task: '%s'\n", agent->memory.task_goal.data);
    printf("  Plan: '%s'\n", agent->memory.plan.data);
    printf("  Scratchpad size: %zu\n", agent->memory.scratchpad.size);
    printf("  History size: %zu\n", agent->memory.recent_history.size);
}
```

### Common Issues and Solutions

#### Buffer Overflow Prevention
```c
// Problem: String too long for buffer
// Solution: Check space before operation
if (strlen(input) >= token.capacity) {
    lkj_log_error(__func__, "input string too long for buffer");
    return RESULT_ERR;
}
```

#### Memory Initialization Issues
```c
// Problem: Uninitialized token
// Solution: Always call token_init
char buffer[256];
token_t token;
// Must initialize before use!
if (token_init(&token, buffer, sizeof(buffer)) != RESULT_OK) {
    // Handle initialization error
}
```

#### Configuration Loading Problems
```c
// Problem: Missing configuration file
// Solution: Check file existence and provide defaults
if (access(config_file, R_OK) != 0) {
    lkj_log_error(__func__, "configuration file not accessible");
    // Load default configuration
    load_default_config(config);
}
```

### Debugging with GDB

#### Compile with Debug Symbols
```bash
gcc -g -DDEBUG -Wall -Wextra -std=c11 -o build/lkjagent src/main.c $(SOURCES)
```

#### Useful GDB Commands
```bash
# Start debugging
gdb ./build/lkjagent

# Set breakpoints
(gdb) break agent_init
(gdb) break token_set

# Run with arguments
(gdb) run

# Examine variables
(gdb) print token
(gdb) print *agent
(gdb) print agent->memory.scratchpad

# Step through code
(gdb) step
(gdb) next
(gdb) continue

# Examine memory
(gdb) x/s token.data
(gdb) x/100c buffer
```

---

## Performance Optimization

### Profiling

#### CPU Profiling with gprof
```bash
# Compile with profiling
gcc -pg -O2 -std=c11 -o build/lkjagent src/main.c $(SOURCES)

# Run program
./build/lkjagent

# Generate profile
gprof ./build/lkjagent gmon.out > profile.txt
```

#### Memory Profiling with Valgrind
```bash
# Check for memory leaks
valgrind --tool=memcheck --leak-check=full ./build/lkjagent

# Profile memory usage
valgrind --tool=massif ./build/lkjagent
```

### Optimization Strategies

#### String Operation Optimization
```c
// Optimize for common patterns
result_t build_json_message(token_t* result, const char* type, const char* content) {
    // Pre-calculate total length needed
    size_t total_len = strlen("{\"type\":\"\",\"content\":\"\"}") + 
                       strlen(type) + strlen(content);
    
    if (total_len >= result->capacity) {
        lkj_log_error(__func__, "insufficient buffer space");
        return RESULT_ERR;
    }
    
    // Single operation when possible
    int written = snprintf(result->data, result->capacity,
                          "{\"type\":\"%s\",\"content\":\"%s\"}", type, content);
    
    if (written >= 0 && (size_t)written < result->capacity) {
        result->size = written;
        return RESULT_OK;
    }
    
    return RESULT_ERR;
}
```

#### HTTP Request Optimization
```c
// Reuse connection structures
static struct sockaddr_in cached_addr;
static int cached_addr_valid = 0;

result_t optimized_http_request(const char* host, int port, ...) {
    // Cache DNS resolution
    if (!cached_addr_valid || /* host changed */) {
        // Resolve and cache
        cached_addr_valid = 1;
    }
    
    // Use cached address
    // ...
}
```

### Memory Usage Optimization

#### Buffer Size Tuning
```c
// Configure buffer sizes based on usage patterns
#define TOKEN_SMALL_SIZE  256   // For short strings
#define TOKEN_MEDIUM_SIZE 1024  // For JSON objects  
#define TOKEN_LARGE_SIZE  4096  // For HTTP responses

// Use appropriate sizes
char url_buffer[TOKEN_SMALL_SIZE];      // URLs are typically short
char json_buffer[TOKEN_MEDIUM_SIZE];    // JSON objects
char response_buffer[TOKEN_LARGE_SIZE]; // HTTP responses
```

#### Memory Pool Pattern
```c
// Pre-allocate common buffer sizes
typedef struct {
    char small_buffers[10][TOKEN_SMALL_SIZE];
    char medium_buffers[5][TOKEN_MEDIUM_SIZE];
    char large_buffers[2][TOKEN_LARGE_SIZE];
    int small_used[10];
    int medium_used[5];
    int large_used[2];
} buffer_pool_t;

token_t* get_token_from_pool(buffer_pool_t* pool, size_t size_needed) {
    // Return appropriate sized token from pool
    // Mark as used
    // Return to pool when done
}
```

---

## Security Considerations

### Input Validation

#### URL Validation
```c
result_t validate_url(const token_t* url) {
    if (token_is_empty(url)) {
        lkj_log_error(__func__, "empty URL");
        return RESULT_ERR;
    }
    
    // Check for valid protocol
    if (!token_starts_with(url, "http://") && 
        !token_starts_with(url, "https://")) {
        lkj_log_error(__func__, "invalid protocol");
        return RESULT_ERR;
    }
    
    // Check for malicious patterns
    if (strstr(url->data, "..") != NULL ||
        strstr(url->data, "localhost") != NULL ||
        strstr(url->data, "127.0.0.1") != NULL) {
        lkj_log_error(__func__, "potentially unsafe URL");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}
```

#### JSON Input Sanitization
```c
result_t sanitize_json_input(token_t* json) {
    // Remove potentially dangerous characters
    char* dangerous_chars[] = {"<script", "javascript:", "eval(", NULL};
    
    for (int i = 0; dangerous_chars[i] != NULL; i++) {
        if (strstr(json->data, dangerous_chars[i]) != NULL) {
            lkj_log_error(__func__, "potentially malicious JSON content");
            return RESULT_ERR;
        }
    }
    
    return RESULT_OK;
}
```

### File System Security

#### Path Validation
```c
result_t validate_file_path(const char* path) {
    if (!path) {
        lkj_log_error(__func__, "NULL path");
        return RESULT_ERR;
    }
    
    // Prevent directory traversal
    if (strstr(path, "..") != NULL ||
        strstr(path, "//") != NULL ||
        path[0] == '/') {  // Prevent absolute paths
        lkj_log_error(__func__, "unsafe file path");
        return RESULT_ERR;
    }
    
    // Only allow files in data directory
    if (strncmp(path, "data/", 5) != 0) {
        lkj_log_error(__func__, "file must be in data directory");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}
```

### Network Security

#### Request Size Limits
```c
#define MAX_REQUEST_SIZE 8192
#define MAX_RESPONSE_SIZE 4096

result_t safe_http_request(token_t* url, token_t* body, token_t* response) {
    // Validate request size
    if (body && body->size > MAX_REQUEST_SIZE) {
        lkj_log_error(__func__, "request body too large");
        return RESULT_ERR;
    }
    
    // Validate response buffer
    if (response->capacity > MAX_RESPONSE_SIZE) {
        lkj_log_error(__func__, "response buffer too large");
        return RESULT_ERR;
    }
    
    return http_request_internal(url, body, response);
}
```

---

## Contributing

### Code Review Checklist

#### Before Submitting
- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] Code follows project style guidelines
- [ ] Functions have appropriate error handling
- [ ] Memory operations are bounds-checked
- [ ] New features have tests
- [ ] Documentation is updated

#### Code Quality Standards
- [ ] Functions return explicit error codes
- [ ] All parameters are validated
- [ ] Error messages are descriptive
- [ ] No magic numbers (use named constants)
- [ ] Functions have single responsibility
- [ ] Code is commented appropriately

### Git Workflow

#### Branch Naming
```bash
# Feature branches
git checkout -b feature/new-tool-implementation

# Bug fix branches  
git checkout -b bugfix/memory-leak-in-tokens

# Documentation branches
git checkout -b docs/api-reference-update
```

#### Commit Messages
```bash
# Good commit messages
git commit -m "Add HTTP timeout configuration option

- Add timeout_seconds to http_config_t structure
- Implement socket timeout in connect_to_host()
- Update default configuration with 30-second timeout
- Add timeout validation in config loading"

# Bad commit messages
git commit -m "fix stuff"
git commit -m "update code"
```

### Testing Requirements

#### Test Coverage Goals
- **Unit Tests**: Cover all public functions
- **Integration Tests**: Test component interactions  
- **Error Path Tests**: Test all error conditions
- **Memory Tests**: Verify no leaks or corruption
- **Performance Tests**: Ensure acceptable performance

#### Test Template
```c
void test_new_feature() {
    printf("=== Testing New Feature ===\n");
    
    // Setup
    agent_t agent;
    assert(agent_init(&agent, "test/test_config.json") == RESULT_OK);
    
    // Test normal operation
    result_t result = new_feature_function(&agent, "test_input");
    assert(result == RESULT_OK);
    
    // Test error conditions
    result = new_feature_function(NULL, "test_input");
    assert(result == RESULT_ERR);
    
    result = new_feature_function(&agent, NULL);
    assert(result == RESULT_ERR);
    
    // Cleanup
    agent_cleanup(&agent);
    
    printf("New feature tests: PASSED\n");
}
```

---

This development guide provides comprehensive information for extending, debugging, and maintaining the LKJAgent system. Follow these practices to ensure code quality, security, and maintainability.
