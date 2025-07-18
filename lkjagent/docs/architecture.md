# LKJAgent Architecture Overview

## Core Design Principles

1. **Zero External Dependencies**: Pure C implementation using only standard POSIX libraries
2. **Memory Safety**: Stack-based allocation with bounded buffers and explicit capacity management
3. **Error Handling**: Comprehensive error propagation using `result_t` enum and concrete `RETURN_ERR` macro implementation
4. **Modular Design**: Clean separation between core functionality, utilities, memory management, and state handling
5. **Tagged Memory System**: Advanced memory management with multi-tag queries and LLM integration
6. **State Machine Architecture**: Four-state autonomous execution cycle (thinking, executing, evaluating, paging)
7. **Perpetual Operation**: The agent operates continuously without termination, perpetually enriching disk storage

## Project Structure

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
