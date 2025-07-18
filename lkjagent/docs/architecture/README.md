# LKJAgent Architecture

## Core Design Principles

1. **Zero External Dependencies**: Pure C implementation using only standard POSIX libraries
2. **Memory Safety**: Stack-based allocation with bounded buffers and explicit capacity management
3. **Error Handling**: Comprehensive error propagation using `result_t` enum and concrete `RETURN_ERR` macro
4. **Modular Design**: Clean separation between core functionality, utilities, memory management, and state handling
5. **Tagged Memory System**: Advanced memory management with multi-tag queries and LLM integration
6. **State Machine Architecture**: Four-state autonomous execution cycle (thinking, executing, evaluating, paging)
7. **Perpetual Operation**: The agent operates continuously without termination, perpetually enriching disk storage

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
- **Unified Memory Query Interface**: Single interface for both working and disk memory
- **Smart Context Paging**: LLM determines what context to preserve, archive, or retrieve
- **Context Key Directory**: Maintained in `context_keys.json` for efficient retrieval

## State Machine

The agent operates through four main states in continuous cycles:

### 1. THINKING State
- **Purpose**: Deep analysis and strategic planning
- **Activities**: Problem decomposition, insight generation, context key identification
- **Output**: `<thinking>` blocks with analysis, planning, and context keys
- **Duration**: Variable based on complexity

### 2. EXECUTING State  
- **Purpose**: Action performance and disk enrichment
- **Activities**: Task execution, data storage operations, progress tracking
- **Output**: `<action>` blocks with storage directives and context keys
- **Focus**: Quality data accumulation

### 3. EVALUATING State
- **Purpose**: Progress assessment and quality metrics
- **Activities**: Performance analysis, success measurement, improvement identification
- **Output**: `<evaluation>` blocks with metrics and recommendations
- **Metrics**: Quality scores, enrichment rates, progress assessments

### 4. PAGING State
- **Purpose**: LLM-controlled memory management
- **Activities**: Context analysis, memory optimization, disk storage operations
- **Output**: `<paging>` blocks with specific memory directives
- **Operations**: Context key archival, retrieval, and cleanup

## Module Architecture

### Core Layer
- **Agent Core** (`src/agent/core.c`): State machine and lifecycle management
- **Memory Management** (`src/memory/`): Tagged memory and context management
- **Configuration** (`src/config/`): Settings and state-specific prompts

### LLM Layer
- **LLM Client** (`src/llm/llm_client.c`): HTTP communication with LMStudio
- **Prompt Manager** (`src/llm/prompt_manager.c`): State-specific prompt construction
- **Response Parser** (`src/llm/response_parser.c`): Tag format validation and parsing

### Utilities Layer
- **Data Tokens** (`src/utils/data.c`): Safe string handling
- **Tag Processor** (`src/utils/tag_processor.c`): Simple tag format processing
- **File Operations** (`src/utils/file.c`): I/O with error handling
- **HTTP Client** (`src/utils/http.c`): Network communication

### Persistence Layer
- **Memory Persistence** (`src/persistence/memory_persistence.c`): Unified memory.json storage
- **Config Persistence** (`src/persistence/config_persistence.c`): Configuration management

## Communication Format

### LLM Tag Format

All LLM interactions use simple `<tag>content</tag>` format:

```
<thinking>
<analysis>Situation analysis</analysis>
<planning>Strategic planning</planning>
<context_keys>key1,key2,key3</context_keys>
</thinking>
```

**Format Rules:**
- Simple opening/closing tags only
- No attributes or complex markup
- Plain text content
- Consistent tag names
- Context key specification

### Context Key System

Context keys enable efficient memory management:

- **Identification**: LLM identifies important context elements
- **Storage**: Elements stored with unique keys
- **Retrieval**: Efficient access by key
- **Archival**: Automatic cleanup of old contexts
- **Relationships**: Tracking dependencies between contexts

## Data Flow

1. **Input Processing**: Configuration and memory loading
2. **State Execution**: LLM interaction with state-specific prompts
3. **Response Processing**: Tag format parsing and validation
4. **Memory Operations**: Context key management and storage
5. **State Transition**: Context width management and paging
6. **Persistence**: Unified storage to memory.json and context_keys.json

## Error Handling Architecture

- **Result Types**: All functions return `result_t` (OK/ERR)
- **Error Propagation**: Consistent error handling with `RETURN_ERR` macro
- **Input Validation**: Comprehensive parameter checking
- **Resource Management**: Bounded operations and cleanup
- **Recovery**: Graceful degradation and error reporting

## Performance Characteristics

- **Memory Usage**: Fixed allocation, no dynamic memory
- **Context Management**: LLM-controlled paging prevents overflow
- **Disk I/O**: Efficient JSON operations with atomic writes
- **Network**: HTTP client with timeout and retry logic
- **CPU**: Lightweight state machine with optimized parsing

- **Agent Core**: State machine and lifecycle management
- **Memory System**: Tagged memory with LLM integration
- **Configuration**: JSON-based configuration management
- **LLM Integration**: Simple tag-based communication

### Utility Modules

- **Data Tokens**: Safe string handling with bounded buffers
- **File Operations**: Atomic file I/O operations
- **HTTP Client**: Socket-based HTTP implementation
- **JSON Parser**: Manual JSON processing (no external deps)

## File Structure

```
src/
├── lkjagent.h              # Main header with all type definitions
├── lkjagent.c              # Application entry point
├── agent/                  # Agent lifecycle and states
├── config/                 # Configuration management
├── memory/                 # Tagged memory and persistence
├── state/                  # State implementations
├── llm/                    # LLM client and prompt management
├── utils/                  # Utility functions
└── persistence/            # Data persistence layer
```
