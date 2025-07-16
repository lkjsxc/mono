Of course. Here is the English translation:

# lkjagent - An Autonomous AI Agent in C

A minimal, autonomous AI agent implemented using only the C11 standard library, featuring persistent memory management and capable of executing complex tasks through integration with the LMStudio API.

## Overview

lkjagent is a functional programming-based AI agent focused on enhancing disk storage capabilities to achieve complex tasks. The agent manages both volatile (RAM) and persistent (disk) memory through a single JSON file architecture and operates without a user interface.

## Key Features

  - **C11 Standard**: Pure C implementation using only standard library functions.
  - **Zero Dynamic Allocation**: Uses only static memory allocation (no malloc/free).
  - **Functional Programming**: Employs immutable data structures and pure functions.
  - **LMStudio Integration**: Direct API communication for AI inference.
  - **Persistent Memory**: JSON-based disk storage for long-term knowledge retention.
  - **Stateful Execution**: Multi-state agent lifecycle with clear state transitions.

-----

## Architecture

### Memory Management

The agent operates with a dual-memory system:

#### RAM (Volatile Memory)

Provides context to the AI model through a structured prompt:

  - **`system_prompt`**: Fixed behavioral guidelines and agent definition.
  - **`current_state`**: The agent's current operational state.
  - **`task_goal`**: The final objective to be achieved.
  - **`plan`**: A step-by-step execution strategy.
  - **`scratchpad`**: Temporary notes and tool execution results.
  - **`recent_history`**: A log of recent agent activities.
  - **`retrieved_from_disk`**: Relevant knowledge fetched from persistent storage.

#### Disk (Persistent Memory)

A key-value store with an optional tagging system:

  - **`working_memory`**: Task-specific information and context.
  - **`knowledge_base`**: Accumulated learning and insights.
  - **`log`**: A complete execution history and audit trail.
  - **`file`**: Generated artifacts (code, documents, data).
  - **`Arbitrary tags`**: Tags with no special meaning.

### Agent States

The agent operates in four distinct states:

1.  **`thinking`**: Receives a request and formulates an execution plan.
2.  **`executing`**: Performs actions based on the current plan.
3.  **`evaluating`**: Assesses results and determines the next step.
4.  **`paging`**: Manages memory by moving data between RAM and disk.

### Available Tools

  - **`search`**: Queries disk storage for relevant information.
  - **`retrieve`**: Reads specific data from persistent storage.
  - **`write`**: Saves information to disk with optional tags.
  - **`execute_code`**: Runs code snippets and captures the results.
  - **`forget`**: Deletes unnecessary information for memory optimization.

-----

## Build and Compilation

### Prerequisites

  - A C11 compatible compiler (GCC 4.9+ or Clang 3.1+).
  - Standard C library.
  - curl library for HTTP requests (usually pre-installed).

### Compilation

```bash
# Basic compilation
gcc -std=c11 -Wall -Wextra -O2 -o lkjagent lkjagent/main.c

# With debug symbols
gcc -std=c11 -Wall -Wextra -g -DDEBUG -o lkjagent lkjagent/main.c

# Production build
gcc -std=c11 -Wall -Wextra -O3 -DNDEBUG -o lkjagent lkjagent/main.c
```

### CMake Build (Recommended)

```bash
mkdir build
cd build
cmake ..
make
```

-----

## Configuration

### LMStudio Setup

1.  Install and run LMStudio.
2.  Load your preferred language model.
3.  Start the local server (usually at `http://localhost:1234`).
4.  Set the API endpoint in the agent's configuration.

### Agent Configuration

Create a `config.json` file:

```json
{
  "lmstudio": {
    "endpoint": "http://localhost:1234/v1/chat/completions",
    "model": "your-model-name",
    "max_tokens": 2048
  },
  "memory": {
    "ram_size": 8192,
    "disk_file": "agent_memory.json",
    "max_history": 100
  },
  "agent": {
    "max_iterations": 50,
    "evaluation_threshold": 0.8
  }
}
```

-----

## Usage

### Basic Execution

```bash
# Run
./lkjagent
```

## Memory Architecture Details

### JSON Storage Format

