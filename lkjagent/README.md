# LKJAgent Source Code Regeneration Guide

## Overview

This document provides comprehensive instructions for AI agents to regenerate the complete LKJAgent source code. LKJAgent is an autonomous AI agent system implemented in C with zero external dependencies, featuring tagged memory, state machine architecture, and LLM integration capabilities.

## Architecture Overview

### Core Design Principles

1. **Zero External Dependencies**: Pure C implementation using only standard POSIX libraries
2. **Memory Safety**: Stack-based allocation with bounded buffers and explicit capacity management
3. **Error Handling**: Comprehensive error propagation using `result_t` enum and concrete `RETURN_ERR` macro implementation
4. **Modular Design**: Clean separation between core functionality, utilities, memory management, and state handling
5. **Tagged Memory System**: Advanced memory management with multi-tag queries and LLM integration
6. **State Machine Architecture**: Four-state autonomous execution cycle (thinking, executing, evaluating, paging)
7. **Perpetual Operation**: The agent operates continuously without termination, perpetually enriching disk storage

### Project Structure

```
/workspaces/mono/lkjagent/
/workspaces/mono/lkjagent/src/
/workspaces/mono/lkjagent/src/lkjagent.h              # Main header with all type definitions and APIs
/workspaces/mono/lkjagent/src/lkjagent.c              # Application entry point (renamed from main.c)
/workspaces/mono/lkjagent/src/agent/
/workspaces/mono/lkjagent/src/agent/core.c              # Agent lifecycle and state machine
/workspaces/mono/lkjagent/src/agent/execution.c         # Task execution engine
/workspaces/mono/lkjagent/src/agent/evaluation.c        # Progress assessment and metrics
/workspaces/mono/lkjagent/src/agent/decision.c          # Decision making logic
/workspaces/mono/lkjagent/src/config/
/workspaces/mono/lkjagent/src/config/config.c            # Core configuration management
/workspaces/mono/lkjagent/src/config/validation.c        # Configuration validation
/workspaces/mono/lkjagent/src/config/defaults.c          # Default configuration values
/workspaces/mono/lkjagent/src/memory/
/workspaces/mono/lkjagent/src/memory/tagged_memory.c     # Tagged memory system implementation
/workspaces/mono/lkjagent/src/memory/enhanced_llm.c      # LLM integration for memory decisions
/workspaces/mono/lkjagent/src/memory/disk_storage.c      # Disk storage operations
/workspaces/mono/lkjagent/src/memory/context_manager.c   # Context width and paging management
/workspaces/mono/lkjagent/src/memory/memory_optimizer.c  # Memory optimization and cleanup
/workspaces/mono/lkjagent/src/state/
/workspaces/mono/lkjagent/src/state/enhanced_states.c   # Enhanced state management
/workspaces/mono/lkjagent/src/state/thinking.c          # Thinking state implementation
/workspaces/mono/lkjagent/src/state/executing.c         # Executing state implementation
/workspaces/mono/lkjagent/src/state/evaluating.c        # Evaluating state implementation
/workspaces/mono/lkjagent/src/state/paging.c            # LLM-controlled paging state
/workspaces/mono/lkjagent/src/llm/
/workspaces/mono/lkjagent/src/llm/llm_client.c        # LLM client interface
/workspaces/mono/lkjagent/src/llm/prompt_manager.c    # Prompt construction and management
/workspaces/mono/lkjagent/src/llm/context_builder.c   # Context preparation for LLM calls
/workspaces/mono/lkjagent/src/llm/response_parser.c   # LLM response parsing and validation
/workspaces/mono/lkjagent/src/utils/
/workspaces/mono/lkjagent/src/utils/data.c              # Safe data token management (renamed from token.c)
/workspaces/mono/lkjagent/src/utils/file.c              # File I/O operations
/workspaces/mono/lkjagent/src/utils/http.c              # HTTP client implementation
/workspaces/mono/lkjagent/src/utils/json.c              # JSON parsing and generation
/workspaces/mono/lkjagent/src/utils/tag_processor.c     # Simple tag format handling
/workspaces/mono/lkjagent/src/utils/string_utils.c      # String manipulation utilities
/workspaces/mono/lkjagent/src/utils/time_utils.c        # Time and timestamp utilities
/workspaces/mono/lkjagent/src/utils/error_handler.c     # Centralized error handling
/workspaces/mono/lkjagent/src/persistence/
/workspaces/mono/lkjagent/src/persistence/memory_persistence.c # Memory.json persistence
/workspaces/mono/lkjagent/src/persistence/config_persistence.c # Configuration persistence
/workspaces/mono/lkjagent/src/persistence/disk_operations.c    # Low-level disk operations
/workspaces/mono/lkjagent/build/                      # Compiled object files and executable
/workspaces/mono/lkjagent/data/
/workspaces/mono/lkjagent/data/config.json             # Runtime configuration with state prompts
/workspaces/mono/lkjagent/data/memory.json             # Persistent storage for both disk and working memory
/workspaces/mono/lkjagent/data/context_keys.json       # LLM-specified context keys for disk storage
/workspaces/mono/lkjagent/docs/                       # Documentation
/workspaces/mono/lkjagent/Makefile                    # Build configuration
```

