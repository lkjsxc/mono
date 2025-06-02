```markdown
# 🤖 lkjagent

[![build_status](https://img.shields.io/your_ci_badge_url)](https://your_ci_link)
[![npm_version](https://img.shields.io/npm/v/lkjagent.svg?style=flat)](https://www.npmjs.com/package/lkjagent)
[![license:_mit](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![prs_welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](http://makeapullrequest.com)

**lkjagent** is a sophisticated ai_agent framework specifically designed for small language_learning_models (llms). It solves the critical challenge of memory limitations in smaller models by implementing a dual_memory_architecture: finite **working_memory** for immediate context and infinite persistent **storage** for long_term data. This enables complex, persistent, and long_running task execution.

This framework empowers developers to build advanced ai_agents that can handle tasks requiring long_term context and data retention, even when using llms with constrained memory capacities. Communication with the llm is performed using a **simple_xml_format that can be straightforwardly rewritten as json**, ensuring clarity and ease of parsing. Attributes and complex xml features are not supported; tags map to keys and text content to values.

## 🌟 key_features

-   **🧠 dual_memory_architecture**: Finite **working_memory** (for immediate context) + infinite persistent **storage** (for long_term data).
-   **📡 simple_xml_based_communication**: Structured action protocol using a simple_xml_format (easily convertible to json) for reliable and verifiable llm interaction.
-   **⚡ modular_typescript_design**: Clean, maintainable, and scalable architecture built with typescript.
-   **🔄 persistent_task_management**: Enables long_running tasks with state preservation across sessions.
-   **📊 cumulative_action_logging_and_results**: Full audit trail of all agent operations. Actions are numbered cumulatively (e.g., `_1`, `_2`, ...). Execution results are stored in `/working_memory/action_result/_${total_index}`.
-   **🛡️ robust_error_handling**: Graceful failure recovery with detailed error reporting for enhanced stability.
-   **🎯 cumulative_action_numbering**: Sequential tracking of all operations, facilitating debugging and state recovery. Each action processed increments a total index.
-   **🔍 advanced_xml_processing**: Safe xml parsing for the simple supported format, with validation and content escaping.

## 📋 table_of_contents

-   [🌟 key_features](#-key_features)
-   [📋 table_of_contents](#-table_of_contents)
-   [🚀 quick_start](#-quick_start)
-   [🏗️ architecture_overview](#️-architecture_overview)
    -   [🧠 memory_system](#-memory_system)
    -   [📡 communication_protocol](#-communication_protocol)
-   [🔧 installation](#-installation)
    -   [📦 prerequisites](#-prerequisites)
    -   [🛠️ installation_steps](#️-installation_steps)
    -   [📁 data_directory_setup](#-data_directory_setup)
-   [⚙️ configuration](#️-configuration)
    -   [🔧 configuration_options](#-configuration_options)
    -   [🎯 action_numbering_system_details](#-action_numbering_system_details)
-   [🧩 modular_utilities_and_core_concepts](#-modular_utilities_and_core_concepts)
    -   [🏗️ architectural_benefits](#️-architectural_benefits)
    -   [🔧 core_utility_modules (src/util/)](#-core_utility_modules-srcutil)
-   [🛠️ tool_system](#️-tool_system)
    -   [available_actions](#available_actions)
    -   [action_execution_flow](#action_execution_flow)
-   [📊 data_management](#-data_management)
    -   [working_memory_structure (memory.json)](#working_memory_structure-memoryjson)
    -   [storage_structure (storage.json)](#storage_structure-storagejson)
    -   [action_log_structure (log.json)](#action_log_structure-logjson)
-   [💻 usage](#-usage)
    -   [basic_operation](#basic_operation)
    -   [integration_with_lm_studio (or_similar)](#integration_with_lm_studio-or_similar)
    -   [example_workflow](#example_workflow)
-   [🎮 example_use_cases](#-example_use_cases)
-   [📁 project_structure](#-project_structure)
-   [🔄 development_workflow](#-development_workflow)
    -   [building](#building)
    -   [testing](#testing)
    -   [linting_and_formatting](#linting_and_formatting)
-   [📚 api_reference (core_components)](#-api_reference-core_components)
    -   [core_utility_functions](#core_utility_functions)
    -   [core_interfaces](#core_interfaces)
    -   [configuration_interface](#configuration_interface)
-   [🤝 contributing](#-contributing)
    -   [development_guidelines](#development_guidelines)
    -   [adding_new_utilities](#adding_new_utilities)
    -   [adding_new_tools](#adding_new_tools)
-   [✨ implementation_order (todo_list_from_scratch)](#-implementation_order-todo_list_from_scratch)
-   [📄 license](#-license)
-   [🙏 acknowledgements](#-acknowledgements)

## 🚀 quick_start

Get lkjagent up and running in a few simple steps:

```bash
# 1. Clone the repository
git clone https://github.com/your_username/lkjagent.git # Replace with actual URL
cd lkjagent

