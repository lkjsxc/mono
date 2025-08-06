# LKJAgent Architecture Overview

## System Design Philosophy

LKJAgent implements a **reactive AI agent architecture** that combines:
- **Finite State Machine** for predictable behavior patterns
- **Dual-Layer Memory System** for efficient information management
- **Pool-Based Memory Allocation** for performance and reliability
- **LLM Integration** for intelligent reasoning and decision-making

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        LKJAgent                             │
├─────────────────────────────────────────────────────────────┤
│  Main Loop (lkjagent.c)                                    │
│  ├─ Initialize: Pool, Config, Agent Memory                 │
│  ├─ Run: 5 iterations of agent processing                  │
│  └─ Cleanup: Memory deallocation and statistics            │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    Agent Core (core.c)                     │
├─────────────────────────────────────────────────────────────┤
│  State Machine Controller                                   │
│  ├─ State: thinking → executing → thinking                 │
│  ├─ Prompt Generation (context + memory)                   │
│  ├─ LLM Communication (HTTP POST)                          │
│  ├─ Response Processing (XML/JSON parsing)                 │
│  └─ Action Execution (memory operations)                   │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────┬─────────────────┬─────────────────────────┐
│   Memory Pool   │   Object System │    Utility Libraries    │
│   (pool.c)      │   (object.c)    │                         │
├─────────────────┼─────────────────┼─────────────────────────┤
│ • String pools  │ • JSON parsing  │ • HTTP client (http.c)  │
│   - 16 bytes    │ • XML parsing   │ • File I/O (file.c)     │
│   - 256 bytes   │ • Object        │ • String ops (string.c) │
│   - 4KB         │   manipulation  │                         │
│   - 64KB        │ • Path-based    │                         │
│   - 1MB         │   access        │                         │
│ • Object pool   │                 │                         │
│ • Freelist mgmt │                 │                         │
└─────────────────┴─────────────────┴─────────────────────────┘
```

## Core Components

### 1. Agent State Machine

The agent operates in three distinct states:

- **Thinking State**: Processes information, builds reasoning chains, decides on next actions
- **Executing State**: Performs concrete actions on memory (add, remove, load, save)
- **Paging State**: Manages memory overflow through intelligent context switching

### 2. Memory Architecture

#### Working Memory
- **Purpose**: Active information processing and immediate context
- **Capacity**: Limited by configuration (token limits)
- **Storage**: In-memory key-value pairs
- **Access Pattern**: Fast read/write operations

#### Persistent Storage
- **Purpose**: Long-term information retention
- **Capacity**: Unlimited (file-based)
- **Storage**: JSON serialized to `data/memory.json`
- **Access Pattern**: Load/save operations through agent actions

### 3. Memory Pool System

```
Pool Type        | Count    | Size      | Total Memory
-----------------|----------|-----------|-------------
string16         | 1,048,576| 16 bytes  | 16 MB
string256        | 65,536   | 256 bytes | 16 MB  
string4096       | 4,096    | 4 KB      | 16 MB
string65536      | 256      | 64 KB     | 16 MB
string1048576    | 16       | 1 MB      | 16 MB
object           | 65,536   | struct    | Variable
-----------------|----------|-----------|-------------
Total Pre-allocated Memory: ~80-100 MB
```

### 4. LLM Integration

#### Request Flow
1. **Prompt Generation**: Combines base prompt, state prompt, working memory, and configuration
2. **HTTP Request**: POST to configured LLM endpoint with JSON payload
3. **Response Processing**: Extract content from LLM response
4. **Action Execution**: Parse XML response and execute specified actions

#### Prompt Structure
```
[Base Prompt] - Role and general instructions
[State Prompt] - Current state-specific guidance  
[Working Memory] - Current context and information
[Configuration] - Available actions and examples
```

## Data Flow

```
Configuration ──┐
                ├─→ Prompt Generation ──→ LLM Request ──→ Response
Memory State ───┘                                         │
                                                          ▼
Action Execution ←── XML Parsing ←── Content Extraction ──┘
      │
      ▼
Memory Update ──→ State Transition ──→ Next Iteration
```

## Component Interactions

### Initialization Sequence
1. **Memory Pool**: Initialize all string and object pools
2. **Configuration**: Load and parse `data/config.json`
3. **Agent Memory**: Load and parse `data/memory.json`
4. **Validation**: Verify all required configuration paths exist

### Processing Cycle
1. **Extract Context**: Get working memory, agent state, configuration
2. **Build Prompt**: Combine context according to current state
3. **LLM Communication**: Send request, receive response
4. **Parse Response**: Extract XML content, validate structure
5. **Execute Actions**: Modify memory based on agent decisions
6. **Update State**: Transition to next state if specified
7. **Persist**: Save updated memory state

### Cleanup Sequence
1. **Resource Deallocation**: Free all dynamically allocated objects
2. **Pool Statistics**: Report freelist counts for debugging
3. **Memory Verification**: Ensure no memory leaks

## Error Handling Strategy

- **Result Types**: All functions return `result_t` (OK/ERR)
- **Error Propagation**: Consistent error bubbling with cleanup
- **Resource Management**: RAII-style resource acquisition/release
- **Validation**: Input validation at all API boundaries
- **Logging**: Comprehensive error messages with context

## Thread Safety

LKJAgent is designed as a **single-threaded system**:
- No concurrent access to memory pools
- Sequential processing of agent iterations  
- Atomic file operations for persistence
- HTTP requests are synchronous and blocking

## Performance Characteristics

### Memory Allocation
- **O(1)** allocation/deallocation through freelists
- **Zero malloc() calls** during normal operation
- **Memory locality** through pool-based allocation

### Processing Speed
- **Microsecond-level** memory operations
- **Network-bound** by LLM response times
- **Linear scaling** with working memory size

### Storage I/O
- **Batch persistence** after each agent cycle
- **JSON serialization** optimized for readability
- **File-based storage** for simplicity and debugging
