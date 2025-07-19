# LKJAgent Source Code Regeneration TODO List

This document outlines the tasks required to regenerate the complete source code for the LKJAgent system, based on the provided regeneration guide. Each task should be completed following the specified coding styles, memory management rules, and architectural principles.

---

## 📝 Phase 0: Project Setup

- [ ] **Create Project Directories**: Establish the complete directory structure as defined in the `README.md`.
    - `src/`, `src/agent/`, `src/config/`, `src/memory/`, `src/state/`, `src/llm/`, `src/utils/`, `src/persistence/`
    - `build/`, `data/`, `docs/`
- [ ] **Initialize Git**: Set up the `git` repository and create a comprehensive `.gitignore` file to exclude build artifacts and temporary files.
- [ ] **Create Placeholder Data Files**:
    - [ ] `data/config.json`: Create with the full schema, including `lmstudio`, `agent`, `tag_format`, `context_management`, and `http` sections.
    - [ ] `data/memory.json`: Create with the unified schema for working and disk memory.
    - [ ] `data/context_keys.json`: Create with the schema for the context key directory.
- [ ] **Create Makefile**:
    - [ ] Define targets for `all`, `clean`, and `debug`.
    - [ ] Set compiler flags: `-Wall`, `-Wextra`, `-Werror`.
    - [ ] Ensure modular compilation with correct dependencies.
- [ ] **Set Up Testing Infrastructure**:
    - [ ] Create a `tests/` directory.
    - [ ] Write a simple test runner script (e.g., a shell script or Makefile target) to compile and run all tests.

---

## ✅ Phase 1: Core Infrastructure

This phase establishes the foundational utilities and data structures for the entire application.

### `src/lkjagent.h` (Main Header)
- [ ] **Type Definitions**: Define all core types: `result_t`, `data_t`, and structs for configuration, memory, and the agent state. Include types for the tagged memory system, simple tag parsing, and context key management.
- [ ] **API Prototypes**: Declare all public function prototypes with Doxygen comments and `__attribute__((warn_unused_result))`.
- [ ] **Macros**: Define the concrete `RETURN_ERR` macro for error handling.

### `src/utils/data.c` (Data Token Management)
- [ ] **Initialization**: `data_init()` to safely initialize a `data_t` token with a static buffer.
- [ ] **Manipulation**: Implement safe string operations: `data_set()`, `data_append()`, `data_find()`, `data_substring()`.
- [ ] **Validation**: Ensure all functions perform capacity validation to prevent buffer overflows and guarantee null termination.
- [ ] **Context Utilities**: Add functions for managing context width, like `data_trim_context()`.

### `src/utils/tag_parser.c` (Simple Tag Processing)
- [ ] **Parsing**: Implement `tag_parse_block()` to extract content from simple tags like `<thinking>`, `<action>`, etc.
- [ ] **Validation**: Enforce the simple `<tag>content</tag>` format; reject any complex markup.
- [ ] **Extraction**: Create functions to extract specific values, such as context keys from `<context_keys>` tags.

### `src/utils/file_io.c` (File Operations)
- [ ] **Safe I/O**: Implement `file_read_safe()` to read file contents into a `data_t` token and `file_write_atomic()` for safe writes.
- [ ] **Unified Storage**: Ensure functions can handle the unified `memory.json` file.
- [ ] **Directory Management**: Add utilities for checking file existence and managing directories.

### `src/utils/json_parser.c` and `src/utils/json_builder.c` (JSON Processing)
- [ ] **Parser**: Implement manual, type-safe JSON parsing functions to extract values by key-path.
- [ ] **Builder**: Implement functions to construct JSON objects and arrays, including proper string escaping.
- [ ] **State Support**: Ensure the parser can handle state-specific prompts within `config.json`.

### Testing (Phase 1)
- [ ] **`data.c` Unit Tests**:
    - [ ] Test `data_init()` for correct initialization.
    - [ ] Test `data_set()` and `data_append()` at buffer limits.
    - [ ] Test `data_find()` with and without matches.
    - [ ] Test `data_trim_context()` for correct trimming.
- [ ] **`tag_parser.c` Unit Tests**:
    - [ ] Test `tag_parse_block()` with valid and malformed tags.
    - [ ] Test extraction of keys from `<context_keys>`.
- [ ] **`file_io.c` Unit Tests**:
    - [ ] Test `file_read_safe()` with existing and non-existing files.
    - [ ] Test `file_write_atomic()` for atomicity (e.g., by interrupting the process).
- [ ] **`json_parser.c` / `json_builder.c` Unit Tests**:
    - [ ] Test parser with complex, nested, and malformed JSON.
    - [ ] Test builder to ensure it produces valid, correctly escaped JSON.

### `src/config/config_loader.c` (Configuration Management)
- [ ] **Loading**: Implement `config_load()` to read `config.json`, including state-specific system prompts.
- [ ] **Validation**: Add `config_validate()` to check for required fields and correct types.
- [ ] **Defaults**: Implement `config_load_defaults()` to provide fallback values.

### `src/persistence/persist_memory.c` (Memory Persistence)
- [ ] **Memory I/O**: Implement `persist_memory_save()` and `persist_memory_load()` for `memory.json`.
- [ ] **Context Keys**: Add functions to manage the `context_keys.json` directory.
- [ ] **Atomic Writes**: Ensure all write operations are atomic to prevent data corruption.

### Testing (Phase 2)
- [ ] **`config_loader.c` Unit Tests**:
    - [ ] Test `config_load()` with missing files and invalid JSON.
    - [ ] Test `config_validate()` with incomplete and incorrect configurations.
