# Implementation Guidelines for AI Agents

## Implementation Order

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
4. `src/persistence/config_persistence.c` - Configuration persistence
5. `src/persistence/disk_operations.c` - Low-level disk operations
6. `src/lkjagent.c` - Application entry point (renamed from main.c)

## Key Implementation Patterns

### Context Width Management with LLM Paging

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
    
    // Calculate available context window size
    size_t max_context = agent->config.agent.llm_decisions.context_window_size;
    size_t current_context_size = context_buffer->size;
    
    // Invoke LLM paging if approaching limit (reserve 20% buffer)
    if (current_context_size > (max_context * 0.8)) {
        // Request LLM to specify context keys for disk storage
        if (llm_request_context_paging(agent, context_buffer, target_state) != RESULT_OK) {
            RETURN_ERR("Failed to execute LLM-controlled context paging");
            return RESULT_ERR;
        }
    }
    
    // Add state transition marker
    char transition_marker[256];
    snprintf(transition_marker, sizeof(transition_marker), 
             "\n--- STATE_TRANSITION: %s ---\n", target_state);
    
    if (data_append(context_buffer, transition_marker) != RESULT_OK) {
        RETURN_ERR("Failed to add state transition marker");
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

### Perpetual State Machine Pattern

The agent operates in four distinct states with perpetual cycling and integrated LLM-controlled paging:

1. **THINKING**: Analysis and planning with context accumulation
2. **EXECUTING**: Action performance with disk enrichment
3. **EVALUATING**: Progress assessment and memory optimization
4. **PAGING**: LLM-controlled memory management

Each state function manages context width, processes simple tag format outputs, and returns the next state, ensuring continuous operation without termination while maintaining efficient memory usage through LLM-directed paging.

## Validation and Testing

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
