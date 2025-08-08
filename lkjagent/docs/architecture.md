# Architecture

This document explains the architecture of `lkjagent`, focusing on `src/agent/*` (redesigned) and the stable runtime layers (`src/global/*`, `src/utils/*`).

## Modules

- src/global/*
  - const.h — compile-time constants and limits
  - macro.h — RETURN_ERR logging macro, COUNTOF
  - std.h — platform/system headers
  - types.h — result_t, string_t, object_t, pool_t and top-level structs (config_t, agent_t, lkjagent_t)
- src/utils/* (stable)
  - file.h — file_read, file_write
  - http.h — http_get, http_post
  - object.h — dynamic JSON-like tree API (create/destroy, set/get, stringify, parse)
  - string.h — arena-backed string primitives
  - pool.h — memory pool management (freelists for strings/objects)
- src/agent/* (current implementation)
  - core.{h,c} — orchestrates the loop: prompt -> http -> parse/act or update -> logs -> persist
    - lkjagent_agent(pool, config, agent)
    - lkjagent_agent_execute(pool, config, agent, recv)
  - prompt.{h,c} — builds JSON request body with escaped XML content derived from config and memory
  - http.{h,c} — wraps utils/http; extracts `choices[0].message.content` from OpenAI-compatible responses
  - state.{h,c} — agent state transitions and logs; paging hooks; working_memory sync stub
  - actions.{h,c} — executes working_memory/storage actions; writes execution logs; persists memory.json
- src/lkjagent.{h,c}
  - Main program: loads config.json and memory.json, runs iteration loop (default 5 if unset), prints pool freelist stats on shutdown

## Data Flow

1. Startup
   - config.json -> config.data (object_t)
   - memory.json -> agent.data (object_t)
2. Iterate (N cycles)
   - core: agent_prompt_generate -> agent_http_send_receive -> lkjagent_agent_execute
   - execute: parse LLM content, either dispatch actions or update state/logs
   - state: manage thinking/evaluation/execution logs; decide paging; sync logs (noop)
   - actions: modify working_memory/storage; log execution; save entire memory.json
3. Shutdown
   - Destroy objects, show freelist stats

## Invariants and Contracts

- All APIs return result_t (RESULT_OK/RESULT_ERR); no process aborts from library code
- Pool-backed allocations are created/destroyed via utils/string/object; malloc/free only in main()
- Robust to malformed LLM outputs; falls back to safe defaults (e.g., next_state defaults to "thinking")
- Logs are stored under agent.data.working_memory only; no separate log root
- Persistence saves the entire `agent.data` to data/memory.json each cycle

## Error Handling

- Use RETURN_ERR for logging and early returns; the outer loop continues on soft failures
- Cleanup failures are downgraded to warnings unless pool integrity is at risk

## Concurrency

- Single-threaded loop; no locks

## Extensibility Points

- actions: add new action types (e.g., search, tool use)
- state: implement real paging in agent_state_execute_paging and richer token accounting
- http: add auth headers, timeouts, retries, streaming
- prompt: richer message formats or tool-calling schema
