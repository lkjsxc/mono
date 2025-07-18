# LKJAgent

An autonomous AI agent system implemented in C with zero external dependencies.

## Overview

LKJAgent is a perpetual AI agent featuring tagged memory, state machine architecture, and LLM integration. The agent operates continuously, enriching disk storage through a four-state execution cycle.

## Key Features

- **Zero Dependencies**: Pure C with POSIX libraries only
- **Memory Safety**: Stack-based allocation with bounded buffers
- **Tagged Memory**: Advanced memory management with multi-tag queries
- **State Machine**: Four-state autonomous cycle (thinking → executing → evaluating → paging)
- **Perpetual Operation**: Never-ending execution with persistent storage
- **LLM Integration**: Simple tag-based communication format

## Quick Start

```bash
# Build the project
make

# Run the agent
./build/lkjagent

# View configuration
cat data/config.json
```

## Project Structure

```
├── src/                    # Source code
├── docs/                   # Documentation
├── data/                   # Runtime data and configuration
├── scripts/                # Build and utility scripts
└── build/                  # Compiled output
```

## Documentation

- **[Architecture](docs/architecture/README.md)** - System design and core principles
- **[Development](docs/development/README.md)** - Coding standards and build instructions
- **[API Reference](docs/api/README.md)** - Complete API documentation
- **[User Guide](docs/user/README.md)** - Configuration and usage
- **[Implementation](docs/implementation/README.md)** - Detailed implementation guide

## Build Requirements

- GCC or Clang compiler
- POSIX-compliant system
- Make build system

## License

MIT License - see [LICENSE](../LICENSE) for details.
