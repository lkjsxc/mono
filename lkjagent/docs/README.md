# LKJAgent - AI State Machine Documentation

## Overview

LKJAgent is a sophisticated AI agent system implemented in C that operates as a finite state machine with dynamic memory management. It integrates with Large Language Models (LLMs) to provide intelligent reasoning, decision-making, and memory management capabilities.

## Key Features

- **Modular State Machine Architecture**: Four distinct states (thinking, executing, evaluating, paging) with automatic transitions
- **Reflection and Assessment**: Mandatory evaluating state provides continuous progress assessment
- **Memory Management**: Dual-layer memory system with working memory and persistent storage
- **LLM Integration**: Communicates with external language models via HTTP REST API
- **Memory Pool System**: Custom memory allocator for efficient resource management
- **JSON/XML Processing**: Robust parsing and generation capabilities
- **Docker Support**: Containerized deployment with minimal attack surface
- **Modular Codebase**: Separated concerns across specialized modules for maintainability

## Architecture Components

1. **Agent Core** (`src/agent/`)
   - **Core Orchestration** (`core.c`, `core.h`) - Main processing cycle coordination
   - **State Management** (`state.c`, `state.h`) - State transitions and memory-aware logic
   - **Prompt Generation** (`prompt.c`, `prompt.h`) - LLM prompt construction and formatting
   - **HTTP Communication** (`http.c`, `http.h`) - LLM interaction and response processing
   - **Action Execution** (`actions.c`, `actions.h`) - Memory operations and persistence

2. **Memory Pool System** (`src/utils/pool.c`, `src/utils/pool.h`)
   - Custom memory allocator with multiple size pools
   - Efficient memory reuse and garbage collection
   - Pre-allocated buffers for different object sizes

3. **Object System** (`src/utils/object.c`, `src/utils/object.h`)
   - JSON/XML parsing and serialization
   - Hierarchical object manipulation
   - Path-based object access

4. **Utility Libraries**
   - String manipulation (`src/utils/string.c`, `src/utils/string.h`)
   - File I/O operations (`src/utils/file.c`, `src/utils/file.h`)
   - HTTP client (`src/utils/http.c`, `src/utils/http.h`)

5. **Global Definitions**
   - Constants (`src/global/const.h`)
   - Type definitions (`src/global/types.h`)
   - Macros (`src/global/macro.h`)
   - Standard includes (`src/global/std.h`)

## Documentation Structure

- [Architecture Overview](./architecture.md) - System design and component relationships
- [Modular Architecture](./modular-architecture.md) - Detailed breakdown of the new modular design
- [Memory Management](./memory-management.md) - Pool allocator and object lifecycle
- [State Machine](./state-machine.md) - Agent states and transition logic (including new evaluating state)
- [LLM Integration](./llm-integration.md) - Communication protocol and prompt engineering
- [Configuration](./configuration.md) - System configuration and parameters
- [API Reference](./api-reference.md) - Function documentation and usage
- [Development Guide](./development.md) - Building, testing, and extending the system
- [Deployment Guide](./deployment.md) - Docker deployment and operational considerations

## Quick Start

```bash
# Build the agent
make

# Run with default configuration
./build/lkjagent

# Build and run with Docker
docker build -t lkjagent .
docker run -v $(pwd)/data:/app/data lkjagent
```

## System Requirements

- GCC 12 or compatible C11 compiler
- POSIX-compliant system (Linux, macOS, WSL)
- Accessible LLM endpoint (OpenAI API compatible)
- Minimum 4GB RAM for optimal memory pool operation

## Configuration Files

- `data/config.json` - System configuration, LLM endpoints, agent parameters
- `data/memory.json` - Agent memory state, working memory, and storage
