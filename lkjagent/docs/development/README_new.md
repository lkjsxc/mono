# Development Guide

## Getting Started

### Prerequisites

- GCC or Clang compiler
- POSIX-compliant system (Linux, macOS, Unix)
- Make build system

### Building

```bash
# Clone and enter directory
cd /workspaces/mono/lkjagent

# Build the project
make

# Clean build files
make clean

# Build with debug symbols
make debug
```

## Coding Standards

### File Headers

Every `.c` file must start with a Doxygen header:

```c
/**
 * @file filename.c
 * @brief Brief description of the module
 *
 * Detailed description explaining the module's purpose, key features,
 * and architectural role within the system.
 *
 * Key features:
 * - Feature 1
 * - Feature 2
 * - Feature 3
 */
```

### Function Documentation

All functions must have complete Doxygen documentation:

```c
/**
 * @brief Brief function description
 *
 * Detailed explanation of what the function does, how it works,
 * and any important implementation details.
 *
 * @param param1 Description of parameter 1
 * @param param2 Description of parameter 2
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t function_name(type1 param1, type2 param2) {
    // Implementation
}
```

### Error Handling Pattern

All functions that can fail must:

1. Return `result_t` type
2. Use `__attribute__((warn_unused_result))`
3. Validate all input parameters
4. Use concrete `RETURN_ERR()` macro for error reporting
5. Follow early return pattern for error conditions

```c
__attribute__((warn_unused_result))
result_t example_function(example_t* obj, const char* input) {
    if (!obj) {
        RETURN_ERR("example_function: NULL obj parameter");
        return RESULT_ERR;
    }
    
    if (!input) {
        RETURN_ERR("example_function: NULL input parameter");
        return RESULT_ERR;
    }
    
    // Main implementation
    
    return RESULT_OK;
}
```

### Memory Management Rules

1. **No Dynamic Allocation**: Never use `malloc()`, `calloc()`, or `free()`
2. **Static Buffers**: All buffers are statically allocated or stack-based
3. **Bounded Operations**: All string operations must respect buffer capacity
4. **Data Token System**: Use `data_t` for all string handling
5. **Buffer Initialization**: Always initialize data tokens with static buffers

```c
// Correct pattern for data token initialization
static char buffer[512];
data_t data;
if (data_init(&data, buffer, sizeof(buffer)) != RESULT_OK) {
    RETURN_ERR("Failed to initialize data token");
    return RESULT_ERR;
}
```

### Naming Conventions

- **Files**: `snake_case.c` and `snake_case.h`
- **Functions**: `module_action_object()` pattern (e.g., `data_set()`, `memory_store_entry()`)
- **Types**: `snake_case_t` suffix (e.g., `data_t`, `memory_entry_t`)
- **Constants**: `ALL_CAPS_SNAKE_CASE` (e.g., `MAX_MEMORY_ENTRIES`)
- **Macros**: `ALL_CAPS_SNAKE_CASE` (e.g., `RETURN_ERR`)

## Build System

### Makefile Structure

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -O2
SRCDIR = src
BUILDDIR = build

# Source files organized by module
CORE_SRCS = $(SRCDIR)/agent/core.c $(SRCDIR)/lkjagent.c
MEMORY_SRCS = $(SRCDIR)/memory/tagged_memory.c $(SRCDIR)/memory/context_manager.c
UTILS_SRCS = $(SRCDIR)/utils/data.c $(SRCDIR)/utils/file.c $(SRCDIR)/utils/json.c
# ... additional sources

all: $(BUILDDIR)/lkjagent

debug: CFLAGS += -g -DDEBUG
debug: $(BUILDDIR)/lkjagent

clean:
	rm -rf $(BUILDDIR)/*

.PHONY: all debug clean
```

## Testing

### Unit Testing

Each module should include basic validation:

```c
#ifdef DEBUG
// Basic validation for data token operations
static result_t test_data_operations() {
    char buffer[256];
    data_t token;
    
    if (data_init(&token, buffer, sizeof(buffer)) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (data_set(&token, "test") != RESULT_OK) {
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}
#endif
```

### Integration Testing

Test complete workflows:

```bash
# Test agent initialization
./build/lkjagent --test-init

# Test configuration loading  
./build/lkjagent --test-config

# Test memory operations
./build/lkjagent --test-memory
```

## Development Workflow

### 1. Implementation Order

Follow the order specified in [Implementation Guide](../implementation/README.md):

1. Core infrastructure (data tokens, file I/O, JSON)
2. Memory layer (tagged memory, context management)
3. LLM integration (HTTP client, prompt management)
4. State machine (states and agent core)
5. Application entry point

### 2. Code Quality Checks

Before committing:

```bash
# Compile with all warnings
make clean && make

# Check for memory issues
valgrind --leak-check=full ./build/lkjagent

# Verify coding standards
grep -r "malloc\|calloc\|free" src/ # Should find nothing
grep -r "RETURN_ERR" src/ # Should find error handling
```

### 3. Documentation

Ensure all modules have:
- Complete Doxygen headers
- API documentation in [docs/api/](../api/README.md)
- Usage examples in [docs/user/](../user/README.md)

## Debugging

### Common Issues

**Compilation errors:**
- Check header includes
- Verify function signatures match declarations
- Ensure all required functions are implemented

**Runtime errors:**
- Use `RETURN_ERR` macro consistently
- Check buffer bounds in all operations
- Validate JSON format in configuration files

**Memory issues:**
- All buffers must be statically allocated
- Check for buffer overflows in string operations
- Ensure proper data token initialization

### Debug Tools

```bash
# GDB debugging
gdb ./build/lkjagent
(gdb) set args --debug
(gdb) run

# Memory checking
valgrind --tool=memcheck ./build/lkjagent

# Static analysis
cppcheck --enable=all src/
```