```json
{
  "metadata": {
    "version": "1.0",
    "created": "2025-07-16T00:00:00Z",
    "last_modified": "2025-07-16T12:00:00Z"
  },
  "working_memory": {
    "current_task": "...",
    "context": "...",
    "variables": {}
  },
  "knowledge_base": {
    "concepts": {},
    "procedures": {},
    "facts": {}
  },
  "log": [
    {
      "timestamp": "2025-07-16T12:00:00Z",
      "state": "thinking",
      "action": "plan_task",
      "details": "..."
    }
  ],
  "file": {
    "generated_code": {},
    "documents": {},
    "data": {}
  }
}
```

### State Transition

```
[User Input] -> thinking -> executing -> evaluating
      ^            |           |
      |            +-----------+
      +------------ paging
```

-----

## Development

### Code Structure

```
lkjagent/
├── data.json              # ram, disk
├── config.json            # Configuration
└── src/
    ├── main.c             # Entry point and main loop
    ├── agent.h            # Agent core definitions
    ├── agent.c            # Agent state management
    ├── memory.h           # Memory management interface
    ├── memory.c           # JSON-based memory implementation
    ├── tools.h            # Tool definitions
    ├── tools.c            # Tool implementations
    ├── http.h             # HTTP client interface
    ├── http.c             # LMStudio API communication
    ├── utils.h            # Utility functions
    └── utils.c
```

### Coding Standards

  - **C11 Standard**: Use C11 features where beneficial.
  - **No Dynamic Allocation**: Only static arrays and stack allocation.
  - **Functional Style**: Prefer pure functions and immutable data.
  - **Error Handling**: Explicit error codes for all operations.
  - **Documentation**: Comprehensive inline documentation.

### Memory Constraints

  - **Max JSON Size**: 16MB (configurable).
  - **RAM Buffer**: 8KB statically allocated.
  - **String Limit**: 2KB per string field.
  - **Array Limit**: Max 256 elements.

-----

## API Integration

### LMStudio Communication

The agent communicates with LMStudio using HTTP POST requests:

```c
// Example API call structure
typedef struct {
    char* model;
    char* prompt;
    int max_tokens;
} api_request_t;

int call_lmstudio(const api_request_t* request, char* response, size_t response_size);
```

### Response Handling

The AI response is parsed to extract:

  - The next action to perform.
  - The updated agent state.
  - Information to be saved.
  - Tool calls.

## Error Handling

The agent implements comprehensive error handling:

  - **Network Errors**: Retry with exponential backoff.
  - **JSON Parsing Errors**: Graceful degradation.
  - **Memory Errors**: Safe failure modes.
  - **API Errors**: Fallback strategies.

## Performance Considerations

  - **Memory Usage**: Fixed memory footprint.
  - **Disk I/O**: Efficient JSON operations.
  - **Networking**: Connection pooling and caching.
  - **CPU**: Optimized string manipulation.

-----

## Debugging

### Debug Mode

```bash
./lkjagent --debug --verbose
```

Debug output includes:

  - State transitions.
  - Memory operations.
  - API requests/responses.
  - Tool executions.

### Memory Inspection

```bash
# Display the current memory state
cat agent_memory.json | jq '.'

# Watch for memory changes
watch -n 1 'cat agent_memory.json | jq ".metadata.last_modified"'
```

-----

## Contributing

1.  Fork the repository.
2.  Create a feature branch.
3.  Follow the coding standards.
4.  Add comprehensive tests.
5.  Submit a pull request.

## License

MIT License - see the [LICENSE](https://www.google.com/search?q=LICENSE) file for details.

## Roadmap

  - [ ] Enhanced tool system
  - [ ] Multi-model support
  - [ ] Distributed memory
  - [ ] Performance optimizations
  - [ ] Expanded API compatibility

## Support

For issues and questions:

  - GitHub Issues: [Create an issue](https://github.com/lkjsxc/mono/issues)
  - Documentation: Refer to inline code comments.
  - Examples: Check the `examples/` directory.

-----

**Note**: This agent is designed for autonomous operation. Ensure appropriate monitoring and safety measures are in place when deploying in a production environment.