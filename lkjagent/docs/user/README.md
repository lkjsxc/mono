# User Guide

Complete guide for configuring and using LKJAgent.

## Installation

### Prerequisites

- GCC or Clang compiler
- POSIX-compliant system (Linux, macOS, Unix)
- Make build system

### Building

```bash
# Clone and build
cd /workspaces/mono/lkjagent
make

# Clean build
make clean
```

## Configuration

### Main Configuration (`data/config.json`)

```json
{
  "lmstudio": {
    "base_url": "http://host.docker.internal:1234/v1/chat/completions",
    "model": "qwen/qwen3-8b",
    "temperature": 0.7,
    "max_tokens": 2048,
    "timeout_ms": 30000
  },
  "agent": {
    "max_iterations": -1,
    "self_directed": true,
    "state_prompts": {
      "thinking": {
        "system_prompt": "You are in THINKING state...",
        "objectives": ["deep_analysis", "strategic_planning"]
      },
      "executing": {
        "system_prompt": "You are in EXECUTING state...",
        "objectives": ["disk_enrichment", "action_execution"]
      },
      "evaluating": {
        "system_prompt": "You are in EVALUATING state...",
        "objectives": ["progress_assessment", "quality_measurement"]
      },
      "paging": {
        "system_prompt": "You are in PAGING state...",
        "objectives": ["context_management", "memory_optimization"]
      }
    }
  }
}
```

### Memory Configuration

The agent uses unified storage in `data/memory.json`:

- **Working Memory**: Current context and active data
- **Disk Memory**: Persistent storage with tagged entries
- **Context Keys**: LLM-directed memory management

### Context Management

Context keys in `data/context_keys.json` enable:
- Efficient memory paging
- LLM-controlled data organization
- Automatic cleanup and archival

## Usage

### Running the Agent

```bash
# Start the agent
./build/lkjagent

# The agent runs perpetually through four states:
# 1. THINKING - Analysis and planning
# 2. EXECUTING - Action performance
# 3. EVALUATING - Progress assessment  
# 4. PAGING - Memory management
```

### Monitoring

View agent status:

```bash
# Check memory usage
cat data/memory.json | jq '.context_management.current_usage'

# View context keys
cat data/context_keys.json | jq '.usage_statistics'

# Monitor agent state
tail -f agent.log
```

### LLM Output Format

The agent uses simple tag format for LLM communication:

```
<thinking>
<analysis>Current situation analysis</analysis>
<planning>Next steps and strategy</planning>
<context_keys>key1,key2,key3</context_keys>
</thinking>

<action>
<type>disk_storage</type>
<context_key>specific_key</context_key>
<operation>store</operation>
<data>content to store</data>
</action>
```

## Troubleshooting

### Common Issues

**Agent won't start:**
- Check LMStudio is running on configured port
- Verify configuration file syntax
- Ensure write permissions for data directory

**Memory issues:**
- Monitor context window usage
- Check disk space for memory.json
- Review context key directory integrity

**LLM communication errors:**
- Verify LMStudio model is loaded
- Check network connectivity
- Review timeout settings

### Debug Mode

Build with debug symbols:

```bash
make debug
gdb ./build/lkjagent
```

### Log Analysis

Check error patterns:

```bash
grep "Error:" agent.log
grep "RETURN_ERR" agent.log
```