# 2. Install dependencies
npm install

# 3. Build the project
npm run build
# Or manually with tsc
# npx tsc

# 4. Initialize data files (creates default json files in data/)
npm run init-data

# 5. Configure your llm (edit data/config.json, see configuration section)
# Ensure your llm server (e.g., lm_studio, ollama) is running.

# 6. Start the agent
npm start
# Or: node dist/index.js
```

## 🏗️ architecture_overview

lkjagent is built around a dual_memory_system and a structured, simple_xml_communication_protocol to effectively manage tasks and interact with llms. **Storage information is not directly given to the llm in the prompt.**

### 🧠 memory_system

The dual_memory_architecture is designed to overcome the memory limitations of smaller llms:

```mermaid
graph TB
    A[llm] -->|requests_actions_via_simple_xml| B(xml_parser)
    B -->|outputs_action_objects| C(action_validator)
    C -->|validates| D(executor)
    D -->|operates_on| E[working_memory (in memory.json)]
    D -->|operates_on (indirectly_via_tools)| F[persistent_storage (in storage.json)]
    E -->|provides_context (excluding_direct_storage_view)| G(prompt_generator)
    G -->|sends_prompt_to_llm| A
    D -->|logs_actions_results| H(action_logger)
    H -->|creates| I[audit_trail (in log.json & /working_memory/action_result/)]
```

#### 🐏 working_memory (`memory.json` containing `/working_memory`)
-   **Finite capacity** (configurable, e.g., `working_memory_character_max` default 2048 characters).
-   **Fast access** for current task context, immediate operations, and temporary data. All llm interactions are based on this.
-   **Structured data** as json under the `/working_memory` root key.
-   Holds the **real_time state** for active tasks and system operations, including results of past actions.

#### 💾 persistent_storage (`storage.json` containing `/storage`)
-   **Effectively infinite capacity** for long_term data retention, knowledge bases, and archival.
-   **Hierarchical organization** using unix_style path_based access (e.g., `/storage/path/to/data`).
-   **Accessed by llm indirectly** via actions like `get`, `ls`, `search` targeting `/storage/...` paths.
-   Ensures **cross_session persistence**, maintaining state between agent restarts.
-   **Not directly exposed to the llm in the system_prompt.**

### 📡 communication_protocol

The agent uses a robust xml_based_protocol for structured llm communication. **Crucially, only simple xml that can be directly and unambiguously rewritten as json is allowed.** This means:
-   Tags map to json keys.
-   Text content within tags maps to json values.
-   No xml attributes are used.
-   No mixed content (text interspersed with child elements within a single parent) that doesn't map cleanly to a key_value structure.
-   The primary purpose is to provide a structured format for actions and their parameters.

**example_llm_action_request:**
```xml
<actions>
  <action>
    <kind>set</kind>
    <target_path>/working_memory/user/todo/new_task</target_path>
    <content>{"task_description": "Complete project documentation", "status": "pending", "priority": "high"}</content>
  </action>
  <action>
    <kind>ls</kind>
    <target_path>/storage/archived_data</target_path>
  </action>
  <action>
    <kind>search</kind>
    <target_path>/storage/documents</target_path> <!-- Scope of search -->
    <content>project documentation</content> <!-- Search query -->
  </action>
</actions>
```

**xml_protocol_features:**
-   **schema_validation**: Ensures proper action structure based on the simple format.
-   **content_escaping**: Safe handling of special characters within text content.
-   **error_recovery**: Graceful parsing with detailed error messages for debugging.

## 🔧 installation

### 📦 prerequisites

-   **node_js**: Version 16+ (LTS recommended).
-   **npm** (usually comes with node_js) or **yarn**.
-   **typescript**: Version 4.5+ (for development).
-   **llm_server**:
    -   lm_studio, ollama, or any openai_api_compatible local llm_server.
    -   Ensure the server is running and accessible.
-   **system_ram**: 4GB+ recommended for optimal performance, especially when running llms locally.

### 🛠️ installation_steps

```bash
# 1. Clone the repository
git clone https://github.com/your_username/lkjagent.git # Replace with actual URL
cd lkjagent