- [ ] **`persist_memory.c` Unit Tests**:
    - [ ] Test saving and loading of `memory.json` and `context_keys.json`.
    - [ ] Verify atomicity of write operations.

---

## 🧠 Phase 2: Memory and Context Layer

This phase builds the agent's memory system and the mechanisms for managing LLM context.

### `src/memory/memory_context.c` (Context Management)
- [ ] **LLM-Directed Paging**: Implement logic for transferring context elements to/from disk based on LLM directives.
- [ ] **Key Management**: Create functions to manage the context key directory (`context_keys.json`).
- [ ] **Context Optimization**: Implement context window sizing and archival to prevent overflow.

### `src/memory/tagged_memory.c` (Tagged Memory)
- [ ] **CRUD Operations**: Implement functions to create, read, update, and delete memory entries with multiple tags.
- [ ] **Query Engine**: Build a query system that supports AND, OR, and NOT operations on tags.
- [ ] **Integration**: Integrate with the context key system for unified memory management.
- [ ] **Maintenance**: Add `memory_cleanup()` for defragmentation.

### `src/utils/http_client.c` (HTTP Client)
- [ ] **Socket Implementation**: Build a non-blocking HTTP client using standard sockets.
- [ ] **Request/Response**: Implement `http_post()` to handle JSON payloads and parse responses.
- [ ] **Sizing**: Add context-aware request sizing to avoid oversized payloads.

### Testing (Phase 3)
- [ ] **`memory_context.c` Unit Tests**:
    - [ ] Test context paging logic with various memory states.
    - [ ] Test management of `context_keys.json`.
- [ ] **`tagged_memory.c` Unit Tests**:
    - [ ] Test CRUD operations thoroughly.
    - [ ] Test complex tag queries (AND, OR, NOT).
    - [ ] Test `memory_cleanup()` for correctness.
- [ ] **`http_client.c` Unit Tests**:
    - [ ] Mock an HTTP server to test `http_post()`.
    - [ ] Test handling of network errors and timeouts.

---

## 🤖 Phase 3: LLM Integration

This phase connects the agent to the Large Language Model.

### `src/llm/llm_client.c` (LLM Client)
- [ ] **Communication**: Implement the client to send requests to the LMStudio API endpoint.
- [ ] **Error Handling**: Add timeout management and robust error handling for network issues.

### `src/llm/llm_prompt.c` (Prompt Manager)
- [ ] **Prompt Construction**: Implement `prompt_build()` to construct prompts using state-specific system prompts from the configuration.
- [ ] **Tag Enforcement**: Ensure all generated prompts instruct the LLM to use the simple tag format.

### `src/llm/llm_parser.c` (Response Parser)
- [ ] **Tag Extraction**: Implement `llm_parse_response()` to extract content from `<thinking>`, `<action>`, and other primary tags.
- [ ] **State Updates**: Ensure the parser can update agent state based on parsed content.

### Testing (Phase 4)
- [ ] **`llm_client.c` Unit Tests**:
    - [ ] Mock the LMStudio API to test request/response cycles.
    - [ ] Test error handling for API-specific errors.
- [ ] **`llm_prompt.c` Unit Tests**:
    - [ ] Test `prompt_build()` for each agent state.
    - [ ] Verify correct inclusion of memory and context.
- [ ] **`llm_parser.c` Unit Tests**:
    - [ ] Test `llm_parse_response()` with various valid and malformed LLM outputs.

---

## 🏃 Phase 4: Agent Logic

This phase implements the agent's core decision-making and action execution loop.

### `src/state/state_machine.c` (State Machine)
- [ ] **State Transitions**: Implement the logic for transitioning between agent states (`THINKING`, `ACTING`, `WAITING`).
- [ ] **State Initialization**: Create `state_init()` to set the initial agent state.

### `src/agent/agent_actions.c` (Action Executor)
- [ ] **Action Dispatch**: Implement a dispatcher to execute actions based on the `<action>` tag.
- [ ] **Action Implementations**: Write functions for core actions like `read_file`, `write_file`, `execute_shell`.
- [ ] **Result Handling**: Ensure action results are stored back into memory correctly.

### Testing (Phase 5)
- [ ] **`state_machine.c` Unit Tests**:
    - [ ] Test all valid and invalid state transitions.
- [ ] **`agent_actions.c` Unit Tests**:
    - [ ] Test each agent action with mocked inputs and outputs.
    - [ ] Test the action dispatcher.

---

## 🚀 Phase 5: Main Loop and Finalization

This phase brings all the components together into a running application.

### `src/lkjagent.c` (Main Entry Point)
- [ ] **Initialization**: `main()` function should initialize all subsystems: config, memory, state.
- [ ] **Main Loop**: Implement the primary `while` loop that drives the agent's state machine.
- [ ] **Shutdown**: Implement `agent_shutdown()` for graceful cleanup of resources.

### Integration Testing
- [ ] **Component Integration**: Write tests to ensure seamless data flow between `config`, `memory`, `persistence`, and `llm` modules.
- [ ] **End-to-End Testing**:
    - [ ] Create test scenarios that run the agent from start to finish.
    - [ ] Example Scenario 1: "Read `file.txt`, summarize it, and write the summary to `summary.txt`."
    - [ ] Example Scenario 2: "List files in the current directory and save the list to `file_list.md`."
- [ ] **Stress Testing**:
    - [ ] Test the agent with large memory files and long conversations to identify performance bottlenecks.
    - [ ] Test with rapid, repeated LLM interactions.