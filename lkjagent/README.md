# lkjagent - Lightweight HTTP Client Library

A minimal, zero-dependency HTTP client library written in C. This library provides robust token-based string handling and HTTP request capabilities with a focus on safety, simplicity, and performance.

## Features

### Token Management System
- **Safe string handling**: All string operations use bounded buffers to prevent buffer overflows
- **Memory-efficient**: Uses user-provided static buffers, no dynamic allocation
- **Comprehensive validation**: All functions validate input parameters and token state
- **Rich string operations**: Set, append, copy, find, substring, trim, and comparison functions

### HTTP Client
- **HTTP/1.1 support**: Full support for GET, POST, and other HTTP methods
- **Automatic connection management**: Handles socket creation, connection, and cleanup
- **Robust error handling**: Comprehensive error checking and proper resource cleanup
- **Configurable timeouts**: Network operation timeouts to prevent hanging
- **Token-based interface**: All HTTP data handled through safe token system

## Building

### Using Makefile
```bash
# Build the main binary
make

# Build and run tests
make test

# Clean build artifacts
make clean

# Show help
make help
```

### Manual compilation
```bash
gcc -Wall -Wextra -std=c99 -O2 -o lkjagent src/main.c
```

## API Reference

### Token Functions

#### Core Operations
```c
result_t token_init(token_t* token, char* buffer, size_t capacity);
result_t token_clear(token_t* token);
result_t token_set(token_t* token, const char* str);
result_t token_append(token_t* token, const char* str);
result_t token_copy(token_t* dest, const token_t* src);
```

#### String Manipulation
```c
result_t token_find(const token_t* token, const char* needle, size_t* position);
result_t token_substring(const token_t* token, size_t start, size_t length, token_t* dest);
result_t token_trim(token_t* token);
```

#### Comparison and Validation
```c
int token_equals(const token_t* token1, const token_t* token2);
int token_equals_str(const token_t* token, const char* str);
int token_is_empty(const token_t* token);
int token_available_space(const token_t* token);
result_t token_validate(const token_t* token);
```

### HTTP Functions

#### Core HTTP Operations
```c
result_t http_request(token_t* method, token_t* url, token_t* body, token_t* response);
```

#### Convenience Functions
```c
result_t http_get(token_t* url, token_t* response);
result_t http_post(token_t* url, token_t* body, token_t* response);
```

## Usage Examples

### Basic HTTP GET Request
```c
#include "lkjagent.h"

int main() {
    char url_data[256];
    char response_data[4096];
    
    token_t url, response;
    
    // Initialize tokens
    token_init(&url, url_data, sizeof(url_data));
    token_init(&response, response_data, sizeof(response_data));
    
    // Set URL and make request
    token_set(&url, "http://httpbin.org/get");
    
    if (http_get(&url, &response) == RESULT_OK) {
        printf("Response: %s\n", response.data);
    }
    
    return 0;
}
```

### HTTP POST Request with JSON Body
```c
#include "lkjagent.h"

int main() {
    char url_data[256];
    char body_data[1024];
    char response_data[4096];
    
    token_t url, body, response;
    
    // Initialize tokens
    token_init(&url, url_data, sizeof(url_data));
    token_init(&body, body_data, sizeof(body_data));
    token_init(&response, response_data, sizeof(response_data));
    
    // Set URL and body
    token_set(&url, "http://httpbin.org/post");
    token_set(&body, "{\"name\":\"lkjagent\",\"version\":\"1.0\"}");
    
    if (http_post(&url, &body, &response) == RESULT_OK) {
        printf("Response: %s\n", response.data);
    }
    
    return 0;
}
```

### String Manipulation with Tokens
```c
#include "lkjagent.h"

int main() {
    char buffer1[100], buffer2[100];
    token_t token1, token2;
    
    token_init(&token1, buffer1, sizeof(buffer1));
    token_init(&token2, buffer2, sizeof(buffer2));
    
    // Build a string
    token_set(&token1, "Hello");
    token_append(&token1, " World");
    
    // Find substring
    size_t position;
    if (token_find(&token1, "World", &position) == RESULT_OK) {
        printf("Found 'World' at position: %zu\n", position);
    }
    
    // Extract substring
    token_substring(&token1, 0, 5, &token2);
    printf("First 5 characters: %s\n", token2.data);
    
    return 0;
}
```

## Design Principles

### Memory Safety
- All string operations use bounded buffers
- No dynamic memory allocation
- Comprehensive bounds checking
- Buffer overflow protection

### Error Handling
- All functions return explicit error codes
- Consistent error handling patterns
- Input validation on all public APIs
- Graceful degradation on errors

### Performance
- Zero-copy string operations where possible
- Minimal system calls
- Efficient network operations
- Stack-based memory management

### Portability
- Standard C99 compliance
- POSIX socket API
- No external dependencies
- Tested on Linux platforms

## Configuration

### HTTP Settings
- Default timeout: 30 seconds
- Default ports: HTTP (80), HTTPS (443 - not implemented)
- User agent: "lkjagent/1.0"
- Maximum URL length: 256 characters
- Maximum request size: 8192 bytes
- Response chunk size: 4096 bytes

### Compilation Options
- Standard: `-std=c99`
- Warnings: `-Wall -Wextra`
- Optimization: `-O2` (recommended for production)

## Limitations

- HTTPS is not currently supported (HTTP only)
- No support for HTTP/2 or HTTP/3
- No built-in JSON parsing
- No authentication mechanisms
- IPv4 only (no IPv6 support)

## License

See LICENSE file for details.

## Contributing

1. Follow the existing code style
2. Add tests for new features
3. Ensure all functions handle errors gracefully
4. Update documentation for API changes
5. Test on multiple platforms

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