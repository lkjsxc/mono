# LKJAgent Coding Style Guidelines

## File Headers

Every `.c` file must start with a comprehensive Doxygen header:

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

## Function Documentation

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

## Error Handling Pattern

All functions that can fail must:

1. Return `result_t` type
2. Use `__attribute__((warn_unused_result))`
3. Validate all input parameters
4. Use concrete `RETURN_ERR()` macro implementation for error reporting
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

## Concrete RETURN_ERR Implementation

The `RETURN_ERR` macro must be implemented with concrete logic for comprehensive error reporting:

```c
#define RETURN_ERR3(n) #n
#define RETURN_ERR2(n) RETURN_ERR3(n)
#define RETURN_ERR(error_message)                                                   \
    do {                                                                            \
        _Pragma("GCC diagnostic push")                                              \
        _Pragma("GCC diagnostic ignored \"-Wunused-result\"")                      \
        write(STDERR_FILENO, "{Error: { file: \"", 18);                             \
        write(STDERR_FILENO, __FILE__, sizeof(__FILE__));                           \
        write(STDERR_FILENO, "\", func: \"", 11);                                   \
        write(STDERR_FILENO, __func__, sizeof(__func__));                           \
        write(STDERR_FILENO, "\", line: ", 10);                                     \
        write(STDERR_FILENO, RETURN_ERR2(__LINE__), sizeof(RETURN_ERR2(__LINE__))); \
        write(STDERR_FILENO, "\", message: \"", 13);                                \
        write(STDERR_FILENO, error_message, strlen(error_message));                 \
        write(STDERR_FILENO, "\"}}\n", 5);                                          \
        _Pragma("GCC diagnostic pop")                                               \
    } while(0)
```

## Memory Management Rules

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

## Naming Conventions

- **Files**: `snake_case.c` and `snake_case.h`
- **Functions**: `module_action_object()` pattern (e.g., `data_set()`, `memory_store_entry()`)
- **Types**: `snake_case_t` suffix (e.g., `data_t`, `memory_entry_t`)
- **Constants**: `ALL_CAPS_SNAKE_CASE` (e.g., `MAX_MEMORY_ENTRIES`)
- **Macros**: `ALL_CAPS_SNAKE_CASE` (e.g., `RETURN_ERR`)

## Memory Safety Patterns

```c
// Always validate pointers
if (!ptr) {
    RETURN_ERR("function_name: NULL parameter");
    return RESULT_ERR;
}

// Use bounded operations
if (length >= data->capacity) {
    RETURN_ERR("Buffer capacity exceeded");
    return RESULT_ERR;
}

// Ensure null termination
data->data[data->size] = '\0';
```

## Error Propagation with Concrete Implementation

```c
result_t high_level_function() {
    result_t result = low_level_function();
    if (result != RESULT_OK) {
        RETURN_ERR("high_level_function: Failed in low level operation");
        return result;
    }
    return RESULT_OK;
}
```

## Quality Standards

All generated code must:
- Compile without warnings using `-Wall -Wextra -Werror`
- Follow the established coding style consistently
- Include comprehensive error handling with concrete RETURN_ERR implementation
- Provide complete Doxygen documentation
- Maintain zero external dependencies
- Use only stack-based memory allocation
- Implement all declared APIs from the header file
- Support perpetual operation without termination
- Manage context width properly during state transitions with LLM-controlled paging
- Utilize unified memory.json storage for both working and disk memory
- Process simple tag format for all LLM interactions
- Implement state-specific system prompts correctly
- Support context key identification, storage, and retrieval
- Validate simple tag format responses from LLM
- Handle context key directory operations
- Integrate LLM-directed paging operations seamlessly
