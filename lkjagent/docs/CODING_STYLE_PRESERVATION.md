# LKJAgent Coding Style Guide - Preservation Requirements

## Overview

This document captures the exact coding style and patterns used in the current LKJAgent implementation that **MUST BE PRESERVED** in any rewrite. These patterns represent excellent C programming practices and should be maintained exactly.

## 1. Function Documentation Pattern

### REQUIRED Format
```c
/**
 * @brief Brief description of function purpose
 * @param parameter_name Description of parameter
 * @param another_param Description of another parameter
 * @return Description of return value and possible error codes
 */
__attribute__((warn_unused_result)) result_t function_name(type* param1, const type* param2) {
    // Function implementation
}
```

### Example from Codebase
```c
/**
 * @brief Initialize an agent structure with configuration
 * @param agent Pointer to pre-allocated agent structure
 * @param config_file Path to configuration file
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
result_t agent_init(agent_t* agent, const char* config_file) {
    if (!agent) {
        lkj_log_error(__func__, "agent parameter is NULL");
        return RESULT_ERR;
    }
    if (!config_file) {
        lkj_log_error(__func__, "config_file is NULL");
        return RESULT_ERR;
    }
    
    // Initialize to zero
    memset(agent, 0, sizeof(agent_t));
    
    // Load configuration
    if (config_load(config_file, &agent->loaded_config) != RESULT_OK) {
        lkj_log_error(__func__, "failed to load configuration");
        return RESULT_ERR;
    }
    
    // Apply configuration to agent
    if (config_apply_to_agent(agent, &agent->loaded_config) != RESULT_OK) {
        lkj_log_error(__func__, "failed to apply configuration");
        return RESULT_ERR;
    }
    
    // Initialize state to thinking
    agent->state = AGENT_STATE_THINKING;
    agent->iteration_count = 0;
    
    printf("Agent initialized successfully\n");
    return RESULT_OK;
}
```

## 2. Parameter Validation Pattern

### REQUIRED Pattern
```c
result_t function_name(type* required_param, const type* optional_param) {
    // Always validate required pointer parameters first
    if (!required_param) {
        lkj_log_error(__func__, "required_param parameter is NULL");
        return RESULT_ERR;
    }
    
    // Validate other required parameters
    if (!optional_param) {
        lkj_log_error(__func__, "optional_param parameter is NULL");
        return RESULT_ERR;
    }
    
    // Additional validation for specific conditions
    if (some_condition_check(required_param) != EXPECTED_VALUE) {
        lkj_log_error(__func__, "descriptive error about the condition");
        return RESULT_ERR;
    }
    
    // Function logic here...
}
```

### Examples from Codebase
```c
result_t token_init(token_t* token, char* buffer, size_t capacity) {
    if (!token) {
        lkj_log_error(__func__, "token parameter is NULL");
        return RESULT_ERR;
    }
    if (!buffer) {
        lkj_log_error(__func__, "buffer parameter is NULL");
        return RESULT_ERR;
    }
    if (capacity < 2) {  // Need at least 2 bytes for content + null terminator
        lkj_log_error(__func__, "capacity too small (minimum 2 bytes required)");
        return RESULT_ERR;
    }
    
    token->data = buffer;
    token->size = 0;
    token->capacity = capacity;
    
    // Clear the buffer to ensure it's in a clean state
    memset(buffer, 0, capacity);
    
    return RESULT_OK;
}
```

## 3. Error Handling Pattern

### REQUIRED Pattern
```c
// Error checking and propagation
if (some_operation(param1, param2) != RESULT_OK) {
    lkj_log_error(__func__, "descriptive message about what failed");
    return RESULT_ERR;
}

// For complex operations, use intermediate variables
result_t operation_result = complex_operation(params);
if (operation_result != RESULT_OK) {
    lkj_log_error(__func__, "complex operation failed");
    return operation_result;  // Propagate specific error code
}

// For cleanup scenarios
result_t cleanup_result = cleanup_resources(resource);
if (cleanup_result != RESULT_OK) {
    lkj_log_error(__func__, "failed to cleanup resources");
    // Continue with other cleanup but track the error
}
```

### Error Logging Pattern
```c
void lkj_log_error(const char* function, const char* message);
void lkj_log_errno(const char* function, const char* operation);

// Usage examples:
lkj_log_error(__func__, "agent parameter is NULL");
lkj_log_errno(__func__, "failed to open file");
```

## 4. Memory Management Pattern

### Stack-Based Allocation (REQUIRED)
```c
// Static buffers for token operations
static char buffer_name[BUFFER_SIZE];
token_t token_name;

if (token_init(&token_name, buffer_name, sizeof(buffer_name)) != RESULT_OK) {
    lkj_log_error(__func__, "failed to initialize token");
    return RESULT_ERR;
}

// Array of buffers for multiple tokens
static char memory_buffers[NUM_BUFFERS][BUFFER_SIZE];
if (agent_memory_init(&agent->memory, memory_buffers, NUM_BUFFERS) != RESULT_OK) {
    lkj_log_error(__func__, "failed to initialize memory");
    return RESULT_ERR;
}
```

