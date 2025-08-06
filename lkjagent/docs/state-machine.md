# State Machine Documentation

## Overview

LKJAgent implements a finite state machine that governs AI behavior through distinct operational modes. The state machine ensures predictable agent behavior while providing flexibility for complex reasoning tasks.

## State Definitions

### Primary States

1. **Thinking State**
   - **Purpose**: Analysis, reasoning, and decision-making
   - **Duration**: Variable (until decision reached)
   - **Output**: `next_state` and `thinking_log`
   - **Transitions**: → `thinking` (continue analysis) or → `executing` (take action)

2. **Executing State**
   - **Purpose**: Concrete actions on memory and storage
   - **Duration**: Single action per cycle
   - **Output**: `action` with `type`, `tags`, and `value`
   - **Transitions**: → `thinking` (default return)

3. **Paging State** (Planned)
   - **Purpose**: Memory overflow management
   - **Duration**: Single paging operation
   - **Output**: Context switching decisions
   - **Transitions**: → `thinking` (after context management)

## State Transition Diagram

```
┌─────────────────┐
│   Application   │
│     Start       │
└─────────┬───────┘
          │
          ▼
┌─────────────────┐
│   Initialize    │◄──────────────┐
│   Agent State   │               │
└─────────┬───────┘               │
          │                       │
          ▼                       │
┌─────────────────┐               │
│    THINKING     │               │
│                 │               │
│ • Analyze info  │◄──────────────┤
│ • Build chains  │               │
│ • Make decisions│               │
└─────────┬───────┘               │
          │                       │
          ▼                       │
    ┌─────────┐                   │
    │Decision?│                   │
    └────┬────┘                   │
         │                       │
    ┌────▼────┐                   │
    │Continue │  ┌─────────────┐  │
    │thinking?│  │   Action    │  │
    └────┬────┘  │  required?  │  │
         │       └─────┬───────┘  │
         │ No          │ Yes      │
         │             ▼          │
         │    ┌─────────────────┐ │
         │    │   EXECUTING     │ │
         │    │                 │ │
         │    │ • Memory ops    │ │
         │    │ • Storage ops   │─┘
         │    │ • State updates │
         │    └─────────────────┘
         │
         ▼
┌─────────────────┐
│   Agent Cycle   │
│    Complete     │
└─────────────────┘
```

## State Configuration

### Base Configuration
Located in `data/config.json` under `agent.state.base.prompt`:

```json
{
  "role": "An AI state machine that utilizes a finite working memory to output both next_state and thinking_log, while managing infinite storage. You are the ultimate librarian-like system that manages information intelligently.",
  "instructions": "Always respond with valid XML containing an 'agent' tag. In thinking state, provide 'next_state' and 'thinking_log'. In executing state, provide 'action' with 'type', 'tags', and optionally 'value'."
}
```

### State-Specific Prompts

#### Thinking State Configuration
```json
{
  "state_description": {
    "thinking": "Build reasoning chains and accumulate thinking_log entries. Analyze information and decide next actions.",
    "executing": "Perform concrete actions: working_memory_add, working_memory_remove, storage_load, storage_save"
  }
}
```

#### Executing State Configuration
```json
{
  "available_actions": {
    "working_memory_add": "Add new information to working memory with tags and value",
    "working_memory_remove": "Remove information from working memory using tags", 
    "storage_load": "Load information from persistent storage to working memory using tags",
    "storage_save": "Save information from working memory to persistent storage using tags and value"
  }
}
```

## State Transition Logic

### Thinking State Processing

```c
// In thinking state, agent outputs:
{
  "agent": {
    "next_state": "thinking|executing",
    "thinking_log": "Reasoning and analysis content"
  }
}
```

**Transition Rules:**
- `next_state: "thinking"` → Continue analysis in next cycle
- `next_state: "executing"` → Switch to action execution
- No `next_state` specified → Default to thinking

### Executing State Processing

```c
// In executing state, agent outputs:
{
  "agent": {
    "action": {
      "type": "working_memory_add|working_memory_remove|storage_load|storage_save",
      "tags": "identifier_for_information",
      "value": "content_to_store" // Optional, required for add/save
    }
  }
}
```

**Transition Rules:**
- After executing any action → Automatically return to thinking state
- Invalid action → Return to thinking state with error logged

## Action Execution Details

### Working Memory Actions

#### working_memory_add
```json
{
  "action": {
    "type": "working_memory_add",
    "tags": "character_alice_personality", 
    "value": "adventurous, nature-loving, 25 years old"
  }
}
```

**Process:**
1. Extract `tags` and `value` from action
2. Convert spaces to underscores in tags for valid identifiers
3. Add key-value pair to agent's working memory
4. Log successful addition

