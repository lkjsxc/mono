# TODO-01: Core Infrastructure Implementation

## Overview
This file contains the foundational infrastructure tasks that must be completed first. These components form the backbone of the LKJAgent system and are prerequisites for all other modules.

## Core Type System

### 1. Main Entry Header (`src/lkjagent.h`)
- [ ] Create main header file with complete type definitions
- [ ] Define core enums: `result_t`, `agent_state_t`, `memory_layer_t`
- [ ] Define struct forward declarations: `lkjagent_t`, `tagged_memory_t`, `config_t`, `data_t`
- [ ] Include all system headers: `stdio.h`, `stdlib.h`, `string.h`, `stdbool.h`, `stdint.h`, `time.h`, `unistd.h`
- [ ] Include all modular component headers from `include/` directory
- [ ] Define core constants: `MAX_DATA_SIZE`, `MAX_FILENAME_SIZE`, `MAX_TAG_SIZE`, `MAX_CONTEXT_KEYS`
- [ ] Define `RETURN_ERR()` macro for consistent error reporting
- [ ] Define `__attribute__((warn_unused_result))` for all function declarations

### 2. Core Types Header (`src/include/types.h`)
- [ ] Define `result_t` enum with `RESULT_OK` and `RESULT_ERR`
- [ ] Define `agent_state_t` enum: `STATE_THINKING`, `STATE_EXECUTING`, `STATE_EVALUATING`, `STATE_PAGING`
- [ ] Define `memory_layer_t` enum: `LAYER_WORKING`, `LAYER_DISK`, `LAYER_ARCHIVED`
- [ ] Define `data_t` structure with `char* data`, `size_t size`, `size_t capacity`
- [ ] Define `context_key_t` structure with `char key[MAX_TAG_SIZE]`, `memory_layer_t layer`, `size_t importance_score`
- [ ] Define core buffer size constants and validation macros
- [ ] Define forward declarations for all major structures
- [ ] Define function pointer types for state handlers and callbacks

### 3. Data Management Foundation (`src/utils/data.c`)
- [ ] Implement `data_init()` for safe buffer initialization
- [ ] Implement `data_set()` for safe string assignment with bounds checking
- [ ] Implement `data_append()` for safe string concatenation
- [ ] Implement `data_trim_front()` for context window management
- [ ] Implement `data_trim_context()` for LLM context size management
- [ ] Implement `data_clear()` for buffer reset
- [ ] Implement `data_copy()` for safe buffer copying
- [ ] Implement `data_validate()` for buffer integrity checking
- [ ] Add comprehensive parameter validation for all functions
- [ ] Include detailed Doxygen documentation with usage examples

### 4. Simple Tag Format Parser (`src/utils/tag_parser.c`)
- [ ] Implement `tag_parse_simple()` for basic `<tag>content</tag>` extraction
- [ ] Implement `tag_extract_content()` for content between tags
- [ ] Implement `tag_parse_thinking()` for `<thinking>` block processing
- [ ] Implement `tag_parse_action()` for `<action>` block processing
- [ ] Implement `tag_parse_evaluation()` for `<evaluation>` block processing
- [ ] Implement `tag_parse_paging()` for `<paging>` block processing
- [ ] Implement `tag_parse_context_keys()` for context key extraction from LLM responses
- [ ] Implement `tag_validate_format()` for simple tag format validation
- [ ] Implement `tag_parse_paging_directives()` for LLM paging instruction processing
- [ ] Add comprehensive error handling for malformed tag formats

### 5. File I/O Foundation (`src/utils/file_io.c`)
- [ ] Implement `file_read_all()` for complete file reading with size limits
- [ ] Implement `file_write_atomic()` for safe file writing with backup
- [ ] Implement `file_exists()` for file existence checking
- [ ] Implement `file_size()` for file size determination
- [ ] Implement `file_backup()` for creating backup copies
- [ ] Implement `file_ensure_directory()` for directory creation
- [ ] Implement `file_lock()` and `file_unlock()` for concurrent access protection
- [ ] Add support for `memory.json` and `context_keys.json` file handling
- [ ] Implement atomic operations for configuration and memory persistence
- [ ] Include comprehensive error handling for all file operations

## JSON Processing Infrastructure

