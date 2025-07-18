# Source Code Structure

Detailed organization of LKJAgent source files.

## Directory Structure

```
src/
├── lkjagent.h              # Main header with all type definitions and APIs
├── lkjagent.c              # Application entry point
├── agent/
│   ├── core.c              # Agent lifecycle and state machine
│   ├── execution.c         # Task execution engine
│   ├── evaluation.c        # Progress assessment and metrics
│   └── decision.c          # Decision making logic
├── config/
│   ├── config.c            # Core configuration management
│   ├── validation.c        # Configuration validation
│   └── defaults.c          # Default configuration values
├── memory/
│   ├── tagged_memory.c     # Tagged memory system implementation
│   ├── enhanced_llm.c      # LLM integration for memory decisions
│   ├── disk_storage.c      # Disk storage operations
│   ├── context_manager.c   # Context width and paging management
│   └── memory_optimizer.c  # Memory optimization and cleanup
├── state/
│   ├── enhanced_states.c   # Enhanced state management
│   ├── thinking.c          # Thinking state implementation
│   ├── executing.c         # Executing state implementation
│   ├── evaluating.c        # Evaluating state implementation
│   └── paging.c            # LLM-controlled paging state
├── llm/
│   ├── llm_client.c        # LLM client interface
│   ├── prompt_manager.c    # Prompt construction and management
│   ├── context_builder.c   # Context preparation for LLM calls
│   └── response_parser.c   # LLM response parsing and validation
├── utils/
│   ├── data.c              # Safe data token management
│   ├── file.c              # File I/O operations
│   ├── http.c              # HTTP client implementation
│   ├── json.c              # JSON parsing and generation
│   ├── tag_processor.c     # Simple tag format handling
│   ├── string_utils.c      # String manipulation utilities
│   └── time_utils.c        # Time and timestamp utilities
└── persistence/
    ├── memory_persistence.c # Memory.json persistence
    ├── config_persistence.c # Configuration persistence
    └── disk_operations.c    # Low-level disk operations
```

## Core Header File (`lkjagent.h`)

Contains all type definitions and API declarations:

```c
#ifndef LKJAGENT_H
#define LKJAGENT_H

#include <stddef.h>
#include <stdint.h>

// Basic result type
typedef enum {
    RESULT_OK = 0,
    RESULT_ERR = 1
} result_t;

// Data token for safe string handling
typedef struct {
    char* data;
    size_t capacity;
    size_t size;
} data_t;

// Agent states
typedef enum {
    STATE_THINKING,
    STATE_EXECUTING,
    STATE_EVALUATING,
    STATE_PAGING
} agent_state_t;

// Tagged memory entry
typedef struct {
    char content[1024];
    char tags[8][64];
    size_t tag_count;
    uint64_t timestamp;
    char context_key[64];
} memory_entry_t;

// Configuration structure
typedef struct {
    struct {
        char base_url[256];
        char model[64];
        double temperature;
        int max_tokens;
        int timeout_ms;
    } lmstudio;
    
    struct {
        int max_iterations;
        int self_directed;
        struct {
            char thinking_prompt[1024];
            char executing_prompt[1024];
            char evaluating_prompt[1024];
            char paging_prompt[1024];
        } state_prompts;
    } agent;
} config_t;

// Main agent structure
typedef struct {
    config_t config;
    agent_state_t current_state;
    memory_entry_t memory[1000];
    size_t memory_count;
    char working_context[4096];
    char context_keys[100][64];
    size_t context_key_count;
} lkjagent_t;

// Function declarations for all modules
// ... (API declarations)

#endif // LKJAGENT_H
```

## Key Implementation Files

### Core Agent (`src/agent/core.c`)

Main state machine and agent lifecycle:

```c
/**
 * @file core.c
 * @brief Agent core functionality and state machine
 * 
 * Implements the four-state execution cycle with LLM-controlled paging,
 * perpetual operation mode, and context width management.
 */

result_t agent_init(lkjagent_t* agent, const config_t* config);
result_t agent_run_cycle(lkjagent_t* agent);
result_t agent_transition_state(lkjagent_t* agent, agent_state_t new_state);
result_t agent_manage_context_width(lkjagent_t* agent);
```

