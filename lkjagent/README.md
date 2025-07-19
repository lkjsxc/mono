# LKJAgent - Autonomous AI Agent System

## Project Overview

LKJAgent is a high-quality autonomous AI agent system implemented in pure C with zero external dependencies. The system operates in a perpetual four-state cycle (thinking, executing, evaluating, paging) with LLM integration and sophisticated memory management.

### Key Features

- **Zero External Dependencies**: Pure C implementation using only standard POSIX libraries
- **Modular Architecture**: Clean separation of concerns with split header files
- **Memory Safety**: Stack-based allocation with bounded buffers and explicit capacity management
- **Tagged Memory System**: Advanced memory management with LLM-directed paging
- **State Machine Architecture**: Four-state autonomous execution cycle
- **Perpetual Operation**: Continuous operation with disk storage enrichment

## Architecture

### Core Components

- **Agent Core**: Main execution engine with perpetual state machine
- **Memory System**: Unified storage with context key management and LLM-directed paging
- **LLM Integration**: Communication with LMStudio using simple tag format
- **Configuration**: Runtime configuration with state-specific system prompts
- **Persistence**: Atomic file operations with backup and recovery
### State Machine

Four-state autonomous execution cycle:

1. **Thinking State**: Deep analysis and strategic planning
2. **Executing State**: Action execution and data gathering  
3. **Evaluating State**: Progress assessment and outcome evaluation
4. **Paging State**: LLM-controlled memory optimization and context management

### Memory Architecture

- **Unified Storage**: Both working memory and disk memory stored in `memory.json`
- **LLM-Directed Paging**: LLM specifies context keys for memory layer transitions
- **Context Key Management**: LLM identifies and tags important context elements
- **Simple Tag Format**: All LLM interactions use `<tag>content</tag>` format

## Building and Running

### Prerequisites

- GCC compiler with C11 support
- Standard POSIX development environment
- Make build system

### Building

```bash
# Standard build
make

# Debug build with symbols
make debug

# Clean build artifacts
make clean
```

### Development Environment

The project works in the provided dev container with Ubuntu 24.04.2 LTS, GCC, debugging tools, and C/C++ development extensions.

## Project Status

### Current Phase: Complete Rebuild

The project requires a complete rebuild from scratch using the comprehensive TODO list files:

- `TODO-01-CORE-INFRASTRUCTURE.md` - Foundation components
- `TODO-02-MEMORY-CONTEXT.md` - Memory and context management
- `TODO-03-LLM-INTEGRATION.md` - LLM communication and parsing
- `TODO-04-STATE-AGENT.md` - State machine and agent core
- `TODO-05-CONFIG-PERSISTENCE.md` - Configuration and persistence
- `TODO-06-UTILITIES.md` - Utility functions and helpers
- `TODO-07-BUILD-TESTING.md` - Build system and testing
- `TODO-08-INTEGRATION-PRODUCTION.md` - Final integration and production

## Getting Started

1. Review the TODO files for comprehensive implementation tasks
2. Start with core infrastructure (TODO-01)
3. Follow the sequential implementation order
4. Maintain quality standards throughout development
5. Test thoroughly at each stage

## License

This project is licensed under the terms specified in the LICENSE file.