### 6. JSON Parser (`src/utils/json_parser.c`)
- [ ] Implement `json_parse_object()` for JSON object parsing
- [ ] Implement `json_parse_array()` for JSON array parsing
- [ ] Implement `json_parse_string()` for string value extraction
- [ ] Implement `json_parse_number()` for numeric value extraction
- [ ] Implement `json_parse_boolean()` for boolean value extraction
- [ ] Implement `json_find_key()` for key lookup in objects
- [ ] Implement `json_validate_structure()` for JSON structure validation
- [ ] Add support for parsing `memory.json` unified storage format
- [ ] Add support for parsing `context_keys.json` directory format
- [ ] Include robust error handling for malformed JSON input

### 7. JSON Builder (`src/utils/json_builder.c`)
- [ ] Implement `json_build_object()` for JSON object construction
- [ ] Implement `json_build_array()` for JSON array construction
- [ ] Implement `json_add_string()` for string field addition
- [ ] Implement `json_add_number()` for numeric field addition
- [ ] Implement `json_add_boolean()` for boolean field addition
- [ ] Implement `json_build_memory()` for `memory.json` format generation
- [ ] Implement `json_build_context_keys()` for context key directory generation
- [ ] Implement `json_build_config()` for configuration file generation
- [ ] Add proper JSON escaping for string values
- [ ] Include memory safety checks for all builder operations

## Configuration Management

### 8. Configuration Loader (`src/config/config_loader.c`)
- [ ] Implement `config_load()` for configuration file loading
- [ ] Implement `config_load_defaults()` for default value initialization
- [ ] Implement `config_validate()` for configuration validation
- [ ] Implement `config_get_state_prompt()` for state-specific system prompt retrieval
- [ ] Implement `config_get_llm_settings()` for LLM communication parameters
- [ ] Implement `config_get_memory_settings()` for memory management configuration
- [ ] Add support for loading state-specific system prompts (thinking, executing, evaluating, paging)
- [ ] Add support for LLMStudio API endpoint and authentication configuration
- [ ] Include comprehensive validation for all configuration parameters
- [ ] Add fallback mechanisms for missing or invalid configuration values

### 9. Memory Persistence Foundation (`src/persistence/persist_memory.c`)
- [ ] Implement `persist_memory_load()` for loading `memory.json` unified storage
- [ ] Implement `persist_memory_save()` for saving `memory.json` with atomic operations
- [ ] Implement `persist_context_keys_load()` for loading context key directory
- [ ] Implement `persist_context_keys_save()` for saving context key directory
- [ ] Implement `persist_memory_backup()` for creating backup copies
- [ ] Implement `persist_memory_recover()` for recovery from corruption
- [ ] Add support for unified storage format with working and disk memory layers
- [ ] Add support for context key metadata persistence (importance scores, timestamps)
- [ ] Include integrity checking for all persistence operations
- [ ] Add proper error recovery and rollback mechanisms

## Foundation Validation

### 10. Integration Testing Infrastructure
- [ ] Create test harness for core infrastructure validation
- [ ] Test `data_t` operations with boundary conditions
- [ ] Test simple tag format parsing with various inputs
- [ ] Test JSON parsing and building with complex structures
- [ ] Test configuration loading with valid and invalid inputs
- [ ] Test file I/O operations with permission and space constraints
- [ ] Test memory persistence with corruption scenarios
- [ ] Validate error propagation through all layers
- [ ] Test concurrent access scenarios for file operations
- [ ] Verify memory safety with valgrind integration

### 11. Documentation and Standards
- [ ] Complete Doxygen documentation for all functions
- [ ] Add usage examples for each core component
- [ ] Document error handling patterns and best practices
- [ ] Create API reference documentation
- [ ] Document JSON format specifications for `memory.json` and `context_keys.json`
- [ ] Document simple tag format specification for LLM interactions
- [ ] Create development workflow documentation
- [ ] Add code style guide and formatting standards
- [ ] Document testing procedures and quality gates
- [ ] Create troubleshooting guide for common issues

## Success Criteria
- [ ] All core infrastructure compiles without warnings
- [ ] Complete test coverage for all foundation components
- [ ] Zero memory leaks detected by valgrind
- [ ] All functions include comprehensive error handling
- [ ] Complete API documentation with examples
- [ ] JSON parsing handles all edge cases gracefully
- [ ] Simple tag format parsing validates LLM responses correctly
- [ ] Configuration system supports all required parameters
- [ ] Memory persistence maintains data integrity
- [ ] File I/O operations are atomic and safe
