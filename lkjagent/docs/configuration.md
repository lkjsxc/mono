# Configuration Documentation

## Overview

LKJAgent uses a hierarchical JSON configuration system that controls all aspects of agent behavior, LLM integration, memory management, and operational parameters. The configuration is loaded at startup and provides comprehensive control over the agent's operation.

## Configuration File Structure

### Primary Configuration File: `data/config.json`

```json
{
  "version": "1.0.0",
  "llm": { ... },
  "agent": { ... }
}
```

### Memory State File: `data/memory.json`

```json
{
  "working_memory": { ... },
  "storage": { ... },
  "state": "thinking",
  "thinking_log_001": "...",
  "thinking_log_002": "..."
}
```

## LLM Configuration

### Endpoint Configuration

```json
{
  "llm": {
    "endpoint": "http://host.docker.internal:1234/v1/chat/completions",
    "model": "qwen/qwen3-8b",
    "temperature": 0.7
  }
}
```

**Parameters:**
- `endpoint`: Full URL to LLM API endpoint (OpenAI compatible)
- `model`: Model identifier for the LLM service
- `temperature`: Sampling temperature (0.0-1.0) for response generation

**Supported Endpoints:**
- OpenAI API: `https://api.openai.com/v1/chat/completions`
- Local LLM servers: `http://localhost:1234/v1/chat/completions`
- Custom endpoints: Any OpenAI-compatible REST API

## Agent Configuration

### Core Agent Settings

```json
{
  "agent": {
    "thinking_log": {
      "enable": true,
      "max_entries": 10,
      "key_prefix": "thinking_log_"
    },
    "paging_limit": {
      "enable": true,
      "max_tokens": 4096
    },
    "hard_limit": {
      "enable": true,
      "max_tokens": 8192
    },
    "iterate": {
      "enable": true,
      "max_iterations": 1000
    }
  }
}
```

#### Thinking Log Configuration
- `enable`: Whether to maintain thinking log entries
- `max_entries`: Maximum number of thinking log entries to retain
- `key_prefix`: Prefix for thinking log keys in memory

#### Memory Limit Configuration
- `paging_limit.max_tokens`: Soft limit triggering memory paging
- `hard_limit.max_tokens`: Absolute maximum context size
- `enable`: Whether to enforce token limits

#### Iteration Control
- `max_iterations`: Maximum agent processing cycles
- `enable`: Whether to enforce iteration limits

## State Machine Configuration

### Base State Configuration

```json
{
  "agent": {
    "state": {
      "base": {
        "prompt": {
          "role": "An AI state machine that utilizes a finite working memory to output both next_state and thinking_log, while managing infinite storage. You are the ultimate librarian-like system that manages information intelligently.",
          "instructions": "Always respond with valid XML containing an 'agent' tag. In thinking state, provide 'next_state' and 'thinking_log'. In executing state, provide 'action' with 'type', 'tags', and optionally 'value'."
        }
      }
    }
  }
}
```

### Thinking State Configuration

```json
{
  "thinking": {
    "prompt": {
      "state_description": {
        "thinking": "Build reasoning chains and accumulate thinking_log entries. Analyze information and decide next actions.",
        "executing": "Perform concrete actions: working_memory_add, working_memory_remove, storage_load, storage_save"
      },
      "output_example_case1": {
        "agent": {
          "next_state": "thinking",
          "thinking_log": "Perhaps Alice is an important person. We may need to think more deeply about this."
        }
      },
      "output_example_case2": {
        "agent": {
          "next_state": "executing", 
          "thinking_log": "I need a little more context. Let's search the storage."
        }
      },
      "output_example_case3": {
        "agent": {
          "next_state": "thinking",
          "thinking_log": "It seems we need to process less important information. We won't need any information about John in particular."
        }
      }
    }
  }
}
```

### Executing State Configuration

```json
{
  "executing": {
    "prompt": {
      "available_actions": {
        "working_memory_add": "Add new information to working memory with tags and value",
        "working_memory_remove": "Remove information from working memory using tags",
        "storage_load": "Load information from persistent storage to working memory using tags",
        "storage_save": "Save information from working memory to persistent storage using tags and value"
      },
      "output_example_case1": {
        "agent": {
          "action": {
            "type": "working_memory_add",
            "tags": "character_david_personality",
            "value": "introverted programmer, 28 years old, passionate about AI, prefers working alone"
          }
        }
      },
      "output_example_case2": {
        "agent": {
          "action": {
            "type": "working_memory_remove",
            "tags": "character_bob_old_description"
          }
        }
      },
      "output_example_case3": {
        "agent": {
          "action": {
            "type": "storage_load",
            "tags": "character_alice_background"
          }
        }
      },
      "output_example_case4": {
        "agent": {
          "action": {
            "type": "storage_save",
            "tags": "story_plot_chapter1",
            "value": "Alice discovers the mysterious letter in her grandmother's attic, setting the stage for her adventure"
          }
        }
      }
    }
  }
}
```

