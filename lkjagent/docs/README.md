# LKJAgent Documentation

LKJAgent is a sophisticated autonomous AI agent system written in C that provides a framework for building intelligent agents capable of interacting with Large Language Models (LLMs), managing memory, and executing complex workflows through a state-based architecture.

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Quick Start](#quick-start)
- [Configuration](#configuration)
- [Agent States](#agent-states)
- [Memory Management](#memory-management)
- [Actions System](#actions-system)
- [Building and Deployment](#building-and-deployment)
- [API Reference](#api-reference)
- [Development](#development)
- [Troubleshooting](#troubleshooting)

## Overview

LKJAgent is designed as a high-performance, memory-efficient AI agent framework that:

- **Communicates with LLMs** via HTTP/REST APIs
- **Manages persistent memory** through JSON-based storage
- **Implements state-based workflows** with automatic transitions
- **Provides memory pooling** for optimal performance
- **Supports containerized deployment** with Docker
- **Offers comprehensive logging** for debugging and monitoring

### Key Features

- **Zero-dependency C implementation** for maximum performance
- **Custom memory pool management** to avoid fragmentation
- **JSON parsing and manipulation** without external libraries
- **HTTP client** for LLM communication
- **State machine** for complex agent behaviors
- **Working memory and persistent storage** separation
- **Configurable logging and rotation**
- **Docker containerization** support

## Architecture

LKJAgent follows a modular architecture with clear separation of concerns:

```
lkjagent/
├── src/
│   ├── lkjagent.c          # Main application entry point
│   ├── lkjagent.h          # Primary header file
│   ├── agent/              # Core agent functionality
│   │   ├── core.c/.h       # Main agent processing loop
│   │   ├── actions.c/.h    # Action dispatch and execution
│   │   ├── state.c/.h      # State management and transitions
│   │   ├── prompt.c/.h     # Prompt generation
│   │   └── http.c/.h       # HTTP communication with LLM
│   ├── utils/              # Utility libraries
│   │   ├── pool.c/.h       # Memory pool management
│   │   ├── string.c/.h     # String manipulation
│   │   ├── object.c/.h     # JSON object handling
│   │   ├── file.c/.h       # File I/O operations
│   │   └── http.c/.h       # HTTP client utilities
│   └── global/             # Global definitions
│       ├── types.h         # Type definitions
│       ├── const.h         # Constants
│       ├── macro.h         # Utility macros
│       └── std.h           # Standard includes
├── data/
│   ├── config.json         # Agent configuration
│   └── memory.json         # Persistent agent memory
└── docs/                   # Documentation
```

### Core Components

1. **Main Application (`lkjagent.c`)**: Entry point, initialization, and main execution loop
2. **Agent Core (`agent/core.c`)**: Orchestrates LLM interactions and agent cycles
3. **Actions System (`agent/actions.c`)**: Handles action dispatch and execution
4. **State Manager (`agent/state.c`)**: Manages agent states and transitions
5. **Memory Pool (`utils/pool.c`)**: High-performance memory management
6. **JSON Parser (`utils/object.c`)**: Custom JSON parsing and manipulation
7. **HTTP Client (`utils/http.c`)**: Communication with LLM endpoints

## Quick Start

### Prerequisites

- GCC 12 or later
- Make
- Docker (optional, for containerized deployment)
- Access to an LLM endpoint (e.g., local LLM server, OpenAI-compatible API)

### Building

1. **Clone and navigate to the project:**
   ```bash
   cd lkjagent
   ```

2. **Build the project:**
   ```bash
   make
   ```

3. **Configure the agent** (see [Configuration](#configuration) section)

4. **Run the agent:**
   ```bash
   ./build/lkjagent
   ```

### Docker Deployment

1. **Build the Docker image:**
   ```bash
   docker build -t lkjagent .
   ```

2. **Run the container:**
   ```bash
   docker run -v $(pwd)/data:/app/data lkjagent
   ```

## Configuration

LKJAgent uses `data/config.json` for configuration. The configuration is structured hierarchically:

### LLM Configuration

```json
{
  "llm": {
    "endpoint": "http://host.docker.internal:1234/v1/chat/completions",
    "model": "qwen/qwen3-8b",
    "temperature": 0.7
  }
}
```

- **endpoint**: HTTP endpoint for the LLM API
- **model**: Model identifier to use
- **temperature**: Sampling temperature for LLM responses

### Agent Configuration

```json
{
  "agent": {
    "think_log": {
      "enable": true,
      "max_entries": 4,
      "key_prefix": "think_log_"
    },
    "evaluation_log": {
      "enable": true,
      "max_entries": 1,
      "key_prefix": "evaluation_log_"
    },
    "command_log": {
      "enable": true,
      "max_entries": 1,
      "key_prefix": "command_log_"
    },
    "paging_limit": {
      "enable": false,
      "max_tokens": 1024
    },
    "hard_limit": {
      "enable": false,
      "max_tokens": 2048
    },
    "iterate": {
      "max_iterations": 10
    }
  }
}
```

### State Configuration

Each agent state has its own configuration section defining prompts and behavior:

```json
{
  "agent": {
    "state": {
      "thinking": {
        "prompt": "You are an AI agent. Analyze the task and think about your approach..."
      },
      "evaluating": {
        "prompt": "Evaluate your progress and determine next steps..."
      },
      "commanding": {
        "prompt": "Execute specific actions based on your analysis..."
      }
    }
  }
}
```

## Agent States

LKJAgent implements a finite state machine with the following states:

### 1. Thinking State
- **Purpose**: Initial analysis and planning
- **Behavior**: Agent analyzes the current situation and plans actions
- **Transitions**: Can transition to any other state based on LLM response

### 2. Evaluating State
- **Purpose**: Progress assessment and decision making
- **Behavior**: Agent evaluates completed actions and determines next steps
- **Transitions**: Usually transitions back to thinking or commanding

### 3. Commanding State
- **Purpose**: Action execution
- **Behavior**: Agent executes specific actions through the actions system
- **Transitions**: Automatically transitions to evaluating after action completion

### State Transitions

State transitions are managed automatically by the agent based on:
- LLM responses containing `next_state` fields
- Action completion in commanding state
- Error handling and recovery logic

## Memory Management

LKJAgent implements a sophisticated two-tier memory system:

### Working Memory
- **Scope**: Current session data
- **Access**: Fast, in-memory access
- **Content**: Current task, recent logs, temporary data
- **Persistence**: Lost between sessions unless explicitly saved

### Persistent Storage
- **Scope**: Long-term knowledge and experience
- **Access**: Disk-based with caching
- **Content**: Saved memories, learned patterns, important data
- **Persistence**: Maintained across sessions

### Memory Pool Architecture

LKJAgent uses a custom memory pool system for optimal performance:

```c
typedef struct {
    char string16_data[POOL_STRING16_MAXCOUNT * 16];      // Small strings
    char string256_data[POOL_STRING256_MAXCOUNT * 256];   // Medium strings
    char string4096_data[POOL_STRING4096_MAXCOUNT * 4096]; // Large strings
    char string65536_data[POOL_STRING65536_MAXCOUNT * 65536]; // Very large
    char string1048576_data[POOL_STRING1048576_MAXCOUNT * 1048576]; // Huge
    object_t object_data[POOL_OBJECT_MAXCOUNT];           // JSON objects
} pool_t;
```

**Benefits:**
- **No fragmentation**: Pre-allocated pools prevent memory fragmentation
- **Predictable performance**: Constant-time allocation/deallocation
- **Resource limits**: Bounded memory usage prevents system exhaustion
- **Type-specific optimization**: Different pool sizes for different use cases

## Actions System

The actions system provides a comprehensive framework for agent capabilities:

### Action Structure

Actions are JSON objects with the following structure:

```json
{
  "action": {
    "type": "working_memory_add",
    "tags": ["important", "task"],
    "value": "Remember this important information"
  }
}
```

### Available Actions

#### Working Memory Actions
- **working_memory_add**: Add information to working memory
- **working_memory_remove**: Remove information from working memory

#### Storage Actions
- **storage_save**: Save information to persistent storage
- **storage_load**: Load information from persistent storage
- **storage_search**: Search through stored information

### Action Processing Flow

1. **Parsing**: Extract action from LLM response
2. **Validation**: Verify action structure and parameters
3. **Dispatch**: Route to appropriate handler function
4. **Execution**: Perform the requested operation
5. **Logging**: Record action results
6. **State Update**: Update agent state based on results

### Custom Actions

To add new actions:

1. Define action handler in `agent/actions.c`
2. Add function declaration to `agent/actions.h`
3. Register action in the dispatch table
4. Update documentation

## Building and Deployment

### Local Build

The project uses a standard Makefile build system:

```bash
# Clean build
make clean && make

# Debug build
make debug

# Release build
make release
```

### Build Configuration

The Makefile supports various configuration options:

```makefile
CC = gcc
CFLAGS = -Werror -Wall -Wextra -std=c11 -O2 -march=native -g
LDFLAGS = -static
INCLUDES = -Isrc/
```

### Docker Build

Multi-stage Docker build for minimal image size:

```dockerfile
FROM gcc:12 as builder
COPY ./src/ /app/src
COPY ./Makefile /app
WORKDIR /app
RUN make

FROM scratch
COPY --from=builder /app/build/lkjagent /app/build/lkjagent
CMD ["/app/build/lkjagent"]
```

### Deployment Considerations

- **Data Volume**: Mount `data/` directory for configuration persistence
- **Network Access**: Ensure LLM endpoint accessibility
- **Resource Limits**: Configure appropriate memory limits
- **Monitoring**: Set up logging and health checks

## API Reference

### Core Functions

#### `lkjagent_init(lkjagent_t* lkjagent)`
Initializes the LKJAgent system including memory pools, configuration, and agent state.

**Parameters:**
- `lkjagent`: Pointer to LKJAgent instance

**Returns:**
- `RESULT_OK` on success
- `RESULT_ERR` on failure

#### `lkjagent_agent(pool_t* pool, config_t* config, agent_t* agent)`
Executes one complete agent cycle including prompt generation, LLM communication, and response processing.

**Parameters:**
- `pool`: Memory pool for allocations
- `config`: Agent configuration
- `agent`: Agent instance with current state

**Returns:**
- `RESULT_OK` on success
- `RESULT_ERR` on failure

### Memory Management Functions

#### `pool_init(pool_t* pool)`
Initializes memory pool structures.

#### `string_create(pool_t* pool, string_t** string)`
Allocates a new string from the memory pool.

#### `object_parse_json(pool_t* pool, object_t** object, string_t* json)`
Parses JSON string into object structure.

### Action Functions

#### `agent_actions_dispatch(pool_t* pool, config_t* config, agent_t* agent, object_t* action)`
Dispatches action to appropriate handler based on action type.

### State Functions

#### `agent_state_update_state(pool_t* pool, agent_t* agent, const char* new_state)`
Updates agent state and handles transitions.

## Development

### Code Style

LKJAgent follows strict C coding standards:

- **C11 standard compliance**
- **Zero external dependencies** (except libc)
- **Comprehensive error handling** with `result_t` returns
- **Memory safety** through pool management
- **Function attributes** for compiler optimization

### Error Handling

All functions use consistent error handling:

```c
__attribute__((warn_unused_result)) result_t function_name(args) {
    if (condition_check() != RESULT_OK) {
        RETURN_ERR("Descriptive error message");
    }
    return RESULT_OK;
}
```

### Memory Management Guidelines

- **Always use memory pools** for dynamic allocation
- **Check return values** from all allocation functions
- **Properly free resources** in reverse allocation order
- **Handle allocation failures** gracefully

### Testing

Testing strategies for LKJAgent:

1. **Unit Tests**: Test individual functions in isolation
2. **Integration Tests**: Test component interactions
3. **End-to-End Tests**: Test complete agent workflows
4. **Performance Tests**: Validate memory and CPU usage
5. **Stress Tests**: Test under high load conditions

### Debugging

Common debugging techniques:

- **Preflight Validation**: Each cycle includes validation checks
- **Comprehensive Logging**: Detailed logs at each step
- **Memory Pool Statistics**: Track allocation patterns
- **State Transitions**: Log all state changes
- **Error Context**: Rich error messages with context

## Troubleshooting

### Common Issues

#### 1. LLM Communication Failures
**Symptoms**: HTTP errors, timeouts, or connection refused
**Solutions**:
- Verify LLM endpoint URL in `config.json`
- Check network connectivity
- Ensure LLM server is running and accessible
- Validate API credentials if required

#### 2. Memory Pool Exhaustion
**Symptoms**: Allocation failures, degraded performance
**Solutions**:
- Increase pool sizes in `global/const.h`
- Enable memory paging in configuration
- Optimize string usage patterns
- Review memory leak patterns

#### 3. JSON Parsing Errors
**Symptoms**: Configuration or memory loading failures
**Solutions**:
- Validate JSON syntax in configuration files
- Check file permissions and accessibility
- Verify JSON structure matches expected schema
- Review error logs for specific parsing issues

#### 4. State Transition Problems
**Symptoms**: Agent stuck in one state, infinite loops
**Solutions**:
- Review LLM responses for proper state indicators
- Check state configuration in `config.json`
- Validate transition logic in agent code
- Enable detailed state logging

### Performance Optimization

#### Memory Usage
- Monitor pool utilization statistics
- Tune pool sizes based on usage patterns
- Enable memory paging for large datasets
- Implement garbage collection strategies

#### CPU Usage
- Profile function performance
- Optimize string operations
- Cache frequently accessed data
- Implement asynchronous operations where appropriate

#### Network Performance
- Implement connection pooling
- Add request/response caching
- Optimize payload sizes
- Handle network timeouts gracefully

### Monitoring and Logging

#### Log Levels
- **DEBUG**: Detailed execution information
- **INFO**: Normal operation events
- **WARN**: Recoverable error conditions
- **ERROR**: Serious error conditions

#### Key Metrics
- **Cycle Time**: Time per agent iteration
- **Memory Usage**: Pool utilization statistics
- **Network Latency**: LLM communication timing
- **Error Rates**: Failure frequency by component

#### Health Checks
- Memory pool availability
- LLM endpoint connectivity
- Configuration file validity
- Agent state consistency

---

For additional support or questions, please refer to the source code documentation or create an issue in the project repository.
