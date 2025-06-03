# lkjagent - Program Specifications

## Overview

lkjagent is an autonomous AI agent framework designed for small Language Learning Models (LLMs). It provides a sophisticated memory management system with automatic data organization, tool execution capabilities, and continuous autonomous operation. The agent operates with a B-tree-like memory structure and can manage both working memory and persistent storage efficiently.

## Core Architecture

### 1. Agent Loop System

The main execution flow is orchestrated by the `agent_loop.ts` module, which implements a continuous operation cycle:

- **Initialization**: Tests LLM connection and validates configuration
- **Iteration Loop**: Executes indefinitely with the following steps:
  1. Generate system prompt with current memory context
  2. Call LLM with structured prompt
  3. Parse XML-formatted actions from LLM response
  4. Validate actions against security and format rules
  5. Execute validated actions on memory/storage
  6. Log results and continue to next iteration
- **Error Handling**: Graceful recovery from errors with configurable retry delays
- **Shutdown**: Handles SIGINT for graceful termination

### 2. Memory Management System

#### Working Memory Structure
```
/working_memory/
├── user_data/          # User-specific data and tasks
│   └── todo/           # Task management
├── action_result/      # Execution results (auto-cleaned)
│   └── _N/            # Numbered result entries
└── system_info/        # System state information
```

#### Storage Structure  
```
/storage/               # Persistent storage (not in prompts)
└── [any_path]/         # Hierarchical data storage
```

#### Memory Characteristics
- **Working Memory**: Limited by `working_memory_character_max` (default: 2048 chars)
- **Storage**: Unlimited persistent storage, excluded from LLM prompts
- **Auto-cleanup**: Action results automatically removed after processing
- **B-tree Organization**: Data organized hierarchically for efficient access

### 3. Tool System

#### Available Tools

1. **set** - Store data at specified path
   - Target: `/working_memory/` or `/storage/` paths
   - Content: Any JSON-serializable data
   - Creates intermediate paths automatically

2. **get** - Retrieve data from specified path
   - Returns data at path or error if not found
   - Works with both memory and storage paths

3. **rm** - Remove data at specified path
   - Deletes files/directories recursively
   - Validates path permissions

4. **mv** - Move/rename data between paths
   - Source and target paths required
   - Cross-memory/storage moves supported

5. **ls** - List contents of directory path
   - Shows directory structure
   - Indicates file vs directory entries

6. **search** - Query-based content search
   - Searches within specified scope path
   - Returns matching entries with content preview

7. **mkdir** - Create directory structure
   - Creates parent directories as needed
   - Idempotent operation

#### Tool Validation Rules
- All paths must start with `/working_memory/` or `/storage/`
- Required parameters validated before execution
- Path traversal attacks prevented
- JSON content validation for complex data

### 4. LLM Integration

#### Communication Protocol
- **API Format**: OpenAI-compatible chat completions API
- **Default Endpoint**: `http://localhost:1234/v1/chat/completions`
- **Request Format**: JSON with system message containing full context
- **Response Format**: XML-structured action commands

#### LLM Configuration
```typescript
interface LLMConfig {
  llm_api_url: string;      // API endpoint URL
  llm_model: string;        // Model identifier
  llm_max_tokens: number;   // Response length limit
  llm_temperature: number;  // Creativity parameter (0.0-1.0)
}
```

#### Response Format Specification
LLM must respond with XML in this exact format:
```xml
<actions>
  <action>
    <kind>set|get|rm|mv|ls|search|mkdir</kind>
    <target_path>/working_memory/path or /storage/path</target_path>
    <source_path>source path (for mv only)</source_path>
    <content>data content (for set only)</content>
  </action>
  <!-- Additional actions -->
</actions>
```

### 5. Configuration System

#### Configuration File: `data/config.json`

```typescript
interface AppConfig {
  // Memory Management
  working_memory_character_max: number;    // Memory size limit (default: 2048)
  key_token_max?: number;                  // Key length limit (default: 4)
  working_memory_children_max?: number;    // Max child objects (default: 8)
  
  // LLM Settings
  llm_api_url?: string;                    // LLM endpoint
  llm_model?: string;                      // Model name
  llm_max_tokens?: number;                 // Response token limit
  llm_temperature?: number;                // Sampling temperature
  
  // System Settings
  system_max_log_entries?: number;         // Log retention limit
  system_auto_cleanup?: boolean;           // Auto-remove old results
  system_debug_mode: boolean;              // Debug output control
}
```

#### Data Files Structure
```
data/
├── config.json         # Application configuration
├── memory.json          # Working memory persistence
├── storage.json         # Storage data persistence
└── log.json            # Action execution logs
```

### 6. Prompt Engineering

#### System Prompt Structure
The agent receives a comprehensive system prompt containing:

