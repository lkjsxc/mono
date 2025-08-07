# Agent Core Architecture - Modular Design

## Overview

The LKJAgent core has been refactored into a modular architecture that separates concerns and improves maintainability. The monolithic `core.c` file has been partitioned into specialized modules, each handling a specific aspect of agent functionality.

## Module Breakdown

### 1. Core Orchestration (`agent/core.c`, `agent/core.h`)

**Purpose**: High-level orchestration and coordination between modules
**Size**: ~100 lines (down from 999 lines)
**Responsibilities**:
- Main agent processing cycle coordination
- Module integration and error handling
- Resource lifecycle management

**Key Functions**:
- `lkjagent_agent()` - Main agent processing cycle
- `lkjagent_agent_execute()` - Response processing and action execution

### 2. State Management (`agent/state.c`, `agent/state.h`)

**Purpose**: Agent state transitions and memory-aware decision making
**Size**: ~300 lines
**Responsibilities**:
- State transition logic (thinking ↔ executing ↔ evaluating ↔ paging)
- Token counting and memory overflow detection
- Thinking and evaluation log management
- Automatic state transitions based on memory usage

**Key Functions**:
- `agent_state_auto_transition()` - Executing → Evaluating transitions
- `agent_state_update_and_log()` - Regular state updates with logging
- `agent_state_handle_evaluation_transition()` - Memory-aware evaluating → thinking/paging transitions
- `agent_state_estimate_tokens()` - Memory usage estimation

### 3. Prompt Generation (`agent/prompt.c`, `agent/prompt.h`)

**Purpose**: LLM prompt construction and JSON request building
**Size**: ~200 lines
**Responsibilities**:
- Configuration object extraction and validation
- Incremental prompt building (header, base, state, memory, footer)
- JSON request formatting for LLM API
- String escaping and XML serialization

**Key Functions**:
- `agent_prompt_generate()` - Main prompt generation orchestrator
- `agent_prompt_extract_config_objects()` - Configuration parsing
- `agent_prompt_build_header()` - JSON request header
- `agent_prompt_append_base()` - Base prompt from config
- `agent_prompt_append_state()` - State-specific prompt
- `agent_prompt_append_memory()` - Working memory serialization
- `agent_prompt_append_footer()` - Model configuration and closing

### 4. HTTP Communication (`agent/http.c`, `agent/http.h`)

**Purpose**: LLM communication and response processing
**Size**: ~150 lines
**Responsibilities**:
- HTTP resource management (request/response strings)
- POST request execution to LLM endpoint
- JSON response parsing and content extraction
- Resource cleanup and error handling

**Key Functions**:
- `agent_http_send_receive()` - High-level HTTP communication
- `agent_http_create_resources()` - Resource allocation
- `agent_http_extract_response_content()` - Response parsing
- `agent_http_cleanup_resources()` - Memory cleanup

### 5. Action Execution (`agent/actions.c`, `agent/actions.h`)

**Purpose**: Agent action processing and memory persistence
**Size**: ~250 lines
**Responsibilities**:
- Working memory operations (add, remove)
- Storage operations (load, save)
- Action dispatching based on type
- Memory persistence to JSON files
- XML response parsing

**Key Functions**:
- `agent_actions_dispatch()` - Action type routing
- `agent_actions_execute_working_memory_add()` - Memory addition
- `agent_actions_execute_working_memory_remove()` - Memory removal
- `agent_actions_execute_storage_load()` - Storage → memory loading
- `agent_actions_execute_storage_save()` - Memory → storage saving
- `agent_actions_save_memory()` - Agent state persistence
- `agent_actions_parse_response()` - XML response parsing

## State Machine Enhancements

### New Evaluating State

The architecture introduces a mandatory **evaluating** state that provides reflection and assessment capabilities:

**Flow**: `thinking` → `executing` → `evaluating` → `thinking` (or `paging`)

**Benefits**:
- **Explicit Reflection**: Every action is followed by evaluation
- **Progress Tracking**: Agent assesses goal progress after each action
- **Memory-Aware Transitions**: Evaluation phase checks memory usage
- **Improved Learning**: Continuous assessment improves decision quality

