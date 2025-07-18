# Module Implementation Requirements

This document provides detailed requirements for implementing each module in the LKJAgent system.

## 1. Data Token Management (`src/utils/data.c`)

Implements safe string handling with:
- Buffer overflow prevention
- Null termination guarantees
- Capacity validation
- String manipulation operations (set, append, find, substring)
- Comparison functions
- Context width management utilities

## 2. Simple Tag Processing (`src/utils/tag_processor.c`)

Handles LLM output in simple `<tag>content</tag>` format only:
- Simple tag validation and parsing (no complex markup)
- Block extraction (`<thinking>`, `<action>`, `<evaluation>`, `<paging>`)
- Parameter parsing and validation from plain text content
- Context key extraction from tag content
- Error handling for malformed simple tags
- Conversion between simple tag format and structured data
- Strict enforcement of simple tag-only format

## 3. File Operations (`src/utils/file.c`)

Provides file I/O with:
- Safe file reading into data tokens
- Atomic write operations
- Directory management
- File existence checks
- Error handling for all I/O operations
- Memory.json unified storage support

## 4. Context Management (`src/memory/context_manager.c`)

Advanced context width and paging management:
- LLM-directed context key identification
- Context element transfer to/from disk
- Context key directory maintenance
- Context window optimization
- Automatic context archival
- Context retrieval by key

## 5. LLM-Controlled Paging (`src/state/paging.c`)

LLM-driven memory paging operations:
- Context analysis and key identification
- Disk storage directive processing
- Context element prioritization
- Automatic context cleanup
- Paging decision rationale tracking
- Integration with memory persistence layer

## 6. HTTP Client (`src/utils/http.c`)

Implements HTTP client without external dependencies:
- Socket-based implementation
- GET and POST request support
- JSON payload handling
- Timeout management
- Response parsing
- Context-aware request sizing

## 7. JSON Processing (`src/utils/json.c`)

Manual JSON parsing and generation:
- Validation functions
- Key-path extraction
- Type-safe value retrieval
- Object and array creation
- String escaping
- Memory.json format support

## 8. Configuration Management (`src/config/config.c`)

Configuration management with state-specific prompts:
- Default value initialization
- JSON file loading with state prompt support
- Validation rules
- Type-safe access patterns
- Save functionality
- Context width configuration
- State-specific system prompt management

## 9. Tagged Memory (`src/memory/tagged_memory.c`)

Advanced memory system featuring:
- Entry storage with multiple tags
- Complex query execution
- Memory statistics
- Cleanup and defragmentation
- Persistence to memory.json
- Working and disk memory unification
- Integration with context key system

## 10. LLM Integration (`src/memory/enhanced_llm.c`)

AI-driven memory decisions:
- Memory pattern analysis
- Tag suggestion algorithms
- Retention strategy decisions
- Context building for LLM calls
- Semantic memory organization
- Context width optimization
- Tag format output processing

## 11. Agent Core (`src/agent/core.c`)

State machine implementation:
- Four-state execution cycle with LLM-controlled paging
- LMStudio API integration with tag format parsing
- Memory management with context key tracking
- Task goal tracking
- Autonomous decision making
- Perpetual operation mode
- Context width management with LLM directives

## 12. Enhanced States (`src/state/enhanced_states.c`)

State management utilities:
- State transition logic with context paging
- Context preservation during transitions
- State-specific memory handling
- Progress tracking
- Context width management
- Perpetual state cycling
- Integration with state-specific system prompts

## 13. State-Specific Implementations

### Thinking State (`src/state/thinking.c`)
- Deep analysis and contemplation
- Context accumulation and organization
- Problem decomposition
- Strategic planning with context key identification

### Executing State (`src/state/executing.c`)
- Action execution with disk enrichment
- Task performance monitoring
- Resource utilization tracking
- Progress measurement

### Evaluating State (`src/state/evaluating.c`)
- Progress assessment and metrics
- Quality evaluation
- Performance analysis
- Improvement recommendations

### Paging State (`src/state/paging.c`)
- LLM-controlled context management
- Memory optimization
- Context key processing
- Disk storage operations

## 14. Persistence Layer

### Memory Persistence (`src/persistence/memory_persistence.c`)
- Memory.json operations
- Context key directory management
- Atomic write operations
- Data integrity validation

### Configuration Persistence (`src/persistence/config_persistence.c`)
- Config.json with state prompts
- Runtime configuration updates
- Validation and error handling

## 15. LLM Client Architecture

### LLM Client (`src/llm/llm_client.c`)
- HTTP communication with LMStudio
- Request/response handling
- Timeout and error management
- Connection pooling

### Prompt Manager (`src/llm/prompt_manager.c`)
- State-specific prompt construction
- Context integration
- Tag format enforcement
- Template management

### Response Parser (`src/llm/response_parser.c`)
- Tag format validation
- Block extraction and parsing
- Context key extraction
- Error handling for malformed responses

## 16. Utility Extensions

### String Utilities (`src/utils/string_utils.c`)
- Advanced string operations
- Pattern matching
- Text processing utilities

### Time Utilities (`src/utils/time_utils.c`)
- Timestamp management
- Duration calculations
- Time-based operations

## 17. Agent Memory (`src/memory.c`)

High-level memory management:
- Working memory initialization
- Persistent storage integration in memory.json
- Memory validation
- Statistics and utilization tracking
- Unified memory access interface
- Context key integration

## 18. Main Application (`src/lkjagent.c`)

Application entry point providing:
- Agent initialization
- Perpetual execution loop
- Error handling and recovery
- Signal handling for graceful operation
- Context management across restarts

## Context Width Management Implementation

```c
/**
 * @brief Manage context width during state transitions with LLM-controlled paging
 *
 * Ensures LLM context windows are properly sized and maintained
 * across agent state transitions to prevent context overflow.
 * The LLM directs which context elements to move to disk storage.
 *
 * @param agent Agent instance
 * @param new_state Target state for transition
 * @param context_buffer Buffer for context management
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t manage_context_width_transition(lkjagent_t* agent, const char* new_state, data_t* context_buffer);

/**
 * @brief LLM-controlled context paging request
 *
 * Requests the LLM to analyze current context and specify which
 * context keys should be moved to disk storage, retrieved, or archived.
 *
 * @param agent Agent instance
 * @param context_buffer Current context buffer
 * @param target_state Target state for transition
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t llm_request_context_paging(lkjagent_t* agent, data_t* context_buffer, const char* target_state);
```
