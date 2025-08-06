# LLM Integration Documentation

## Overview

LKJAgent integrates with Large Language Models through a standardized HTTP REST API interface. The system constructs sophisticated prompts, manages communication protocols, and processes structured responses to enable intelligent agent behavior.

## LLM Communication Architecture

### Protocol Flow

```
Agent Core ──┐
             ├─→ Prompt Generation ──→ HTTP Request ──→ LLM Endpoint
Memory State ┘                                         │
                                                       ▼
Action Execution ←── XML Parsing ←── Response Processing ←── LLM Response
```

### API Endpoint Configuration

Configuration in `data/config.json`:

```json
{
  "llm": {
    "endpoint": "http://host.docker.internal:1234/v1/chat/completions",
    "model": "qwen/qwen3-8b", 
    "temperature": 0.7
  }
}
```

**Supported Endpoints:**
- OpenAI API (GPT-3.5, GPT-4)
- Local LLM servers (Ollama, LocalAI, LM Studio)
- Custom API endpoints with OpenAI-compatible interface

## Prompt Engineering

### Prompt Structure

The system constructs prompts by combining multiple components:

```
[Base Prompt]        ← Role definition and core instructions
[State Prompt]       ← Current state-specific guidance
[Working Memory]     ← Current context and information
[Configuration]      ← Available actions and examples
```

### Base Prompt Template

```json
{
  "role": "An AI state machine that utilizes a finite working memory to output both next_state and thinking_log, while managing infinite storage. You are the ultimate librarian-like system that manages information intelligently.",
  "instructions": "Always respond with valid XML containing an 'agent' tag. In thinking state, provide 'next_state' and 'thinking_log'. In executing state, provide 'action' with 'type', 'tags', and optionally 'value'."
}
```

### State-Specific Prompts

#### Thinking State Prompt
```json
{
  "state_description": {
    "thinking": "Build reasoning chains and accumulate thinking_log entries. Analyze information and decide next actions.",
    "executing": "Perform concrete actions: working_memory_add, working_memory_remove, storage_load, storage_save"
  },
  "output_example_case1": {
    "agent": {
      "next_state": "thinking",
      "thinking_log": "Perhaps Alice is an important person. We may need to think more deeply about this."
    }
  }
}
```

#### Executing State Prompt
```json
{
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
  }
}
```

## HTTP Request Construction

### Request Building Process

```c
static result_t lkjagent_agent_makeprompt(pool_t* pool, config_t* config, 
                                          agent_t* agent, string_t** dst) {
    // 1. Build JSON request header
    if (build_json_request_header(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to build JSON request header");
    }
    
    // 2. Append base prompt
    if (append_base_prompt(pool, dst, config_agent_state_base_prompt) != RESULT_OK) {
        RETURN_ERR("Failed to append base prompt");
    }
    
    // 3. Append state-specific prompt
    if (append_state_prompt(pool, dst, config_agent_state_main_prompt) != RESULT_OK) {
        RETURN_ERR("Failed to append state prompt");
    }
    
    // 4. Append working memory context
    if (append_working_memory(pool, dst, agent_workingmemory) != RESULT_OK) {
        RETURN_ERR("Failed to append working memory");
    }
    
    // 5. Append JSON request footer
    if (append_json_request_footer(pool, dst, config) != RESULT_OK) {
        RETURN_ERR("Failed to append JSON request footer");
    }
    
    return RESULT_OK;
}
```

### JSON Request Format

```json
{
  "model": "qwen/qwen3-8b",
  "messages": [
    {
      "role": "system",
      "content": "[Base Prompt + State Prompt + Working Memory + Configuration]"
    }
  ],
  "temperature": 0.7,
  "max_tokens": 4096,
  "stream": false
}
```

### HTTP Headers

```c
static result_t build_json_request_header(pool_t* pool, string_t** dst) {
    const char* header = "{\n  \"model\": \"";
    if (string_append_str(pool, dst, header) != RESULT_OK) {
        RETURN_ERR("Failed to append JSON header");
    }
    // ... continue building request
}
```