### Enhanced State Transitions

**Executing State Changes**:
- No longer specifies `next_state` in output
- Always automatically transitions to `evaluating`
- Simplified action-only output format

**Evaluating State Logic**:
- Always specifies `next_state: "thinking"`
- Includes `evaluation_log` for reflection content
- System checks memory usage before actual transition
- May override to `paging` if memory limits exceeded

**Memory-Aware Transitions**:
- Token counting happens during evaluation phase
- Automatic `paging` transition when memory limits reached
- Preserves LLM's intended flow while ensuring memory safety

## Benefits of Modular Architecture

### 1. Maintainability
- **Separation of Concerns**: Each module has a single, well-defined responsibility
- **Smaller Files**: Easier to understand and modify individual components
- **Clear Interfaces**: Well-defined function signatures and module boundaries
- **Reduced Complexity**: Simplified debugging and testing

### 2. Extensibility
- **Pluggable Components**: Modules can be enhanced or replaced independently
- **Clean Interfaces**: New functionality can be added without affecting other modules
- **Modular Testing**: Individual components can be tested in isolation
- **Feature Development**: New capabilities can be developed in focused modules

### 3. Code Quality
- **Reduced Duplication**: Common patterns extracted into reusable functions
- **Consistent Error Handling**: Standardized error patterns across modules
- **Better Documentation**: Each module can be documented independently
- **Improved Readability**: Clear module boundaries and responsibilities

### 4. Performance
- **Optimized Memory Usage**: Better resource management in specialized modules
- **Efficient State Handling**: Dedicated state management reduces overhead
- **Streamlined HTTP**: Focused communication module improves efficiency
- **Smart Transitions**: Memory-aware transitions prevent overflow conditions

## Module Dependencies

```
┌─────────────┐
│    core     │ ← Main orchestrator
└─────┬───────┘
      │ imports all modules
      ▼
┌─────────────┬─────────────┬─────────────┬─────────────┐
│    state    │   prompt    │    http     │   actions   │
└─────────────┴─────────────┴─────────────┴─────────────┘
      │             │             │             │
      ▼             ▼             ▼             ▼
┌─────────────────────────────────────────────────────────┐
│              Utility Libraries                          │
│  pool.h  object.h  string.h  http.h  file.h           │
└─────────────────────────────────────────────────────────┘
```

## Implementation Notes

### Error Handling
All modules follow consistent error handling patterns:
- Functions return `result_t` (OK/ERR)
- Detailed error messages with context
- Proper resource cleanup on error paths
- Graceful degradation where possible

### Memory Management
Memory management remains centralized through the pool system:
- All allocations go through the memory pool
- Consistent cleanup patterns across modules
- RAII-style resource management
- Leak prevention through structured cleanup

### Configuration Integration
All modules access configuration through the shared config object:
- Centralized configuration management
- Module-specific configuration sections
- Runtime configuration updates supported
- Validation at module boundaries

## Migration Path

The modular architecture is designed for backward compatibility:

1. **Existing Functionality**: All original features remain intact
2. **API Compatibility**: Public interfaces unchanged
3. **Configuration**: Existing config files work without modification
4. **Incremental Adoption**: Modules can be enhanced independently
5. **Testing**: Existing test cases continue to work

## Future Enhancements

The modular design enables several future improvements:

### 1. Advanced State Management
- Dynamic state addition
- Conditional state transitions
- State-specific memory limits
- Custom evaluation criteria

### 2. Enhanced Communication
- Multiple LLM endpoints
- Streaming responses
- Retry mechanisms
- Load balancing

### 3. Sophisticated Actions
- Composite actions
- Conditional execution
- Action validation
- Rollback capabilities

### 4. Memory Optimization
- Smart paging strategies
- Context compression
- Priority-based retention
- Adaptive limits

## Conclusion

The modular architecture represents a significant improvement in code organization, maintainability, and extensibility. By separating concerns into focused modules, the system becomes more robust, easier to understand, and simpler to enhance. The introduction of the evaluating state adds valuable reflection capabilities while maintaining the system's efficiency and reliability.
