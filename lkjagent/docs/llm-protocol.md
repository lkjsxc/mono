# LLM Protocol

The agent talks to an OpenAI-compatible chat/completions endpoint specified by `config.llm.endpoint`.

## Request

- HTTP POST to `config.llm.endpoint`
- Content-Type: `application/json`
- Body shape (constructed by prompt.c):

```
{
  "messages": [
    {
      "role": "user",
      "content": "<escaped-xml: base + state + working_memory>"
    }
  ],
  "model": "${config.llm.model}",
  "temperature": ${config.llm.temperature},
  "max_tokens": -1,
  "stream": false
}
```

Content assembly details:
- Base prompt: `config.agent.state.base.prompt` -> XML -> JSON-escaped
- State prompt: `config.agent.state.<current>.prompt` -> XML -> JSON-escaped
- Working memory: `object_tostring_xml(agent.data.working_memory)` wrapped with `<working_memory>...</working_memory>` then JSON-escaped

## Response

- Expects OpenAI-like JSON; `choices.[0].message.content` is extracted
- The content string should contain simple XML-like tags parsed by a tolerant substring search

### XML-in-JSON Content Contract

Supported tags (any order; optional unless noted):
- `<next_state>STATE</next_state>` (optional; defaults to `thinking` if absent)
- `<think_log>...</think_log>` (optional)
- `<evaluation_log>...</evaluation_log>` (optional)
- `<action>` (optional)
  - `<type>working_memory_add|working_memory_remove|storage_load|storage_save</type>`
  - `<tags>key or label (spaces become underscores)</tags>`
  - `<value>string value</value>` (required for add/save)

Error tolerance and defaults:
- Missing tags are ignored; next_state defaults to `thinking`
- Unknown action types are logged as execution failures and ignored
- Oversized fragments are clipped by conservative buffers (1–2 KB per field)

## Robustness Considerations

- The agent uses lenient string search instead of full XML parsing to handle imperfect outputs
- HTTP, JSON, and content extraction failures produce soft errors; the main loop continues
