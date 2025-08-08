# Data Models

This describes the core runtime types and persisted schemas.

## C Types (src/global/types.h)

- result_t: RESULT_OK, RESULT_ERR
- string_t: { char* data; uint64_t capacity; uint64_t size; }
- object_t: { string_t* string; object_t* child; object_t* next; }
- pool_t: arena with freelists for multiple string sizes and objects
- config_t: { object_t* data }
- agent_t: { object_t* data }
- lkjagent_t: { pool_t pool; config_t config; agent_t agent }

## Config JSON (data/config.json)

- version: string
- llm: { endpoint: string, model: string, temperature: number }
- agent:
  - think_log: { enable: bool|number, max_entries: number, key_prefix: string }
  - evaluation_log: { enable: bool|number, max_entries: number, key_prefix: string }
  - execution_log: { enable: bool|number, max_entries: number, key_prefix: string }
  - paging_limit: { enable: bool|number, max_tokens: number }
  - hard_limit: { enable: bool|number, max_tokens: number }  // reserved; not actively enforced
  - iterate: { max_iterations: number }  // when missing or invalid, defaults to 5
    - Note: `agent.iterate.enable` (if present) is ignored by the current implementation; only `max_iterations` is read.
  - state:
    - base.prompt: object (arbitrary keys)
    - thinking.prompt: object
    - executing.prompt: object
    - evaluating.prompt: object
    - paging.prompt: object

Notes:
- enable values accept "true" or non-zero numeric strings; missing keys are treated as disabled.
- llm.temperature is appended as-is to the request JSON; ensure it is a numeric literal in the config JSON.

## Memory JSON (data/memory.json)

- working_memory: object (arbitrary kv)
- storage: object (optional)
- state: string (thinking|executing|evaluating|paging)

Persistence behavior:
- After each cycle, the entire `agent.data` object is serialized and written to data/memory.json.
  - Missing `working_memory`/`storage` are created on demand by actions.

## Paths (object API)

- object_provide_str(pool, &obj, root, "a.b.c") walks children; arrays use index notation like "choices.[0]"
- object_provide_string(&obj, root, key_string) uses string_t as key
- object_set_string(pool, root, key_string, value_string) assigns leaf strings
- object_tostring_json(pool, &dst_string, root) serializes
- object_tostring_xml(pool, &dst_string, root) serializes simple XML-like form

## Token Estimation

- agent_state_estimate_tokens serializes working_memory to JSON and approximates: tokens ~ bytes/4
- Paging decision compares this count to agent.paging_limit.max_tokens when agent.paging_limit.enable is truthy.