### Zero Dynamic Allocation (FORBIDDEN)
```c
// NEVER use these in LKJAgent:
// malloc(), calloc(), realloc(), free()
// new, delete (C++)
// Any dynamic memory allocation

// ALWAYS use:
// Static arrays
// Stack allocation
// User-provided buffers
```

## 5. Function Return Patterns

### REQUIRED Return Type
```c
// All functions that can fail MUST return result_t
__attribute__((warn_unused_result)) result_t function_name(/*params*/);

// Functions that return values should use output parameters
__attribute__((warn_unused_result)) result_t get_value(input_t* input, output_t* output);

// Simple getter functions can return values directly if they cannot fail
const char* agent_state_to_string(agent_state_t state);
int agent_is_valid_transition(agent_state_t current_state, agent_state_t new_state);
```

### Result Type Definition
```c
typedef enum {
    RESULT_OK = 0,
    RESULT_ERR = 1,
    RESULT_TASK_COMPLETE = 2,
    // Add new error codes as needed
} result_t;
```

## 6. Structure Definition Pattern

### REQUIRED Structure Style
```c
// Type definitions with clear naming
typedef struct {
    char* data;
    size_t size;
    size_t capacity;
} token_t;

typedef struct {
    int max_iterations;
    double evaluation_threshold;
    char memory_file[256];
    size_t ram_size;
    size_t max_history;
    int autonomous_mode;
    int continuous_thinking;
    int self_directed;
} agent_config_detailed_t;

// Enum definitions with explicit values
typedef enum {
    AGENT_STATE_THINKING = 0,
    AGENT_STATE_EXECUTING = 1,
    AGENT_STATE_EVALUATING = 2,
    AGENT_STATE_PAGING = 3
} agent_state_t;
```

## 7. File Organization Pattern

### Header File Pattern (`lkjagent.h`)
```c
#ifndef _LKJAGENT_H
#define _LKJAGENT_H

#define _GNU_SOURCE
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// Result type definition
typedef enum {
    RESULT_OK = 0,
    RESULT_ERR = 1,
    RESULT_TASK_COMPLETE = 2,
} result_t;

// Error logging functions
void lkj_log_error(const char* function, const char* message);
// ... other declarations

// Structure definitions
typedef struct {
    // ... fields
} structure_name_t;

// Function declarations organized by category
// Configuration management functions
__attribute__((warn_unused_result)) result_t config_load(const char* config_file, full_config_t* config);

// Agent functions
__attribute__((warn_unused_result)) result_t agent_init(agent_t* agent, const char* config_file);

#endif
```

### Source File Pattern
```c
/**
 * @file filename.c
 * @brief Brief description of file purpose
 * 
 * Detailed description of what this file contains and implements.
 */

#include "../lkjagent.h"  // or appropriate relative path

// Static/local function declarations if needed
static result_t local_helper_function(/*params*/);

/**
 * @brief Public function implementation
 */
result_t public_function(/*params*/) {
    // Implementation
}

/**
 * @brief Static helper function implementation
 */
static result_t local_helper_function(/*params*/) {
    // Implementation
}
```

## 8. Control Flow Patterns

### If-Else Pattern
```c
// Single condition
if (condition) {
    // Action
} else {
    // Alternative action
}

// Multiple conditions with clear logic
if (primary_condition) {
    // Primary action
} else if (secondary_condition) {
    // Secondary action
} else {
    // Default action
}
```

### Switch Statement Pattern
```c
switch (enum_value) {
    case ENUM_VALUE_1:
        // Action for value 1
        break;
        
    case ENUM_VALUE_2:
        // Action for value 2
        break;
        
    default:
        lkj_log_error(__func__, "unknown enum value");
        return RESULT_ERR;
}
```

### Loop Patterns
```c
// For loops with clear bounds
for (int i = 0; i < max_count; i++) {
    // Loop body
}

// While loops with clear conditions
while (condition && safety_counter < MAX_ITERATIONS) {
    // Loop body
    safety_counter++;
}
```

## 9. Variable Naming Patterns

### Naming Conventions
```c
// Local variables: snake_case
int iteration_count;
char buffer_name[SIZE];
token_t result_token;

// Structure members: snake_case
typedef struct {
    int max_iterations;
    char memory_file[256];
    size_t ram_size;
} config_t;

// Function names: module_action format
result_t agent_init(/*...*/);
result_t token_append(/*...*/);
result_t memory_save_to_disk(/*...*/);

// Constants: UPPER_CASE
#define MAX_BUFFER_SIZE 2048
#define DEFAULT_TIMEOUT 30

// Enums: MODULE_SPECIFIC_VALUE
typedef enum {
    AGENT_STATE_THINKING,
    AGENT_STATE_EXECUTING,
    TOOL_TYPE_SEARCH,
    RESULT_OK
} enum_type_t;
```

