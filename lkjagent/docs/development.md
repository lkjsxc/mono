# Development Guide

## Overview

This guide covers building, testing, debugging, and extending LKJAgent. It provides comprehensive information for developers who want to modify, enhance, or integrate with the LKJAgent system.

## Development Environment Setup

### Prerequisites

**Required:**
- GCC 12 or compatible C11 compiler
- GNU Make
- POSIX-compliant system (Linux, macOS, WSL)
- Git for version control

**Optional:**
- Docker for containerized builds
- GDB for debugging
- Valgrind for memory analysis
- IDE with C language support (VS Code, CLion, etc.)

### System Requirements

**Minimum:**
- 4GB RAM for compilation and memory pools
- 100MB disk space for source and build artifacts
- Network access for LLM API communication

**Recommended:**
- 8GB+ RAM for optimal performance
- SSD storage for faster builds
- Stable internet connection for LLM services

## Building the Project

### Standard Build Process

```bash
# Clone the repository
git clone <repository-url>
cd lkjagent

# Build the project
make

# The executable will be created at: build/lkjagent
```

### Build System Details

The project uses a GNU Makefile with the following features:

```makefile
# Compiler configuration
CC = gcc
CFLAGS = -Werror -Wall -Wextra -std=c11 -O2 -g
LDFLAGS = -static
INCLUDES = -Isrc/
```

**Compiler Flags Explanation:**
- `-Werror`: Treat warnings as errors
- `-Wall -Wextra`: Enable comprehensive warnings
- `-std=c11`: Use C11 standard
- `-O2`: Optimize for performance
- `-g`: Include debug information
- `-static`: Create statically linked executable

### Build Targets

```bash
# Default build (same as 'make all')
make

# Clean build artifacts
make clean

# Force rebuild
make clean && make

# Debug build (additional debug flags)
make DEBUG=1
```

### Debug Build Configuration

```makefile
ifdef DEBUG
CFLAGS += -DDEBUG -O0 -fno-omit-frame-pointer
LDFLAGS += -rdynamic
endif
```

**Debug flags:**
- `-DDEBUG`: Enable debug macros
- `-O0`: Disable optimizations
- `-fno-omit-frame-pointer`: Better stack traces
- `-rdynamic`: Dynamic symbol resolution

## Docker Development

### Building Docker Image

```bash
# Build the Docker image
docker build -t lkjagent .

# Multi-stage build for minimal size
# Builder stage: ~1GB (includes GCC toolchain)
# Final stage: ~10MB (static binary only)
```

### Development with Docker

```bash
# Run with mounted source for development
docker run -it \
  -v $(pwd):/workspace \
  -v $(pwd)/data:/app/data \
  gcc:12 bash

# Inside container:
cd /workspace
make
./build/lkjagent
```

### Docker Compose for Development

```yaml
version: '3.8'
services:
  lkjagent:
    build: .
    volumes:
      - ./data:/app/data
    environment:
      - LLM_ENDPOINT=http://host.docker.internal:1234
    depends_on:
      - llm-server
      
  llm-server:
    image: ollama/ollama
    ports:
      - "1234:11434"
    volumes:
      - ollama_data:/root/.ollama

volumes:
  ollama_data:
```

## Project Structure and Organization

### Directory Layout

```
lkjagent/
├── src/                    # Source code
│   ├── lkjagent.c         # Main application entry point
│   ├── lkjagent.h         # Main header file
│   ├── agent/             # Agent core logic
│   │   ├── core.c         # State machine and LLM integration
│   │   └── core.h         # Agent API definitions
│   ├── utils/             # Utility libraries
│   │   ├── pool.c/.h      # Memory pool allocator
│   │   ├── string.c/.h    # String manipulation
│   │   ├── object.c/.h    # JSON/XML object system
│   │   ├── file.c/.h      # File I/O operations
│   │   └── http.c/.h      # HTTP client
│   └── global/            # Global definitions
│       ├── const.h        # Constants
│       ├── types.h        # Type definitions
│       ├── macro.h        # Utility macros
│       └── std.h          # Standard includes
├── data/                  # Configuration and state
│   ├── config.json        # System configuration
│   └── memory.json        # Agent memory state
├── build/                 # Build artifacts (created by make)
│   ├── obj/               # Object files
│   └── lkjagent           # Final executable
├── docs/                  # Documentation
├── Makefile              # Build configuration
├── Dockerfile            # Container definition
└── README.md             # Project overview
```

