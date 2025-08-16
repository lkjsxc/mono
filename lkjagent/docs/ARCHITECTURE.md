# LKJAgent Architecture

This document provides a comprehensive overview of LKJAgent's architecture, including design principles, system components, data flows, and technical decisions that shape the implementation.

## Table of Contents

- [Design Philosophy](#design-philosophy)
- [System Overview](#system-overview)
- [Component Architecture](#component-architecture)
- [Memory Architecture](#memory-architecture)
- [State Machine Design](#state-machine-design)
- [Data Flow Architecture](#data-flow-architecture)
- [API Design](#api-design)
- [Error Handling Architecture](#error-handling-architecture)
- [Performance Architecture](#performance-architecture)
- [Security Architecture](#security-architecture)
- [Extensibility Architecture](#extensibility-architecture)
- [Deployment Architecture](#deployment-architecture)

## Design Philosophy

### Core Principles

1. **Zero External Dependencies**: LKJAgent is built using only standard C library functions to ensure maximum portability and minimal attack surface.

2. **Memory Safety**: Custom memory pool management prevents fragmentation, provides bounded resource usage, and eliminates most memory-related bugs.

3. **Predictable Performance**: Pre-allocated memory pools and fixed-size structures ensure consistent performance characteristics regardless of runtime duration.

4. **Fail-Fast Error Handling**: Errors are detected early and propagated clearly through the system with comprehensive context information.

5. **Modularity**: Clear separation of concerns with well-defined interfaces between components enables independent development and testing.

6. **Simplicity**: The architecture favors straightforward, readable implementations over complex optimizations that sacrifice maintainability.

### Design Goals

- **Reliability**: System should handle errors gracefully and continue operation when possible
- **Performance**: Efficient memory usage and fast execution for real-time agent operations
- **Maintainability**: Code should be easy to understand, modify, and extend
- **Portability**: Should compile and run on any system with a C11-compliant compiler
- **Scalability**: Architecture should support different deployment scenarios from embedded to server

## System Overview

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              LKJAgent System                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐         │
│  │   Application   │    │   Configuration │    │   Persistent    │         │
│  │   Controller    │◄──►│    Manager      │◄──►│    Storage      │         │
│  │   (lkjagent.c)  │    │                 │    │  (JSON files)   │         │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘         │
│           │                       │                       │                 │
│           ▼                       ▼                       ▼                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │                        Agent Core Engine                                │ │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐        │ │
│  │  │     Prompt      │  │      State      │  │     Action      │        │ │
│  │  │   Generation    │  │   Management    │  │     System      │        │ │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘        │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
│           │                       │                       │                 │
│           ▼                       ▼                       ▼                 │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐         │
│  │      HTTP       │    │     Memory      │    │     JSON        │         │
│  │   Client        │    │     Pool        │    │   Processing    │         │
│  │                 │    │   Management    │    │                 │         │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘         │
│           │                       │                       │                 │
│           ▼                       ▼                       ▼                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │                       Utility Foundation                                │ │
│  │    String Utils   │   File I/O   │   Object Management  │  HTTP Utils   │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### System Layers

#### Layer 1: Application Layer
- **Purpose**: Main application entry point and lifecycle management
- **Components**: `lkjagent.c`, initialization, main loop, cleanup
- **Responsibilities**: System startup, configuration loading, agent execution orchestration

#### Layer 2: Agent Core Layer
- **Purpose**: Core agent intelligence and decision-making
- **Components**: Agent core, state management, action system, prompt generation
- **Responsibilities**: Agent reasoning, state transitions, action execution, LLM communication

#### Layer 3: Service Layer
- **Purpose**: Specialized services supporting agent operations
- **Components**: HTTP client, memory pool manager, JSON processor
- **Responsibilities**: External communication, memory management, data serialization

#### Layer 4: Utility Layer
- **Purpose**: Foundation utilities and primitives
- **Components**: String utilities, file I/O, object management, HTTP utilities
- **Responsibilities**: Basic data structures, I/O operations, utility functions

## Component Architecture

### Core Components

#### Application Controller (`lkjagent.c`)

```c
// Responsibility: System lifecycle management
typedef struct {
    pool_t pool;        // Memory management
    config_t config;    // System configuration  
    agent_t agent;      // Agent instance
} lkjagent_t;

// Main functions:
// - lkjagent_init(): System initialization
// - lkjagent_run(): Main execution loop
// - lkjagent_deinit(): Cleanup and shutdown
```

**Design Decisions:**
- Single instance design for simplicity
- Clear initialization/cleanup lifecycle
- Centralized resource management

#### Agent Core Engine (`agent/core.c`)

```c
// Responsibility: Agent reasoning and execution
// Main cycle:
// 1. Preflight validation
// 2. Prompt generation  
// 3. LLM communication
// 4. Response processing
// 5. Action execution
// 6. State management
// 7. Memory synchronization

result_t lkjagent_agent(pool_t* pool, config_t* config, agent_t* agent);
```

**Design Decisions:**
- Synchronous execution for predictable behavior
- Clear separation of concerns within the cycle
- Comprehensive error handling at each step

#### State Management System (`agent/state.c`)

```c
// Responsibility: Agent state transitions and memory
// States: thinking, evaluating, commanding
// Features:
// - Automatic state transitions
// - Log management with rotation
// - Memory limit monitoring
// - Context synchronization

result_t agent_state_update_state(pool_t* pool, agent_t* agent, const char* new_state);
```

**Design Decisions:**
- Finite state machine for predictable behavior
- Automatic transitions based on context
- Built-in memory management

#### Action System (`agent/actions.c`)

```c
// Responsibility: Action dispatch and execution
// Actions: working_memory_*, storage_*
// Features:
// - Type-based dispatch
// - Parameter validation
// - Result logging

result_t agent_actions_dispatch(pool_t* pool, config_t* config, agent_t* agent, object_t* action);
```

**Design Decisions:**
- Extensible action registry
- Consistent parameter handling
- Isolated action execution

### Support Components

#### Memory Pool Manager (`utils/pool.c`)

```c
// Responsibility: High-performance memory management
// Features:
// - Size-based allocation pools
// - Zero fragmentation
// - Bounded resource usage
// - Freelist management

typedef struct {
    char string16_data[POOL_STRING16_MAXCOUNT * 16];
    string_t string16[POOL_STRING16_MAXCOUNT];
    string_t* string16_freelist_data[POOL_STRING16_MAXCOUNT];
    uint64_t string16_freelist_count;
    // ... additional pools for different sizes
} pool_t;
```

**Design Decisions:**
- Pre-allocated pools for consistent performance
- Multiple size classes for efficiency
- Freelist management for O(1) allocation/deallocation

#### JSON Object System (`utils/object.c`)

```c
// Responsibility: JSON parsing and manipulation
// Features:
// - Custom parser with no dependencies
// - Tree-based object representation
// - Path-based navigation
// - Memory pool integration

typedef struct object_t {
    string_t* string;           // Leaf value
    struct object_t* child;     // First child (for objects/arrays)
    struct object_t* next;      // Next sibling
} object_t;
```

**Design Decisions:**
- Tree structure for natural JSON representation
- Path-based access for convenience
- Memory pool integration for performance

#### HTTP Client (`utils/http.c`)

```c
// Responsibility: LLM communication
// Features:
// - OpenAI-compatible API support
// - Configurable timeouts and retries
// - JSON request/response handling
// - Error recovery

result_t agent_http_send_receive(pool_t* pool, config_t* config, 
                                const string_t* prompt, string_t** response);
```

**Design Decisions:**
- Synchronous communication for simplicity
- Built-in retry logic for reliability
- Configuration-driven endpoint management

## Memory Architecture

### Memory Pool Design

#### Pool Structure

```
Memory Pool Layout:
┌─────────────────────────────────────────────────────────────────────────────┐
│                              Pool Structure                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│  String Pools:                                                             │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐         │
│  │   16-byte   │ │  256-byte   │ │   4KB       │ │   64KB      │ │ 1MB    │
│  │ 1M entries  │ │ 64K entries │ │ 4K entries  │ │ 256 entries │ │16 ents │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘ └───────┘ │
│                                                                             │
│  Object Pool:                                                              │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │                     64K Object Entries                                 │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
│                                                                             │
│  Freelists (per pool):                                                     │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │  Available Object Pointers + Count                                     │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### Allocation Strategy

```c
// Size-based allocation
result_t pool_string_alloc(pool_t* pool, string_t** string, uint64_t capacity) {
    if (capacity <= 16) return pool_string16_alloc(pool, string);
    if (capacity <= 256) return pool_string256_alloc(pool, string);
    if (capacity <= 4096) return pool_string4096_alloc(pool, string);
    if (capacity <= 65536) return pool_string65536_alloc(pool, string);
    if (capacity <= 1048576) return pool_string1048576_alloc(pool, string);
    RETURN_ERR("Capacity exceeds maximum pool size");
}
```

#### Memory Layout Benefits

1. **Predictable Performance**: O(1) allocation/deallocation
2. **Zero Fragmentation**: Fixed-size allocations
3. **Bounded Resources**: Clear memory limits
4. **Cache Efficiency**: Contiguous memory layout
5. **Simple Management**: No complex garbage collection

### Memory Usage Patterns

#### Typical Agent Memory Profile

```
Agent Memory Usage:
┌─────────────────────────────────────────────────────────────────────────────┐
│ Component          │ Pool Type    │ Typical Usage    │ Peak Usage         │
├─────────────────────────────────────────────────────────────────────────────┤
│ Configuration      │ 4KB strings  │ 1-2 objects     │ 3-4 objects        │
│ Agent Memory       │ 4KB strings  │ 2-3 objects     │ 5-6 objects        │
│ Working Memory     │ 256B strings │ 10-20 objects   │ 50-100 objects     │
│ LLM Communication  │ 64KB strings │ 2-4 strings     │ 6-8 strings        │
│ Temporary Objects  │ 16B strings  │ 50-100 objects  │ 200-500 objects    │
│ JSON Processing    │ Objects      │ 100-200 objects │ 500-1000 objects   │
└─────────────────────────────────────────────────────────────────────────────┘
```

## State Machine Design

### State Definitions

#### State Transition Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           Agent State Machine                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                           ┌─────────────┐                                   │
│                           │             │                                   │
│                     ┌────►│  THINKING   │◄────┐                             │
│                     │     │             │     │                             │
│                     │     └─────────────┘     │                             │
│                     │           │             │                             │
│                     │           │ analyze     │ reassess                    │
│                     │           │ situation   │                             │
│                     │           ▼             │                             │
│                     │     ┌─────────────┐     │                             │
│              reset  │     │             │ evaluate                          │
│                     │     │ COMMANDING  │ progress                          │
│                     │     │             │     │                             │
│                     │     └─────────────┘     │                             │
│                     │           │             │                             │
│                     │           │ execute     │                             │
│                     │           │ action      │                             │
│                     │           ▼             │                             │
│                     │     ┌─────────────┐     │                             │
│                     └─────│             │─────┘                             │
│                           │ EVALUATING  │                                   │
│                           │             │                                   │
│                           └─────────────┘                                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### State Behaviors

```c
// State-specific behaviors
typedef struct {
    const char* name;
    const char* prompt_template;
    result_t (*enter_state)(pool_t* pool, config_t* config, agent_t* agent);
    result_t (*process_response)(pool_t* pool, config_t* config, agent_t* agent, object_t* response);
    result_t (*exit_state)(pool_t* pool, config_t* config, agent_t* agent);
} state_definition_t;

// Example state definitions
static const state_definition_t states[] = {
    {
        .name = "thinking",
        .prompt_template = "thinking_prompt_template",
        .enter_state = thinking_enter,
        .process_response = thinking_process,
        .exit_state = thinking_exit
    },
    // ... other states
};
```

### State Management Architecture

#### State Data Structure

```c
// Agent state representation
typedef struct {
    object_t* data;    // Complete agent state as JSON tree
    // Key fields within data:
    // - "state": current state name
    // - "working_memory": current session data
    // - "storage": persistent data
    // - "think_log_*": thinking history
    // - "evaluation_log_*": evaluation history
    // - "command_log_*": action history
} agent_t;
```

#### State Persistence

```c
// State synchronization
result_t agent_state_sync_logs_to_working_memory(pool_t* pool, agent_t* agent) {
    // Synchronize all logs with working memory
    // Ensures consistent view across components
    // Manages memory limits through log rotation
}

result_t agent_actions_save_memory(pool_t* pool, agent_t* agent) {
    // Persist agent state to disk
    // Atomic write operations
    // Backup and recovery support
}
```

## Data Flow Architecture

### Request Processing Flow

#### Complete Agent Cycle

```
Agent Processing Cycle:
┌─────────────────────────────────────────────────────────────────────────────┐
│  Start Cycle                                                                │
│       │                                                                     │
│       ▼                                                                     │
│  ┌─────────────┐                                                            │
│  │ Preflight   │ ◄── Validate configuration, memory, state                 │
│  │ Validation  │                                                            │
│  └─────────────┘                                                            │
│       │                                                                     │
│       ▼                                                                     │
│  ┌─────────────┐                                                            │
│  │ Prompt      │ ◄── Generate prompt based on current state and memory     │
│  │ Generation  │                                                            │
│  └─────────────┘                                                            │
│       │                                                                     │
│       ▼                                                                     │
│  ┌─────────────┐                                                            │
│  │ HTTP        │ ◄── Send prompt to LLM, receive response                  │
│  │ Communication│                                                            │
│  └─────────────┘                                                            │
│       │                                                                     │
│       ▼                                                                     │
│  ┌─────────────┐                                                            │
│  │ Response    │ ◄── Parse LLM response, extract actions/state changes     │
│  │ Processing  │                                                            │
│  └─────────────┘                                                            │
│       │                                                                     │
│       ▼                                                                     │
│  ┌─────────────┐                                                            │
│  │ Action      │ ◄── Execute actions if present in response                │
│  │ Execution   │                                                            │
│  └─────────────┘                                                            │
│       │                                                                     │
│       ▼                                                                     │
│  ┌─────────────┐                                                            │
│  │ State       │ ◄── Update agent state, manage transitions                │
│  │ Management  │                                                            │
│  └─────────────┘                                                            │
│       │                                                                     │
│       ▼                                                                     │
│  ┌─────────────┐                                                            │
│  │ Memory      │ ◄── Synchronize logs, save state, manage memory limits    │
│  │ Sync        │                                                            │
│  └─────────────┘                                                            │
│       │                                                                     │
│       ▼                                                                     │
│  End Cycle                                                                  │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Data Transformation Pipeline

#### JSON Processing Pipeline

```
JSON Data Flow:
┌─────────────────────────────────────────────────────────────────────────────┐
│  Raw JSON String                                                           │
│       │                                                                     │
│       ▼                                                                     │
│  ┌─────────────┐                                                            │
│  │ Tokenizer   │ ◄── Break JSON into tokens                                │
│  └─────────────┘                                                            │
│       │                                                                     │
│       ▼                                                                     │
│  ┌─────────────┐                                                            │
│  │ Parser      │ ◄── Build object tree from tokens                         │
│  └─────────────┘                                                            │
│       │                                                                     │
│       ▼                                                                     │
│  ┌─────────────┐                                                            │
│  │ Object Tree │ ◄── Navigate using path strings                          │
│  │ Navigation  │                                                            │
│  └─────────────┘                                                            │
│       │                                                                     │
│       ▼                                                                     │
│  ┌─────────────┐                                                            │
│  │ Value       │ ◄── Extract/modify values                                 │
│  │ Access      │                                                            │
│  └─────────────┘                                                            │
│       │                                                                     │
│       ▼                                                                     │
│  Application Data                                                           │
└─────────────────────────────────────────────────────────────────────────────┘
```

## API Design

### Interface Design Principles

1. **Consistent Return Types**: All functions that can fail return `result_t`
2. **Clear Ownership**: Memory ownership explicitly managed through pools
3. **Immutable Inputs**: Input parameters are const where possible
4. **Output Parameters**: Output values returned through pointer parameters
5. **Error Context**: Rich error messages with context information

### API Layers

#### Low-Level Utilities

```c
// String management
result_t string_create(pool_t* pool, string_t** string);
result_t string_assign_str(string_t* string, const char* str);
result_t string_append_str(string_t* string, const char* str);

// Object management  
result_t object_create(pool_t* pool, object_t** object);
result_t object_parse_json(pool_t* pool, object_t** object, const string_t* json);
result_t object_provide_str(pool_t* pool, object_t** result, object_t* obj, const char* path);
```

#### Mid-Level Services

```c
// File operations
result_t file_read(pool_t* pool, const char* filename, string_t** content);
result_t file_write(const char* filename, const string_t* content);

// HTTP communication
result_t agent_http_send_receive(pool_t* pool, config_t* config, 
                                const string_t* prompt, string_t** response);
```

#### High-Level Agent APIs

```c
// Agent lifecycle
result_t lkjagent_init(lkjagent_t* lkjagent);
result_t lkjagent_agent(pool_t* pool, config_t* config, agent_t* agent);
result_t lkjagent_deinit(lkjagent_t* lkjagent);

// State management
result_t agent_state_update_state(pool_t* pool, agent_t* agent, const char* new_state);
result_t agent_state_auto_transition(pool_t* pool, config_t* config, agent_t* agent);

// Action system
result_t agent_actions_dispatch(pool_t* pool, config_t* config, agent_t* agent, object_t* action);
```

### API Evolution Strategy

- **Backward Compatibility**: Maintain API stability across minor versions
- **Deprecation Process**: Clear deprecation warnings before API removal
- **Extension Points**: Plugin-style interfaces for new functionality
- **Versioning**: Semantic versioning for API changes

## Error Handling Architecture

### Error Categories

#### System Errors
- Memory allocation failures
- File I/O errors
- Network communication failures
- Configuration errors

#### Logic Errors
- Invalid state transitions
- Malformed JSON
- Protocol violations
- Action execution failures

#### Resource Errors
- Pool exhaustion
- Memory limits exceeded
- Timeout errors
- Rate limiting

### Error Propagation Strategy

```c
// Error context propagation
typedef struct {
    result_t code;
    const char* message;
    const char* function;
    const char* file;
    int line;
} error_context_t;

#define RETURN_ERR_CTX(msg) do { \
    printf("ERROR: %s (%s:%s:%d)\n", msg, __FILE__, __FUNCTION__, __LINE__); \
    return RESULT_ERR; \
} while(0)
```

### Recovery Strategies

#### Graceful Degradation
- Continue operation with reduced functionality
- Fall back to default behaviors
- Retry operations with backoff

#### State Recovery
- Reset to known good states
- Clear problematic memory
- Reload configuration

#### Error Isolation
- Contain errors within components
- Prevent error propagation
- Maintain system stability

## Performance Architecture

### Performance Goals

1. **Memory Efficiency**: Minimal memory footprint with bounded usage
2. **CPU Efficiency**: Fast execution with predictable performance
3. **I/O Efficiency**: Optimal file and network operations
4. **Scalability**: Performance scales with system resources

### Optimization Strategies

#### Memory Optimizations

```c
// Pool-based allocation for O(1) performance
result_t pool_string16_alloc(pool_t* pool, string_t** string) {
    if (pool->string16_freelist_count == 0) {
        RETURN_ERR("String16 pool exhausted");
    }
    
    *string = pool->string16_freelist_data[--pool->string16_freelist_count];
    (*string)->size = 0;
    return RESULT_OK;
}
```

#### CPU Optimizations

```c
// Efficient string comparison
uint64_t string_equal_str(const string_t* string, const char* str) {
    size_t str_len = strlen(str);
    if (string->size != str_len) return 0;
    return memcmp(string->data, str, str_len) == 0;
}
```

#### I/O Optimizations

```c
// Buffered file operations
result_t file_read_buffered(pool_t* pool, const char* filename, string_t** content) {
    // Use large buffer for efficient file reading
    // Minimize system calls
    // Handle partial reads correctly
}
```

### Performance Monitoring

#### Built-in Metrics

```c
// Pool utilization tracking
typedef struct {
    uint64_t total_allocations;
    uint64_t current_usage;
    uint64_t peak_usage;
    uint64_t allocation_failures;
} pool_stats_t;

void pool_report_stats(const pool_t* pool) {
    printf("Pool Statistics:\n");
    printf("  String16: %lu/%lu used\n", 
           POOL_STRING16_MAXCOUNT - pool->string16_freelist_count,
           POOL_STRING16_MAXCOUNT);
    // ... other pools
}
```

## Security Architecture

### Security Principles

1. **Input Validation**: All external inputs validated and sanitized
2. **Memory Safety**: Bounds checking and pool-based allocation
3. **Privilege Separation**: Minimal required permissions
4. **Error Information**: Avoid leaking sensitive information in errors

### Security Features

#### Input Sanitization

```c
// JSON input validation
result_t object_parse_json_safe(pool_t* pool, object_t** object, const string_t* json) {
    // Validate JSON size limits
    if (json->size > MAX_JSON_SIZE) {
        RETURN_ERR("JSON size exceeds limit");
    }
    
    // Validate JSON depth limits  
    // Validate character encoding
    // Prevent injection attacks
    
    return object_parse_json(pool, object, json);
}
```

#### Memory Protection

```c
// Bounds-checked string operations
result_t string_append_str_safe(string_t* string, const char* str) {
    size_t str_len = strlen(str);
    if (string->size + str_len >= string->capacity) {
        RETURN_ERR("String append would exceed capacity");
    }
    
    memcpy(string->data + string->size, str, str_len);
    string->size += str_len;
    string->data[string->size] = '\0';
    
    return RESULT_OK;
}
```

## Extensibility Architecture

### Extension Points

#### Action System Extensions

```c
// New action registration
typedef struct {
    const char* action_type;
    result_t (*handler)(pool_t* pool, config_t* config, agent_t* agent, object_t* action);
} action_handler_t;

// Action registry for dynamic dispatch
static const action_handler_t action_handlers[] = {
    {"working_memory_add", agent_actions_command_working_memory_add},
    {"storage_save", agent_actions_command_storage_save},
    // ... extensible action list
    {NULL, NULL}  // Sentinel
};
```

#### State System Extensions

```c
// Custom state behaviors
typedef struct {
    const char* state_name;
    result_t (*enter)(pool_t* pool, config_t* config, agent_t* agent);
    result_t (*process)(pool_t* pool, config_t* config, agent_t* agent, object_t* response);
    result_t (*exit)(pool_t* pool, config_t* config, agent_t* agent);
} state_handler_t;
```

#### Utility Extensions

```c
// Plugin-style utility registration
typedef struct {
    const char* utility_name;
    void* function_pointer;
} utility_registration_t;
```

### Configuration Extensions

#### Dynamic Configuration

```json
{
  "extensions": {
    "custom_actions": [
      {
        "name": "my_custom_action",
        "handler": "my_custom_handler",
        "config": { /* action-specific config */ }
      }
    ],
    "custom_states": [
      {
        "name": "my_custom_state", 
        "prompt": "Custom state prompt template",
        "transitions": ["thinking", "commanding"]
      }
    ]
  }
}
```

## Deployment Architecture

### Deployment Scenarios

#### Standalone Deployment

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Standalone Deployment                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │                        Host System                                      │ │
│  │                                                                         │ │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐               │ │
│  │  │  LKJAgent   │    │    Data     │    │   Config    │               │ │
│  │  │ Executable  │◄──►│   Files     │◄──►│   Files     │               │ │
│  │  └─────────────┘    └─────────────┘    └─────────────┘               │ │
│  │         │                                       │                     │ │
│  │         ▼                                       ▼                     │ │
│  │  ┌─────────────────────────────────────────────────────────────────────┐ │ │
│  │  │                  External LLM Service                              │ │ │
│  │  └─────────────────────────────────────────────────────────────────────┘ │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### Container Deployment

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Container Deployment                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │                       Docker Host                                       │ │
│  │                                                                         │ │
│  │  ┌─────────────────────────────────────────────────────────────────────┐ │ │
│  │  │                  LKJAgent Container                                 │ │ │
│  │  │                                                                     │ │ │
│  │  │  ┌─────────────┐                                                   │ │ │
│  │  │  │  LKJAgent   │                                                   │ │ │
│  │  │  │ Executable  │                                                   │ │ │
│  │  │  └─────────────┘                                                   │ │ │
│  │  └─────────────────────────────────────────────────────────────────────┘ │ │
│  │          │                                                               │ │
│  │          ▼                                                               │ │
│  │  ┌─────────────────────────────────────────────────────────────────────┐ │ │
│  │  │                   Mounted Volumes                                   │ │ │
│  │  │                                                                     │ │ │
│  │  │  /app/data ◄──► /host/path/data                                    │ │ │
│  │  └─────────────────────────────────────────────────────────────────────┘ │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
│                                  │                                           │
│                                  ▼                                           │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │                     External LLM Service                                │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Build Architecture

#### Multi-Stage Docker Build

```dockerfile
# Build stage - compile the application
FROM gcc:12 as builder
COPY ./src/ /app/src
COPY ./Makefile /app
WORKDIR /app
RUN make

# Runtime stage - minimal image with just the binary
FROM scratch
COPY --from=builder /app/build/lkjagent /app/build/lkjagent
CMD ["/app/build/lkjagent"]
```

#### Cross-Platform Build

```makefile
# Support multiple architectures
ARCH := $(shell uname -m)

ifeq ($(ARCH),x86_64)
    CFLAGS += -march=x86-64
else ifeq ($(ARCH),aarch64)
    CFLAGS += -march=armv8-a
else
    CFLAGS += -march=native
endif
```

### Configuration Management

#### Environment-Specific Configurations

```bash
# Development
cp config/dev.json data/config.json

# Production  
cp config/prod.json data/config.json

# Testing
cp config/test.json data/config.json
```

#### Configuration Validation

```c
result_t config_validate(const config_t* config) {
    // Validate required fields
    // Check value ranges
    // Verify endpoint accessibility
    // Validate file permissions
    
    return RESULT_OK;
}
```

---

This architecture document provides a comprehensive view of LKJAgent's design and implementation. The architecture prioritizes simplicity, performance, and maintainability while providing a solid foundation for building sophisticated AI agents.
