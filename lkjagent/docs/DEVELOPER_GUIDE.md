# LKJAgent Developer Guide

This guide provides comprehensive information for developers working on LKJAgent, including architecture details, development setup, coding standards, testing procedures, and contribution guidelines.

## Table of Contents

- [Development Environment Setup](#development-environment-setup)
- [Architecture Deep Dive](#architecture-deep-dive)
- [Code Organization](#code-organization)
- [Coding Standards](#coding-standards)
- [Memory Management](#memory-management)
- [Error Handling](#error-handling)
- [Testing](#testing)
- [Debugging](#debugging)
- [Performance Optimization](#performance-optimization)
- [Extending the Agent](#extending-the-agent)
- [Contributing](#contributing)

## Development Environment Setup

### Prerequisites

- **GCC 12+**: For C11 support and modern optimizations
- **Make**: Build system
- **Git**: Version control
- **Docker**: For containerized development and testing
- **GDB**: For debugging
- **Valgrind**: For memory analysis (optional)
- **Clang-format**: For code formatting

### Setting Up the Development Environment

1. **Clone the repository:**
   ```bash
   git clone <repository-url>
   cd lkjagent
   ```

2. **Install development dependencies:**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install gcc make git docker.io gdb valgrind clang-format
   
   # CentOS/RHEL
   sudo yum install gcc make git docker gdb valgrind clang-tools-extra
   ```

3. **Configure development tools:**
   ```bash
   # Set up git hooks for code formatting
   cp .git-hooks/pre-commit .git/hooks/
   chmod +x .git/hooks/pre-commit
   ```

4. **Build and test:**
   ```bash
   make clean && make
   ./build/lkjagent
   ```

### Development Tools Configuration

#### GDB Configuration
Create `.gdbinit` for debugging:
```gdb
set print pretty on
set print array on
set print array-indexes on
define pool_status
  printf "String pools:\n"
  printf "  16-byte: %lu/%lu\n", $arg0.string16_freelist_count, POOL_STRING16_MAXCOUNT
  printf "  256-byte: %lu/%lu\n", $arg0.string256_freelist_count, POOL_STRING256_MAXCOUNT
  printf "  4KB: %lu/%lu\n", $arg0.string4096_freelist_count, POOL_STRING4096_MAXCOUNT
  printf "  64KB: %lu/%lu\n", $arg0.string65536_freelist_count, POOL_STRING65536_MAXCOUNT
  printf "  1MB: %lu/%lu\n", $arg0.string1048576_freelist_count, POOL_STRING1048576_MAXCOUNT
  printf "Objects: %lu/%lu\n", $arg0.object_freelist_count, POOL_OBJECT_MAXCOUNT
end
```

#### Clang-format Configuration
The project includes `.clang-format`:
```yaml
BasedOnStyle: Google
IndentWidth: 4
TabWidth: 4
UseTab: Never
ColumnLimit: 120
AllowShortFunctionsOnASingleLine: None
AllowShortIfStatementsOnASingleLine: false
```

## Architecture Deep Dive

### System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    LKJAgent Application                     │
├─────────────────────────────────────────────────────────────┤
│  lkjagent.c: Main application loop and initialization      │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      Agent Core                             │
├─────────────────────────────────────────────────────────────┤
│  agent/core.c: Agent processing cycle orchestration        │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │
│  │   Prompt    │ │    HTTP     │ │   Response  │           │
│  │ Generation  │ │ Communication│ │ Processing  │           │
│  └─────────────┘ └─────────────┘ └─────────────┘           │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Agent Subsystems                         │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │
│  │   Actions   │ │    State    │ │   Prompt    │           │
│  │   System    │ │ Management  │ │ Generation  │           │
│  └─────────────┘ └─────────────┘ └─────────────┘           │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Utility Layer                            │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │
│  │   Memory    │ │   String    │ │   Object    │           │
│  │    Pool     │ │ Management  │ │   (JSON)    │           │
│  └─────────────┘ └─────────────┘ └─────────────┘           │
│  ┌─────────────┐ ┌─────────────┐                           │
│  │   File I/O  │ │    HTTP     │                           │
│  │             │ │   Client    │                           │
│  └─────────────┘ └─────────────┘                           │
└─────────────────────────────────────────────────────────────┘
```

### Data Flow

1. **Initialization Phase:**
   - Memory pool initialization
   - Configuration loading and parsing
   - Agent memory loading

2. **Main Loop:**
   - Preflight validation
   - Prompt generation based on state
   - HTTP communication with LLM
   - Response parsing and processing
   - Action execution (if present)
   - State transitions
   - Memory synchronization

3. **Cleanup Phase:**
   - Memory saving
   - Resource deallocation
   - Pool statistics reporting

### Component Interactions

```c
// Main execution flow
lkjagent_init() → {
    pool_init()
    config_load() 
    agent_load()
}

lkjagent_agent() → {
    agent_prompt_generate()
    agent_http_send_receive()
    lkjagent_agent_command()
    agent_state_sync_logs_to_working_memory()
    agent_actions_save_memory()
}

lkjagent_agent_command() → {
    agent_actions_parse_response()
    agent_actions_dispatch()  // If action present
    agent_state_update_and_log()
}
```

## Code Organization

### Directory Structure

```
src/
├── lkjagent.c              # Main entry point
├── lkjagent.h              # Primary header
├── global/                 # Global definitions
│   ├── types.h             # Type definitions
│   ├── const.h             # Constants
│   ├── macro.h             # Utility macros
│   └── std.h               # Standard includes
├── utils/                  # Utility libraries
│   ├── pool.{c,h}          # Memory pool management
│   ├── string.{c,h}        # String utilities
│   ├── object.{c,h}        # JSON object handling
│   ├── file.{c,h}          # File I/O operations
│   └── http.{c,h}          # HTTP client
└── agent/                  # Agent functionality
    ├── core.{c,h}          # Main agent logic
    ├── actions.{c,h}       # Action system
    ├── state.{c,h}         # State management
    ├── prompt.{c,h}        # Prompt generation
    └── http.{c,h}          # LLM communication
```

### Header Dependencies

```c
// Dependency hierarchy (low to high level)
global/ → utils/ → agent/ → lkjagent.h

// Each module includes only what it needs:
// utils/pool.h includes global/{types,const,std,macro}.h
// agent/core.h includes utils/{pool,string,object}.h + agent/{state,actions}.h
// lkjagent.h includes all necessary headers for main application
```

### Naming Conventions

#### Functions
```c
// Pattern: module_[submodule_]operation[_object]
pool_init()                    // Initialize pool
pool_string_alloc()           // Allocate string from pool
agent_actions_dispatch()      // Dispatch action in agent actions module
agent_state_update_state()    // Update state in agent state module
```

#### Types
```c
// Pattern: name_t for all types
result_t                      // Result enumeration
string_t                      // String structure
pool_t                        // Memory pool structure
lkjagent_t                    // Main agent structure
```

#### Constants
```c
// Pattern: ALL_CAPS for constants
#define POOL_STRING16_MAXCOUNT  // Pool size constant
#define CONFIG_PATH             // File path constant
#define RESULT_OK               // Result value constant
```

#### Variables
```c
// Pattern: lowercase with underscores
pool_t memory_pool;           // Structure instance
string_t* temp_string;        // Pointer variable
uint64_t max_iterations;      // Numeric variable
```

## Coding Standards

### C Standard and Compiler Requirements

- **C11 Standard**: Use modern C features appropriately
- **GCC 12+**: Leverage compiler optimizations and warnings
- **Static linking**: Minimize external dependencies

### Code Style

#### Function Definitions
```c
// Good: Clear return type, descriptive name, documented parameters
__attribute__((warn_unused_result)) result_t pool_string_alloc(
    pool_t* pool,           // Memory pool to allocate from
    string_t** string,      // Output: allocated string
    uint64_t capacity       // Required string capacity
) {
    // Implementation
}
```

#### Error Handling
```c
// Good: Consistent error handling pattern
result_t example_function(pool_t* pool) {
    string_t* temp_string;
    
    if (pool_string_alloc(pool, &temp_string, 256) != RESULT_OK) {
        RETURN_ERR("Failed to allocate temporary string");
    }
    
    // Use temp_string...
    
    if (pool_string_free(pool, temp_string) != RESULT_OK) {
        RETURN_ERR("Failed to free temporary string");
    }
    
    return RESULT_OK;
}
```

#### Memory Management
```c
// Good: Clear allocation/deallocation pairing
result_t process_data(pool_t* pool) {
    string_t* buffer = NULL;
    object_t* parsed_object = NULL;
    
    // Allocate resources
    if (string_create(pool, &buffer) != RESULT_OK) {
        RETURN_ERR("Buffer allocation failed");
    }
    
    if (object_create(pool, &parsed_object) != RESULT_OK) {
        string_destroy(pool, buffer);  // Clean up on error
        RETURN_ERR("Object allocation failed");
    }
    
    // Use resources...
    
    // Clean up in reverse order
    if (object_destroy(pool, parsed_object) != RESULT_OK ||
        string_destroy(pool, buffer) != RESULT_OK) {
        RETURN_ERR("Cleanup failed");
    }
    
    return RESULT_OK;
}
```

### Documentation Standards

#### Function Documentation
```c
/**
 * @brief Allocates a string from the memory pool
 * 
 * Selects appropriate pool based on requested capacity and returns
 * a pre-allocated string. The string is initialized but empty.
 * 
 * @param pool Memory pool to allocate from
 * @param string Output pointer to allocated string
 * @param capacity Required string capacity in bytes
 * @return RESULT_OK on success, RESULT_ERR if pool exhausted
 * 
 * @note Caller must free string with pool_string_free()
 * @warning String content is uninitialized after allocation
 */
__attribute__((warn_unused_result)) result_t pool_string_alloc(
    pool_t* pool, 
    string_t** string, 
    uint64_t capacity
);
```

#### Structure Documentation
```c
/**
 * @brief Dynamic string with explicit capacity management
 * 
 * Provides efficient string operations with pre-allocated capacity.
 * Size tracks current content length, capacity tracks total space.
 */
typedef struct string_s {
    char* data;           /**< Pointer to string data buffer */
    uint64_t capacity;    /**< Total allocated capacity in bytes */
    uint64_t size;        /**< Current string length (excluding null terminator) */
} string_t;
```

## Memory Management

### Pool Architecture

LKJAgent uses a custom memory pool system to avoid fragmentation and provide predictable performance.

#### Pool Design Principles

1. **Size-based allocation**: Different pools for different size requirements
2. **Pre-allocation**: All memory allocated at startup
3. **Freelist management**: Efficient allocation/deallocation through freelists
4. **No fragmentation**: Fixed-size allocations prevent fragmentation
5. **Bounded resources**: Clear memory limits prevent system exhaustion

#### Pool Implementation

```c
// Pool structure contains arrays of pre-allocated objects
typedef struct {
    // Example: 16-byte string pool
    char string16_data[POOL_STRING16_MAXCOUNT * 16];      // Actual data
    string_t string16[POOL_STRING16_MAXCOUNT];            // String structures
    string_t* string16_freelist_data[POOL_STRING16_MAXCOUNT]; // Freelist
    uint64_t string16_freelist_count;                     // Available count
    
    // Similar structures for other sizes...
} pool_t;
```

#### Allocation Strategy

```c
result_t pool_string_alloc(pool_t* pool, string_t** string, uint64_t capacity) {
    // Select appropriate pool based on capacity
    if (capacity <= 16) {
        return pool_string16_alloc(pool, string);
    } else if (capacity <= 256) {
        return pool_string256_alloc(pool, string);
    } 
    // ... continue for other sizes
    
    RETURN_ERR("Requested capacity exceeds maximum pool size");
}
```

### Memory Usage Patterns

#### Typical Memory Lifecycle
1. **Startup**: All pools initialized with full freelists
2. **Runtime**: Objects allocated from freelists, returned when done
3. **Steady state**: Freelists maintain relatively stable levels
4. **Shutdown**: Pool statistics reported, memory implicitly freed

#### Memory Debugging

```c
// Add memory debugging to functions
result_t debug_function(pool_t* pool) {
    printf("DEBUG: Before allocation - String16 free: %lu\n", 
           pool->string16_freelist_count);
    
    string_t* temp;
    if (pool_string16_alloc(pool, &temp) != RESULT_OK) {
        RETURN_ERR("Allocation failed");
    }
    
    printf("DEBUG: After allocation - String16 free: %lu\n", 
           pool->string16_freelist_count);
    
    // Use temp...
    
    if (pool_string_free(pool, temp) != RESULT_OK) {
        RETURN_ERR("Free failed");
    }
    
    printf("DEBUG: After free - String16 free: %lu\n", 
           pool->string16_freelist_count);
    
    return RESULT_OK;
}
```

## Error Handling

### Error Handling Philosophy

1. **Fail fast**: Detect errors as early as possible
2. **Propagate clearly**: Return meaningful error information
3. **Clean up resources**: Always free allocated resources on error paths
4. **Log appropriately**: Provide useful debugging information
5. **Graceful degradation**: Continue operation when possible

### Error Handling Patterns

#### Standard Error Return
```c
result_t function_that_can_fail(pool_t* pool) {
    if (some_condition_check() != RESULT_OK) {
        RETURN_ERR("Descriptive error message explaining what failed");
    }
    
    return RESULT_OK;
}
```

#### Resource Cleanup on Error
```c
result_t complex_function(pool_t* pool) {
    string_t* resource1 = NULL;
    object_t* resource2 = NULL;
    
    // Allocate first resource
    if (string_create(pool, &resource1) != RESULT_OK) {
        RETURN_ERR("Failed to allocate resource1");
    }
    
    // Allocate second resource
    if (object_create(pool, &resource2) != RESULT_OK) {
        // Clean up first resource before returning error
        string_destroy(pool, resource1);
        RETURN_ERR("Failed to allocate resource2");
    }
    
    // Process with both resources...
    if (some_processing_fails()) {
        // Clean up both resources in reverse order
        object_destroy(pool, resource2);
        string_destroy(pool, resource1);
        RETURN_ERR("Processing failed");
    }
    
    // Normal cleanup
    if (object_destroy(pool, resource2) != RESULT_OK ||
        string_destroy(pool, resource1) != RESULT_OK) {
        RETURN_ERR("Cleanup failed");
    }
    
    return RESULT_OK;
}
```

#### Error Context Propagation
```c
result_t high_level_function(pool_t* pool) {
    if (low_level_function(pool) != RESULT_OK) {
        RETURN_ERR("High-level operation failed due to low-level error");
    }
    
    return RESULT_OK;
}
```

### Error Debugging

#### Adding Context to Errors
```c
#define RETURN_ERR_CTX(msg, ctx) do { \
    printf("ERROR: %s (context: %s)\n", msg, ctx); \
    return RESULT_ERR; \
} while(0)

result_t function_with_context(pool_t* pool, const char* operation) {
    if (some_failure()) {
        RETURN_ERR_CTX("Operation failed", operation);
    }
    
    return RESULT_OK;
}
```

## Testing

### Testing Strategy

1. **Unit Tests**: Test individual functions in isolation
2. **Integration Tests**: Test component interactions
3. **System Tests**: Test complete agent workflows
4. **Performance Tests**: Validate memory and CPU characteristics
5. **Stress Tests**: Test under resource pressure

### Unit Testing Framework

Since LKJAgent has no external dependencies, implement a simple testing framework:

```c
// test_framework.h
#define TEST(name) \
    static result_t test_##name(void); \
    static result_t test_##name(void)

#define ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf("ASSERTION FAILED: %s:%d - Expected %ld, got %ld\n", \
               __FILE__, __LINE__, (long)(expected), (long)(actual)); \
        return RESULT_ERR; \
    } \
} while(0)

#define RUN_TEST(name) do { \
    printf("Running test_%s... ", #name); \
    if (test_##name() == RESULT_OK) { \
        printf("PASSED\n"); \
    } else { \
        printf("FAILED\n"); \
        return RESULT_ERR; \
    } \
} while(0)
```

### Example Unit Tests

```c
// test_pool.c
#include "test_framework.h"
#include "utils/pool.h"

TEST(pool_init) {
    pool_t pool;
    ASSERT_EQ(RESULT_OK, pool_init(&pool));
    ASSERT_EQ(POOL_STRING16_MAXCOUNT, pool.string16_freelist_count);
    return RESULT_OK;
}

TEST(string_allocation) {
    pool_t pool;
    string_t* str;
    
    ASSERT_EQ(RESULT_OK, pool_init(&pool));
    ASSERT_EQ(RESULT_OK, pool_string16_alloc(&pool, &str));
    ASSERT_EQ(POOL_STRING16_MAXCOUNT - 1, pool.string16_freelist_count);
    ASSERT_EQ(RESULT_OK, pool_string_free(&pool, str));
    ASSERT_EQ(POOL_STRING16_MAXCOUNT, pool.string16_freelist_count);
    
    return RESULT_OK;
}

int main() {
    RUN_TEST(pool_init);
    RUN_TEST(string_allocation);
    printf("All tests passed!\n");
    return 0;
}
```

### Integration Testing

```c
// test_agent_integration.c
TEST(agent_full_cycle) {
    lkjagent_t agent;
    
    // Initialize agent
    ASSERT_EQ(RESULT_OK, lkjagent_init(&agent));
    
    // Run one cycle
    ASSERT_EQ(RESULT_OK, lkjagent_agent(&agent.pool, &agent.config, &agent.agent));
    
    // Verify state is valid
    object_t* state_obj;
    ASSERT_EQ(RESULT_OK, object_provide_str(&agent.pool, &state_obj, 
                                           agent.agent.data, "state"));
    
    // Clean up
    ASSERT_EQ(RESULT_OK, lkjagent_deinit(&agent));
    
    return RESULT_OK;
}
```

### Performance Testing

```c
// test_performance.c
TEST(memory_pool_performance) {
    pool_t pool;
    clock_t start, end;
    const int iterations = 100000;
    
    ASSERT_EQ(RESULT_OK, pool_init(&pool));
    
    start = clock();
    for (int i = 0; i < iterations; i++) {
        string_t* str;
        ASSERT_EQ(RESULT_OK, pool_string256_alloc(&pool, &str));
        ASSERT_EQ(RESULT_OK, pool_string_free(&pool, str));
    }
    end = clock();
    
    double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Performance: %d allocations in %f seconds\n", iterations, cpu_time);
    
    return RESULT_OK;
}
```

## Debugging

### Debugging Tools and Techniques

#### GDB Usage

```bash
# Compile with debug symbols
make clean && CFLAGS="-g -O0" make

# Start debugging
gdb ./build/lkjagent

# Useful GDB commands
(gdb) break lkjagent_agent
(gdb) run
(gdb) pool_status pool    # Custom function defined in .gdbinit
(gdb) print agent->data
(gdb) call object_print(agent->data)  # If you implement object_print
```

#### Memory Debugging with Valgrind

```bash
# Check for memory leaks
valgrind --leak-check=full --show-leak-kinds=all ./build/lkjagent

# Check for memory errors
valgrind --tool=memcheck --track-origins=yes ./build/lkjagent
```

#### Runtime Debugging

```c
// Add debugging macros
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) printf("DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

result_t function_with_debugging(pool_t* pool) {
    DEBUG_PRINT("Entering function_with_debugging");
    DEBUG_PRINT("Pool string16 free count: %lu", pool->string16_freelist_count);
    
    // Function implementation...
    
    DEBUG_PRINT("Exiting function_with_debugging");
    return RESULT_OK;
}
```

### Common Debugging Scenarios

#### Memory Pool Exhaustion
```c
// Add pool monitoring
void debug_pool_status(pool_t* pool, const char* context) {
    printf("POOL STATUS [%s]:\n", context);
    printf("  String16: %lu/%lu free\n", 
           pool->string16_freelist_count, POOL_STRING16_MAXCOUNT);
    printf("  String256: %lu/%lu free\n", 
           pool->string256_freelist_count, POOL_STRING256_MAXCOUNT);
    // ... other pools
    printf("  Objects: %lu/%lu free\n", 
           pool->object_freelist_count, POOL_OBJECT_MAXCOUNT);
}

// Use in functions that might exhaust pools
result_t monitored_function(pool_t* pool) {
    debug_pool_status(pool, "Start of monitored_function");
    
    // Function implementation...
    
    debug_pool_status(pool, "End of monitored_function");
    return RESULT_OK;
}
```

#### JSON Parsing Issues
```c
// Add JSON debugging
result_t debug_json_parse(pool_t* pool, const string_t* json, object_t** result) {
    printf("DEBUG: Parsing JSON (%lu bytes): %.*s\n", 
           json->size, (int)json->size, json->data);
    
    result_t parse_result = object_parse_json(pool, result, json);
    
    if (parse_result != RESULT_OK) {
        printf("DEBUG: JSON parse failed\n");
        // Add character-by-character analysis if needed
        for (uint64_t i = 0; i < json->size && i < 100; i++) {
            printf("  [%lu]: '%c' (0x%02x)\n", i, json->data[i], json->data[i]);
        }
    } else {
        printf("DEBUG: JSON parse succeeded\n");
    }
    
    return parse_result;
}
```

## Performance Optimization

### Profiling and Measurement

#### Basic Performance Monitoring
```c
#include <time.h>

typedef struct {
    clock_t start;
    clock_t end;
    const char* operation;
} perf_timer_t;

void perf_start(perf_timer_t* timer, const char* operation) {
    timer->operation = operation;
    timer->start = clock();
}

void perf_end(perf_timer_t* timer) {
    timer->end = clock();
    double cpu_time = ((double)(timer->end - timer->start)) / CLOCKS_PER_SEC;
    printf("PERF: %s took %f seconds\n", timer->operation, cpu_time);
}

// Usage
result_t timed_function(pool_t* pool) {
    perf_timer_t timer;
    perf_start(&timer, "timed_function");
    
    // Function implementation...
    
    perf_end(&timer);
    return RESULT_OK;
}
```

#### Memory Usage Tracking
```c
typedef struct {
    uint64_t strings_allocated;
    uint64_t objects_allocated;
    uint64_t peak_memory_usage;
} memory_stats_t;

void update_memory_stats(pool_t* pool, memory_stats_t* stats) {
    uint64_t current_usage = 
        (POOL_STRING16_MAXCOUNT - pool->string16_freelist_count) * 16 +
        (POOL_STRING256_MAXCOUNT - pool->string256_freelist_count) * 256 +
        // ... other pools
        (POOL_OBJECT_MAXCOUNT - pool->object_freelist_count) * sizeof(object_t);
    
    if (current_usage > stats->peak_memory_usage) {
        stats->peak_memory_usage = current_usage;
    }
}
```

### Optimization Strategies

#### String Operations
```c
// Efficient string building
result_t build_large_string(pool_t* pool, string_t** result) {
    // Pre-calculate required size to avoid reallocations
    uint64_t estimated_size = calculate_size_needed();
    
    if (string_create_with_capacity(pool, result, estimated_size) != RESULT_OK) {
        RETURN_ERR("Failed to create string with estimated capacity");
    }
    
    // Build string efficiently...
    
    return RESULT_OK;
}
```

#### Object Traversal
```c
// Cache frequently accessed objects
typedef struct {
    object_t* working_memory;
    object_t* storage;
    object_t* current_state;
} agent_cache_t;

result_t cache_agent_objects(pool_t* pool, agent_t* agent, agent_cache_t* cache) {
    if (object_provide_str(pool, &cache->working_memory, 
                          agent->data, "working_memory") != RESULT_OK) {
        RETURN_ERR("Failed to cache working_memory");
    }
    
    // Cache other frequently accessed objects...
    
    return RESULT_OK;
}
```

## Extending the Agent

### Adding New Actions

1. **Define the action handler:**
```c
// In agent/actions.c
result_t agent_actions_command_my_new_action(
    pool_t* pool, 
    config_t* config, 
    agent_t* agent, 
    object_t* action_obj
) {
    object_t* type_obj, *tags_obj, *value_obj;
    
    // Extract action parameters
    if (agent_actions_extract_action_params(pool, action_obj, 
                                           &type_obj, &tags_obj, &value_obj) != RESULT_OK) {
        RETURN_ERR("Failed to extract action parameters");
    }
    
    // Validate parameters
    if (agent_actions_validate_action_params(type_obj, tags_obj, value_obj, 
                                           "my_new_action", 1) != RESULT_OK) {
        RETURN_ERR("Invalid action parameters");
    }
    
    // Implement action logic here...
    printf("Executing my_new_action with value: %.*s\n", 
           (int)value_obj->string->size, value_obj->string->data);
    
    return RESULT_OK;
}
```

2. **Add function declaration to header:**
```c
// In agent/actions.h
__attribute__((warn_unused_result)) result_t agent_actions_command_my_new_action(
    pool_t* pool, 
    config_t* config, 
    agent_t* agent, 
    object_t* action_obj
);
```

3. **Register in dispatch table:**
```c
// In agent_actions_dispatch function
if (string_equal_str(type_obj->string, "my_new_action")) {
    return agent_actions_command_my_new_action(pool, config, agent, action_obj);
}
```

### Adding New States

1. **Add state configuration:**
```json
{
  "agent": {
    "state": {
      "my_new_state": {
        "prompt": "You are in my_new_state. Your role is to..."
      }
    }
  }
}
```

2. **Handle state transitions:**
```c
// In agent/state.c, modify transition logic
result_t agent_state_handle_my_new_state_transition(
    pool_t* pool, 
    config_t* config, 
    agent_t* agent, 
    object_t* response_obj
) {
    // Implement state-specific transition logic
    return RESULT_OK;
}
```

### Adding New Utility Functions

1. **Create utility function:**
```c
// In utils/my_utility.c
#include "utils/my_utility.h"

result_t my_utility_function(pool_t* pool, const string_t* input, string_t** output) {
    // Implement utility function
    return RESULT_OK;
}
```

2. **Add header file:**
```c
// utils/my_utility.h
#ifndef LKJAGENT_UTILS_MY_UTILITY_H
#define LKJAGENT_UTILS_MY_UTILITY_H

#include "global/types.h"
#include "utils/pool.h"
#include "utils/string.h"

__attribute__((warn_unused_result)) result_t my_utility_function(
    pool_t* pool, 
    const string_t* input, 
    string_t** output
);

#endif
```

3. **Update build system:**
```makefile
# In Makefile, add to SRCS if needed (auto-discovered)
# Update includes if necessary
```

## Contributing

### Contribution Workflow

1. **Fork the repository**
2. **Create a feature branch:** `git checkout -b feature/my-new-feature`
3. **Make changes following coding standards**
4. **Add tests for new functionality**
5. **Ensure all tests pass**
6. **Update documentation as needed**
7. **Submit a pull request**

### Code Review Checklist

- [ ] Code follows established coding standards
- [ ] All functions have appropriate error handling
- [ ] Memory allocation/deallocation is properly paired
- [ ] New functionality includes tests
- [ ] Documentation is updated
- [ ] No compiler warnings
- [ ] Performance impact is acceptable

### Commit Message Format

```
type(scope): brief description

Longer description if needed, explaining what changed and why.

- List specific changes
- Include any breaking changes
- Reference issues if applicable

Closes #123
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation update
- `style`: Code style changes
- `refactor`: Code refactoring
- `test`: Adding tests
- `perf`: Performance improvements

### Release Process

1. **Update version numbers**
2. **Update CHANGELOG.md**
3. **Create release branch**
4. **Run full test suite**
5. **Create release tag**
6. **Build and test release artifacts**
7. **Publish release**

---

This developer guide provides comprehensive information for contributing to and extending LKJAgent. For specific questions or clarifications, please refer to the source code or create an issue in the project repository.
