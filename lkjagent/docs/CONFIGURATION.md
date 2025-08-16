# LKJAgent Configuration Guide

This comprehensive guide covers all configuration options for LKJAgent, including detailed explanations, examples, and best practices for setting up and tuning your AI agent.

## Table of Contents

- [Configuration Overview](#configuration-overview)
- [Configuration File Structure](#configuration-file-structure)
- [LLM Configuration](#llm-configuration)
- [Agent Configuration](#agent-configuration)
- [State Configuration](#state-configuration)
- [Logging Configuration](#logging-configuration)
- [Memory Management Configuration](#memory-management-configuration)
- [Performance Tuning](#performance-tuning)
- [Environment-Specific Configurations](#environment-specific-configurations)
- [Troubleshooting Configuration Issues](#troubleshooting-configuration-issues)

## Configuration Overview

LKJAgent uses a JSON-based configuration system located in `data/config.json`. The configuration is hierarchically structured and loaded at startup. Changes to the configuration require restarting the agent.

### Key Configuration Principles

1. **Hierarchical Structure**: Configuration is organized in logical groups (llm, agent, etc.)
2. **Type Safety**: All values are validated during parsing
3. **Sensible Defaults**: Missing optional values fall back to reasonable defaults
4. **Environment Flexibility**: Support for different deployment scenarios

### Configuration Loading Process

1. **Startup**: Configuration loaded from `data/config.json`
2. **Validation**: Structure and required fields validated
3. **Default Application**: Missing optional fields set to defaults
4. **Runtime Access**: Configuration accessible throughout agent lifecycle

## Configuration File Structure

The configuration file is structured as follows:

```json
{
  "version": "1.0.0",
  "llm": { /* LLM settings */ },
  "agent": { /* Agent behavior settings */ }
}
```

### Root Level Properties

#### `version` (string, required)
```json
{
  "version": "1.0.0"
}
```

**Description**: Configuration schema version for compatibility checking.

**Valid Values**: 
- `"1.0.0"`: Current version

**Purpose**: Ensures configuration compatibility across agent versions.

## LLM Configuration

The `llm` section configures communication with Large Language Model endpoints.

### Complete LLM Configuration

```json
{
  "llm": {
    "endpoint": "http://host.docker.internal:1234/v1/chat/completions",
    "model": "qwen/qwen3-8b",
    "temperature": 0.7,
    "max_tokens": 2048,
    "timeout": 30,
    "retry_attempts": 3,
    "retry_delay": 5
  }
}
```

### LLM Properties

#### `endpoint` (string, required)
```json
{
  "llm": {
    "endpoint": "http://host.docker.internal:1234/v1/chat/completions"
  }
}
```

**Description**: HTTP endpoint for LLM API communication.

**Examples**:
- Local LLM server: `"http://localhost:1234/v1/chat/completions"`
- Docker host: `"http://host.docker.internal:1234/v1/chat/completions"`
- Remote API: `"https://api.openai.com/v1/chat/completions"`
- Custom endpoint: `"https://my-llm-server.com/api/v1/chat"`

**Requirements**:
- Must be a valid HTTP/HTTPS URL
- Endpoint must support OpenAI-compatible chat completions API
- Must be accessible from agent runtime environment

#### `model` (string, required)
```json
{
  "llm": {
    "model": "qwen/qwen3-8b"
  }
}
```

**Description**: Model identifier to use for LLM requests.

**Examples**:
- Qwen models: `"qwen/qwen3-8b"`, `"qwen/qwen3-14b"`
- OpenAI models: `"gpt-4"`, `"gpt-3.5-turbo"`
- Local models: `"llama2-7b"`, `"mistral-7b"`
- Custom models: `"my-custom-model"`

**Requirements**:
- Model must be available on the configured endpoint
- Model should support structured output for best results

#### `temperature` (number, optional, default: 0.7)
```json
{
  "llm": {
    "temperature": 0.7
  }
}
```

**Description**: Controls randomness in LLM responses.

**Range**: `0.0` to `2.0`
- `0.0`: Deterministic, focused responses
- `0.7`: Balanced creativity and consistency (recommended)
- `1.0`: High creativity
- `2.0`: Maximum randomness

**Recommendations**:
- **Analytical tasks**: `0.1` to `0.3`
- **General agent work**: `0.5` to `0.8`
- **Creative tasks**: `0.8` to `1.5`

#### `max_tokens` (integer, optional, default: 2048)
```json
{
  "llm": {
    "max_tokens": 2048
  }
}
```

**Description**: Maximum tokens for LLM responses.

**Considerations**:
- Higher values allow longer responses but increase latency
- Lower values reduce costs but may truncate important information
- Should be coordinated with context window management

#### `timeout` (integer, optional, default: 30)
```json
{
  "llm": {
    "timeout": 30
  }
}
```

**Description**: HTTP timeout in seconds for LLM requests.

**Recommendations**:
- **Fast local models**: `10` to `20` seconds
- **Remote APIs**: `30` to `60` seconds
- **Large models**: `60` to `120` seconds

## Agent Configuration

The `agent` section controls core agent behavior, memory management, and state transitions.

### Complete Agent Configuration

```json
{
  "agent": {
    "think_log": {
      "enable": true,
      "max_entries": 4,
      "key_prefix": "think_log_"
    },
    "evaluation_log": {
      "enable": true,
      "max_entries": 1,
      "key_prefix": "evaluation_log_"
    },
    "command_log": {
      "enable": true,
      "max_entries": 1,
      "key_prefix": "command_log_"
    },
    "paging_limit": {
      "enable": false,
      "max_tokens": 1024
    },
    "hard_limit": {
      "enable": false,
      "max_tokens": 2048
    },
    "iteration": {
      "max_iterations": 10
    },
    "state": {
      "thinking": {
        "prompt": "You are an AI agent..."
      },
      "evaluating": {
        "prompt": "Evaluate your progress..."
      },
      "commanding": {
        "prompt": "Execute actions..."
      }
    }
  }
}
```

### Iteration Control

#### `iteration.max_iterations` (integer, optional, default: 5)
```json
{
  "agent": {
    "iteration": {
      "max_iterations": 10
    }
  }
}
```

**Description**: Maximum number of agent cycles per run.

**Considerations**:
- Higher values allow more complex task completion
- Lower values reduce resource usage and runtime
- Set based on typical task complexity

**Recommendations**:
- **Simple tasks**: `3` to `5` iterations
- **Complex tasks**: `10` to `20` iterations
- **Continuous operation**: `50` to `100` iterations

## State Configuration

Each agent state has its own configuration section defining prompts and behavior.

### State Structure

```json
{
  "agent": {
    "state": {
      "thinking": {
        "prompt": "Detailed thinking prompt...",
        "max_response_tokens": 1024,
        "temperature_override": 0.8
      },
      "evaluating": {
        "prompt": "Detailed evaluation prompt...",
        "max_response_tokens": 512,
        "temperature_override": 0.5
      },
      "commanding": {
        "prompt": "Detailed commanding prompt...",
        "max_response_tokens": 256,
        "temperature_override": 0.3
      }
    }
  }
}
```

### Thinking State Configuration

```json
{
  "thinking": {
    "prompt": "You are an AI agent in thinking mode. Your role is to:\n\n1. Analyze the current situation and available information\n2. Consider multiple approaches to the task\n3. Plan your next actions carefully\n4. Decide on the most appropriate next state\n\nCurrent task: {{task}}\nWorking memory: {{working_memory}}\nRecent logs: {{think_log}}\n\nPlease provide your analysis and determine your next action. If you need to gather information, transition to 'commanding' state. If you need to assess progress, transition to 'evaluating' state.\n\nRespond in JSON format:\n{\n  \"analysis\": \"Your detailed analysis\",\n  \"next_state\": \"thinking|evaluating|commanding\",\n  \"reasoning\": \"Why you chose this state\"\n}"
  }
}
```

### Evaluating State Configuration

```json
{
  "evaluating": {
    "prompt": "You are an AI agent in evaluation mode. Your role is to:\n\n1. Assess progress toward the current task\n2. Identify what has been accomplished\n3. Determine what still needs to be done\n4. Decide on the next course of action\n\nTask: {{task}}\nCommand log: {{command_log}}\nEvaluation history: {{evaluation_log}}\n\nEvaluate your progress and determine next steps.\n\nRespond in JSON format:\n{\n  \"evaluation\": \"Assessment of current progress\",\n  \"accomplished\": [\"list of completed items\"],\n  \"remaining\": [\"list of remaining tasks\"],\n  \"next_state\": \"thinking|commanding\",\n  \"confidence\": 0.8\n}"
  }
}
```

### Commanding State Configuration

```json
{
  "commanding": {
    "prompt": "You are an AI agent in command execution mode. Your role is to:\n\n1. Execute specific actions to advance toward your goal\n2. Use available actions to manipulate memory and storage\n3. Gather or store information as needed\n4. Always transition to 'evaluating' after completing actions\n\nAvailable actions:\n- working_memory_add: Add information to working memory\n- working_memory_remove: Remove information by tags\n- storage_save: Save information to long-term storage\n- storage_load: Load information from storage by tags\n- storage_search: Search storage for information\n\nCurrent context:\nTask: {{task}}\nWorking memory: {{working_memory}}\nStorage: {{storage}}\n\nExecute your planned action.\n\nRespond in JSON format:\n{\n  \"action\": {\n    \"type\": \"action_type\",\n    \"tags\": [\"tag1\", \"tag2\"],\n    \"value\": \"action_value\"\n  },\n  \"reasoning\": \"Why you chose this action\",\n  \"next_state\": \"evaluating\"\n}"
  }
}
```

## Logging Configuration

LKJAgent supports multiple log types with configurable rotation and storage.

### Log Types

#### Think Log Configuration
```json
{
  "think_log": {
    "enable": true,
    "max_entries": 4,
    "key_prefix": "think_log_",
    "rotate_on_overflow": true,
    "include_timestamp": true
  }
}
```

**Purpose**: Stores agent's thinking and analysis entries.

**Properties**:
- `enable`: Whether to maintain think logs
- `max_entries`: Maximum log entries before rotation
- `key_prefix`: Prefix for log entry keys in memory
- `rotate_on_overflow`: Remove oldest when limit reached
- `include_timestamp`: Add timestamps to entries

#### Evaluation Log Configuration
```json
{
  "evaluation_log": {
    "enable": true,
    "max_entries": 1,
    "key_prefix": "evaluation_log_",
    "rotate_on_overflow": true
  }
}
```

**Purpose**: Stores evaluation and progress assessment entries.

#### Command Log Configuration
```json
{
  "command_log": {
    "enable": true,
    "max_entries": 1,
    "key_prefix": "command_log_",
    "include_action_details": true,
    "include_results": true
  }
}
```

**Purpose**: Stores executed actions and their results.

**Additional Properties**:
- `include_action_details`: Log full action parameters
- `include_results`: Log action execution results

## Memory Management Configuration

### Paging Configuration

```json
{
  "paging_limit": {
    "enable": false,
    "max_tokens": 1024,
    "strategy": "lru",
    "preserve_recent": true,
    "preserve_count": 2
  }
}
```

**Purpose**: Manages memory when approaching context limits.

**Properties**:
- `enable`: Whether to enable automatic paging
- `max_tokens`: Token threshold for triggering paging
- `strategy`: Paging strategy ("lru", "fifo", "priority")
- `preserve_recent`: Keep most recent entries
- `preserve_count`: Number of recent entries to preserve

### Hard Limit Configuration

```json
{
  "hard_limit": {
    "enable": false,
    "max_tokens": 2048,
    "action": "truncate",
    "emergency_cleanup": true
  }
}
```

**Purpose**: Prevents memory from exceeding absolute limits.

**Properties**:
- `enable`: Whether to enforce hard limits
- `max_tokens`: Absolute maximum token count
- `action`: Action when limit reached ("truncate", "reset", "error")
- `emergency_cleanup`: Perform aggressive cleanup if needed

## Performance Tuning

### Optimizing for Different Workloads

#### High-Throughput Configuration
```json
{
  "llm": {
    "temperature": 0.3,
    "max_tokens": 512,
    "timeout": 15
  },
  "agent": {
    "think_log": { "max_entries": 2 },
    "evaluation_log": { "max_entries": 1 },
    "command_log": { "max_entries": 1 },
    "iteration": { "max_iterations": 5 }
  }
}
```

**Use Case**: Fast, focused agent responses for simple tasks.

#### Deep Analysis Configuration
```json
{
  "llm": {
    "temperature": 0.8,
    "max_tokens": 4096,
    "timeout": 60
  },
  "agent": {
    "think_log": { "max_entries": 8 },
    "evaluation_log": { "max_entries": 3 },
    "command_log": { "max_entries": 5 },
    "iteration": { "max_iterations": 25 }
  }
}
```

**Use Case**: Complex reasoning tasks requiring extensive analysis.

#### Memory-Constrained Configuration
```json
{
  "agent": {
    "think_log": { "max_entries": 1 },
    "evaluation_log": { "max_entries": 1 },
    "command_log": { "max_entries": 1 },
    "paging_limit": {
      "enable": true,
      "max_tokens": 512
    },
    "hard_limit": {
      "enable": true,
      "max_tokens": 1024
    }
  }
}
```

**Use Case**: Resource-limited environments or edge deployment.

## Environment-Specific Configurations

### Development Environment

```json
{
  "llm": {
    "endpoint": "http://localhost:1234/v1/chat/completions",
    "model": "llama2-7b-chat",
    "temperature": 0.7,
    "timeout": 30
  },
  "agent": {
    "iteration": { "max_iterations": 3 },
    "think_log": { "enable": true, "max_entries": 2 },
    "evaluation_log": { "enable": true, "max_entries": 1 },
    "command_log": { "enable": true, "max_entries": 1 }
  }
}
```

**Features**:
- Local LLM endpoint for development
- Reduced iterations for faster testing
- All logging enabled for debugging

### Production Environment

```json
{
  "llm": {
    "endpoint": "https://api.openai.com/v1/chat/completions",
    "model": "gpt-4",
    "temperature": 0.6,
    "timeout": 45,
    "retry_attempts": 3,
    "retry_delay": 5
  },
  "agent": {
    "iteration": { "max_iterations": 15 },
    "paging_limit": {
      "enable": true,
      "max_tokens": 1536
    },
    "hard_limit": {
      "enable": true,
      "max_tokens": 3072
    }
  }
}
```

**Features**:
- Production LLM endpoint with retries
- Memory management enabled
- Higher iteration limits for complex tasks

### Docker Environment

```json
{
  "llm": {
    "endpoint": "http://host.docker.internal:1234/v1/chat/completions",
    "model": "qwen/qwen3-8b",
    "temperature": 0.7,
    "timeout": 45
  },
  "agent": {
    "iteration": { "max_iterations": 10 }
  }
}
```

**Features**:
- Docker host networking for LLM access
- Standard configuration for containerized deployment

## Troubleshooting Configuration Issues

### Common Configuration Problems

#### 1. LLM Connection Issues

**Problem**: Agent fails to connect to LLM endpoint.

**Check**:
```json
{
  "llm": {
    "endpoint": "http://correct-endpoint:port/path"
  }
}
```

**Solutions**:
- Verify endpoint URL is correct and accessible
- Check network connectivity from agent environment
- Ensure LLM server is running and responsive
- Validate API credentials if required

#### 2. Memory Exhaustion

**Problem**: Agent runs out of memory during execution.

**Solutions**:
```json
{
  "agent": {
    "paging_limit": {
      "enable": true,
      "max_tokens": 1024
    },
    "hard_limit": {
      "enable": true,
      "max_tokens": 2048
    },
    "think_log": { "max_entries": 2 },
    "evaluation_log": { "max_entries": 1 },
    "command_log": { "max_entries": 1 }
  }
}
```

#### 3. Response Parsing Errors

**Problem**: Agent fails to parse LLM responses.

**Check**:
- LLM model supports structured JSON output
- Prompts request proper JSON formatting
- Temperature not too high (causing malformed responses)

**Solutions**:
```json
{
  "llm": {
    "temperature": 0.3,
    "model": "model-with-json-support"
  }
}
```

#### 4. Slow Performance

**Problem**: Agent responses are too slow.

**Solutions**:
```json
{
  "llm": {
    "max_tokens": 512,
    "timeout": 15,
    "temperature": 0.3
  },
  "agent": {
    "iteration": { "max_iterations": 5 },
    "think_log": { "max_entries": 1 }
  }
}
```

### Configuration Validation

#### Manual Validation
1. **JSON Syntax**: Ensure valid JSON formatting
2. **Required Fields**: Verify all required fields are present
3. **Value Types**: Check that values match expected types
4. **Value Ranges**: Ensure numeric values are within valid ranges

#### Runtime Validation
The agent performs validation at startup:
- Reports missing required fields
- Warns about invalid value types
- Uses defaults for missing optional fields

### Configuration Testing

#### Test Configuration Changes
1. **Start with minimal config**: Use basic configuration first
2. **Add features incrementally**: Enable one feature at a time
3. **Monitor logs**: Watch for configuration-related errors
4. **Validate behavior**: Ensure agent behaves as expected

#### Configuration Templates

Create templates for different scenarios:

```bash
# Copy base configuration
cp config.json config-dev.json
cp config.json config-prod.json
cp config.json config-test.json

# Customize each for specific environment
```

---

This configuration guide provides comprehensive coverage of all LKJAgent configuration options. For specific use cases or advanced configurations, refer to the examples and adapt them to your requirements.