## Agent Philosophy

### Continuous Operation

The LKJAgent is designed with the following operational principles:

- **Never-Ending Processing**: The agent never terminates its execution cycle
- **Perpetual Enrichment**: Continuously strives to enrich disk storage with valuable data
- **Endless Thinking Mode**: Operates in infinite iteration mode (`max_iterations: -1`)
- **Self-Sustaining**: Maintains its own memory and context through persistent storage

### Memory Architecture

Both working memory (RAM) and persistent disk memory are stored in the unified `memory.json` file, with LLM-controlled paging operations managing the flow of context between memory layers:

- **LLM-Directed Paging**: The LLM specifies context keys to move between working and disk storage
- **Context Key Management**: LLM identifies and tags important context elements for persistence
- **Automatic Context Transfer**: Context elements are moved to disk based on LLM directives
- **Seamless transitions between volatile and persistent states**
- **Comprehensive memory history preservation**
- **Context continuity across agent restarts**
- **Unified memory query interface**
- **Smart Context Paging**: LLM determines what context to preserve, archive, or retrieve
- **Context Key Directory**: Maintained in `context_keys.json` for efficient retrieval

### LLM Output Format

**IMPORTANT**: All LLM interactions must use a simple `<tag>` and `</tag>` format only. Do NOT use XML markup, HTML markup, or any complex formatting. Use only simple opening and closing tags with plain text content.

Required format for all LLM responses:

```
<thinking>
<analysis>Current situation analysis</analysis>
<planning>Next steps and strategy</planning>
<context_keys>key1,key2,key3</context_keys>
</thinking>

<action>
<type>disk_storage</type>
<context_key>specific_key</context_key>
<operation>store</operation>
<data>content to store/process</data>
</action>

<evaluation>
<progress>Assessment of current progress</progress>
<quality_score>0.85</quality_score>
<enrichment_rate>0.92</enrichment_rate>
<recommendations>rec1,rec2</recommendations>
</evaluation>

<paging>
<operation>context_management</operation>
<move_to_disk>context_key1,context_key2</move_to_disk>
<retrieve_from_disk>context_key3</retrieve_from_disk>
<archive_old>context_key4</archive_old>
<rationale>Explanation for paging decisions</rationale>
</paging>
```

**Format Rules**:
- Use only simple `<tag>content</tag>` pairs
- No attributes, namespaces, or complex structures
- Plain text content only
- No nested complex elements
- Consistent tag names as shown above

## Coding Style Guidelines

### File Headers

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

### Concrete RETURN_ERR Implementation

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

### LLM Output Format

All LLM interactions use a standardized simple tag format to ensure consistent parsing and processing:

### Context Width Management