1. **Current Memory State**: Full working memory as XML
2. **Persona Definition**: Role as "The most powerful AI that thinks autonomously"
3. **Behavioral Specifications**:
   - Manage information in B-tree-like structure
   - Fill working memory to capacity limits
   - Evacuate less important data to storage when near limits
   - Generate creative content when no tasks present
   - Continuously update TODO and next_action
   - Save all action results for reference

4. **Output Format Rules**: Strict XML response format requirements

#### Autonomous Behavior Guidelines
- **Memory Optimization**: Automatically manage memory efficiency
- **Task Management**: Maintain and update TODO lists
- **Creative Mode**: Generate fantasy novels when idle
- **Self-Direction**: Determine next actions autonomously
- **Result Preservation**: Log all actions for learning

### 7. Data Persistence

#### File Format: JSON
All data stored in JSON format for human readability and debugging.

#### Persistence Strategy
- **Immediate Write**: Changes saved immediately after execution
- **Atomic Operations**: File writes are atomic to prevent corruption
- **Backup Strategy**: Previous versions can be manually preserved
- **Recovery**: Graceful handling of corrupted data files

#### Memory vs Storage
- **Memory**: Included in every LLM prompt, size-limited
- **Storage**: Excluded from prompts, unlimited size
- **Migration**: Agent can move data between memory/storage as needed

### 8. Error Handling and Validation

#### Validation Layers
1. **Schema Validation**: Action structure compliance
2. **Path Validation**: Security and format checks  
3. **Content Validation**: Data type and size limits
4. **Execution Validation**: Runtime error handling

#### Error Recovery
- **Continue on Error**: Non-fatal errors don't stop execution
- **Error Logging**: All errors logged with context
- **Retry Logic**: Automatic retry with exponential backoff
- **Graceful Degradation**: Partial functionality maintained

### 9. Security Considerations

#### Path Security
- **Whitelist Approach**: Only `/working_memory/` and `/storage/` allowed
- **Path Traversal Prevention**: `../` sequences blocked
- **Sandbox Enforcement**: No access to system files

#### Content Security  
- **JSON Validation**: All content must be valid JSON
- **Size Limits**: Configurable limits prevent memory exhaustion
- **Type Safety**: TypeScript enforcement throughout

### 10. Testing Framework

#### Test Categories
1. **Unit Tests**: Individual tool functionality
2. **Integration Tests**: Full workflow testing
3. **Comprehensive Tests**: All tools in sequence
4. **Performance Tests**: Memory and speed benchmarks

#### Test Coverage
- All 7 tool types with success/failure cases
- Memory management edge cases
- LLM communication error scenarios
- Configuration validation

### 11. Development and Deployment

#### Build Process
```bash
npm run build      # Compile TypeScript to JavaScript
npm run start      # Run compiled agent
npm run dev        # Development mode with watch
npm run test       # Run all tests
```

#### Dependencies
- **Runtime**: Node.js, TypeScript, axios, fs-extra
- **Development**: @types packages for TypeScript support
- **External**: LLM server (LMStudio, Ollama, etc.)

#### Directory Structure
```
lkjagent/
├── src/                 # TypeScript source code
│   ├── index.ts         # Main entry point
│   ├── config/          # Configuration management
│   ├── tool/            # Tool implementations
│   ├── types/           # TypeScript interfaces
│   └── util/            # Utility functions
├── data/                # Runtime data files
├── dist/                # Compiled JavaScript
├── test-*.js            # Test suites
└── package.json         # Project configuration
```

### 12. Operational Characteristics

#### Performance
- **Memory Efficient**: Configurable memory limits
- **CPU Light**: Minimal processing between LLM calls
- **Network Dependent**: Performance tied to LLM response time

#### Scalability
- **Single Instance**: Designed for single-agent operation
- **Stateful**: Maintains persistent state across restarts
- **Extensible**: New tools can be added modularly

#### Monitoring
- **Console Logging**: Detailed operation logs with emojis
- **Debug Mode**: Additional diagnostic information
- **Action Tracking**: Complete audit trail of all operations

## Usage Patterns

### Initialization
1. Install dependencies: `npm install`
2. Initialize data files: `npm run init-data`
3. Configure LLM endpoint in `data/config.json`
4. Start LLM server on configured endpoint
5. Launch agent: `npm start`

### Typical Operation Flow
1. Agent starts and tests LLM connection
2. Loads current memory state into prompt
3. LLM receives context and responds with actions
4. Actions are parsed, validated, and executed
5. Results stored in action_result and memory updated
6. Process repeats indefinitely

### Customization Points
- **Tool Extensions**: Add new tools in `src/tool/`
- **Prompt Modifications**: Customize behavior in `src/util/prompt.ts`
- **Memory Structure**: Modify data organization patterns
- **LLM Integration**: Adapt for different API formats

This comprehensive specification provides the foundation for complete regeneration of the lkjagent system, capturing all essential functionality, architecture decisions, and operational characteristics.