### Coding Standards

#### Naming Conventions

```c
// Functions: module_action() pattern
result_t string_create(pool_t* pool, string_t** string);
result_t object_parse_json(pool_t* pool, object_t** dst, const string_t* src);

// Types: lowercase with _t suffix
typedef struct string_s string_t;
typedef enum result_e result_t;

// Constants: UPPERCASE with underscores
#define POOL_STRING16_MAXCOUNT (1048576)
#define CONFIG_PATH "data/config.json"

// Variables: lowercase with underscores
pool_t memory_pool;
string_t* work_string;
```

#### Function Attributes

```c
// All functions should warn on unused result
__attribute__((warn_unused_result)) 
result_t function_name(parameters);

// Static functions for internal use
static __attribute__((warn_unused_result)) 
result_t internal_function(parameters);
```

#### Error Handling Pattern

```c
result_t example_function(pool_t* pool, string_t** output) {
    string_t* temp_string = NULL;
    object_t* temp_object = NULL;
    
    // Allocate resources
    if (string_create(pool, &temp_string) != RESULT_OK) {
        RETURN_ERR("Failed to create temporary string");
    }
    
    if (object_create(pool, &temp_object) != RESULT_OK) {
        // Clean up on error
        if (string_destroy(pool, temp_string) != RESULT_OK) {
            RETURN_ERR("Failed to cleanup string after object creation failure");
        }
        RETURN_ERR("Failed to create temporary object");
    }
    
    // Use resources...
    
    // Successful cleanup
    if (object_destroy(pool, temp_object) != RESULT_OK) {
        RETURN_ERR("Failed to destroy temporary object");
    }
    
    // Transfer ownership to caller
    *output = temp_string;
    return RESULT_OK;
}
```

## Testing and Debugging

### Manual Testing

```bash
# Build and run basic test
make
./build/lkjagent

# Expected output:
# - Configuration loading messages
# - 5 agent processing cycles
# - Memory pool statistics
# - Clean exit
```

### Memory Testing with Valgrind

```bash
# Install Valgrind (Ubuntu/Debian)
sudo apt-get install valgrind

# Run with memory checking
valgrind --leak-check=full --track-origins=yes ./build/lkjagent

# Expected results:
# - No memory leaks
# - No invalid memory access
# - All allocations properly freed
```

### Debugging with GDB

```bash
# Build debug version
make DEBUG=1

# Run with GDB
gdb ./build/lkjagent

# Useful GDB commands:
(gdb) break main
(gdb) run
(gdb) print pool->string16_freelist_count
(gdb) backtrace
(gdb) continue
```

### Debug Build Features

```c
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) printf("DEBUG: " fmt "\\n", ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

// Usage in code:
DEBUG_PRINT("Allocated string from pool, size: %lu", string->capacity);
```

### Memory Pool Debugging

The system provides comprehensive memory statistics:

```bash
# At shutdown, check for memory leaks:
string16 freelist: 1048576     # Should equal POOL_STRING16_MAXCOUNT
string256 freelist: 65536      # Should equal POOL_STRING256_MAXCOUNT
string4096 freelist: 4096      # Should equal POOL_STRING4096_MAXCOUNT
string65536 freelist: 256      # Should equal POOL_STRING65536_MAXCOUNT
string1048576 freelist: 16     # Should equal POOL_STRING1048576_MAXCOUNT
object freelist: 65536         # Should equal POOL_OBJECT_MAXCOUNT
```