# 2. Install dependencies
npm install

# 3. Build the typescript project
npm run build
# Alternative for manual build:
# npx tsc

# 4. Initialize data files (creates default json files in data/)
npm run init-data
# This script ensures data/config.json, data/memory.json, data/storage.json, and data/log.json exist.
```

### 📁 data_directory_setup

After running `npm run init-data` (or manual setup), the `data/` directory will be initialized:

```
data/
├── config.json      # Agent configuration (root_level_settings)
├── memory.json      # Contains /working_memory (volatile, for current tasks & action results)
├── storage.json     # Contains /storage (persistent_storage, long_term knowledge, archives)
└── log.json         # Action execution log (history of operations)
```

**manual_setup (if `npm run init-data` is unavailable or for custom initialization):**

```bash
# Create data directory
mkdir -p data

# Create default configuration file (data/config.json) - All settings at root level
echo '{
  "working_memory_character_max": 2048,
  "working_memory_direct_child_max": 8,
  "llm_api_url": "http://localhost:1234/v1/chat/completions",
  "llm_model": "your-local-model-name",
  "llm_max_tokens": 1000,
  "llm_temperature": 0.7,
  "system_max_log_entries": 1000,
  "system_auto_cleanup": true,
  "system_debug_mode": false
}' > data/config.json