### Paging State Configuration

```json
{
  "paging": {
    "prompt": {
      "purpose": "Manage memory overflow by intelligent context switching",
      "strategy": "Preserve important information, archive less critical data to storage"
    }
  }
}
```

## Memory Configuration

### Working Memory Structure

Working memory contains the agent's current active context:

```json
{
  "working_memory": {
    "todo": "Add characters until 10",
    "character_alice_about": "female, 25 years old, adventurous, loves nature",
    "current_task": "character_development",
    "story_context": "fantasy_adventure"
  }
}
```

**Characteristics:**
- **Key-Value Pairs**: Simple string-based storage
- **Dynamic Size**: Grows and shrinks based on agent actions
- **Token Counting**: Subject to paging limits
- **Fast Access**: In-memory operations

### Persistent Storage Structure

Persistent storage maintains long-term information:

```json
{
  "storage": {
    "character_charlie_about": "male, 28 years old, easygoing, enjoys hiking",
    "world_building_rules": "magic system based on elemental affinities",
    "plot_outline": "hero's journey with mystery elements",
    "character_relationships": "alice and charlie are childhood friends"
  }
}
```

**Characteristics:**
- **Unlimited Size**: No token limits applied
- **Persistent**: Survives agent restarts
- **Searchable**: Tag-based retrieval system
- **Archival**: Long-term information storage

## Configuration Access Patterns

### Path-Based Access

The configuration system uses dot notation for hierarchical access:

```c
// Access nested configuration values
object_provide_str(pool, &url_object, config->data, "llm.endpoint");
object_provide_str(pool, &model_object, config->data, "llm.model");
object_provide_str(pool, &temp_object, config->data, "llm.temperature");

// Access agent configuration
object_provide_str(pool, &base_prompt, config->data, "agent.state.base.prompt.role");
object_provide_str(pool, &thinking_prompt, config->data, "agent.state.thinking.prompt");
object_provide_str(pool, &executing_prompt, config->data, "agent.state.executing.prompt");
```

### Configuration Validation

```c
static result_t validate_configuration(pool_t* pool, config_t* config) {
    object_t* required_obj;
    
    // Validate required LLM configuration
    if (object_provide_str(pool, &required_obj, config->data, "llm.endpoint") != RESULT_OK) {
        RETURN_ERR("Missing required configuration: llm.endpoint");
    }
    
    if (object_provide_str(pool, &required_obj, config->data, "llm.model") != RESULT_OK) {
        RETURN_ERR("Missing required configuration: llm.model");
    }
    
    // Validate agent state configuration
    if (object_provide_str(pool, &required_obj, config->data, "agent.state.base.prompt") != RESULT_OK) {
        RETURN_ERR("Missing required configuration: agent.state.base.prompt");
    }
    
    return RESULT_OK;
}
```

## Environment-Specific Configuration

### Development Configuration

```json
{
  "llm": {
    "endpoint": "http://localhost:1234/v1/chat/completions",
    "model": "llama3-8b-instruct",
    "temperature": 0.3
  },
  "agent": {
    "thinking_log": {
      "enable": true,
      "max_entries": 50
    },
    "paging_limit": {
      "enable": false
    },
    "iterate": {
      "max_iterations": 100
    }
  }
}
```

### Production Configuration

```json
{
  "llm": {
    "endpoint": "https://api.openai.com/v1/chat/completions",
    "model": "gpt-4",
    "temperature": 0.7
  },
  "agent": {
    "thinking_log": {
      "enable": true,
      "max_entries": 10
    },
    "paging_limit": {
      "enable": true,
      "max_tokens": 4096
    },
    "hard_limit": {
      "enable": true,
      "max_tokens": 8192
    },
    "iterate": {
      "max_iterations": 1000
    }
  }
}
```

### Docker Configuration

```json
{
  "llm": {
    "endpoint": "http://host.docker.internal:1234/v1/chat/completions",
    "model": "qwen/qwen3-8b",
    "temperature": 0.7
  }
}
```

## Configuration File Locations

### Default Paths

```c
#define CONFIG_PATH "data/config.json"
#define MEMORY_PATH "data/memory.json"
```

### Docker Volume Mounting

```bash
# Mount configuration directory
docker run -v $(pwd)/data:/app/data lkjagent

# Mount specific configuration files
docker run \
  -v $(pwd)/config.json:/app/data/config.json \
  -v $(pwd)/memory.json:/app/data/memory.json \
  lkjagent
```

