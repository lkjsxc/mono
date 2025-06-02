# lkjagent

**lkjagent** is an advanced AI Agent framework designed for small Language Learning Models (LLMs). It implements a sophisticated memory architecture with finite working memory and infinite persistent storage, communicating through structured XML actions to manage complex, long-running tasks.

The framework features a modular TypeScript architecture with distributed utilities for maintainable and scalable agent development.

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Installation](#installation)
- [Configuration](#configuration)
- [Modular Utilities](#modular-utilities)
- [Core Components](#core-components)
- [Tool System](#tool-system)
- [Data Management](#data-management)
- [Usage](#usage)
- [Development](#development)
- [Project Structure](#project-structure)
- [Examples](#examples)
- [API Reference](#api-reference)
- [Contributing](#contributing)
- [License](#license)

## Overview

lkjagent addresses the challenge of enabling small LLMs to handle complex, persistent tasks by providing:

- **Dual Memory Architecture**: Finite working memory (RAM) and infinite persistent storage
- **XML-based Communication**: Structured action protocol for reliable LLM interaction
- **Persistent Task Management**: Long-running tasks with state preservation
- **Configurable Memory Limits**: Adaptive to different LLM capabilities
- **Action Logging**: Comprehensive tracking of all agent operations

## Architecture

### Memory System

lkjagent implements a dual-memory architecture inspired by human cognitive systems:

#### Working Memory (RAM)
- **Finite capacity** (configurable, default: 2048 characters)
- **Fast access** for current task context
- **Structured data** with predefined schemas
- **Automatic cleanup** when limits are reached

#### Persistent Storage
- **Infinite capacity** for long-term data retention
- **Hierarchical organization** with path-based access
- **Search capabilities** across all stored content
- **Archival system** for completed tasks

### Communication Protocol

The agent communicates using a structured XML format:

```xml
<actions>
  <action>
    <kind>memory_set</kind>
    <target_path>/user/todo/new_task</target_path>
    <content>Complete the project documentation</content>
  </action>
</actions>
```

## Installation

### Prerequisites

- Node.js 16+ and npm
- TypeScript 4.5+
- LM Studio or compatible local LLM server

### Setup

```bash
# Clone the repository
git clone <repository-url>
cd lkjagent

# Install dependencies
npm install

# Build the project
npx tsc

# Initialize data files (if not present)
mkdir -p data
echo '{}' > data/memory.json
echo '{}' > data/storage.json
echo '{"memory":{"MemoryCharacterMax":2048,"DirectChildMax":8}}' > data/config.json
```

## Configuration

Configure lkjagent through `data/config.json`:

```json
{
  "memory": {
    "MemoryCharacterMax": 2048,
    "DirectChildMax": 8
  }
}
```

### Configuration Options

- **MemoryCharacterMax**: Maximum characters in working memory (default: 2048)
- **DirectChildMax**: Maximum direct children per memory directory (default: 8)

### Modular Architecture Benefits

The distributed utility design provides several advantages:

#### Development Benefits
- **Clear Separation**: Each module has a single, well-defined responsibility
- **Easy Testing**: Individual utilities can be unit tested in isolation
- **Maintainable Code**: Bugs and features can be isolated to specific modules
- **Type Safety**: Full TypeScript support prevents runtime errors

#### Operational Benefits
- **Reduced Complexity**: Main entry point is minimal and easy to understand
- **Error Isolation**: Failures in one utility don't cascade to others
- **Performance**: Lazy loading and optimized execution paths
- **Debugging**: Clear module boundaries simplify troubleshooting
```

## Modular Utilities

lkjagent features a clean separation of concerns with distributed utility functions:

### Core Utilities (`src/util/`)

#### Agent Loop (`agent-loop.ts`)
The main operational loop that orchestrates the agent's execution cycle:

```typescript
import { runAgent } from './util/agent-loop';

// Starts the continuous agent execution loop
await runAgent();
```

**Features:**
- **Continuous Operation**: Infinite loop for persistent agent execution
- **Error Recovery**: Graceful error handling with detailed logging
- **Modular Integration**: Coordinates all utility components

#### Action Validation (`action-validator.ts`)
Robust validation system for tool actions:

```typescript
import { validateAction, isValidAction } from './util/action-validator';

const errors = validateAction(action);
if (errors.length === 0) {
  // Action is valid, proceed with execution
}
```

**Features:**
- **Pre-execution Validation**: Prevents invalid actions from executing
- **Detailed Error Reporting**: Specific validation error messages
- **Type Safety**: TypeScript-based validation rules

#### Action Execution (`executor.ts`)
Centralized action execution with comprehensive error handling:

```typescript
import { executeAction } from './util/executor';

await executeAction({
  kind: 'memory_set',
  target_path: '/user/task',
  content: 'Task description'
});
```

**Features:**
- **Unified Execution**: Single point for all action processing
- **Automatic Logging**: All actions logged with timestamps and status
- **Error Recovery**: Graceful failure handling with detailed error reporting

#### LLM Communication (`llm.ts`)
Streamlined interface for Large Language Model interaction:

```typescript
import { callLLM } from './util/llm';

const response = await callLLM(systemPrompt);
```

**Features:**
- **LM Studio Integration**: Direct connection to local LLM servers
- **Response Validation**: Ensures XML format compliance
- **Fallback Handling**: Graceful degradation on communication errors

#### System Prompt Generation (`prompt.ts`)
Dynamic system prompt creation with current state:

```typescript
import { generateSystemPrompt } from './util/prompt';

const prompt = await generateSystemPrompt();
```

**Features:**
- **Dynamic State Injection**: Includes current memory and storage state
- **Configuration Integration**: Respects memory limits and constraints
- **XML Format Specification**: Ensures proper LLM response format

#### XML Processing (`xml.ts`)
Robust XML parsing and generation utilities:

```typescript
import { parseActionsFromXml, jsonToXml } from './util/xml';

const actions = parseActionsFromXml(llmResponse);
const xmlString = jsonToXml(dataObject, 'root');
```

**Features:**
- **Bidirectional Conversion**: JSON to XML and XML to action parsing
- **Content Processing**: Handles nested XML structures in action content
- **Error Resilience**: Graceful handling of malformed XML

#### JSON Path Operations (`json.ts`)
Powerful path-based data manipulation:

```typescript
import { getValueAtPath, setValueAtPath, updateObjectAtPath } from './util/json';

const value = getValueAtPath(data, '/user/todo/task1');
setValueAtPath(data, '/new/path', 'value');
updateObjectAtPath(data, '/user/settings', { theme: 'dark' });
```

**Features:**
- **Unix-style Paths**: Intuitive `/path/to/data` notation
- **Safe Operations**: Automatic intermediate object creation
- **Merge Support**: Non-destructive object updates

### Configuration Management (`src/config/`)

#### ConfigManager (`config-manager.ts`)
Singleton configuration management with async loading:

```typescript
const configManager = await ConfigManager.getInstance();
const memoryConfig = configManager.getMemoryConfig();
```

**Features:**
- **Singleton Pattern**: Ensures consistent configuration across the application
- **Async Loading**: Non-blocking configuration initialization
- **Type-safe Access**: Strongly typed configuration interfaces

## Core Components

### 1. Main Entry Point (`src/index.ts`)

Clean and minimal entry point that delegates to utility modules:

```typescript
import { runAgent } from './util/agent-loop';

// Start the agent
if (require.main === module) {
  runAgent().catch(console.error);
}
```

**Features:**
- **Minimal Footprint**: Delegates all functionality to utility modules
- **Error Handling**: Top-level error catching and reporting
- **Module Coordination**: Orchestrates the modular architecture

### 2. Type System (`src/types/common.ts`)

Comprehensive TypeScript interfaces for type safety:

```typescript
interface ToolAction {
  kind: ToolKind;
  target_path?: string;
  source_path?: string;
  content?: any;
}

type ToolKind = 'memory_set' | 'memory_remove' | 'memory_mv' | 
                'storage_get' | 'storage_set' | 'storage_search' | 
                'storage_remove' | 'storage_ls';
```

**Features:**
- **Strong Typing**: Prevents runtime errors through compile-time checks
- **Interface Definitions**: Clear contracts for all data structures
- **Tool Specifications**: Comprehensive action and result type definitions

### 3. Tool System (`src/tool/`)

Individual tool implementations with focused functionality:

#### Memory Operations
- **`memory_set.ts`**: Add/update data in working memory
- **`memory_remove.ts`**: Delete data from working memory  
- **`memory_mv.ts`**: Move data between memory locations

#### Storage Operations
- **`storage_set.ts`**: Move data from memory to persistent storage
- **`storage_get.ts`**: Load data from storage to memory
- **`storage_remove.ts`**: Delete data from persistent storage
- **`storage_search.ts`**: Search storage content by keywords
- **`storage_ls.ts`**: List storage directory contents

#### Action Logging
- **`action_logger.ts`**: Comprehensive operation tracking and audit trail

### Configuration Options

- **MemoryCharacterMax**: Maximum characters in working memory
- **DirectChildMax**: Maximum direct children per memory directory

## Tool System

### Available Actions

| Action | Description | Parameters | Example |
|--------|-------------|------------|---------|
| `memory_set` | Add/update memory data | `target_path`, `content` | Set current task |
| `memory_remove` | Delete memory data | `target_path` | Remove completed task |
| `memory_mv` | Move memory data | `source_path`, `target_path` | Reorganize data |
| `storage_set` | Save to persistent storage | `source_path`, `target_path` | Archive completed work |
| `storage_get` | Load from storage | `target_path` | Retrieve archived data |
| `storage_remove` | Delete from storage | `target_path` | Clean up old data |
| `storage_search` | Search storage content | `content` | Find relevant information |
| `storage_ls` | List storage directory | `target_path` | Browse storage structure |

### Action Execution Flow

1. **Validation**: Action validation using `action-validator.ts`
2. **Execution**: Performed by `executor.ts` with comprehensive error handling
3. **Logging**: Automatic logging via `action_logger.ts`
4. **Error Recovery**: Graceful failure handling with detailed error messages

### Modular Design Benefits

- **Separation of Concerns**: Each utility handles a specific responsibility
- **Maintainability**: Clear module boundaries for easy debugging and updates
- **Testability**: Individual modules can be unit tested in isolation
- **Reusability**: Utility functions can be used across different components
- **Type Safety**: Full TypeScript support with comprehensive interfaces

## Data Management

### Memory Structure (`data/memory.json`)

```json
{
  "user": {
    "todo": {
      "task1": {
        "task_description": "Complete documentation",
        "status": "pending",
        "details": "Write comprehensive README"
      }
    },
    "current_task": {
      "id": "task1",
      "description": "Documentation task",
      "status": "in_progress"
    }
  },
  "sys": {
    "result_data": null
  }
}
```

### Storage Structure (`data/storage.json`)

```json
{
  "knowledge_base": {
    "system_policy_summary": "Agent operational guidelines",
    "greeting_message": "Hello! I'm lkjagent."
  },
  "archived_data": {
    "completed_tasks": {},
    "project_history": {}
  }
}
```

### Action Log Structure (`data/log.json`)

```json
[
  {
    "timestamp": 1672531200000,
    "actionType": "memory_set",
    "target_path": "/user/todo/task1",
    "content": "Task description",
    "status": "success"
  }
]
```

## Usage

### Basic Operation

```bash
# Start the agent
npm start

# Or run directly with Node.js
node dist/index.js
```

### Integration with LM Studio

1. **Start LM Studio** with your preferred small LLM
2. **Configure API endpoint** in the agent code
3. **Run lkjagent** - it will automatically connect and begin operation

### Example Workflow

1. Agent receives user input
2. Generates system prompt with current memory state
3. Sends prompt to LLM via API
4. Parses XML response into actions
5. Executes each action sequentially
6. Logs all operations
7. Repeats the cycle

## Development

### Building

```bash
# Development build with watch
npx tsc --watch

# Production build
npx tsc
```

### Testing

```bash
# Run tests (when available)
npm test

# Type checking
npx tsc --noEmit
```

### Code Structure

The project follows a modular TypeScript architecture with distributed utilities:

- **`src/index.ts`**: Minimal entry point that delegates to utility modules
- **`src/util/`**: Core utility functions organized by functionality
  - **`agent-loop.ts`**: Main agent execution loop
  - **`action-validator.ts`**: Action validation logic
  - **`executor.ts`**: Centralized action execution
  - **`llm.ts`**: LLM communication interface
  - **`prompt.ts`**: System prompt generation
  - **`xml.ts`**: XML processing utilities
  - **`json.ts`**: JSON path manipulation utilities
- **`src/types/`**: TypeScript interfaces and type definitions
- **`src/config/`**: Configuration management system
- **`src/tool/`**: Individual tool implementations with focused functionality
- **`data/`**: Runtime data files (JSON)

## Project Structure

```
lkjagent/
├── package.json              # Node.js dependencies and scripts
├── tsconfig.json             # TypeScript configuration
├── README.md                 # This comprehensive documentation
├── data/                     # Runtime data directory
│   ├── config.json          # Agent configuration
│   ├── memory.json          # Working memory state
│   ├── storage.json         # Persistent storage
│   └── log.json             # Action execution log
├── docs/                     # Additional documentation
│   └── readme_jp.md         # Japanese documentation
├── example/                  # Example configurations and use cases
│   ├── fantasy_story/       # Fantasy story writing example
│   └── longlong_story/      # Long-form narrative example
└── src/                      # Modular TypeScript source code
    ├── index.ts             # Minimal entry point
    ├── types/               # Type definitions and interfaces
    │   └── common.ts        # Core interfaces and type definitions
    ├── config/              # Configuration management
    │   └── config-manager.ts # Singleton configuration manager
    ├── util/                # Distributed utility functions
    │   ├── agent-loop.ts    # Main agent execution loop
    │   ├── action-validator.ts # Action validation utilities
    │   ├── executor.ts      # Centralized action execution
    │   ├── llm.ts          # LLM communication interface
    │   ├── prompt.ts       # System prompt generation
    │   ├── xml.ts          # XML processing utilities
    │   └── json.ts         # JSON path manipulation
    └── tool/                # Individual tool implementations
        ├── action_logger.ts # Action logging and audit trail
        ├── memory_set.ts    # Memory write operations
        ├── memory_remove.ts # Memory deletion operations
        ├── memory_mv.ts     # Memory move operations
        ├── storage_get.ts   # Storage read operations
        ├── storage_set.ts   # Storage write operations
        ├── storage_remove.ts # Storage deletion operations
        ├── storage_search.ts # Storage search functionality
        └── storage_ls.ts    # Storage directory listing
```

## Examples

### Task Management

```xml
<!-- Add a new task -->
<actions>
  <action>
    <kind>memory_set</kind>
    <target_path>/user/todo/write_docs</target_path>
    <content>{"task_description": "Write comprehensive documentation", "status": "pending"}</content>
  </action>
</actions>

<!-- Archive completed task -->
<actions>
  <action>
    <kind>storage_set</kind>
    <source_path>/user/todo/write_docs</source_path>
    <target_path>/archived_data/completed_tasks/write_docs</target_path>
  </action>
  <action>
    <kind>memory_remove</kind>
    <target_path>/user/todo/write_docs</target_path>
  </action>
</actions>
```

### Knowledge Management

```xml
<!-- Search for relevant information -->
<actions>
  <action>
    <kind>storage_search</kind>
    <content>documentation best practices</content>
  </action>
</actions>

<!-- Load specific knowledge -->
<actions>
  <action>
    <kind>storage_get</kind>
    <target_path>/knowledge_base/writing_guidelines</target_path>
  </action>
</actions>
```

## API Reference

### Core Utility Functions

#### Agent Loop (`util/agent-loop.ts`)
```typescript
async function runAgent(): Promise<void>
```
Main agent execution loop that coordinates all system components.

#### Action Validation (`util/action-validator.ts`)
```typescript
function validateAction(action: ToolAction): string[]
function isValidAction(action: ToolAction): boolean
```
Validates tool actions before execution, returning validation errors or boolean result.

#### Action Execution (`util/executor.ts`)
```typescript
async function executeAction(action: ToolAction): Promise<void>
```
Centralized action execution with automatic validation, logging, and error handling.

#### LLM Communication (`util/llm.ts`)
```typescript
async function callLLM(prompt: string): Promise<string>
```
Interface for communicating with Large Language Models via LM Studio API.

#### System Prompts (`util/prompt.ts`)
```typescript
async function generateSystemPrompt(): Promise<string>
```
Generates dynamic system prompts including current memory and storage state.

#### XML Processing (`util/xml.ts`)
```typescript
function parseActionsFromXml(xml: string): ToolAction[]
function jsonToXml(obj: any, rootTag: string, indent?: string): string
```
Bidirectional XML processing for LLM communication and data conversion.

#### JSON Path Operations (`util/json.ts`)
```typescript
function getValueAtPath(obj: any, path: string): any
function setValueAtPath(obj: any, path: string, value: any): void
function updateObjectAtPath(obj: any, path: string, updates: any): void
```
Unix-style path operations for nested data manipulation.

### Core Interfaces

#### ToolAction
```typescript
interface ToolAction {
  kind: ToolKind;
  target_path?: string;
  source_path?: string;
  content?: any;
}
```

#### ToolKind
```typescript
type ToolKind = 'memory_set' | 'memory_remove' | 'memory_mv' | 
                'storage_get' | 'storage_set' | 'storage_search' | 
                'storage_remove' | 'storage_ls';
```

#### LogEntry
```typescript
interface LogEntry {
  timestamp: number;
  actionType: ToolKind;
  target_path?: string;
  source_path?: string;
  content?: any;
  status: 'success' | 'error';
  error?: string;
}
```

### Configuration Interface

#### MemoryConfig
```typescript
interface MemoryConfig {
  MemoryCharacterMax: number;
  DirectChildMax: number;
}
```

## Contributing

1. **Fork the repository**
2. **Create a feature branch**: `git checkout -b feature/amazing-feature`
3. **Commit changes**: `git commit -m 'Add amazing feature'`
4. **Push to branch**: `git push origin feature/amazing-feature`
5. **Open a Pull Request**

### Development Guidelines

- Follow TypeScript best practices and maintain strict type safety
- Add comprehensive JSDoc comments for all public functions
- Include error handling for all operations with specific error messages
- Write unit tests for new utility functions and tools
- Update documentation for API changes and new features
- Maintain modular architecture principles:
  - Keep utilities focused on single responsibilities
  - Avoid cross-dependencies between utility modules
  - Use dependency injection where appropriate
  - Follow consistent naming conventions across modules

### Adding New Utilities

When adding new utility functions:

1. **Create focused modules** in `src/util/` with single responsibilities
2. **Export clean interfaces** with comprehensive TypeScript types
3. **Include comprehensive error handling** with descriptive messages
4. **Add JSDoc documentation** for all public functions
5. **Update the main README** to document new functionality
6. **Write unit tests** to ensure reliability

### Adding New Tools

When implementing new tools:

1. **Create tool files** in `src/tool/` following existing patterns
2. **Update ToolKind type** in `src/types/common.ts`
3. **Add validation rules** in `src/util/action-validator.ts`
4. **Include execution logic** in `src/util/executor.ts`
5. **Test thoroughly** with various input scenarios

## License

This project is licensed under the ISC License - see the LICENSE file for details.

---

**lkjagent** - Empowering small LLMs with persistent memory, structured reasoning capabilities, and a clean modular architecture for maintainable AI agent development.