**Debugging Memory Issues:**
- **Lower than expected counts**: Memory leaks
- **Higher than expected counts**: Double-free errors
- **Zero counts**: Pool exhaustion

## Extending the System

### Adding New Agent Actions

1. **Define action in configuration:**

```json
{
  "available_actions": {
    "new_action_name": "Description of what this action does"
  },
  "output_example": {
    "agent": {
      "action": {
        "type": "new_action_name",
        "parameter1": "value1",
        "parameter2": "value2"
      }
    }
  }
}
```

2. **Implement action handler:**

```c
static __attribute__((warn_unused_result)) 
result_t execute_action_new_action(pool_t* pool, agent_t* agent, object_t* action) {
    object_t* param1_obj;
    object_t* param2_obj;
    
    // Extract parameters
    if (object_provide_str(pool, &param1_obj, action, "parameter1") != RESULT_OK) {
        RETURN_ERR("Failed to get parameter1 from new_action");
    }
    
    if (object_provide_str(pool, &param2_obj, action, "parameter2") != RESULT_OK) {
        RETURN_ERR("Failed to get parameter2 from new_action");
    }
    
    // Implement action logic
    // ...
    
    printf("Executed new_action: %.*s, %.*s\\n",
           (int)param1_obj->string->size, param1_obj->string->data,
           (int)param2_obj->string->size, param2_obj->string->data);
    
    return RESULT_OK;
}
```

3. **Register action in dispatcher:**

```c
// In lkjagent_agent_execute function, add:
} else if (strncmp(action_type_obj->string->data, "new_action_name", 
                   action_type_obj->string->size) == 0) {
    if (execute_action_new_action(pool, agent, action_obj) != RESULT_OK) {
        // Error cleanup...
        RETURN_ERR("Failed to execute new_action action");
    }
```

### Adding New String Pool Sizes

1. **Update constants:**

```c
#define POOL_STRING_NEWSIZE_MAXCOUNT (1024)
```

2. **Update pool structure:**

```c
typedef struct {
    // Existing pools...
    
    // New pool
    char string_newsize_data[POOL_STRING_NEWSIZE_MAXCOUNT * NEWSIZE];
    string_t string_newsize[POOL_STRING_NEWSIZE_MAXCOUNT];
    string_t* string_newsize_freelist_data[POOL_STRING_NEWSIZE_MAXCOUNT];
    uint64_t string_newsize_freelist_count;
} pool_t;
```

3. **Implement pool functions:**

```c
result_t pool_string_newsize_alloc(pool_t* pool, string_t** string) {
    if (pool->string_newsize_freelist_count > 0) {
        pool->string_newsize_freelist_count--;
        *string = pool->string_newsize_freelist_data[pool->string_newsize_freelist_count];
        return RESULT_OK;
    }
    RETURN_ERR("String newsize pool exhausted");
}

result_t pool_string_newsize_free(pool_t* pool, string_t* string) {
    if (pool->string_newsize_freelist_count >= POOL_STRING_NEWSIZE_MAXCOUNT) {
        RETURN_ERR("String newsize freelist full");
    }
    pool->string_newsize_freelist_data[pool->string_newsize_freelist_count] = string;
    pool->string_newsize_freelist_count++;
    return RESULT_OK;
}
```

4. **Update pool initialization and allocation logic**

### Adding New Agent States

1. **Define state configuration:**

```json
{
  "agent": {
    "state": {
      "new_state": {
        "prompt": {
          "description": "What this state does",
          "available_actions": {},
          "output_examples": {}
        }
      }
    }
  }
}
```

2. **Update state transition logic:**

```c
// In state transition handler:
if (strncmp(next_state_obj->string->data, "new_state", next_state_obj->string->size) == 0) {
    // Handle transition to new state
    // Update prompt generation logic
    // Add any state-specific initialization
}
```

3. **Update prompt generation for new state**

### Performance Optimization

#### Memory Pool Tuning

Monitor freelist statistics and adjust pool sizes:

