# LKJAgent Source Code Regeneration TODO List

This document outlines the tasks required to regenerate the complete source code for the LKJAgent system, based on the provided regeneration guide. Each task should be completed following the specified coding styles, memory management rules, and architectural principles.

---

## 📝 Phase 0: Project Setup

- [ ] Create the full project directory structure as specified in the guide.
- [ ] Initialize `git` repository and add a `.gitignore` file.
- [ ] Create initial placeholder data files:
    - [ ] `data/config.json` (with the full schema)
    - [ ] `data/memory.json` (with the full schema)
    - [ ] `data/context_keys.json` (with the full schema)
- [ ] Create the initial `Makefile` with basic targets (`all`, `clean`, `debug`) and compiler flags (`-Wall`, `-Wextra`, `-Werror`).

---

## ✅ Phase 1: Core Infrastructure

This phase establishes the foundational utilities and data structures for the entire application.

### `src/lkjagent.h` (Main Header)
- [ ] Define `result_t` enum.
- [ ] Define `data_t` struct for safe string handling.
- [ ] Define the `RETURN_ERR` macro with the concrete `write(STDERR_FILENO, ...)` implementation.
- [ ] Declare all core structs for configuration (`config_t`, `lmstudio_config_t`, etc.).
- [ ] Declare all core structs for memory (`memory_t`, `memory_entry_t`, etc.).
- [ ] Declare all core structs for the agent (`lkjagent_t`, `agent_state_t`).
- [ ] Declare function prototypes for all public APIs from all modules.

### `src/utils/data.c` (Data Token Management)
- [ ] Implement `data_init()` to initialize a `data_t` token with a static buffer.
- [ ] Implement safe string manipulation functions (`data_set`, `data_append`, `data_copy`).
- [ ] Implement find/substring functions (`data_find`, `data_substring`).
- [ ] Implement comparison functions (`data_equals`, `data_starts_with`).
- [ ] Implement `data_trim_context` and `data_trim_front` for context width management.
- [ ] Ensure all functions perform strict bounds checking to prevent overflows.
- [ ] Ensure all functions guarantee null termination.

### `src/utils/tag_processor.c` (Simple Tag Processing)
- [ ] Implement a function to extract the content of a specific simple tag (e.g., `<tag>content</tag>`).
- [ ] Implement a function to extract larger blocks (`<thinking>`, `<action>`, etc.).
- [ ] Implement extraction for comma-separated values like `context_keys`.
- [ ] Add robust error handling for malformed or missing tags.
- [ ] Strictly enforce the "simple tag only" rule; reject any complex markup.

### `src/utils/file.c` (File Operations)
- [ ] Implement `file_read_to_data` to safely read entire file contents into a `data_t`.
- [ ] Implement `file_write_from_data` for atomic file writes (e.g., write to temp then rename).
- [ ] Implement `file_exists` and directory management functions.
- [ ] Ensure all file operations have comprehensive error handling.

### `src/utils/json.c` (JSON Processing)
- [ ] Implement a lightweight, dependency-free JSON parser.
- [ ] Implement a function to extract values by key-path (e.g., "agent.llm_decisions.context_window_size").
- [ ] Implement type-safe retrieval functions for strings, numbers, and booleans.
- [ ] Implement a lightweight JSON generator for saving configuration and memory state.
- [ ] Add validation for expected JSON structure and types.

### `src/config/` (Configuration Management)
- [ ] **`defaults.c`**: Implement a function to populate the `config_t` struct with default values.
- [ ] **`validation.c`**: Implement a function to validate the loaded configuration (e.g., check for valid URLs, positive numbers).
- [ ] **`config.c`**: Implement `config_load()` to read `data/config.json`, parse it, and populate the `config_t` struct, including state-specific prompts.
- [ ] **`config.c`**: Implement `config_save()` to write the current configuration back to `data/config.json`.

### `src/persistence/` (Persistence Layer)
- [ ] **`memory_persistence.c`**: Implement `memory_persistence_load()` and `memory_persistence_save()` for `data/memory.json`.
- [ ] **`memory_persistence.c`**: Implement logic to manage the context key directory within `memory.json`.
- [ ] **`config_persistence.c`**: Implement persistence logic for `config.json`.
- [ ] **`disk_operations.c`**: Implement low-level disk I/O for context stored in `data/disk_context/`.

---

## 🧠 Phase 2: Memory and Context Layer

This phase builds the agent's memory system and the mechanisms for managing LLM context.

### `src/memory/context_manager.c`
- [ ] Implement logic to track current context window size.
- [ ] Implement LLM-directed context key identification.
- [ ] Implement functions to transfer context elements to and from disk based on keys.
- [ ] Implement maintenance of the `context_keys.json` directory.
- [ ] Implement `manage_context_width_transition` as described in the guide.