## Configuration Loading Process

### Startup Sequence

```c
static result_t lkjagent_config_init(pool_t* pool, config_t* config) {
    string_t* config_string;
    
    // 1. Create string for file content
    if (string_create(pool, &config_string) != RESULT_OK) {
        RETURN_ERR("Failed to create config string");
    }
    
    // 2. Read configuration file
    if (file_read(pool, CONFIG_PATH, &config_string) != RESULT_OK) {
        string_destroy(pool, config_string);
        RETURN_ERR("Failed to read config file");
    }
    
    // 3. Parse JSON configuration
    if (object_parse_json(pool, &config->data, config_string) != RESULT_OK) {
        string_destroy(pool, config_string);
        RETURN_ERR("Failed to parse config JSON");
    }
    
    // 4. Cleanup temporary string
    if (string_destroy(pool, config_string) != RESULT_OK) {
        RETURN_ERR("Failed to destroy config string");
    }
    
    return RESULT_OK;
}
```

### Memory Loading Process

```c
static result_t lkjagent_agent_init(pool_t* pool, agent_t* agent) {
    string_t* agent_string;
    
    // Similar process for memory.json
    if (string_create(pool, &agent_string) != RESULT_OK) {
        RETURN_ERR("Failed to create agent string");
    }
    
    if (file_read(pool, MEMORY_PATH, &agent_string) != RESULT_OK) {
        string_destroy(pool, agent_string);
        RETURN_ERR("Failed to read agent file");
    }
    
    if (object_parse_json(pool, &agent->data, agent_string) != RESULT_OK) {
        string_destroy(pool, agent_string);
        RETURN_ERR("Failed to parse agent JSON");
    }
    
    string_destroy(pool, agent_string);
    return RESULT_OK;
}
```

## Configuration Best Practices

### Security Considerations

1. **API Keys**: Store in environment variables, not configuration files
2. **File Permissions**: Restrict access to configuration files (600)
3. **Validation**: Always validate configuration before use
4. **Sanitization**: Sanitize all string values for injection attacks

### Performance Optimization

1. **Configuration Caching**: Load once, use throughout lifecycle
2. **Path Compilation**: Pre-compile frequently used paths
3. **Memory Efficiency**: Use pool allocator for configuration objects
4. **Validation Caching**: Cache validation results

### Maintainability

1. **Schema Documentation**: Document all configuration options
2. **Version Control**: Track configuration changes
3. **Environment Separation**: Use different configs for dev/prod
4. **Default Values**: Provide sensible defaults for all options

## Configuration Debugging

### Validation Errors

```c
// Example validation with detailed error messages
if (object_provide_str(pool, &endpoint_obj, config->data, "llm.endpoint") != RESULT_OK) {
    printf("ERROR: Missing or invalid llm.endpoint in configuration\n");
    printf("Expected: String value with valid HTTP(S) URL\n");
    printf("Example: \"http://localhost:1234/v1/chat/completions\"\n");
    RETURN_ERR("Configuration validation failed");
}
```

### Configuration Dumping

```c
// Debug function to dump configuration
static void dump_configuration(pool_t* pool, config_t* config) {
    string_t* config_json;
    if (object_tostring_json(pool, &config_json, config->data) == RESULT_OK) {
        printf("Configuration:\n%.*s\n", 
               (int)config_json->size, config_json->data);
        string_destroy(pool, config_json);
    }
}
```

### Common Configuration Issues

1. **Invalid JSON Syntax**: Use JSON validator tools
2. **Missing Required Fields**: Check error messages for specific fields
3. **Incorrect Path References**: Verify dot notation paths
4. **Type Mismatches**: Ensure string/number types match expectations
5. **Encoding Issues**: Use UTF-8 encoding for configuration files

## Configuration Schema Reference

### Complete Schema Structure

```json
{
  "version": "string",
  "llm": {
    "endpoint": "string (URL)",
    "model": "string",
    "temperature": "number (0.0-1.0)"
  },
  "agent": {
    "thinking_log": {
      "enable": "boolean",
      "max_entries": "integer",
      "key_prefix": "string"
    },
    "paging_limit": {
      "enable": "boolean",
      "max_tokens": "integer"
    },
    "hard_limit": {
      "enable": "boolean",
      "max_tokens": "integer"
    },
    "iterate": {
      "enable": "boolean",
      "max_iterations": "integer"
    },
    "state": {
      "base": { "prompt": { ... } },
      "thinking": { "prompt": { ... } },
      "executing": { "prompt": { ... } },
      "paging": { "prompt": { ... } }
    }
  }
}
```