# Create empty data files
echo '{"working_memory": {"user_data": {"todo":{}}, "action_result":{}, "system_info":{}}}' > data/memory.json
echo '{"storage": {"knowledge_base":{"system_policy_summary":"","greeting_message":""},"archived_data":{}}}' > data/storage.json
echo '[]' > data/log.json
```
**Note:** Ensure `working_memory.action_result` exists in `memory.json` as it's used for storing action results.

## ⚙️ configuration

Configure `lkjagent` via `data/config.json`. **All configuration settings are at the root level (not nested).**

### 🔧 configuration_options

```json
{
  "working_memory_character_max": 4096,  // Max characters in working_memory (default: 2048)
  "working_memory_direct_child_max": 12,    // Max direct children per working_memory/storage directory node (default: 8)
  "llm_api_url": "http://localhost:1234/v1/chat/completions", // lm_studio/ollama api endpoint
  "llm_model": "llama-3.2-3b-instruct", // Specific model name to use for inference
  "llm_max_tokens": 1000,               // Maximum tokens per llm response
  "llm_temperature": 0.7,              // llm creativity/randomness (0.0-1.0)
  "system_max_log_entries": 1000,        // Maximum entries in data/log.json before potential cleanup
  "system_auto_cleanup": true,          // Enable automatic cleanup of old data/logs (future feature)
  "system_debug_mode": false            // Enable verbose logging for debugging purposes
}
```

#### working_memory_configuration
-   **`working_memory_character_max`**: Maximum total characters allowed in the `/working_memory` structure within `memory.json`. This helps constrain the context provided to the llm.
-   **`working_memory_direct_child_max`**: Maximum number of direct children a json object node can have in `working_memory` or `storage`. This helps prevent overly wide structures.

#### llm_configuration
-   **`llm_api_url`**: The api endpoint for your llm_server.
-   **`llm_model`**: The identifier for the llm model.
-   **`llm_max_tokens`**: The maximum number of tokens the llm should generate.
-   **`llm_temperature`**: Controls the randomness of the llm's output.

#### system_configuration
-   **`system_max_log_entries`**: Maximum number of entries to keep in `data/log.json`.
-   **`system_auto_cleanup`**: Boolean to enable/disable automatic cleanup processes.
-   **`system_debug_mode`**: Boolean to enable more verbose logging.

### 🎯 action_numbering_system_details

`lkjagent` features an action numbering system that tracks all operations **cumulatively and sequentially**. Actions are processed one by one. The results and status of each action requested by the llm are stored in `/working_memory/action_result/_${total_index}/`, where `total_index` is a counter that increments for every action processed by the agent, starting from `_1`. Once an action is processed and its result logged, it is considered final.

**Example structure in `memory.json` after actions:**
```json
{
  "working_memory": {
    "user_data": { /* ... user data ... */ },
    "system_info": { /* ... system data ... */ },
    "action_result": {
      "_1": {
        "action_index": 1,
        "timestamp": 1672531200000,
        "kind": "ls",
        "target_path": "/storage/",
        "status": "success",
        "data": { "knowledge_base": { "_is_directory": true }, "archived_data": { "_is_directory": true } } // Example result for ls
      },
      "_2": {
        "action_index": 2,
        "timestamp": 1672531201000,
        "kind": "set",
        "target_path": "/working_memory/user_data/current_task",
        "status": "success",
        "message": "Value set successfully at /working_memory/user_data/current_task"
      },
      "_3": {
        "action_index": 3,
        "timestamp": 1672531202000,
        "kind": "get",
        "target_path": "/working_memory/non_existent_path",
        "status": "error",
        "error": "Path /working_memory/non_existent_path not found."
      }
    }
  }
}
```

**Benefits:**
-   **cumulative_sequential_tracking**: Every llm-requested action gets a unique sequential number (e.g., `_1`, `_2`, `_3`, ...).
-   **result_and_status_feedback**: The llm receives feedback on the outcome of its requested actions in the next turn via the system_prompt (which includes the content of `/working_memory/action_result/`).
-   **error_isolation**: Failed actions are logged with detailed error information, allowing the llm to potentially retry or adjust its strategy.
-   **debugging_support**: Easy to trace the execution flow and identify issues by examining `/working_memory/action_result/`.

## 🧩 modular_utilities_and_core_concepts

(Content remains largely the same, with minor wording adjustments for `working_memory` and the action system if needed. Focus on the distributed nature)

### 🏗️ architectural_benefits

#### developer_experience
-   **clear_separation_of_concerns**: Each module has a single, well_defined responsibility.
-   **easy_testing**: Individual utilities can be unit_tested in isolation.
-   **maintainable_code**: Bugs and new features can often be isolated to specific modules.
-   **type_safety**: Full typescript support.

#### operational_stability_and_performance
-   **reduced_complexity**: Main entry point delegates tasks.
-   **error_isolation**: Failures in one utility are less likely to cascade.
-   **performance**: Allows for optimized execution paths.

### 🔧 core_utility_modules (`src/util/`)

#### 🔄 agent_loop (`agent_loop.ts`)
Orchestrates the continuous execution cycle: listen, think, act.

#### ✅ action_validation (`action_validator.ts`)
Validates actions from llm against the simple xml structure and defined action kinds/parameters.

#### ⚙️ action_execution (`executor.ts`)
Central engine for carrying out validated actions. It parses the `target_path` to determine if an action applies to `working_memory` or `storage`. Records results in `/working_memory/action_result/_${total_index}`.

#### 🤖 llm_communication (`llm.ts`)
Interface for interacting with the llm, sending prompts and receiving simple xml responses.

#### 📝 system_prompt_generation (`prompt.ts`)
Dynamically creates the system_prompt. **Crucially, it does NOT include direct storage information.** It will include:
-   Current relevant `/working_memory` data (respecting `working_memory_character_max`).
-   Results from previous actions (content of `/working_memory/action_result/`).
-   List of available tools and their xml format.
-   The current goal or task.

#### 🔄 xml_processing (`xml.ts`)
Utilities for parsing simple xml responses from the llm (convertible to json) and generating such xml if needed.

#### 📁 json_path_operations (`json.ts`)
unix_style path operations for `working_memory` (within `memory.json`) and `storage` (within `storage.json`). Paths like `/working_memory/foo/bar` or `/storage/alpha/beta` are resolved by the executor to operate on the correct json structure.

## 🛠️ tool_system

The tool_system defines operations the llm can request. Actions are specified in simple xml. **Behavior of actions changes depending on whether `target_path` (or `source_path`) starts with `/working_memory/` or `/storage/`.**

### available_actions

| action_kind | Description                                                                 | parameters (xml_tags)                  | example_llm_request_fragment                                                                                                                               |
|-------------|-----------------------------------------------------------------------------|----------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `set`       | Add or update data in `working_memory` or `storage`.                        | `target_path`, `content`               | `<kind>set</kind><target_path>/working_memory/user/task</target_path><content>{"details": "foo"}</content>`                                                |
| `get`       | Retrieve data from `working_memory` or `storage`. Result in `action_result`.| `target_path`                          | `<kind>get</kind><target_path>/storage/documents/doc1</target_path>`                                                                                        |
| `rm`        | Delete data from `working_memory` or `storage`.                             | `target_path`                          | `<kind>rm</kind><target_path>/working_memory/user/old_task</target_path>`                                                                                   |
| `mv`        | Move/rename data within the same area (wm->wm, store->store) or between areas. | `source_path`, `target_path`           | `<kind>mv</kind><source_path>/working_memory/temp/data</source_path><target_path>/storage/archived/data</target_path>`                                      |
| `ls`        | List contents of a path in `working_memory` or `storage`. Result in `action_result`. Indicates if items are directories. | `target_path`                          | `<kind>ls</kind><target_path>/storage/project_files</target_path>`                                                                                          |
| `search`    | Search content within `working_memory` or `storage` by keywords. Result in `action_result`. | `target_path` (scope of search), `content` (search query) | `<kind>search</kind><target_path>/storage/knowledge_base</target_path><content>backup policy</content>`                                                       |

**Note on `ls` results:** The data returned for `ls` in `action_result` will be an object where keys are item names. Each item value will be an object, minimally containing `{"_is_directory": boolean}`. E.g., `{"file.txt": {"_is_directory": false}, "folder": {"_is_directory": true}}`.

### action_execution_flow

1.  **llm_generates_simple_xml**: llm outputs xml with `<action>` elements.
2.  **xml_parsing**: `xml.ts` parses into `tool_action` objects.
3.  **validation**: `action_validator.ts` checks structure and parameters.
4.  **execution**: `executor.ts` processes each action:
    *   Determines target (e.g., `working_memory` or `storage`) based on `target_path` (and `source_path`).
    *   Calls appropriate underlying functions (e.g., json path operations on `memory.json` data or `storage.json` data).
5.  **result_recording**: Outcome is recorded in `/working_memory/action_result/_${total_index}/`. `total_index` is incremented.
6.  **logging**: `action_logger.ts` logs to `data/log.json`.
7.  **feedback_loop**: `prompt.ts` includes `/working_memory/action_result/` in the next system_prompt.

## 📊 data_management

### working_memory_structure (`memory.json`)

Contains the `/working_memory` key. This is the agent's volatile, short_term context.
Limited by `working_memory_character_max`.

```json
{
  "working_memory": {
    "user_data": {
      "current_focus": "Writing a fantasy story.",
      "todo": {
        "task1": { "description": "Outline chapter 1", "status": "pending" }
      }
    },
    "system_info": {
      "iteration_count": 5,
      "last_error": null
    },
    "action_result": {
      "_1": {
        "action_index": 1,
        "timestamp": 1672531200000,
        "kind": "set",
        "target_path": "/working_memory/user_data/current_focus",
        "status": "success",
        "message": "Value set successfully."
      },
      "_2": { /* ... more results ... */ }
    }
  }
}
```

### storage_structure (`storage.json`)

Contains the `/storage` key. Used for long_term data, knowledge bases, and archives. Persists across sessions. **Not directly shown to llm.**

```json
{
  "storage": {
    "knowledge_base": {
      "common_greetings": ["Hello!", "Hi there!"],
      "project_alpha_details": { "client": "ACME Corp", "deadline": "2024-12-31" }
    },
    "archived_data": {
      "completed_stories": { "story_abc_summary": "A tale..." }
    }
  }
}
```

### action_log_structure (`data/log.json`)

Persistent audit trail of all actions. Limited by `system_max_log_entries`.

```json
[
  {
    "timestamp": 1672531200000,
    "total_action_index": 101, // Cumulative index
    "kind": "set",
    "target_path": "/working_memory/user_data/todo/task1",
    "content_summary": "{\"description\":\"Outline chapter 1\",\"status\":\"pending\"}",
    "status": "success",
    "message": "Value set successfully."
  },
  {
    "timestamp": 1672531201500,
    "total_action_index": 102,
    "kind": "search",
    "target_path": "/storage/knowledge_base",
    "content_summary": "dragon lore", // Search query
    "status": "success",
    "result_summary": "Found 3 relevant items.", // Summary of data placed in action_result
    "error": null
  }
]
```

## 💻 usage

### basic_operation

```bash
npm start
# Or: node dist/index.js
```
The agent loop:
1.  Generate system_prompt (with `working_memory` data and `action_result`).
2.  Call llm.
3.  Parse simple xml actions.
4.  Validate and execute actions, updating `working_memory` or `storage`, and populating `/working_memory/action_result/_${total_index}`.
5.  Log results. Repeat.

### integration_with_lm_studio (or_similar)

1.  Launch llm_server & load model.
2.  Configure `data/config.json` with `llm_api_url` and `llm_model`.
3.  Run `npm start`.

### example_workflow

1.  **agent_starts**: Loads config, `memory.json` (for `/working_memory`), and `storage.json`. `total_action_index` is implicitly known or reset if stateless on start (or loaded from a persistent counter if implemented).
2.  **system_prompt**: `prompt.ts` creates prompt using `/working_memory` (including `/working_memory/action_result/`), tool list, and goal. **No direct `storage` content.**
3.  **llm_interaction**: `llm.ts` sends prompt, gets simple xml back.
4.  **action_processing**:
    *   `xml.ts` parses xml.
    *   `action_validator.ts` validates.
    *   `executor.ts` executes actions. If `target_path` is `/working_memory/...`, operates on `memory.json` data. If `/storage/...`, operates on `storage.json` data.
    *   Results stored in `/working_memory/action_result/_${total_index}/`. `total_index` increments.
5.  **logging**: `action_logger.ts` writes to `data/log.json`.
6.  **loop**.

## 🎮 example_use_cases

(Examples remain similar but emphasize interaction via actions for storage)

-   **📝 long_form_content_generation**:
    -   llm uses `set` to draft sections in `/working_memory/drafts/chapter1`.
    -   Uses `set` with `target_path` like `/storage/story_elements/characters/draco` to save character details.
    -   Uses `get` from `/storage/story_elements/plot_points` to retrieve information into `/working_memory/action_result/` for planning.
-   **🤖 task_management**:
    -   `set` to `/working_memory/tasks/todo/task_id` to add tasks.
    -   `mv` from `/working_memory/tasks/todo/task_id` to `/working_memory/tasks/done/task_id`.
    -   `set` to `/storage/archived_tasks/YYYY-MM/task_id` to archive.

## 📁 project_structure

```
lkjagent/
├── data/
│   ├── config.json           # agent_configuration (root_level_settings)
│   ├── memory.json           # contains /working_memory (runtime_context, action_results)
│   ├── storage.json          # contains /storage (persistent_long_term_data)
│   └── log.json              # action_execution_log
├── src/
│   ├── index.ts              # main_entry_point
│   ├── types/
│   │   └── common.ts         # core_interfaces (tool_action, log_entry, app_config etc.)
│   ├── config/
│   │   └── config_manager.ts # utility_for_loading_root_level_config
│   ├── util/
│   │   ├── agent_loop.ts
│   │   ├── action_validator.ts
│   │   ├── executor.ts       # dispatches_actions_to_working_memory_or_storage_based_on_path
│   │   ├── llm.ts
│   │   ├── prompt.ts         # generates_prompt (no_direct_storage_view)
│   │   ├── xml.ts            # handles_simple_xml (json_like)
│   │   └── json.ts           # json_path_ops_for_memory_storage_structures
│   └── tool/
│       ├── action_logger.ts
│       ├── set_tool.ts       # handles 'set' for_both working_memory_and_storage
│       ├── get_tool.ts       # handles 'get' for_both
│       ├── rm_tool.ts        # handles 'rm' for_both
│       ├── mv_tool.ts        # handles 'mv' for_both
│       ├── ls_tool.ts        # handles 'ls' for_both (indicates directory status)
│       └── search_tool.ts    # handles 'search' for_both
├── ... (package.json, tsconfig.json, etc.)
└── README.md
```
*(Tool implementations in `src/tool/` would now be more generic, e.g., `set_tool.ts` would inspect the path prefix)*

## 🔄 development_workflow

(Content remains similar)

### building
### testing
### linting_and_formatting

## 📚 api_reference (core_components)

### core_utility_functions

#### action_execution (`util/executor.ts`)
```typescript
/**
 * Executes an array of validated tool_actions.
 * Results are stored in working_memory at /working_memory/action_result/_${total_index}/.
 * Increments a global action counter for ${total_index}.
 * @param actions An array of tool_action objects to execute.
 * @returns Promise<void>
 */