### Data Tokens (`src/utils/data.c`)

Foundation for all string operations:

```c
/**
 * @file data.c
 * @brief Safe string handling with bounded operations
 * 
 * Provides memory-safe string operations with capacity validation,
 * null termination guarantees, and context width management.
 */

result_t data_init(data_t* token, char* buffer, size_t capacity);
result_t data_set(data_t* token, const char* str);
result_t data_append(data_t* token, const char* str);
result_t data_find(const data_t* token, const char* substr, size_t* position);
result_t data_trim_context(data_t* context, size_t max_size);
```

### Tagged Memory (`src/memory/tagged_memory.c`)

Advanced memory system with context key integration:

```c
/**
 * @file tagged_memory.c
 * @brief Tagged memory system with LLM integration
 * 
 * Implements multi-tag memory entries, complex queries, context key
 * management, and unified storage with memory.json persistence.
 */

result_t tagged_memory_init(tagged_memory_t* memory);
result_t tagged_memory_add_entry(tagged_memory_t* memory, const char* content, 
                                const char** tags, size_t tag_count, 
                                const char* context_key);
result_t tagged_memory_query_by_tags(const tagged_memory_t* memory, 
                                    const char** tags, size_t tag_count,
                                    memory_entry_t** results, size_t* result_count);
result_t tagged_memory_query_by_context_key(const tagged_memory_t* memory,
                                           const char* context_key,
                                           memory_entry_t** entry);
```

### LLM Integration (`src/llm/llm_client.c`)

HTTP communication with LMStudio:

```c
/**
 * @file llm_client.c
 * @brief LLM client for LMStudio communication
 * 
 * Handles HTTP requests to LMStudio, manages timeouts, processes
 * responses, and enforces simple tag format validation.
 */

result_t llm_client_init(llm_client_t* client, const config_t* config);
result_t llm_send_request(const llm_client_t* client, const data_t* prompt, 
                         data_t* response);
result_t llm_validate_tag_format(const data_t* response);
```

### Tag Processing (`src/utils/tag_processor.c`)

Simple tag format parsing for LLM outputs:

```c
/**
 * @file tag_processor.c
 * @brief Simple tag format processing for LLM outputs
 * 
 * Handles parsing of simple <tag>content</tag> format, validates
 * structure, extracts content blocks, and processes context keys.
 */

result_t tag_find_block(const data_t* text, const char* tag_name, 
                       size_t* start_pos, size_t* end_pos);
result_t tag_extract_content(const data_t* text, const char* tag_name,
                            size_t search_start, size_t search_end,
                            data_t* content);
result_t tag_parse_context_keys(const data_t* content, char context_keys[][64],
                               size_t* key_count, size_t max_keys);
result_t tag_validate_simple_format(const data_t* text);
```

## Module Dependencies

```
lkjagent.c
├── agent/core.c
│   ├── memory/tagged_memory.c
│   ├── llm/llm_client.c
│   ├── state/*.c
│   └── config/config.c
├── utils/data.c (foundation for all)
├── utils/file.c
├── utils/json.c
└── persistence/memory_persistence.c
```

## Build Order

1. **Foundation**: `utils/data.c`, `utils/file.c`, `utils/json.c`
2. **Configuration**: `config/config.c`, `persistence/config_persistence.c`
3. **Memory Layer**: `memory/tagged_memory.c`, `memory/context_manager.c`
4. **LLM Layer**: `utils/http.c`, `llm/llm_client.c`, `utils/tag_processor.c`
5. **State Machine**: `state/*.c`, `agent/core.c`
6. **Application**: `lkjagent.c`

## Quality Standards

Each file must:
- Include comprehensive Doxygen documentation
- Implement all declared APIs from `lkjagent.h`
- Use only bounded string operations via data tokens
- Follow consistent error handling with `RETURN_ERR`
- Support perpetual operation without memory leaks
- Integrate with unified memory.json storage
- Process simple tag format correctly
- Maintain context key directory integrity