### `src/memory/tagged_memory.c`
- [ ] Implement data structures for memory entries with multiple tags.
- [ ] Implement functions to add, retrieve, and delete memory entries.
- [ ] Implement the complex query system (AND, OR, NOT operations on tags).
- [ ] Implement memory cleanup and defragmentation logic.
- [ ] Integrate with the context key system for unified memory.

### `src/utils/http.c` (HTTP Client)
- [ ] Implement a socket-based HTTP client (no `libcurl` or other dependencies).
- [ ] Implement `http_post` function to handle JSON payloads for the LMStudio API.
- [ ] Implement timeout management using `select()` or `poll()`.
- [ ] Implement response parsing to separate headers and body content.

---

## 🤖 Phase 3: LLM Integration

This phase connects the agent to the Large Language Model.

### `src/llm/` (LLM Client Architecture)
- [ ] **`context_builder.c`**: Implement functions to prepare the full context string to be sent to the LLM.
- [ ] **`prompt_manager.c`**: Implement logic to construct the final prompt, combining the state-specific system prompt with the current context.
- [ ] **`llm_client.c`**: Implement `llm_send_request()` to use the HTTP client to send the request to LMStudio and receive the response.
- [ ] **`response_parser.c`**: Implement functions to parse the LLM's simple tag response, validate its format, and extract blocks and context keys.

---

## 🔄 Phase 4: State Machine & Core Logic

This phase implements the agent's autonomous behavior and state management.

### `src/state/` (State Implementations)
- [ ] **`enhanced_states.c`**: Implement shared utilities for state transitions, including context width management calls.
- [ ] **`thinking.c`**: Implement `state_thinking_execute()`. This function should build the prompt, call the LLM, parse the `<thinking>` block, and update the agent's memory.
- [ ] **`executing.c`**: Implement `state_executing_execute()`. This function should parse the `<action>` block from the previous state and perform the specified disk enrichment operations.
- [ ] **`evaluating.c`**: Implement `state_evaluating_execute()`. This function should build a prompt to request evaluation, call the LLM, and parse the `<evaluation>` block.
- [ ] **`paging.c`**: Implement `state_paging_execute()` and `llm_request_context_paging()`. This function will call the LLM with a paging prompt and execute the returned directives (`move_to_disk`, etc.) by calling the context manager.

### `src/agent/` (Agent Core)
- [ ] **`decision.c`**: Implement logic for decision-making based on LLM output.
- [ ] **`execution.c`**: Implement the engine for performing tasks described in `<action>` tags.
- [ ] **`evaluation.c`**: Implement logic to process metrics from the `<evaluation>` tag.
- [ ] **`core.c`**: Implement the main agent state machine loop (`agent_run()`). The loop should call the appropriate state function, handle the state transition, and run perpetually (`max_iterations: -1`).

### `src/memory/enhanced_llm.c`
- [ ] Implement LLM-driven memory decision functions (e.g., suggesting tags, deciding on retention strategies).

---

## ⚙️ Phase 5: Finalization & Main Application

This phase completes the remaining utilities and the application entry point.

### `src/utils/` (Utility Extensions)
- [ ] **`string_utils.c`**: Implement any additional advanced string operations needed.
- [ ] **`time_utils.c`**: Implement timestamp and duration utilities.

### `src/lkjagent.c` (Main Application)
- [ ] Implement the `main()` function.
- [ ] Initialize the agent (`agent_init()`), including loading configuration and memory.
- [ ] Call the main execution loop (`agent_run()`).
- [ ] Implement signal handling (e.g., `SIGINT`, `SIGTERM`) for graceful state saving.
- [ ] Add top-level error handling for critical failures during initialization.

---

## 🔬 Phase 6: Validation and Testing

This phase ensures the final application is robust, correct, and meets all quality standards.

- [ ] Update `Makefile` to correctly compile all source files with all dependencies.
- [ ] **Compilation**: Ensure the project compiles with zero warnings under `-Wall -Wextra -Werror`.
- [ ] **Unit Testing**: Create a test suite to validate each module's functionality.
    - [ ] Test `data.c` for buffer overflow edge cases.
    - [ ] Test `json.c` and `tag_processor.c` with malformed inputs.
    - [ ] Test the HTTP client against a mock server.
- [ ] **Integration Testing**:
    - [ ] Verify the full state machine cycle (Thinking -> Executing -> Evaluating -> Paging -> Thinking).
    - [ ] Confirm context is correctly passed between states.
    - [ ] Test the LLM-controlled paging mechanism by forcing the context window to its limit.
    - [ ] Verify that `memory.json` and `context_keys.json` are read and written correctly.
- [ ] **Documentation**: Run Doxygen to generate documentation and fix any warnings.
- [ ] **Code Review**: Perform a final code review to ensure all coding standards, memory safety patterns, and error handling rules have been consistently followed.