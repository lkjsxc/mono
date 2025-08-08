# LLM Protocol

The agent talks to an OpenAI-compatible chat/completions endpoint specified by config.llm.endpoint.

## Request

- HTTP POST to config.llm.endpoint
- Content-Type: application/json
- Body shape (assembled by prompt.c):

```
{
  "messages": [
    {
      "role": "user",
      "content": "<escaped XML constructed from base+state prompts and working_memory/>"
    }
  ],
  "model": "${config.llm.model}",
  "temperature": ${config.llm.temperature},
  "max_tokens": -1,
  "stream": false
}
```

The content string is built from:
- Base prompt: config.agent.state.base.prompt (converted to XML then escaped)
- State-specific prompt: config.agent.state.<state>.prompt (XML-escaped)
- Working memory: object_tostring_xml(agent.data.working_memory) wrapped in <working_memory> … </working_memory>

## Response

- Expects OpenAI-like JSON with `choices[0].message.content` containing XML
- http.c extracts `content` into a string_t

### XML-in-JSON Content

The agent parses with a lenient string search (actions.c: agent_actions_parse_response):
- <next_state>STATE</next_state>
- <thinking_log>…</thinking_log>
- <evaluation_log>…</evaluation_log>
- <action><type>…</type><tags>…</tags>[<value>…</value>]</action>

Missing or malformed sections are tolerated; defaults are applied (e.g., next_state="thinking").

## Robustness Considerations

- The parser avoids full XML parsing to handle imperfect outputs
- Always safe-update state and logs; unknown action types are logged and ignored
- HTTP, JSON, and content extraction failures result in a single-cycle soft error