#### working_memory_remove
```json
{
  "action": {
    "type": "working_memory_remove",
    "tags": "character_bob_old_description"
  }
}
```

**Process:**
1. Extract `tags` from action
2. Locate matching object in working memory
3. Remove object and clean up references
4. Log successful removal

### Storage Actions

#### storage_load
```json
{
  "action": {
    "type": "storage_load", 
    "tags": "character_alice_background"
  }
}
```

**Process:**
1. Extract `tags` from action
2. Search persistent storage for matching key
3. Copy value to working memory if found
4. Log load result (success/not found)

#### storage_save
```json
{
  "action": {
    "type": "storage_save",
    "tags": "story_plot_chapter1",
    "value": "Alice discovers mysterious letter in grandmother's attic"
  }
}
```

**Process:**
1. Extract `tags` and `value` from action
2. Add/update key-value pair in persistent storage
3. Mark memory for persistence to disk
4. Log successful save

## State Management Implementation

### State Initialization
```c
// Default state loaded from memory.json
if (object_provide_str(pool, &agent_state, agent->data, "state") != RESULT_OK) {
    RETURN_ERR("Failed to get state from agent");
}
```

### State Transition Execution
```c
// Update agent state based on LLM response
if (object_provide_str(pool, &next_state_obj, response_obj, "next_state") == RESULT_OK) {
    if (object_set_string(pool, agent_state_obj, 
                          string_literal("state"), 
                          next_state_obj->string) != RESULT_OK) {
        RETURN_ERR("Failed to update agent state");
    }
}
```

### Thinking Log Management
```c
// Add thinking log entry to agent memory
static result_t add_thinking_log_entry(pool_t* pool, agent_t* agent, 
                                      const string_t* thinking_log) {
    // Generate unique key with timestamp
    // Add to agent's thinking log collection
    // Manage log rotation based on max_entries
}
```

## State Persistence

### Memory Structure
```json
{
  "working_memory": {
    "key1": "value1",
    "key2": "value2"
  },
  "storage": {
    "persistent_key1": "persistent_value1"
  },
  "state": "thinking",
  "thinking_log_001": "First reasoning entry",
  "thinking_log_002": "Second reasoning entry"
}
```

### Persistence Triggers
- **After Each Cycle**: Complete agent state saved to `data/memory.json`
- **Action Execution**: Immediate persistence of memory modifications
- **Error Conditions**: State saved before error propagation

## Error Handling in State Machine

### Invalid State Transitions
```c
// Validation of state transitions
if (strcmp(current_state, "thinking") != 0 && 
    strcmp(current_state, "executing") != 0) {
    // Reset to default thinking state
    printf("Invalid state detected, resetting to thinking\n");
    current_state = "thinking";
}
```

### Action Execution Failures
- **Parsing Errors**: Return to thinking state with error logged
- **Memory Exhaustion**: Attempt graceful degradation or paging
- **Invalid Actions**: Log error and continue with thinking state

### Recovery Mechanisms
- **State Reset**: Return to known good state (thinking)
- **Memory Restoration**: Reload from last known good memory state
- **Graceful Degradation**: Continue operation with reduced functionality

## Performance Characteristics

### State Transition Overhead
- **Thinking → Executing**: Minimal (XML parsing + validation)
- **Executing → Thinking**: Minimal (automatic transition)
- **Memory Operations**: O(1) for working memory, O(n) for storage search

### Memory Usage per State
- **Thinking State**: Current working memory + prompt context
- **Executing State**: Action parameters + target memory objects
- **State Persistence**: Complete agent state serialization

## Configuration Parameters

### Thinking Log Management
```json
{
  "thinking_log": {
    "enable": true,
    "max_entries": 10,
    "key_prefix": "thinking_log_"
  }
}
```

### Memory Limits
```json
{
  "paging_limit": {
    "enable": true,
    "max_tokens": 4096
  },
  "hard_limit": {
    "enable": true, 
    "max_tokens": 8192
  }
}
```

### Iteration Control
```json
{
  "iterate": {
    "enable": true,
    "max_iterations": 1000
  }
}
```

## Debugging State Machine

### State Visibility
```c
printf("Current state: %.*s\n", 
       (int)agent_state->string->size, 
       agent_state->string->data);
```

### Transition Logging
- **State Changes**: Log all state transitions with timestamps
- **Action Execution**: Log action type, parameters, and results
- **Error Conditions**: Comprehensive error context for debugging

### Memory Inspection
- **Working Memory Dump**: Print all key-value pairs
- **Storage Contents**: Display persistent storage state
- **Thinking Log Review**: Show reasoning chain progression