async function execute_actions(actions: tool_action[]): Promise<void>;

/**
 * Gets the next total_index for an action result.
 * @returns number The next action index.
 */
function get_next_action_total_index(): number; // Or similar mechanism

/** Resets action counter (for testing) */
function reset_action_total_index_counter(): void;
```

#### system_prompts (`util/prompt.ts`)
```typescript
/**
 * Generates the system_prompt for the llm. Includes context from /working_memory (including /working_memory/action_result/),
 * and tool definitions. Does NOT include direct storage data.
 * @returns The complete system_prompt string.
 */
async function generate_system_prompt(): Promise<string>;
```

#### xml_processing (`util/xml.ts`)
```typescript
/**
 * Parses a simple xml string (json_like) from the llm into an array of tool_action objects.
 * @param xml The xml string.
 * @returns An array of tool_action objects.
 * @throws xml_error if parsing fails or format is not simple.
 */
function parse_actions_from_xml(xml: string): tool_action[];
```

### core_interfaces

Located in `src/types/common.ts`.

#### `tool_action`
```typescript
interface tool_action {
  kind: tool_kind;
  target_path: string;     // Path for set, get, rm, ls, search (scope), mv (destination).
                           // Must start with /working_memory/ or /storage/.
  source_path?: string;   // For mv (source only). Must start with /working_memory/ or /storage/.
  content?: any;          // Data for 'set', query for 'search'. Expected to be simple json_like data if complex.
}
```

#### `tool_kind`
```typescript
type tool_kind =
  | 'set'
  | 'get'
  | 'rm'
  | 'mv'
  | 'ls'
  | 'search';
