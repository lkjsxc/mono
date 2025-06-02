# 🤖 lkjagent

[![Build Status](https://img.shields.io/your_ci_badge_url)](https://your_ci_link)
[![npm version](https://img.shields.io/npm/v/lkjagent.svg?style=flat)](https://www.npmjs.com/package/lkjagent)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](http://makeapullrequest.com)

**lkjagent** is a sophisticated AI Agent framework specifically designed for small Language Learning Models (LLMs). It solves the critical challenge of memory limitations in smaller models by implementing a dual-memory architecture that enables complex, persistent, and long-running task execution.

This framework empowers developers to build advanced AI agents that can handle tasks requiring long-term context and data retention, even when using LLMs with constrained memory capacities.

## 🌟 Key Features

-   **🧠 Dual Memory Architecture**: Finite working memory (RAM) for immediate context + infinite persistent storage for long-term data.
-   **📡 XML-Based Communication**: Structured action protocol for reliable and verifiable LLM interaction.
-   **⚡ Modular TypeScript Design**: Clean, maintainable, and scalable architecture built with TypeScript.
-   **🔄 Persistent Task Management**: Enables long-running tasks with state preservation across sessions.
-   **📊 Comprehensive Action Logging**: Full audit trail of all agent operations, with results stored in `/sys/result_data/action_N/`.
-   **🛡️ Robust Error Handling**: Graceful failure recovery with detailed error reporting for enhanced stability.
-   **🎯 Action Numbering System**: Sequential tracking of all operations, facilitating debugging and state recovery.
-   **🔍 Advanced XML Processing**: Safe XML parsing with circular reference detection, validation, and content escaping.

## 📋 Table of Contents

-   [🌟 Key Features](#-key-features)
-   [📋 Table of Contents](#-table-of-contents)
-   [🚀 Quick Start](#-quick-start)
-   [🏗️ Architecture Overview](#️-architecture-overview)
    -   [🧠 Memory System](#-memory-system)
    -   [📡 Communication Protocol](#-communication-protocol)
-   [🔧 Installation](#-installation)
    -   [📦 Prerequisites](#-prerequisites)
    -   [🛠️ Installation Steps](#️-installation-steps)
    -   [📁 Data Directory Setup](#-data-directory-setup)
-   [⚙️ Configuration](#️-configuration)
    -   [🔧 Configuration Options](#-configuration-options)
    -   [🎯 Action Numbering System Details](#-action-numbering-system-details)
-   [🧩 Modular Utilities & Core Concepts](#-modular-utilities--core-concepts)
    -   [🏗️ Architectural Benefits](#️-architectural-benefits)
    -   [🔧 Core Utility Modules (`src/util/`)](#-core-utility-modules-srcutil)
-   [🛠️ Tool System](#️-tool-system)
    -   [Available Actions](#available-actions)
    -   [Action Execution Flow](#action-execution-flow)
-   [📊 Data Management](#-data-management)
    -   [Memory Structure (`data/memory.json`)](#memory-structure-datamemoryjson)
    -   [Storage Structure (`data/storage.json`)](#storage-structure-datastoragejson)
    -   [Action Log Structure (`data/log.json`)](#action-log-structure-datalogjson)
-   [💻 Usage](#-usage)
    -   [Basic Operation](#basic-operation)
    -   [Integration with LM Studio (or similar)](#integration-with-lm-studio-or-similar)
    -   [Example Workflow](#example-workflow)
-   [🎮 Example Use Cases](#-example-use-cases)
-   [📁 Project Structure](#-project-structure)
-   [🔄 Development Workflow](#-development-workflow)
    -   [Building](#building)
    -   [Testing](#testing)
    -   [Linting and Formatting](#linting-and-formatting)
-   [📚 API Reference (Core Components)](#-api-reference-core-components)
    -   [Core Utility Functions](#core-utility-functions)
    -   [Core Interfaces](#core-interfaces)
    -   [Configuration Interface](#configuration-interface)
-   [🤝 Contributing](#-contributing)
    -   [Development Guidelines](#development-guidelines)
    -   [Adding New Utilities](#adding-new-utilities)
    -   [Adding New Tools](#adding-new-tools)
-   [🛣️ Roadmap (Future Enhancements)](#️-roadmap-future-enhancements)
-   [📄 License](#-license)
-   [🙏 Acknowledgements](#-acknowledgements)

## 🚀 Quick Start

Get lkjagent up and running in a few simple steps:

```bash
# 1. Clone the repository
git clone https://github.com/your-username/lkjagent.git # Replace with actual URL
cd lkjagent

# 2. Install dependencies
npm install

# 3. Build the project
npm run build
# Or for development with watching (if tsc --watch is configured in package.json)
# npm run dev
# Or manually with tsc
# npx tsc

# 4. Initialize data files (creates default JSON files in data/)
npm run init-data

# 5. Configure your LLM (edit data/config.json, see Configuration section)
# Ensure your LLM server (e.g., LM Studio, Ollama) is running.

# 6. Start the agent
npm start
# Or: node dist/index.js
```

## 🏗️ Architecture Overview

lkjagent is built around a dual-memory system and a structured XML communication protocol to effectively manage tasks and interact with LLMs.

### 🧠 Memory System

The dual-memory architecture is inspired by human cognitive systems, designed to overcome the memory limitations of smaller LLMs:

```mermaid
graph TB
    A[LLM] -->|Requests Actions| B(XML Actions)
    B -->|Validates| C(Action Validator)
    C -->|Executes| D(Executor)
    D -->|Operates on| E[Working Memory (RAM)]
    D -->|Operates on| F[Persistent Storage (Disk)]
    E -->|Provides Context| G(JSON Path Operations)
    F -->|Provides Context| G
    G -->|Logs Actions/Results| H(Action Logger)
    H -->|Creates| I[Audit Trail (in /sys/result_data & log.json)]
```

#### 🐏 Working Memory (RAM - `data/memory.json`)
-   **Finite capacity** (configurable, e.g., 2048 characters by default).
-   **Fast access** for current task context, immediate operations, and temporary data.
-   **Structured data** typically as JSON, with potential for predefined schemas and type safety.
-   **Automatic cleanup** mechanisms can be implemented when limits are reached to maintain performance.
-   Holds the **real-time state** for active tasks and system operations.

#### 💾 Persistent Storage (Disk - `data/storage.json`)
-   **Effectively infinite capacity** for long-term data retention, knowledge bases, and archival.
-   **Hierarchical organization** using Unix-style path-based access (e.g., `/path/to/data`).
-   **Advanced search capabilities** across all stored content (e.g., keyword matching).
-   Potential for **versioning system** for tracking data evolution over time.
-   Ensures **cross-session persistence**, maintaining state between agent restarts.

### 📡 Communication Protocol

The agent uses a robust XML-based protocol for structured LLM communication, ensuring clarity and verifiability of actions:

**Example LLM Action Request:**
```xml
<actions>
  <action>
    <kind>memory_set</kind>
    <target_path>/user/todo/new_task</target_path>
    <content>{"task_description": "Complete project documentation", "status": "pending", "priority": "high"}</content>
  </action>
  <action>
    <kind>storage_ls</kind>
    <target_path>/archived_data</target_path>
  </action>
  <action>
    <kind>storage_search</kind>
    <content>project documentation</content> <!-- Content is the search query here -->
  </action>
</actions>
```

**XML Protocol Features:**
-   **Schema Validation**: Ensures proper action structure and prevents malformed requests from the LLM.
-   **Content Escaping**: Safe handling of special characters and nested data within XML content.
-   **Error Recovery**: Graceful parsing with detailed error messages for debugging.
-   **Circular Reference Detection**: Prevents infinite loops in complex data structures during XML generation/parsing.

## 🔧 Installation

### 📦 Prerequisites

-   **Node.js**: Version 16+ (LTS recommended).
-   **npm** (usually comes with Node.js) or **yarn**.
-   **TypeScript**: Version 4.5+ (for development).
-   **LLM Server**:
    -   LM Studio, Ollama, or any OpenAI API-compatible local LLM server.
    -   Ensure the server is running and accessible.
-   **RAM**: 4GB+ recommended for optimal performance, especially when running LLMs locally.

### 🛠️ Installation Steps

```bash
# 1. Clone the repository
git clone https://github.com/your-username/lkjagent.git # Replace with actual URL
cd lkjagent

# 2. Install dependencies
npm install

# 3. Build the TypeScript project
npm run build
# Alternative for manual build:
# npx tsc

# 4. Initialize data files (creates default JSON files in data/)
npm run init-data
# This script ensures data/config.json, data/memory.json, data/storage.json, and data/log.json exist.
```

### 📁 Data Directory Setup

After running `npm run init-data` (or manual setup), the `data/` directory will be initialized:

```
data/
├── config.json      # Agent configuration (LLM endpoint, memory limits, etc.)
├── memory.json      # Working memory state (volatile, for current tasks)
├── storage.json     # Persistent storage (long-term knowledge, archives)
└── log.json         # Action execution log (history of operations)
```

**Manual Setup (if `npm run init-data` is unavailable or for custom initialization):**

```bash
# Create data directory
mkdir -p data

# Create default configuration file (data/config.json)
echo '{
  "memory": {
    "MemoryCharacterMax": 2048,
    "DirectChildMax": 8
  },
  "llm": {
    "apiUrl": "http://localhost:1234/v1/chat/completions",
    "model": "your-local-model-name", # IMPORTANT: Change this to your model
    "maxTokens": 1000,
    "temperature": 0.7
  },
  "system": {
    "maxLogEntries": 1000,
    "autoCleanup": true,
    "debugMode": false
  }
}' > data/config.json

# Create empty data files
echo '{"user":{"todo":{}},"sys":{"result_data":{}}}' > data/memory.json
echo '{"knowledge_base":{"system_policy_summary":"","greeting_message":""},"archived_data":{}}' > data/storage.json
echo '[]' > data/log.json
```
**Note:** Ensure `sys.result_data` exists in `memory.json` as it's used for storing action results.

## ⚙️ Configuration

Configure `lkjagent` via `data/config.json`.

### 🔧 Configuration Options

```json
{
  "memory": {
    "MemoryCharacterMax": 4096,    // Max characters in working memory (default: 2048)
    "DirectChildMax": 12           // Max direct children per memory/storage directory node (default: 8)
  },
  "llm": {
    "apiUrl": "http://localhost:1234/v1/chat/completions", // LM Studio/Ollama API endpoint
    "model": "llama-3.2-3b-instruct", // Specific model name to use for inference
    "maxTokens": 1000,             // Maximum tokens per LLM response
    "temperature": 0.7             // LLM creativity/randomness (0.0-1.0)
  },
  "system": {
    "maxLogEntries": 1000,         // Maximum entries in data/log.json before potential cleanup
    "autoCleanup": true,           // Enable automatic cleanup of old data/logs (future feature)
    "debugMode": false             // Enable verbose logging for debugging purposes
  }
}
```

#### Memory Configuration (`memory`)
-   **`MemoryCharacterMax`**: Maximum total characters allowed in the working memory (`memory.json`). This helps constrain the context provided to the LLM.
-   **`DirectChildMax`**: Maximum number of direct children a JSON object node can have in memory or storage. This helps prevent overly wide structures that might be hard for the LLM to parse.

#### LLM Configuration (`llm`)
-   **`apiUrl`**: The API endpoint for your LLM server (e.g., LM Studio, Ollama).
-   **`model`**: The identifier for the LLM model you want the agent to use.
-   **`maxTokens`**: The maximum number of tokens the LLM should generate in its response.
-   **`temperature`**: Controls the randomness of the LLM's output. Lower values (e.g., 0.2) make it more deterministic; higher values (e.g., 0.8) make it more creative.

#### System Configuration (`system`)
-   **`maxLogEntries`**: Maximum number of entries to keep in `data/log.json`.
-   **`autoCleanup`**: Boolean to enable/disable automatic cleanup processes (e.g., old logs, oversized memory sections).
-   **`debugMode`**: Boolean to enable more verbose logging to the console for development and troubleshooting.

### 🎯 Action Numbering System Details

`lkjagent` features an advanced action numbering system that tracks all operations sequentially. The results and status of each action requested by the LLM are stored in working memory under `/sys/result_data/action_N/`.

**Example structure in `data/memory.json` after actions:**
```json
{
  "user": { /* ... user data ... */ },
  "sys": {
    "result_data": {
      "action_1": {
        "action_number": 1,
        "timestamp": 1672531200000,
        "kind": "storage_ls",          // Renamed from 'action' for clarity
        "target_path": "/",
        "status": "success",
        "data": { "knowledge_base": {}, "archived_data": {} } // Example result
      },
      "action_2": {
        "action_number": 2,
        "timestamp": 1672531201000,
        "kind": "memory_set",
        "target_path": "/user/current_task",
        "status": "success",
        "message": "Value set successfully at /user/current_task"
      },
      "action_3": {
        "action_number": 3,
        "timestamp": 1672531202000,
        "kind": "memory_get",
        "target_path": "/non/existent/path",
        "status": "error",
        "error": "Path /non/existent/path not found."
      }
    }
  }
}
```

**Benefits:**
-   **Sequential Tracking**: Every LLM-requested action gets a unique sequential number.
-   **Result & Status Feedback**: The LLM receives feedback on the outcome of its requested actions in the next turn via the system prompt (which includes `/sys/result_data`).
-   **Error Isolation**: Failed actions are logged with detailed error information, allowing the LLM to potentially retry or adjust its strategy.
-   **Debugging Support**: Easy to trace the execution flow and identify issues by examining `sys.result_data`.
-   **State Recovery**: In more advanced scenarios, this log could be used to replay or recover from specific action points.

## 🧩 Modular Utilities & Core Concepts

`lkjagent` features a clean, modular architecture with distributed utility functions, promoting maintainability, testability, and flexibility.

### 🏗️ Architectural Benefits

#### Developer Experience
-   **Clear Separation of Concerns**: Each module has a single, well-defined responsibility (e.g., XML parsing, action execution).
-   **Easy Testing**: Individual utilities can be unit-tested in isolation.
-   **Maintainable Code**: Bugs and new features can often be isolated to specific modules, reducing ripple effects.
-   **Type Safety**: Full TypeScript support helps catch errors at compile-time and improves code understanding.
-   **Hot Reload Potential**: In some development setups, individual modules can be updated without a full system restart.

#### Operational Stability & Performance
-   **Reduced Complexity**: The main entry point (`index.ts`) is minimal, delegating tasks to specialized utilities.
-   **Error Isolation**: Failures in one utility are less likely to cascade and crash the entire system if handled properly.
-   **Performance**: Allows for optimized execution paths and potential for lazy loading of components.
-   **Debugging**: Clear module boundaries simplify troubleshooting by narrowing down the search space for issues.
-   **Scalability**: Easier to add new utilities or tools without significantly refactoring existing code.

### 🔧 Core Utility Modules (`src/util/`)

#### 🔄 Agent Loop (`agent-loop.ts`)
The heart of `lkjagent`, orchestrating the continuous execution cycle:
```typescript
// Example usage from index.ts
import { runAgent } from './util/agent-loop';
runAgent().catch(console.error);
```
**Key Features:**
-   **Continuous Operation**: Manages the primary loop for persistent agent execution (listen, think, act).
-   **Error Recovery**: Implements top-level error handling to gracefully manage unexpected issues and attempt continuation.
-   **Iteration Tracking**: Can include sequential numbering of iterations and logging for operational insight.
-   **Modular Integration**: Coordinates calls to LLM, action parsing, execution, and state updates.

#### ✅ Action Validation (`action-validator.ts`)
A comprehensive validation system ensuring the integrity and safety of actions proposed by the LLM:
```typescript
import { validateAction } from './util/action-validator';
// const action = { kind: 'memory_set', target_path: '/user/task', content: 'data' };
// const errors = validateAction(action);
// if (errors.length === 0) { /* execute */ } else { /* handle errors */ }
```
**Validation Features:**
-   **Pre-execution Checks**: Prevents invalid or malformed actions from being executed.
-   **Detailed Error Reporting**: Provides specific error messages with context for why an action is invalid.
-   **Type Safety**: Leverages TypeScript types for defining valid action structures.
-   **Security Considerations**: Can include checks against path traversal, unauthorized access patterns, etc.

#### ⚙️ Action Execution (`executor.ts`)
The centralized engine for carrying out validated actions:
```typescript
import { executeActions } from './util/executor';
// const actionsToExecute = [ /* array of validated ToolAction objects */ ];
// await executeActions(actionsToExecute); // Results are stored in /sys/result_data
```
**Execution Features:**
-   **Unified Execution Logic**: A single point for processing all types of `ToolAction`.
-   **Automatic Logging**: Integrates with `action_logger.ts` to record executed actions and their outcomes.
-   **Result Management**: Populates `/sys/result_data/action_N/` with the outcome of each action.
-   **Error Handling**: Gracefully handles errors during action execution and records them in `result_data`.

#### 🤖 LLM Communication (`llm.ts`)
A streamlined interface for interacting with the Large Language Model:
```typescript
import { callLLM } from './util/llm';
// const systemPrompt = await generateSystemPrompt();
// const llmResponseXml = await callLLM(systemPrompt);
```
**Communication Features:**
-   **API Integration**: Handles requests to LLM APIs (e.g., LM Studio, Ollama, OpenAI-compatible).
-   **Request Formatting**: Prepares the prompt and other parameters for the API call.
-   **Response Parsing**: Extracts the relevant content (expected to be XML) from the LLM's response.
-   **Error Handling & Retries**: Manages API errors, timeouts, and can implement retry logic.

#### 📝 System Prompt Generation (`prompt.ts`)
Dynamically creates the system prompt sent to the LLM, including relevant context:
```typescript
import { generateSystemPrompt }-> {await generateSystemPrompt(); // Example usage
const systemPrompt = await generateSystemPrompt();
// This prompt will include current memory, storage snippets, tool definitions, etc.
```
**Prompt Features:**
-   **Dynamic State Injection**: Incorporates crucial parts of the current working memory and persistent storage.
-   **Tool Definition Inclusion**: Provides the LLM with a list of available tools and their usage (XML format).
-   **Context Optimization**: Balances providing sufficient information with respecting token limits of the LLM and `MemoryCharacterMax`.
-   **Goal/Instruction Setting**: Includes the overarching goal or task for the LLM.

#### 🔄 XML Processing (`xml.ts`)
Advanced utilities for parsing XML responses from the LLM and generating XML if needed:
```typescript
import { parseActionsFromXml, jsonToXml, XmlError } from './util/xml';
// const actionsArray = parseActionsFromXml(llmResponseXmlString);
// const xmlRepresentation = jsonToXml({ data: "example" }, "rootTag");
```
**XML Features:**
-   **Robust Parsing**: Converts XML strings from the LLM into structured `ToolAction` objects.
-   **JSON to XML Conversion**: Utility for converting JSON objects to XML strings (e.g., for LLM examples or internal use).
-   **Error Resilience**: Gracefully handles malformed XML, providing detailed error context.
-   **Content Processing**: Safely handles nested XML structures or stringified JSON within action content.
-   **Circular Reference Detection**: Prevents infinite loops when processing complex or self-referential data.

#### 📁 JSON Path Operations (`json.ts`)
Powerful Unix-style path operations for navigating and manipulating data within `memory.json` and `storage.json`:
```typescript
import { getValueAtPath, setValueAtPath, updateObjectAtPath, deletePath, validatePath } from './util/json';
// const userData = getValueAtPath(memoryState, '/user/profile');
// setValueAtPath(storageState, '/archived_projects/project_alpha', { status: 'complete' });
```
**Path Features:**
-   **Intuitive Navigation**: Uses `/path/to/data` notation for easy access to nested JSON data.
-   **Safe CRUD Operations**:
    -   `getValueAtPath`: Retrieves a value.
    -   `setValueAtPath`: Sets or creates a value, creating intermediate paths if needed.
    -   `updateObjectAtPath`: Merges an object at a path (non-destructive for existing keys at other levels).
    -   `deletePath`: Removes a key or an entire object at a path.
-   **Path Validation**: Ensures path strings are well-formed.
-   **Performance**: Optimized for frequent operations on potentially large JSON structures.

## 🛠️ Tool System

The Tool System defines the set of operations the LLM can request the agent to perform. These actions are specified in XML and executed by the agent.

### Available Actions

The LLM can request the agent to perform actions by generating XML. Each `<action>` tag must contain a `<kind>` and other relevant parameters.

| Action Kind      | Description                                     | Parameters (XML tags)                 | Example LLM Request Fragment                 |
| ---------------- | ----------------------------------------------- | ------------------------------------- | -------------------------------------------- |
| `memory_set`     | Add or update data in working memory.         | `target_path`, `content`              | `<kind>memory_set</kind><target_path>/user/task</target_path><content>New task details</content>` |
| `memory_remove`  | Delete data from working memory.              | `target_path`                         | `<kind>memory_remove</kind><target_path>/user/old_task</target_path>` |
| `memory_mv`      | Move data within working memory.                | `source_path`, `target_path`          | `<kind>memory_mv</kind><source_path>/temp/data</source_path><target_path>/user/data</target_path>` |
| `storage_set`    | Save data from memory to persistent storage.    | `source_path` (from memory), `target_path` (in storage) | `<kind>storage_set</kind><source_path>/user/completed_task</source_path><target_path>/archive/task1</target_path>` |
| `storage_get`    | Load data from persistent storage to memory.    | `source_path` (from storage), `target_path` (in memory) | `<kind>storage_get</kind><source_path>/archive/task1</source_path><target_path>/user/retrieved_task</target_path>` |
| `storage_remove` | Delete data from persistent storage.            | `target_path`                         | `<kind>storage_remove</kind><target_path>/archive/old_data</target_path>` |
| `storage_search` | Search persistent storage content by keywords.  | `content` (search query), `target_path` (optional, where to store results in memory, defaults to `/sys/search_results`) | `<kind>storage_search</kind><content>project documentation</content>` |
| `storage_ls`     | List directory contents in persistent storage.  | `target_path` (directory in storage), `result_path` (optional, where to store list in memory, defaults to `/sys/ls_results`) | `<kind>storage_ls</kind><target_path>/documents</target_path>` |

*Note: The actual implementation for storing results of `storage_search` and `storage_ls` in memory might vary. The above table suggests a convention.*

### Action Execution Flow

1.  **LLM Generates XML**: The LLM, guided by the system prompt, outputs an XML string containing one or more `<action>` elements.
2.  **XML Parsing**: The `xml.ts` utility parses this string into an array of `ToolAction` objects.
3.  **Validation**: Each `ToolAction` is validated by `action-validator.ts` for correct structure, valid paths, and adherence to constraints. Invalid actions are flagged.
4.  **Execution**: Valid actions are passed to `executor.ts`. Each action is performed by its corresponding tool implementation (e.g., `memory_set.ts` handles `memory_set` kind).
5.  **Result Recording**: The outcome (success or error, along with any data) of each action is recorded in working memory at `/sys/result_data/action_N/`.
6.  **Logging**: `action_logger.ts` logs the action and its outcome to `data/log.json` for auditing.
7.  **Feedback Loop**: In the next cycle, `prompt.ts` includes `/sys/result_data` in the system prompt, informing the LLM about the results of its previous requests.

## 📊 Data Management

lkjagent uses JSON files for managing its state and logs.

### Memory Structure (`data/memory.json`)

Holds the agent's current working context. It's volatile and expected to change frequently.
The `MemoryCharacterMax` config limits its total size.

```json
{
  "user": {
    "current_focus": "Writing a fantasy story about a dragon.",
    "todo": {
      "task1": {
        "description": "Outline chapter 1",
        "status": "pending"
      }
    },
    "scratchpad": "The dragon's name is Ignis."
  },
  "sys": {
    "iteration_count": 5,
    "last_error": null,
    "result_data": { // Populated by the executor after actions
      "action_1": {
        "action_number": 1,
        "timestamp": 1672531200000,
        "kind": "memory_set",
        "target_path": "/user/scratchpad",
        "status": "success",
        "message": "Value set successfully."
      }
    }
  }
}
```

### Storage Structure (`data/storage.json`)

Used for long-term data, knowledge bases, and archives. Data here persists across sessions.

```json
{
  "knowledge_base": {
    "common_greetings": ["Hello!", "Hi there!", "Greetings!"],
    "system_policy_summary": "Agent operational guidelines and core directives...",
    "project_alpha_details": {
      "client": "ACME Corp",
      "deadline": "2024-12-31",
      "notes": "Requires weekly updates."
    }
  },
  "archived_data": {
    "completed_stories": {
      "story_abc_summary": "A tale of a brave knight..."
    },
    "chat_history_2023_01": [ /* ... logs ... */ ]
  }
}
```

### Action Log Structure (`data/log.json`)

A persistent audit trail of all actions executed by the agent. Limited by `maxLogEntries`.

```json
[
  {
    "timestamp": 1672531200000,
    "action_number_in_batch": 1, // If multiple actions are in one LLM response
    "overall_action_sequence": 101, // A global counter if needed
    "kind": "memory_set",
    "target_path": "/user/todo/task1",
    "content_summary": "{\"description\":\"Outline chapter 1\",\"status\":\"pending\"}", // Could be a summary or hash
    "status": "success",
    "message": "Value set successfully."
  },
  {
    "timestamp": 1672531201500,
    "action_number_in_batch": 1,
    "overall_action_sequence": 102,
    "kind": "storage_search",
    "content_summary": "dragon lore",
    "status": "success",
    "result_summary": "Found 3 relevant documents.", // Summary of data placed in result_path
    "error": null
  }
]
```
*(Note: The exact fields in `log.json` can be tailored to specific auditing needs.)*

## 💻 Usage

### Basic Operation

Once installed and configured:

```bash
# Start the agent
npm start

# Alternatively, run the compiled JavaScript directly
node dist/index.js
```
The agent will then start its main loop:
1.  Generate a system prompt.
2.  Call the configured LLM.
3.  Parse XML actions from the LLM response.
4.  Validate and execute actions.
5.  Log results.
6.  Repeat.

### Integration with LM Studio (or similar)

1.  **Launch your LLM Server**: Start LM Studio, Ollama, or your preferred local LLM server.
2.  **Load a Model**: Ensure a compatible model is loaded and the server is serving requests (e.g., on `http://localhost:1234`).
3.  **Configure `lkjagent`**: Update `data/config.json` with the correct `apiUrl` and `model` name:
    ```json
    // data/config.json
    {
      "llm": {
        "apiUrl": "http://localhost:1234/v1/chat/completions", // Ensure this matches your LM Studio server
        "model": "Publisher/ModelName-GGUF",                  // Ensure this matches a loaded model in LM Studio
        // ... other llm settings
      },
      // ... other settings
    }
    ```
4.  **Run `lkjagent`**: `npm start`. The agent will connect to your local LLM.

### Example Workflow

1.  **Agent Starts**: `lkjagent` initializes, loads configuration, memory, and storage.
2.  **System Prompt Generation**: `prompt.ts` creates a system prompt including:
    *   Current relevant `memory.json` data (respecting `MemoryCharacterMax`).
    *   Snippets from `storage.json` (e.g., relevant knowledge).
    *   List of available tools/actions and their XML format.
    *   The current goal or task.
    *   Results from previous actions (`/sys/result_data`).
3.  **LLM Interaction**: `llm.ts` sends this prompt to the LLM via the configured API.
4.  **LLM Responds**: The LLM processes the prompt and returns a response, ideally containing XML-formatted actions.
5.  **Action Processing**:
    *   `xml.ts` parses the XML into `ToolAction` objects.
    *   `action-validator.ts` validates each action.
    *   `executor.ts` executes valid actions, updating `memory.json` or `storage.json`.
    *   Results (success/failure, data) are stored in `memory.json` under `/sys/result_data/action_N/`.
6.  **Logging**: `action_logger.ts` writes details of the executed action to `data/log.json`.
7.  **Loop**: The process repeats, allowing for continuous, stateful interaction.

## 🎮 Example Use Cases

`lkjagent` is designed for tasks where context persistence and structured operations are key, especially with smaller LLMs.

-   **📝 Long-Form Content Generation**:
    -   **Fantasy Story Writing** (`example/fantasy_story/`): The agent can maintain plot points, character details, and world-building information across multiple chapters or sessions. The LLM can request to save drafts to storage, retrieve character backstories, or update plot outlines in memory.
    -   **Technical Documentation**: Generate and maintain documentation, storing different sections and versions.
-   **📊 Data Analysis & Reporting (Conceptual)**:
    -   The LLM could direct the agent to fetch data snippets (simulated via storage), perform transformations (in memory), and then compile a report.
-   **🤖 Task Management & Planning**:
    -   Manage a to-do list, break down complex tasks, and store progress persistently. The LLM can add, remove, or update tasks in `/user/todo/`.
-   **📚 Knowledge Base Interaction**:
    -   Build and query a personal or project-specific knowledge base stored in `storage.json`. The LLM can use `storage_search` to find information or `storage_set` to add new knowledge.
-   ** simulated Web Browsing/Research (Future Tool)**:
    -   With a `web_fetch` tool, the LLM could direct the agent to "visit" a URL, summarize content, and store findings.

The `example/` directory in the project contains sample configurations or scenarios demonstrating these use cases:
-   `example/fantasy_story/`: Contains prompts or initial `memory.json` / `storage.json` setups for a story writing task.
-   `example/longlong_story/`: Similar to above, but perhaps focused on even longer narratives requiring more strategic use of storage.

*(This section can be expanded with more concrete examples as the project evolves.)*

## 📁 Project Structure

```
lkjagent/
├── data/                     # Runtime data directory (gitignored by default)
│   ├── config.json           # Agent configuration (LLM endpoint, memory limits)
│   ├── memory.json           # Working memory state (volatile, current context)
│   ├── storage.json          # Persistent storage (long-term knowledge, archives)
│   └── log.json              # Action execution log (audit trail)
├── docs/                     # Additional documentation (e.g., design docs, advanced guides)
│   └── readme_jp.md          # Example: Japanese documentation
├── example/                  # Example configurations, prompts, and use case setups
│   ├── fantasy_story/        # Files for the fantasy story writing example
│   └── longlong_story/       # Files for a long-form narrative example
├── src/                      # TypeScript source code
│   ├── index.ts              # Main entry point of the application
│   ├── types/                # TypeScript interfaces, enums, and type definitions
│   │   └── common.ts         # Core interfaces (ToolAction, LogEntry, Config etc.)
│   ├── config/               # Configuration management
│   │   └── config-manager.ts # Singleton or utility for loading and accessing config
│   ├── util/                 # Core utility functions, organized by functionality
│   │   ├── agent-loop.ts     # Main agent execution loop orchestration
│   │   ├── action-validator.ts # Logic for validating LLM-proposed actions
│   │   ├── executor.ts       # Centralized execution of validated actions
│   │   ├── llm.ts            # LLM communication interface (API calls)
│   │   ├── prompt.ts         # System prompt generation logic
│   │   ├── xml.ts            # XML parsing and generation utilities
│   │   └── json.ts           # JSON path manipulation utilities (get/set/update at path)
│   └── tool/                 # Implementations for each specific tool/action
│       ├── action_logger.ts  # Utility for logging actions to data/log.json
│       ├── memory_set.ts     # Handles 'memory_set' action
│       ├── memory_remove.ts  # Handles 'memory_remove' action
│       ├── memory_mv.ts      # Handles 'memory_mv' action
│       ├── storage_set.ts    # Handles 'storage_set' action
│       ├── storage_get.ts    # Handles 'storage_get' action
│       ├── storage_remove.ts # Handles 'storage_remove' action
│       ├── storage_search.ts # Handles 'storage_search' action
│       └── storage_ls.ts     # Handles 'storage_ls' action
├── package.json              # Node.js project manifest (dependencies, scripts)
├── tsconfig.json             # TypeScript compiler configuration
├── .gitignore                # Specifies intentionally untracked files by Git
└── README.md                 # This comprehensive documentation
```

## 🔄 Development Workflow

### Building

-   **Development Build (with watch mode)**:
    ```bash
    npm run dev # Assuming 'tsc --watch' is configured in package.json
    # Or manually:
    npx tsc --watch
    ```
-   **Production Build**:
    ```bash
    npm run build
    # Or manually:
    npx tsc
    ```

### Testing

(Assuming a test runner like Jest or Vitest is set up)
```bash
# Run all tests
npm test

# Run tests in watch mode
npm test -- --watch

# Run type checking as a form of testing
npm run typecheck # Assuming 'tsc --noEmit' is configured
# Or manually:
npx tsc --noEmit
```
Unit tests should be created for individual utilities in `src/util/` and tool implementations in `src/tool/`.

### Linting and Formatting

(Assuming ESLint and Prettier are set up)
```bash
# Lint code
npm run lint

# Fix linting issues automatically
npm run lint:fix

# Format code
npm run format
```

## 📚 API Reference (Core Components)

This section highlights key functions and interfaces. For full details, refer to the JSDoc comments within the source code.

### Core Utility Functions

Located in `src/util/`.

#### Agent Loop (`util/agent-loop.ts`)
```typescript
/**
 * Runs the main agent loop: prompt generation, LLM call, action execution.
 * Handles top-level errors and continues the loop.
 */
async function runAgent(): Promise<void>;

/**
 * Resets any internal iteration counters for the agent loop (mainly for testing).
 */
function resetIterationCounter(): void; // If applicable
```

#### Action Validation (`util/action-validator.ts`)
```typescript
/**
 * Validates a single ToolAction object.
 * @param action The ToolAction to validate.
 * @returns An array of error messages. Empty if valid.
 */
function validateAction(action: ToolAction): string[];

/**
 * Checks if a ToolAction is valid.
 * @param action The ToolAction to check.
 * @returns True if valid, false otherwise.
 */
function isValidAction(action: ToolAction): boolean;
```

#### Action Execution (`util/executor.ts`)
```typescript
/**
 * Executes an array of validated ToolActions.
 * Results are stored in memory at /sys/result_data/action_N/.
 * @param actions An array of ToolAction objects to execute.
 */
async function executeActions(actions: ToolAction[]): Promise<void>; // Modified to accept an array

/**
 * Resets the internal action counter (mainly for testing).
 */
function resetActionCounter(): void;
```

#### LLM Communication (`util/llm.ts`)
```typescript
/**
 * Calls the configured LLM with the given prompt.
 * @param prompt The system prompt string.
 * @returns The LLM's response string (expected to be XML).
 */
async function callLLM(prompt: string): Promise<string>;
```

#### System Prompts (`util/prompt.ts`)
```typescript
/**
 * Generates the system prompt for the LLM, including context from memory, storage, and tool definitions.
 * @returns The complete system prompt string.
 */
async function generateSystemPrompt(): Promise<string>;
```

#### XML Processing (`util/xml.ts`)
```typescript
/**
 * Parses an XML string from the LLM into an array of ToolAction objects.
 * @param xml The XML string.
 * @returns An array of ToolAction objects.
 * @throws XmlError if parsing fails or circular references are detected.
 */
function parseActionsFromXml(xml: string): ToolAction[];

/**
 * Converts a JSON object to an XML string.
 * @param obj The JSON object to convert.
 * @param rootTag The root tag name for the XML.
 * @param indent Optional indentation string for pretty printing.
 * @returns The XML string representation.
 * @throws XmlError if circular references are detected.
 */
function jsonToXml(obj: any, rootTag: string, indent?: string): string;

/** Custom error class for XML processing issues. */
class XmlError extends Error {
  constructor(message: string, public context?: any) { /* ... */ }
}
```

#### JSON Path Operations (`util/json.ts`)
```typescript
/** Retrieves a value from an object at a given Unix-style path. */
function getValueAtPath(obj: any, path: string): any;

/** Sets a value in an object at a given Unix-style path, creating intermediate objects if necessary. */
function setValueAtPath(obj: any, path: string, value: any): void;

/** Updates an object at a given path by merging with the provided updates (non-destructive). */
function updateObjectAtPath(obj: any, path: string, updates: any): void;

/** Deletes a key/value or an object at a given path. */
function deletePath(obj: any, path: string): boolean; // Returns true if deleted

/** Validates if a path string is well-formed. Throws error if not. */
function validatePath(path: string): void;
```

### Core Interfaces

Located in `src/types/common.ts`.

#### `ToolAction`
```typescript
interface ToolAction {
  kind: ToolKind;
  target_path?: string;  // For actions operating on a destination
  source_path?: string;  // For actions operating on a source (e.g., mv, storage_get)
  content?: any;         // For actions with data payload (e.g., memory_set, storage_search query)
  result_path?: string;  // Optional: where to store results for actions like ls, search
}
```

#### `ToolKind`
```typescript
type ToolKind =
  | 'memory_set'
  | 'memory_remove'
  | 'memory_mv'
  | 'storage_get'
  | 'storage_set'
  | 'storage_search'
  | 'storage_remove'
  | 'storage_ls';
```

#### `LogEntry`
```typescript
interface LogEntry {
  timestamp: number;
  action_number_in_batch?: number; // If applicable
  overall_action_sequence?: number; // If applicable
  kind: ToolKind;
  target_path?: string;
  source_path?: string;
  content_summary?: string; // A summary or hash of the content
  status: 'success' | 'error';
  message?: string; // Success message or general info
  error?: string;   // Error message if status is 'error'
  result_summary?: string; // Summary of data produced by action
}
```

### Configuration Interface

Likely defined in `src/types/common.ts` or `src/config/config-manager.ts`.

```typescript
interface MemoryConfig {
  MemoryCharacterMax: number;
  DirectChildMax: number;
}

interface LLMConfig {
  apiUrl: string;
  model: string;
  maxTokens: number;
  temperature: number;
}

interface SystemConfig {
  maxLogEntries: number;
  autoCleanup: boolean;
  debugMode: boolean;
}

interface AppConfig {
  memory: MemoryConfig;
  llm: LLMConfig;
  system: SystemConfig;
}
```

## 🤝 Contributing

Contributions are welcome! Whether it's bug fixes, feature enhancements, documentation improvements, or new examples, your help is appreciated.

1.  **Fork the repository** on GitHub.
2.  **Clone your fork** locally: `git clone https://github.com/YOUR_USERNAME/lkjagent.git`
3.  **Create a feature branch**: `git checkout -b feature/your-amazing-feature` or `fix/bug-description`.
4.  **Make your changes**.
5.  **Test your changes thoroughly**. Add unit tests if applicable.
6.  **Ensure code lints and formats correctly**: `npm run lint` and `npm run format`.
7.  **Commit your changes**: `git commit -m 'feat: Add amazing feature'` (follow Conventional Commits).
8.  **Push to your branch**: `git push origin feature/your-amazing-feature`.
9.  **Open a Pull Request** against the `main` (or `develop`) branch of the original repository. Provide a clear description of your changes.

### Development Guidelines

-   **TypeScript Best Practices**: Adhere to modern TypeScript standards and maintain strict type safety.
-   **JSDoc Comments**: Add comprehensive JSDoc comments for all public functions, classes, interfaces, and complex logic.
-   **Error Handling**: Implement robust error handling for all operations, providing specific and informative error messages.
-   **Unit Tests**: Write unit tests for new utility functions, tool implementations, and critical logic paths. Aim for good test coverage.
-   **Documentation**: Update `README.md` and any other relevant documentation for API changes, new features, or significant architectural modifications.
-   **Modularity**:
    -   Keep utility modules focused on single responsibilities.
    -   Minimize cross-dependencies between utility modules where possible.
    -   Use dependency injection or clear parameter passing rather than global state.
    -   Follow consistent naming conventions across modules.
-   **Configuration**: Make new features configurable via `data/config.json` if they involve user-adjustable parameters.

### Adding New Utilities

When adding new utility functions (e.g., to `src/util/`):
1.  **Create Focused Modules**: Place the new utility in a new `.ts` file within `src/util/` (or an appropriate subdirectory) if it represents a distinct area of functionality. Ensure it has a single, clear responsibility.
2.  **Export Clean Interfaces**: Define and export clear TypeScript interfaces for any functions or classes.
3.  **Comprehensive Error Handling**: Include robust error handling and throw specific, descriptive errors.
4.  **JSDoc Documentation**: Document all public functions and types.
5.  **Unit Tests**: Add corresponding unit tests.
6.  **Update README**: If the utility is user-facing or significantly impacts the architecture, document it in the `README.md`.

### Adding New Tools

When implementing new tools (actions the LLM can request):
1.  **Define Action Kind**: Add the new tool's kind to the `ToolKind` type in `src/types/common.ts`.
2.  **Implement Tool Logic**: Create a new file in `src/tool/` (e.g., `src/tool/my_new_tool.ts`) containing the logic for this tool. This function will typically take parameters from the `ToolAction` object and interact with memory, storage, or external systems.
3.  **Update Action Validator**: Add validation rules for the new tool's parameters in `src/util/action-validator.ts`.
4.  **Integrate into Executor**: Modify `src/util/executor.ts` to call your new tool's implementation when its `kind` is encountered.
5.  **Update System Prompt**: Ensure `src/util/prompt.ts` includes the definition and XML usage example of the new tool in the system prompt given to the LLM.
6.  **Documentation**: Add the new tool to the "Available Actions" table in this `README.md`.
7.  **Unit Tests**: Write tests for the new tool's logic and its integration with the executor.

## 🛣️ Roadmap (Future Enhancements)

-   [ ] **Advanced Memory Management**: Strategies for automatic summarization or eviction of older working memory items.
-   [ ] **Tool Schema Generation**: Automatically generate XML schema for tools to provide to the LLM for stricter validation.
-   [ ] **Web Interaction Tools**: Tools for fetching web content, searching online.
-   [ ] **File System Tools**: Tools for interacting with the local file system (with appropriate security sandboxing).
-   [ ] **Plugin System**: Allow users to easily add custom tools and utilities.
-   [ ] **Enhanced UI/Dashboard**: A simple web interface for monitoring agent activity and managing data.
-   [ ] **Vector Store Integration**: For more sophisticated semantic search in persistent storage.
-   [ ] **Multi-Agent Communication**: Protocols for lkjagent instances to interact.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file (you'll need to create this file if it doesn't exist) for details.

```
MIT License

Copyright (c) [Year] [Your Name/Organization]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## 🙏 Acknowledgements

-   Inspiration from existing AI agent frameworks (e.g., AutoGPT, BabyAGI, LangChain).
-   The open-source LLM community.
-   (Any specific libraries or individuals you want to thank).

---

**lkjagent** - Empowering small LLMs with persistent memory, structured reasoning capabilities, and a clean modular architecture for maintainable AI agent development.