## 10. Comment Patterns

### Function Comments (REQUIRED)
```c
/**
 * @brief One-line description of function purpose
 * @param param_name Description of parameter and constraints
 * @return Description of return value and error conditions
 */
```

### Inline Comments
```c
// Clear, descriptive comments for complex logic
if (agent->memory.scratchpad.size > (agent->memory.scratchpad.capacity * 3 / 4)) {
    // Scratchpad is over 75% full, trigger partial cleanup
    // Keep only the last portion of scratchpad
    size_t keep_size = agent->memory.scratchpad.capacity / 2;
    // ... implementation
}

// Section separators for long functions
// ---- Memory initialization ----
// ---- Configuration loading ----
// ---- Error handling ----
```

### File Header Comments
```c
/**
 * @file filename.c
 * @brief Brief description of the file's purpose
 * 
 * Detailed description of what this file contains,
 * what functionality it provides, and how it fits
 * into the overall system architecture.
 */
```

## 11. Makefile Pattern

### REQUIRED Makefile Structure
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDFLAGS = -Wl,-z,stack-size=1073741824

# Source organization by functionality
CORE_SOURCES = $(SRCDIR)/core/agent_core.c \
               $(SRCDIR)/core/agent_runner.c

STATE_SOURCES = $(SRCDIR)/state/state_manager.c \
                $(SRCDIR)/state/state_thinking.c \
                $(SRCDIR)/state/state_executing.c \
                $(SRCDIR)/state/state_evaluating.c \
                $(SRCDIR)/state/state_paging.c

# Combined sources for library
LIB_SOURCES = $(CORE_SOURCES) \
              $(STATE_SOURCES) \
              $(MEMORY_SOURCES) \
              $(TOOL_SOURCES) \
              $(API_SOURCES) \
              $(UTIL_SOURCES)

# Build rules
all: $(TARGET)

$(TARGET): $(MAIN_SOURCE) $(LIB_SOURCES) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(MAIN_SOURCE) $(LIB_SOURCES)
```

## 12. Testing Pattern

### Test Function Structure
```c
/**
 * @brief Test function description
 * @param param Test-specific parameters
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t test_function_name(test_params_t* param) {
    printf("Test description: what is being tested\n");
    
    // Setup phase
    // ... setup code
    
    // Test execution
    // ... test the functionality
    
    // Validation phase
    if (expected_result != actual_result) {
        printf("Test failed: expected X, got Y\n");
        return RESULT_ERR;
    }
    
    // Cleanup phase
    // ... cleanup code
    
    printf("Test passed\n");
    return RESULT_OK;
}
```

## 13. Configuration Pattern

### JSON Configuration Structure
```json
{
    "section_name": {
        "string_setting": "value",
        "numeric_setting": 42,
        "boolean_setting": true,
        "array_setting": ["item1", "item2"]
    },
    "another_section": {
        "nested_object": {
            "inner_setting": "inner_value"
        }
    }
}
```

### Configuration Loading Pattern
```c
result_t config_load(const char* config_file, full_config_t* config) {
    if (!config_file || !config) {
        lkj_log_error(__func__, "NULL parameters provided");
        return RESULT_ERR;
    }

    // Initialize tokens and buffers
    char config_buffer[4096];
    token_t config_token;
    char string_buffer[512];
    token_t string_result;
    double number_result;

    // Start with default configuration
    *config = default_config;

    // Read and validate configuration file
    if (file_read(config_file, &config_token) != RESULT_OK) {
        lkj_log_error(__func__, "failed to read config file");
        return RESULT_ERR;
    }

    // Extract configuration values
    if (json_get_string(&config_token, "section.setting", &string_result) == RESULT_OK) {
        strncpy(config->section.setting, string_result.data, sizeof(config->section.setting) - 1);
        config->section.setting[sizeof(config->section.setting) - 1] = '\0';
    }

    return RESULT_OK;
}
```

## Summary

These patterns represent the high-quality engineering practices that make the current LKJAgent implementation robust, safe, and maintainable. **All new code must follow these exact patterns** to maintain consistency and quality.

Key principles to preserve:
1. **Safety First**: Parameter validation and error checking
2. **Memory Safety**: Stack-based allocation only
3. **Clear Documentation**: Comprehensive function documentation
4. **Error Propagation**: Proper error handling and logging
5. **Modular Design**: Clear separation of concerns
6. **Consistent Style**: Uniform naming and formatting

Any deviation from these patterns should be avoided unless there is a compelling technical reason, and such deviations should be clearly documented and justified.