```

#### `log_entry` (Reflects cumulative numbering)
```typescript
interface log_entry {
  timestamp: number;
  total_action_index: number; // Cumulative index for this action
  kind: tool_kind;
  target_path?: string;
  source_path?: string;
  content_summary?: string;
  status: 'success' | 'error';
  message?: string;
  error?: string;
  result_summary?: string;
}
```

#### `action_result` (Structure within `/working_memory/action_result/_N`)
```typescript
interface action_result {
  action_index: number; // The N in _N
  timestamp: number;
  kind: tool_kind;
  target_path: string;
  source_path?: string; // if applicable (e.g. for mv)
  status: 'success' | 'error';
  data?: any; // For 'get', 'ls', 'search' results
  message?: string; // For 'set', 'rm', 'mv' success, or general info
  error?: string; // Error message if status is 'error'
}
```

### configuration_interface

Defined in `src/types/common.ts` or loaded by `src/config/config_manager.ts`. **All root_level properties.**

```typescript
interface app_config {
  working_memory_character_max: number;
  working_memory_direct_child_max: number;
  llm_api_url: string;
  llm_model: string;
  llm_max_tokens: number;
  llm_temperature: number;
  system_max_log_entries: number;
  system_auto_cleanup: boolean;
  system_debug_mode: boolean;
}
```

## 🤝 contributing

(Content remains similar, but new tools would follow the generic `set`, `get`, etc., pattern, with logic branching on path prefixes.)

### development_guidelines
### adding_new_utilities
### adding_new_tools
If adding a conceptually new *type* of action beyond `set`, `get`, `rm`, `mv`, `ls`, `search`:
1.  Define new `tool_kind`.
2.  Create `src/tool/new_tool_handler.ts`.
3.  Update `action_validator.ts`.
4.  Integrate into `executor.ts`.
5.  Update `prompt.ts` with its xml definition.
6.  Document in README. Add tests.

If extending existing tools (e.g., new options for `search`), modify the respective tool handler and update validation/documentation.

## ✨ implementation_order (todo_list_from_scratch)

Here's a suggested order for implementing `lkjagent` from scratch to achieve a "beautiful" and logical development flow:

1.  **project_setup_and_core_dependencies:**
    *   Initialize node_js project (`npm init`).
    *   Install typescript and basic dev dependencies (`typescript`, `ts-node`, `@types/node`).
    *   Setup `tsconfig.json`.
    *   Initialize Git repository.
    *   Add linters/formatters (eslint, prettier).

2.  **core_types_definition (`src/types/common.ts`):**
    *   `app_config` (root_level properties).
    *   `tool_kind` (set, get, rm, mv, ls, search).
    *   `tool_action` interface.
    *   `action_result` interface.
    *   `log_entry` interface.
    *   Basic json_like types if needed (`json_value`, `json_object`).

3.  **configuration_management (`src/config/config_manager.ts`):**
    *   Function to load `data/config.json`.
    *   Provide a way to access configuration values (e.g., a singleton or exported getter).
    *   Script for `npm run init-data` to create default `config.json`, `memory.json`, `storage.json`, `log.json`.

4.  **json_path_utilities (`src/util/json.ts`):**
    *   `get_value_at_path(obj, path)`
    *   `set_value_at_path(obj, path, value)`
    *   `delete_path(obj, path)`
    *   `list_path(obj, path)` (should return structure indicating directories)
    *   Path validation helper. These will be fundamental for memory/storage operations.

5.  **persistent_data_management (initial_file_io):**
    *   Simple functions to read/write `memory.json` and `storage.json`.
    *   These will be used by the tools later.

6.  **action_logger (`src/tool/action_logger.ts`):**
    *   Function to append `log_entry` objects to `data/log.json`.
    *   Handle `system_max_log_entries` (simple truncation for now).

7.  **tool_implementations (`src/tool/*.ts`):**
    *   Create separate files for each `tool_kind` handler (e.g., `set_tool.ts`, `get_tool.ts`).
    *   Each tool function will:
        *   Accept `tool_action` parameters.
        *   Parse `target_path` (and `source_path`) to determine if it's `/working_memory/` or `/storage/`.
        *   Load the relevant json file (`memory.json` or `storage.json`).
        *   Use `json.ts` utilities to perform the operation.
        *   Save the modified json file.
        *   Return a result structure (data for get/ls/search, or success/error message).
    *   `ls_tool.ts`: Ensure it indicates if list items are directories (objects).
    *   `search_tool.ts`: Basic keyword search logic for now.

8.  **xml_processing_utilities (`src/util/xml.ts`):**
    *   `parse_actions_from_xml(xmlString)`: Convert simple xml to `tool_action[]`. Emphasize only json_like structures.
    *   `json_to_xml(obj, rootTag)`: For generating xml examples for the prompt if needed.

9.  **action_validator (`src/util/action_validator.ts`):**
    *   Function `validate_action(action: tool_action)`:
        *   Check for valid `kind`.
        *   Check for required parameters (`target_path`, `content` for set/search, `source_path` for mv).
        *   Validate path prefixes (`/working_memory/` or `/storage/`).
        *   Check `content` (e.g., if it's supposed to be an object for `set`).

10. **executor (`src/util/executor.ts`):**
    *   `execute_actions(actions: tool_action[])`:
        *   Maintain/increment `total_action_index`.
        *   Loop through validated actions.
        *   Call the appropriate tool implementation based on `action.kind`.
        *   Construct `action_result` object.
        *   Save `action_result` to `/working_memory/action_result/_${total_action_index}` (this means updating `memory.json`).
        *   Call `action_logger.ts` to log the action.
        *   Handle errors from tools and record them in `action_result`.

11. **llm_communication (`src/util/llm.ts`):**
    *   `call_llm(prompt: string)`: Function to make HTTP POST request to `llm_api_url`.
    *   Handle api response and potential errors.

12. **system_prompt_generation (`src/util/prompt.ts`):**
    *   `generate_system_prompt()`:
        *   Load current `memory.json`.
        *   Extract relevant parts of `/working_memory/` (respecting `working_memory_character_max`).
        *   Include the entire `/working_memory/action_result/` object.
        *   Include instructions, goals, and definitions of available tools in their simple xml format.
        *   **Crucially: Do NOT include direct content from `storage.json`.**

13. **agent_loop (`src/util/agent_loop.ts`):**
    *   `run_agent()`: The main `async` function with the `while(true)` loop.
        *   Call `generate_system_prompt()`.
        *   Call `call_llm()`.
        *   Call `parse_actions_from_xml()`.
        *   Call `validate_action()` for each action (or validate all then execute).
        *   Call `execute_actions()`.
        *   Implement top_level error handling and continuation logic.

14. **main_entry_point (`src/index.ts`):**
    *   Import and call `run_agent()`.
    *   Basic setup or initialization if any.

15. **refinement_and_testing:**
    *   Write unit tests for utilities and tools.
    *   Write integration tests for the agent loop.
    *   Refine error handling and logging.
    *   Improve documentation (jsdoc, README).

16. **build_and_run_scripts (`package.json`):**
    *   `build`: `tsc`
    *   `start`: `node dist/index.js`
    *   `dev`: `tsc --watch` & `nodemon dist/index.js` (or similar)
    *   `lint`, `format`, `test` scripts.

This order builds foundational components first, then layers more complex logic on top, leading to a well_structured and "beautiful" implementation process.

## 📄 license

This project is licensed under the mit_license.

## 🙏 acknowledgements
```