Proper management of context width during state transitions is crucial, with LLM-controlled paging operations:

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
result_t manage_context_width_transition(lkjagent_t* agent, const char* new_state, data_t* context_buffer) {
    if (!agent || !new_state || !context_buffer) {
        RETURN_ERR("manage_context_width_transition: NULL parameter");
        return RESULT_ERR;
    }
    
    // Calculate available context window size
    size_t max_context = agent->config.agent.llm_decisions.context_window_size;
    size_t current_context_size = context_buffer->size;
    
    // Invoke LLM paging if approaching limit (reserve 20% buffer)
    if (current_context_size > (max_context * 0.8)) {
        // Request LLM to specify context keys for disk storage
        if (llm_request_context_paging(agent, context_buffer, new_state) != RESULT_OK) {
            RETURN_ERR("Failed to execute LLM-controlled context paging");
            return RESULT_ERR;
        }
    }
    
    // Add state transition marker
    char transition_marker[256];
    snprintf(transition_marker, sizeof(transition_marker), 
             "\n--- STATE_TRANSITION: %s ---\n", new_state);
    
    if (data_append(context_buffer, transition_marker) != RESULT_OK) {
        RETURN_ERR("Failed to add state transition marker");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}
```

### Naming Conventions

- **Files**: `snake_case.c` and `snake_case.h`
- **Functions**: `module_action_object()` pattern (e.g., `data_set()`, `memory_store_entry()`)
- **Types**: `snake_case_t` suffix (e.g., `data_t`, `memory_entry_t`)
- **Constants**: `ALL_CAPS_SNAKE_CASE` (e.g., `MAX_MEMORY_ENTRIES`)
- **Macros**: `ALL_CAPS_SNAKE_CASE` (e.g., `RETURN_ERR`)

## Core Type System

### Basic Types

```c
typedef enum {
    RESULT_OK = 0,    /**< Operation completed successfully */
    RESULT_ERR = 1    /**< General error */
} result_t;

typedef struct {
    char* data;       /**< Pointer to character buffer */
    size_t capacity;  /**< Maximum buffer size including null terminator */
    size_t size;      /**< Current string length (excluding null terminator) */
} data_t;
```

### Tagged Memory System

The tagged memory system is a key innovation featuring:

- Multi-tag entries with configurable limits
- Complex query capabilities (AND, OR, NOT operations)
- Automatic cleanup and defragmentation
- LLM-guided memory organization
- Relationship tracking between entries
- Unified storage in memory.json for both working and disk memory

### Configuration System

JSON-based configuration with:
- Type-safe loading and validation
- Default value initialization
- Runtime reconfiguration support
- Hierarchical structure for different subsystems
- Context width management parameters

## Module Implementation Requirements

### 1. Data Token Management (`src/utils/data.c`)

Implements safe string handling with:
- Buffer overflow prevention
- Null termination guarantees
- Capacity validation
- String manipulation operations (set, append, find, substring)
- Comparison functions
- Context width management utilities

### 2. Simple Tag Processing (`src/utils/tag_processor.c`)

Handles LLM output in simple `<tag>content</tag>` format only:
- Simple tag validation and parsing (no complex markup)
- Block extraction (`<thinking>`, `<action>`, `<evaluation>`, `<paging>`)
- Parameter parsing and validation from plain text content
- Context key extraction from tag content
- Error handling for malformed simple tags
- Conversion between simple tag format and structured data
- Strict enforcement of simple tag-only format

### 3. File Operations (`src/utils/file.c`)

Provides file I/O with:
- Safe file reading into data tokens
- Atomic write operations
- Directory management
- File existence checks
- Error handling for all I/O operations
- Memory.json unified storage support

### 4. Context Management (`src/memory/context_manager.c`)

Advanced context width and paging management:
- LLM-directed context key identification
- Context element transfer to/from disk
- Context key directory maintenance
- Context window optimization
- Automatic context archival
- Context retrieval by key

### 5. LLM-Controlled Paging (`src/state/paging.c`)

LLM-driven memory paging operations:
- Context analysis and key identification
- Disk storage directive processing
- Context element prioritization
- Automatic context cleanup
- Paging decision rationale tracking
- Integration with memory persistence layer

### 6. HTTP Client (`src/utils/http.c`)

Implements HTTP client without external dependencies:
- Socket-based implementation
- GET and POST request support
- JSON payload handling
- Timeout management
- Response parsing
- Context-aware request sizing

### 7. JSON Processing (`src/utils/json.c`)

Manual JSON parsing and generation:
- Validation functions
- Key-path extraction
- Type-safe value retrieval
- Object and array creation
- String escaping
- Memory.json format support

### 8. Configuration Management (`src/config/config.c`)

Configuration management with state-specific prompts:
- Default value initialization
- JSON file loading with state prompt support
- Validation rules
- Type-safe access patterns
- Save functionality
- Context width configuration
- State-specific system prompt management

### 9. Tagged Memory (`src/memory/tagged_memory.c`)

Advanced memory system featuring:
- Entry storage with multiple tags
- Complex query execution
- Memory statistics
- Cleanup and defragmentation
- Persistence to memory.json
- Working and disk memory unification
- Integration with context key system

### 10. LLM Integration (`src/memory/enhanced_llm.c`)

AI-driven memory decisions:
- Memory pattern analysis
- Tag suggestion algorithms
- Retention strategy decisions
- Context building for LLM calls
- Semantic memory organization
- Context width optimization
- Tag format output processing

### 11. Agent Core (`src/agent/core.c`)

State machine implementation:
- Four-state execution cycle with LLM-controlled paging
- LMStudio API integration with tag format parsing
- Memory management with context key tracking
- Task goal tracking
- Autonomous decision making
- Perpetual operation mode
- Context width management with LLM directives

### 12. Enhanced States (`src/state/enhanced_states.c`)

State management utilities:
- State transition logic with context paging
- Context preservation during transitions
- State-specific memory handling
- Progress tracking
- Context width management
- Perpetual state cycling
- Integration with state-specific system prompts

### 13. State-Specific Implementations

#### Thinking State (`src/state/thinking.c`)
- Deep analysis and contemplation
- Context accumulation and organization
- Problem decomposition
- Strategic planning with context key identification

#### Executing State (`src/state/executing.c`)
- Action execution with disk enrichment
- Task performance monitoring
- Resource utilization tracking
- Progress measurement

#### Evaluating State (`src/state/evaluating.c`)
- Progress assessment and metrics
- Quality evaluation
- Performance analysis
- Improvement recommendations

#### Paging State (`src/state/paging.c`)
- LLM-controlled context management
- Memory optimization
- Context key processing
- Disk storage operations

### 14. Persistence Layer

#### Memory Persistence (`src/persistence/memory_persistence.c`)
- Memory.json operations
- Context key directory management
- Atomic write operations
- Data integrity validation

#### Configuration Persistence (`src/persistence/config_persistence.c`)
- Config.json with state prompts
- Runtime configuration updates
- Validation and error handling

### 15. LLM Client Architecture

#### LLM Client (`src/llm/llm_client.c`)
- HTTP communication with LMStudio
- Request/response handling
- Timeout and error management
- Connection pooling

#### Prompt Manager (`src/llm/prompt_manager.c`)
- State-specific prompt construction
- Context integration
- Tag format enforcement
- Template management

#### Response Parser (`src/llm/response_parser.c`)
- Tag format validation
- Block extraction and parsing
- Context key extraction
- Error handling for malformed responses

### 16. Utility Extensions

#### String Utilities (`src/utils/string_utils.c`)
- Advanced string operations
- Pattern matching
- Text processing utilities

#### Time Utilities (`src/utils/time_utils.c`)
- Timestamp management
- Duration calculations
- Time-based operations

#### Error Handler (`src/utils/error_handler.c`)
- Centralized error processing
- Error categorization
- Recovery strategies

### 17. Agent Memory (`src/memory.c`)

High-level memory management:
- Working memory initialization
- Persistent storage integration in memory.json
- Memory validation
- Statistics and utilization tracking
- Unified memory access interface
- Context key integration

### 18. Main Application (`src/lkjagent.c`)

Application entry point providing:
- Agent initialization
- Perpetual execution loop
- Error handling and recovery
- Signal handling for graceful operation
- Context management across restarts

## Build System

The Makefile provides:
- Modular compilation with proper dependencies
- Debug symbol generation
- Warning-as-error enforcement
- Clean build targets
- Test compilation support
- Data token system compilation

## Configuration Schema

The `data/config.json` file configures all aspects of the agent, including state-specific system prompts:

```json
{
  "lmstudio": {
    "base_url": "http://host.docker.internal:1234/v1/chat/completions",
    "model": "qwen/qwen3-8b",
    "temperature": 0.7,
    "max_tokens": 2048,
    "timeout_ms": 30000,
    "tag_format_enforced": true
  },
  "agent": {
    "max_iterations": -1,
    "self_directed": 1,
    "state_prompts": {
      "thinking": {
        "system_prompt": "You are in THINKING state. Analyze deeply, contemplate thoroughly, and identify context keys for important insights. Always respond in simple tag format with <thinking> blocks containing analysis, planning, and context_keys.",
        "objectives": ["deep_analysis", "strategic_planning", "context_identification"],
        "tag_blocks": ["<thinking>"],
        "context_key_focus": true
      },
      "executing": {
        "system_prompt": "You are in EXECUTING state. Perform actions to enrich disk storage with valuable data. Always respond in simple tag format with <action> blocks specifying disk storage operations and context keys.",
        "objectives": ["disk_enrichment", "action_execution", "data_storage"],
        "tag_blocks": ["<action>"],
        "context_key_focus": true
      },
      "evaluating": {
        "system_prompt": "You are in EVALUATING state. Assess progress, measure quality, and provide metrics. Always respond in simple tag format with <evaluation> blocks containing progress assessments and quality metrics.",
        "objectives": ["progress_assessment", "quality_measurement", "performance_analysis"],
        "tag_blocks": ["<evaluation>"],
        "context_key_focus": false
      },
      "paging": {
        "system_prompt": "You are in PAGING state. Manage context and memory efficiently by specifying which context keys to move to disk storage, retrieve, or archive. Always respond in simple tag format with <paging> blocks containing specific directives.",
        "objectives": ["context_management", "memory_optimization", "disk_organization"],
        "tag_blocks": ["<paging>"],
        "context_key_focus": true,
        "paging_directives": {
          "move_to_disk": "Specify context keys to move from working memory to disk",
          "retrieve_from_disk": "Specify context keys to retrieve from disk to working memory",
          "archive_old": "Specify context keys to archive for long-term storage"
        }
      }
    },
    "tagged_memory": {
      "max_entries": 1000,
      "max_tags_per_entry": 8,
      "auto_cleanup_threshold": 0.8,
      "tag_similarity_threshold": 0.7,
      "context_key_integration": true
    },
    "llm_decisions": {
      "confidence_threshold": 0.8,
      "decision_timeout_ms": 5000,
      "fallback_enabled": true,
      "context_window_size": 4096,
      "tag_validation": true,
      "context_key_extraction": true
    },
    "enhanced_tools": {
      "tool_chaining_enabled": true,
      "max_tool_chain_length": 5,
      "parallel_tool_execution": false,
      "context_key_tracking": true
    },
    "paging_control": {
      "llm_controlled": true,
      "auto_paging_threshold": 0.8,
      "context_key_retention": 100,
      "disk_storage_priority": "llm_specified"
    }
  },
  "tag_format": {
    "validation_strict": true,
    "required_blocks": {
      "thinking": ["<thinking>"],
      "executing": ["<action>"],
      "evaluating": ["<evaluation>"],
      "paging": ["<paging>"]
    },
    "context_key_format": {
      "max_length": 64,
      "allowed_chars": "alphanumeric_underscore_dash",
      "case_sensitive": false
    }
  },
  "context_management": {
    "keys_file": "data/context_keys.json",
    "max_working_keys": 50,
    "disk_storage_path": "data/disk_context/",
    "compression_enabled": false,
    "encryption_enabled": false
  },
  "http": {
    "timeout_seconds": 30,
    "max_redirects": 3,
    "user_agent": "LKJAgent-Enhanced/1.0"
  }
}
```

## Enhanced Data Schemas

### Memory.json Schema

The `data/memory.json` file contains unified storage for both working and disk memory with context key integration:

```json
{
  "metadata": {
    "version": "2.0",
    "created": "2025-07-18T12:00:00Z",
    "last_modified": "2025-07-18T12:00:00Z",
    "context_key_version": "1.0"
  },
  "working_memory": {
    "current_task": "Perpetual thinking and disk enrichment",
    "context": "Unified context spanning state transitions...",
    "current_state": "thinking",
    "variables": {},
    "context_window_usage": 0.75,
    "active_context_keys": ["analysis_2025_07_18", "strategy_planning", "disk_enrichment_metrics"]
  },
  "disk_memory": {
    "tagged_entries": [],
    "knowledge_base": {
      "concepts": {},
      "procedures": {},
      "facts": {}
    },
    "accumulated_insights": [],
    "enrichment_metrics": {
      "total_entries": 0,
      "quality_score": 0.0,
      "enrichment_rate": 0.0
    },
    "context_storage": {
      "archived_contexts": {},
      "context_relationships": {},
      "access_patterns": {}
    }
  },
  "context_key_directory": {
    "active_keys": {
      "analysis_2025_07_18": {
        "location": "working_memory",
        "created": "2025-07-18T12:00:00Z",
        "size_bytes": 1024,
        "access_count": 15,
        "tags": ["analysis", "current", "important"]
      }
    },
    "archived_keys": {},
    "disk_keys": {}
  },
  "unified_log": [],
  "context_management": {
    "window_size": 4096,
    "current_usage": 0,
    "trim_history": [],
    "paging_operations": [],
    "llm_directives": []
  }
}
```

### Context Keys Schema

The `data/context_keys.json` file maintains the directory of context keys for efficient management:

```json
{
  "metadata": {
    "version": "1.0",
    "last_updated": "2025-07-18T12:00:00Z",
    "total_keys": 25
  },
  "working_memory_keys": {
    "current_analysis": {
      "key": "analysis_2025_07_18",
      "content_type": "analytical_thinking",
      "size_bytes": 1024,
      "created": "2025-07-18T12:00:00Z",
      "last_accessed": "2025-07-18T12:30:00Z",
      "access_count": 15,
      "importance_score": 0.95,
      "tags": ["analysis", "current", "high_priority"]
    }
  },
  "disk_storage_keys": {
    "historical_insights": {
      "key": "insights_batch_001",
      "content_type": "accumulated_knowledge",
      "file_path": "data/disk_context/insights_batch_001.json",
      "size_bytes": 4096,
      "compressed": false,
      "created": "2025-07-17T15:00:00Z",
      "archived": "2025-07-18T09:00:00Z",
      "access_count": 5,
      "importance_score": 0.88,
      "tags": ["insights", "historical", "archived"]
    }
  },
  "archived_keys": {},
  "key_relationships": {
    "analysis_2025_07_18": {
      "related_keys": ["strategy_planning", "execution_results"],
      "dependency_type": "sequential",
      "relationship_strength": 0.9
    }
  },
  "usage_statistics": {
    "most_accessed": "analysis_2025_07_18",
    "least_accessed": "temp_calculation_001",
    "average_size_bytes": 2048,
    "total_disk_usage_bytes": 102400
  }
}
```

## Implementation Guidelines for AI Agents

### 1. Start with Core Infrastructure

Begin implementation in this order:
1. `src/lkjagent.h` - Complete type definitions and API declarations with simple tag and context key types
2. `src/utils/data.c` - Foundation for all string operations (refactored from token.c)
3. `src/utils/tag_processor.c` - Simple tag format processing for LLM outputs
4. `src/utils/file.c` - Basic I/O capabilities with memory.json and context_keys.json support
5. `src/utils/json.c` - Configuration and memory.json support with state prompt handling
6. `src/config/config.c` - Configuration management with state-specific system prompts
7. `src/persistence/memory_persistence.c` - Memory and context key persistence

### 2. Build Memory and Context Layer

Continue with:
1. `src/memory/context_manager.c` - Context key management and LLM-directed paging
2. `src/memory/tagged_memory.c` - Core memory system with context key integration
3. `src/memory.c` - High-level memory management with unified storage
4. `src/utils/http.c` - Network communication with context-aware sizing

### 3. Implement LLM Integration

Build the LLM layer:
1. `src/llm/llm_client.c` - HTTP communication with LMStudio
2. `src/llm/prompt_manager.c` - State-specific prompt construction with simple tag format enforcement
3. `src/llm/response_parser.c` - Simple tag format validation and context key extraction
4. `src/llm/context_builder.c` - Context preparation for LLM calls

### 4. Implement State Machine and Paging

Complete the agent logic:
1. `src/state/enhanced_states.c` - State management with context paging integration
2. `src/state/thinking.c` - Thinking state with analysis and context key identification
3. `src/state/executing.c` - Executing state with disk enrichment operations
4. `src/state/evaluating.c` - Evaluating state with progress assessment
5. `src/state/paging.c` - LLM-controlled paging state implementation
6. `src/agent/core.c` - Main agent functionality with perpetual operation
7. `src/memory/enhanced_llm.c` - LLM integration for memory decisions

### 5. Complete Application and Utilities

Finalize with:
1. `src/utils/string_utils.c` - Advanced string operations
2. `src/utils/time_utils.c` - Time and timestamp utilities
3. `src/utils/error_handler.c` - Centralized error handling
4. `src/persistence/config_persistence.c` - Configuration persistence
5. `src/persistence/disk_operations.c` - Low-level disk operations
6. `src/lkjagent.c` - Application entry point (renamed from main.c)

### 6. Validation and Testing

Ensure all implementations:
- Handle edge cases and error conditions
- Follow memory safety principles
- Include comprehensive parameter validation
- Maintain consistent error reporting with concrete RETURN_ERR
- Provide complete API coverage
- Support perpetual operation without termination
- Manage context width during state transitions with LLM-controlled paging
- Use unified memory.json storage with context key integration
- Validate and process simple tag format outputs
- Implement state-specific system prompts correctly
- Support LLM-directed context key operations
- Maintain context key directory integrity

## Key Implementation Details

### Memory Safety Patterns

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

### Error Propagation with Concrete Implementation

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

### Perpetual State Machine Pattern with LLM-Controlled Paging

The agent operates in four distinct states with perpetual cycling and integrated LLM-controlled paging:

1. **THINKING**: Analysis and planning with context accumulation
   - Deep contemplative analysis using state-specific system prompt
   - Context key identification for important insights
   - Strategic planning with simple tag format output (<thinking> blocks)
   - Automatic context key tagging and categorization

2. **EXECUTING**: Action performance with disk enrichment
   - Task execution using state-specific system prompt
   - Disk storage operations directed by LLM in simple tag format (<action> blocks)
   - Context key specification for storing execution results
   - Quality data accumulation and organization

3. **EVALUATING**: Progress assessment and memory optimization
   - Performance evaluation using state-specific system prompt
   - Quality metrics and progress analysis in simple tag format (<evaluation> blocks)
   - Success measurement and improvement identification
   - Context effectiveness assessment

4. **PAGING**: LLM-controlled memory management
   - Context analysis using specialized paging system prompt
   - LLM directives for context key management in simple tag format (<paging> blocks)
   - Automatic context transfer between working memory and disk storage
   - Memory optimization and cleanup operations
   - Context key archival and retrieval operations

Each state function manages context width, processes simple tag format outputs, and returns the next state, ensuring continuous operation without termination while maintaining efficient memory usage through LLM-directed paging.

### Context Width Management Implementation with LLM Paging

```c
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
result_t llm_request_context_paging(lkjagent_t* agent, data_t* context_buffer, const char* target_state) {
    if (!agent || !context_buffer || !target_state) {
        RETURN_ERR("llm_request_context_paging: NULL parameter");
        return RESULT_ERR;
    }
    
    // Build paging prompt with current context analysis
    static char paging_prompt[2048];
    data_t prompt_token;
    if (data_init(&prompt_token, paging_prompt, sizeof(paging_prompt)) != RESULT_OK) {
        RETURN_ERR("Failed to initialize paging prompt");
        return RESULT_ERR;
    }
    
    // Use state-specific paging system prompt
    const char* paging_system_prompt = agent->config.agent.state_prompts.paging.system_prompt;
    if (data_set(&prompt_token, paging_system_prompt) != RESULT_OK) {
        RETURN_ERR("Failed to set paging system prompt");
        return RESULT_ERR;
    }
    
    // Add context analysis request in simple tag format
    if (data_append(&prompt_token, "\n\nAnalyze current context and respond with @paging block specifying:") != RESULT_OK ||
        data_append(&prompt_token, "\n- move_to_disk: context keys to move to disk storage") != RESULT_OK ||
        data_append(&prompt_token, "\n- retrieve_from_disk: context keys to retrieve from disk") != RESULT_OK ||
        data_append(&prompt_token, "\n- archive_old: context keys to archive for long-term storage") != RESULT_OK) {
        RETURN_ERR("Failed to build paging request");
        return RESULT_ERR;
    }
    
    // Send request to LLM and process simple tag response
    static char llm_response[4096];
    data_t response_token;
    if (data_init(&response_token, llm_response, sizeof(llm_response)) != RESULT_OK) {
        RETURN_ERR("Failed to initialize response token");
        return RESULT_ERR;
    }
    
    if (llm_send_request(agent, &prompt_token, &response_token) != RESULT_OK) {
        RETURN_ERR("Failed to send LLM paging request");
        return RESULT_ERR;
    }
    
    // Parse simple tag response and execute paging directives
    if (tag_parse_paging_directives(&response_token, agent) != RESULT_OK) {
        RETURN_ERR("Failed to parse LLM paging directives");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

/**
 * @brief Trim context data to fit within window limits
 */
__attribute__((warn_unused_result))
result_t data_trim_context(data_t* context, size_t max_size, size_t preserve_recent) {
    if (!context) {
        RETURN_ERR("data_trim_context: NULL context parameter");
        return RESULT_ERR;
    }
    
    if (context->size <= max_size) {
        return RESULT_OK; // No trimming needed
    }
    
    // Preserve recent data and trim older content
    size_t trim_amount = context->size - max_size + preserve_recent;
    
    if (data_trim_front(context, trim_amount) != RESULT_OK) {
        RETURN_ERR("Failed to trim context data");
        return RESULT_ERR;
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

## Testing Requirements

When implementing, ensure:
- All API functions are callable and functional
- Error conditions are properly handled with concrete error reporting
- Memory operations are bounded and safe
- Configuration loading works correctly with state-specific system prompts
- JSON parsing handles malformed input gracefully
- HTTP client can communicate with LMStudio
- Tagged memory operations perform correctly with unified storage and context key integration
- State machine transitions work with context width management and LLM-controlled paging
- Perpetual operation mode functions indefinitely
- Memory.json unified storage operates correctly with context key directory
- Context width management prevents overflow during state transitions
- Simple tag format validation works correctly for all LLM responses
- State-specific system prompts are loaded and used properly
- Context key operations (identification, storage, retrieval, archival) function correctly
- LLM-directed paging operations execute without errors
- Context key directory maintains integrity and consistency
- Simple tag format parsing extracts context keys and directives accurately

This guide provides the complete specification for regenerating the LKJAgent source code while maintaining its architectural integrity, coding standards, functional requirements, and ensuring perpetual operation with LLM-controlled context management, state-specific prompts, simple tag format processing, and unified memory storage with context key integration.