## Response Processing

### HTTP Response Structure

Expected response format from LLM:

```json
{
  "id": "chatcmpl-123",
  "object": "chat.completion",
  "created": 1677652288,
  "model": "qwen/qwen3-8b",
  "choices": [
    {
      "index": 0,
      "message": {
        "role": "assistant",
        "content": "<agent>...</agent>"
      },
      "finish_reason": "stop"
    }
  ],
  "usage": {
    "prompt_tokens": 56,
    "completion_tokens": 31,
    "total_tokens": 87
  }
}
```

### Content Extraction

```c
static result_t extract_llm_response_content(pool_t* pool, object_t* recv_http_object, 
                                           object_t** recv_content_object) {
    object_t* choices_array;
    object_t* first_choice;
    object_t* message_obj;
    
    // Navigate JSON structure: choices[0].message.content
    if (object_provide_str(pool, &choices_array, recv_http_object, "choices") != RESULT_OK) {
        RETURN_ERR("Failed to get choices array from response");
    }
    
    if (object_provide_str(pool, &first_choice, choices_array, "0") != RESULT_OK) {
        RETURN_ERR("Failed to get first choice from choices array");
    }
    
    if (object_provide_str(pool, &message_obj, first_choice, "message") != RESULT_OK) {
        RETURN_ERR("Failed to get message from first choice");
    }
    
    if (object_provide_str(pool, recv_content_object, message_obj, "content") != RESULT_OK) {
        RETURN_ERR("Failed to get content from message");
    }
    
    return RESULT_OK;
}
```

## XML Response Parsing

### Expected XML Format

#### Thinking State Response
```xml
<agent>
  <next_state>thinking</next_state>
  <thinking_log>Perhaps Alice is an important person. We may need to think more deeply about this.</thinking_log>
</agent>
```

#### Executing State Response
```xml
<agent>
  <action>
    <type>working_memory_add</type>
    <tags>character_david_personality</tags>
    <value>introverted programmer, 28 years old, passionate about AI, prefers working alone</value>
  </action>
</agent>
```

### XML Parsing Implementation

```c
static result_t parse_llm_response(pool_t* pool, const string_t* recv, 
                                  object_t** response_obj) {
    if (object_parse_xml(pool, response_obj, recv) != RESULT_OK) {
        RETURN_ERR("Failed to parse XML response from LLM");
    }
    
    // Validate that response contains required 'agent' tag
    object_t* agent_obj;
    if (object_provide_str(pool, &agent_obj, *response_obj, "agent") != RESULT_OK) {
        RETURN_ERR("Response does not contain required 'agent' tag");
    }
    
    return RESULT_OK;
}
```

## Working Memory Integration

### Memory Serialization for Prompts

```c
static result_t append_working_memory(pool_t* pool, string_t** dst, 
                                     object_t* agent_workingmemory) {
    if (string_append_str(pool, dst, "\\n\\nCurrent Working Memory:\\n") != RESULT_OK) {
        RETURN_ERR("Failed to append working memory header");
    }
    
    // Iterate through all working memory entries
    object_t* current = agent_workingmemory->child;
    while (current != NULL) {
        if (current->string != NULL) {
            // Format: key: value
            if (string_append_str(pool, dst, current->string->data) != RESULT_OK) {
                RETURN_ERR("Failed to append working memory key");
            }
            
            if (string_append_str(pool, dst, ": ") != RESULT_OK) {
                RETURN_ERR("Failed to append separator");
            }
            
            if (current->child && current->child->string) {
                if (string_append_string(pool, dst, current->child->string) != RESULT_OK) {
                    RETURN_ERR("Failed to append working memory value");
                }
            }
            
            if (string_append_str(pool, dst, "\\n") != RESULT_OK) {
                RETURN_ERR("Failed to append newline");
            }
        }
        current = current->next;
    }
    
    return RESULT_OK;
}
```

## Error Handling and Retries

### HTTP Communication Errors

