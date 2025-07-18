# LKJAgent

**An autonomous AI agent system implemented in C with zero external dependencies**

LKJAgent is a sophisticated autonomous AI agent featuring tagged memory, state machine architecture, and LLM integration capabilities. The agent operates continuously, perpetually enriching disk storage with valuable data through intelligent context management and LLM-controlled paging operations.

## Key Features

- **Zero External Dependencies**: Pure C implementation using only standard POSIX libraries
- **Memory Safety**: Stack-based allocation with bounded buffers and explicit capacity management
- **Tagged Memory System**: Advanced memory management with multi-tag queries and LLM integration
- **State Machine Architecture**: Four-state autonomous execution cycle (thinking, executing, evaluating, paging)
- **Perpetual Operation**: Continuous operation without termination, perpetually enriching disk storage
- **LLM-Controlled Paging**: Intelligent context management with LLM-directed memory operations
- **Simple Tag Format**: Standardized LLM output processing using simple `<tag>content</tag>` format

## Quick Start

```bash
# Build the project
make

# Run the agent
./build/lkjagent

# Clean build files
make clean
```

## Project Structure

```
lkjagent/
├── src/                    # Source code
│   ├── lkjagent.h         # Main header with type definitions
│   ├── lkjagent.c         # Application entry point
│   ├── agent/             # Agent core functionality
│   ├── config/            # Configuration management
│   ├── memory/            # Memory and context management
│   ├── state/             # State machine implementation
│   ├── llm/               # LLM client and processing
│   ├── utils/             # Utility functions
│   └── persistence/       # Data persistence layer
├── data/                  # Runtime data files
│   ├── config.json        # Configuration with state prompts
│   ├── memory.json        # Unified memory storage
│   └── context_keys.json  # Context key directory
├── docs/                  # Detailed documentation
├── build/                 # Compiled files
└── Makefile              # Build configuration
```

## Configuration

The agent is configured through `data/config.json` with support for:
- LMStudio integration settings
- State-specific system prompts
- Memory management parameters
- Context width and paging controls
- Tag format validation rules

## Documentation

Comprehensive documentation is available in the `docs/` directory:

- **[Architecture Overview](docs/architecture.md)** - System design and structure
- **[Agent Philosophy](docs/agent-philosophy.md)** - Operational principles and memory architecture
- **[Coding Style Guidelines](docs/coding-style.md)** - Code standards and patterns
- **[Module Requirements](docs/module-requirements.md)** - Detailed implementation requirements
- **[Configuration Schemas](docs/configuration-schemas.md)** - Data format specifications
- **[Implementation Guide](docs/implementation-guide.md)** - Step-by-step development guide

## Agent States

The agent operates in four continuous states:

1. **THINKING** - Deep analysis and strategic planning with context key identification
2. **EXECUTING** - Action execution with disk storage enrichment
3. **EVALUATING** - Progress assessment and quality metrics
4. **PAGING** - LLM-controlled context and memory management

## Memory Architecture

- **Unified Storage**: Both working and disk memory stored in `memory.json`
- **Context Keys**: LLM-identified context elements for efficient management
- **Smart Paging**: LLM determines what to preserve, archive, or retrieve
- **Perpetual Enrichment**: Continuous accumulation of valuable data

## LLM Integration

- **Simple Tag Format**: Standardized `<tag>content</tag>` output processing
- **State-Specific Prompts**: Customized system prompts for each agent state
- **Context Management**: Intelligent context width control with LLM guidance
- **Tag Validation**: Strict format enforcement for reliable parsing

## Build System

The Makefile provides:
- Modular compilation with proper dependencies
- Debug symbol generation
- Warning-as-error enforcement (`-Wall -Wextra -Werror`)
- Clean build targets

## Quality Standards

- **Memory Safety**: No dynamic allocation, bounded operations
- **Error Handling**: Comprehensive error propagation with concrete `RETURN_ERR` implementation
- **Documentation**: Complete Doxygen documentation for all functions
- **Testing**: Validation of all API functions and error conditions

## License

See [LICENSE](LICENSE) file for details.

---

For detailed implementation instructions and technical specifications, please refer to the documentation in the `docs/` directory.
