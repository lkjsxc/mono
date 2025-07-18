# Implementation Guide

Detailed guide for implementing LKJAgent components.

## Implementation Order

### Phase 1: Core Infrastructure

1. **`src/lkjagent.h`** - Type definitions and API declarations
2. **`src/utils/data.c`** - Safe string handling foundation
3. **`src/utils/file.c`** - Basic I/O operations
4. **`src/utils/json.c`** - Configuration parsing
5. **`src/config/config.c`** - Configuration management

### Phase 2: Memory Layer

1. **`src/memory/tagged_memory.c`** - Core memory system
2. **`src/memory/context_manager.c`** - Context key management
3. **`src/persistence/memory_persistence.c`** - Disk persistence
4. **`src/utils/tag_processor.c`** - LLM tag format parsing

### Phase 3: LLM Integration

1. **`src/utils/http.c`** - HTTP client for LMStudio
2. **`src/llm/llm_client.c`** - LLM communication
3. **`src/llm/prompt_manager.c`** - Prompt construction
4. **`src/llm/response_parser.c`** - Response parsing

### Phase 4: State Machine

1. **`src/state/enhanced_states.c`** - State management utilities
2. **`src/state/thinking.c`** - Thinking state implementation
3. **`src/state/executing.c`** - Executing state implementation
4. **`src/state/evaluating.c`** - Evaluating state implementation
5. **`src/state/paging.c`** - Paging state implementation
6. **`src/agent/core.c`** - Main agent logic

### Phase 5: Application Entry

1. **`src/lkjagent.c`** - Application entry point

## Key Implementation Patterns

### Error Handling

```c
__attribute__((warn_unused_result))
result_t function_name(param_t* param) {
    if (!param) {
        RETURN_ERR("function_name: NULL parameter");
        return RESULT_ERR;
    }
    
    // Implementation
    
    return RESULT_OK;
}
```

### Data Token Usage

```c
static char buffer[512];
data_t token;
if (data_init(&token, buffer, sizeof(buffer)) != RESULT_OK) {
    RETURN_ERR("Failed to initialize data token");
    return RESULT_ERR;
}

if (data_set(&token, "initial content") != RESULT_OK) {
    RETURN_ERR("Failed to set data content");
    return RESULT_ERR;
}
```

### Memory Safety

```c
// Always validate buffer capacity
if (length >= token->capacity) {
    RETURN_ERR("Buffer capacity exceeded");
    return RESULT_ERR;
}

// Ensure null termination
token->data[token->size] = '\0';
```

### LLM Tag Processing

```c
result_t parse_thinking_block(const data_t* response, thinking_data_t* thinking) {
    if (!response || !thinking) {
        RETURN_ERR("parse_thinking_block: NULL parameter");
        return RESULT_ERR;
    }
    
    // Extract <thinking> block content
    size_t start_pos, end_pos;
    if (tag_find_block(response, "thinking", &start_pos, &end_pos) != RESULT_OK) {
        RETURN_ERR("No thinking block found");
        return RESULT_ERR;
    }
    
    // Parse nested tags: <analysis>, <planning>, <context_keys>
    if (tag_extract_content(response, "analysis", start_pos, end_pos, thinking->analysis) != RESULT_OK ||
        tag_extract_content(response, "planning", start_pos, end_pos, thinking->planning) != RESULT_OK ||
        tag_extract_content(response, "context_keys", start_pos, end_pos, thinking->context_keys) != RESULT_OK) {
        RETURN_ERR("Failed to extract thinking block components");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}
```

## Module Requirements

### Data Token (`src/utils/data.c`)

Must implement:
- `data_init()` - Initialize with static buffer
- `data_set()` - Set content with bounds checking
- `data_append()` - Append with capacity validation
- `data_find()` - Substring search
- `data_substring()` - Extract substring
- `data_compare()` - String comparison
- `data_trim_context()` - Context window management

### Tagged Memory (`src/memory/tagged_memory.c`)

Must implement:
- Multi-tag entry storage
- Complex query operations (AND, OR, NOT)
- Memory cleanup and defragmentation
- Persistence to memory.json
- Context key integration
- LLM-guided organization

### State Machine (`src/agent/core.c`)

Must implement:
- Four-state execution cycle
- LLM-controlled paging integration
- Context width management
- Perpetual operation mode
- State-specific prompt handling
- Memory persistence across restarts

## Testing Requirements

Each module must:
- Compile without warnings (`-Wall -Wextra -Werror`)
- Handle all error conditions gracefully
- Validate input parameters comprehensively
- Use only stack-based memory allocation
- Support perpetual operation
- Integrate with unified memory.json storage
- Process simple tag format correctly
- Maintain context key directory integrity

## Quality Standards

- **Zero Dependencies**: Only POSIX libraries
- **Memory Safety**: Bounded operations only
- **Error Handling**: Comprehensive with concrete RETURN_ERR
- **Documentation**: Complete Doxygen headers
- **Testing**: Edge case coverage
- **Performance**: Efficient context management
- **Reliability**: Perpetual operation capability