```c
if (http_post(pool, url_object->string, content_type, send_string, &recv_http_string) != RESULT_OK) {
    if (cleanup_http_resources(pool, send_string, content_type, recv_http_string, NULL) != RESULT_OK) {
        RETURN_ERR("Failed to cleanup HTTP resources after HTTP POST failure");
    }
    RETURN_ERR("Failed to send HTTP POST request");
}
```

### Response Validation

```c
// Validate JSON structure
if (object_parse_json(pool, &recv_http_object, recv_http_string) != RESULT_OK) {
    RETURN_ERR("Failed to parse HTTP response JSON");
}

// Validate content extraction
if (extract_llm_response_content(pool, recv_http_object, &recv_content_object) != RESULT_OK) {
    RETURN_ERR("Failed to extract content from LLM response");
}

// Validate XML structure
if (parse_llm_response(pool, recv_content_object->string, &response_obj) != RESULT_OK) {
    RETURN_ERR("Failed to parse LLM response XML");
}
```

### Graceful Degradation

- **Network Failures**: Log error and attempt to continue with cached state
- **Malformed Responses**: Reset to thinking state and retry
- **API Rate Limits**: Implement exponential backoff (future enhancement)

## LLM Model Configuration

### Model Selection Criteria

**Recommended Models:**
- **GPT-4**: Excellent reasoning, high token limits, reliable XML generation
- **GPT-3.5-turbo**: Good balance of performance and cost
- **Claude**: Strong reasoning capabilities, good instruction following
- **Qwen**: Open-source alternative with strong performance
- **Llama 3**: Local deployment option with good reasoning

### Temperature Settings

```json
{
  "temperature": 0.7  // Balance between creativity and consistency
}
```

**Temperature Guidelines:**
- **0.0-0.3**: Highly deterministic, best for structured outputs
- **0.4-0.7**: Balanced creativity and consistency (recommended)
- **0.8-1.0**: High creativity, may reduce consistency

### Token Management

```json
{
  "max_tokens": 4096,  // Maximum response length
  "paging_limit": {
    "max_tokens": 4096  // Working memory size limit
  },
  "hard_limit": {
    "max_tokens": 8192  // Absolute maximum context
  }
}
```

## Performance Optimization

### Request Efficiency
- **Prompt Caching**: Reuse base prompts across requests
- **Context Compression**: Intelligent summary of older context
- **Batch Processing**: Group multiple queries when possible

### Response Processing
- **Streaming**: Support for streaming responses (future enhancement)
- **Parallel Processing**: Handle multiple agent instances
- **Memory Reuse**: Efficient object pooling for responses

### Network Optimization
- **Connection Pooling**: Reuse HTTP connections
- **Compression**: Enable gzip compression for large prompts
- **Timeout Management**: Appropriate timeouts for different operations

## Debugging LLM Integration

### Request Logging

```c
printf("Send: \n%.*s\n", (int)send_string->size, send_string->data);
```

### Response Logging

```c
printf("Content: \n%.*s\n", (int)recv_content_object->string->size, 
       recv_content_object->string->data);
```

### Common Issues and Solutions

1. **Invalid XML Responses**
   - Solution: Add XML validation and regeneration prompts
   - Fallback: Return to thinking state with error context

2. **Token Limit Exceeded**
   - Solution: Implement intelligent context truncation
   - Fallback: Trigger paging state for memory management

3. **Network Connectivity Issues**
   - Solution: Implement retry logic with exponential backoff
   - Fallback: Use cached responses or offline mode

4. **Model Instruction Following**
   - Solution: Refine prompts with more explicit examples
   - Fallback: Implement response post-processing and correction

## Security Considerations

### API Key Management
- Store API keys in environment variables
- Use secure configuration files with restricted permissions
- Implement key rotation for production deployments

### Input Sanitization
- Validate all user inputs before including in prompts
- Escape special characters in XML/JSON content
- Implement content filtering for sensitive information

### Response Validation
- Validate all LLM responses before execution
- Sanitize action parameters to prevent injection attacks
- Implement bounds checking for all memory operations