```c
// Add monitoring code:
if (pool->string4096_freelist_count < POOL_STRING4096_MAXCOUNT * 0.1) {
    printf("WARNING: string4096 pool running low: %lu remaining\\n", 
           pool->string4096_freelist_count);
}
```

#### HTTP Request Optimization

```c
// Connection reuse (future enhancement)
static CURL* reusable_curl_handle = NULL;

result_t http_post_optimized(pool_t* pool, const string_t* url, 
                            const string_t* content_type,
                            const string_t* body, string_t** response) {
    if (!reusable_curl_handle) {
        reusable_curl_handle = curl_easy_init();
        // Configure keep-alive, timeouts, etc.
    }
    // Use existing handle for request
}
```

#### JSON/XML Processing Optimization

```c
// Pre-compile frequently used paths
static const char* WORKING_MEMORY_PATH = "working_memory";
static const char* AGENT_STATE_PATH = "state";

// Use compiled paths instead of dynamic strings
```

## Common Development Issues

### Memory Pool Exhaustion

**Symptoms:**
- "Pool exhausted" errors
- Sudden performance degradation
- Application termination

**Solutions:**
- Increase pool sizes in `const.h`
- Implement memory paging
- Add pool monitoring and warnings

**Prevention:**
- Monitor freelist counts during development
- Test with realistic data volumes
- Implement graceful degradation

### Memory Leaks

**Symptoms:**
- Freelist counts don't return to maximum at shutdown
- Gradually increasing memory usage
- Valgrind reports leaked memory

**Solutions:**
- Review error handling paths for missing cleanup
- Ensure all allocations have corresponding deallocations
- Use RAII-style resource management

**Prevention:**
- Follow established error handling patterns
- Use static analysis tools
- Regular memory testing with Valgrind

### JSON/XML Parsing Errors

**Symptoms:**
- "Failed to parse JSON" errors
- Malformed LLM responses
- Unexpected object structures

**Solutions:**
- Add response validation and sanitization
- Implement fallback parsing strategies
- Log malformed responses for analysis

**Prevention:**
- Validate all external inputs
- Test with various LLM models and responses
- Implement robust error recovery

### HTTP Communication Issues

**Symptoms:**
- "Failed to send HTTP POST request" errors
- Timeouts or connection failures
- Invalid LLM endpoint responses

**Solutions:**
- Verify network connectivity and endpoint availability
- Check LLM service configuration and API compatibility
- Implement retry logic with exponential backoff

**Prevention:**
- Test with multiple LLM providers
- Implement comprehensive error handling
- Add connection health checks

## Contributing Guidelines

### Code Review Checklist

- [ ] All functions have proper error handling
- [ ] Memory allocation/deallocation is balanced
- [ ] Code follows naming conventions
- [ ] Functions are properly documented
- [ ] Error messages are descriptive
- [ ] Resource cleanup on all error paths
- [ ] No warnings from compiler with `-Wall -Wextra`
- [ ] Memory testing with Valgrind passes

### Submission Process

1. **Development**
   - Create feature branch from main
   - Implement changes following coding standards
   - Test thoroughly with various configurations

2. **Testing**
   - Build with both optimized and debug configurations
   - Run memory testing with Valgrind
   - Test with different LLM models and responses
   - Verify configuration loading and validation

3. **Documentation**
   - Update API documentation for new functions
   - Update configuration documentation for new options
   - Add examples for new features
   - Update build instructions if needed

4. **Submission**
   - Create pull request with detailed description
   - Include test results and memory analysis
   - Address review feedback promptly
   - Ensure CI/CD checks pass

### Development Best Practices

1. **Start Small**: Begin with simple changes and build complexity gradually
2. **Test Early**: Test changes frequently during development
3. **Memory Safety**: Always consider memory allocation and cleanup
4. **Error Handling**: Implement comprehensive error handling from the start
5. **Documentation**: Document code as you write it
6. **Performance**: Consider performance implications of changes
7. **Compatibility**: Ensure changes work across different environments
8. **Security**: Validate all inputs and sanitize outputs
