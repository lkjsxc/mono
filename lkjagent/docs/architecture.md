# Architecture

This document explains the architecture of `lkjagent` with emphasis on `src/agent/*` (rewrite target) and the stable runtime (`src/global/*`, `src/utils/*`).

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
- src/agent/* (to rewrite)
  - core.{h,c} — orchestrates the loop: prompt -> http -> parse/act -> state/logs -> persist
  - prompt.{h,c} — builds JSON request body containing escaped XML content
  - http.{h,c} — wraps utils/http, extracts LLM content from OpenAI-compatible response
  - state.{h,c} — agent state and logs, paging hooks, working_memory sync
  - actions.{h,c} — executes working_memory and storage actions; saves memory.json
- src/lkjagent.{h,c}
  - Main program: loads config.json and memory.json, runs iteration loop (default 5 when missing), and prints pool freelist stats on shutdown

## Data Flow

1. Startup
   - config.json -> config.data (object_t)
   - memory.json -> agent.data (object_t)
2. Iterate (N cycles)
   - core: agent_prompt_generate -> agent_http_send_receive -> lkjagent_agent_execute
   - execute: parse response, dispatch actions or update state
   - state: manage logs; possible paging decision; sync logs
   - actions: modify working_memory/storage; log execution; save memory.json
3. Shutdown
   - Destroy objects, show freelist stats

## Invariants and Contracts

- All APIs return result_t (RESULT_OK/RESULT_ERR) and never abort the process
- Pool-backed allocations are created and destroyed via utils/string/object; avoid malloc/free except in main()
- Agent must remain resilient to malformed or partial LLM outputs; fallback to safe states
- Logs are written into agent.data.working_memory only; no separate log root
- Memory persistence writes the entire `agent.data` JSON to data/memory.json

## Error Handling

- Use RETURN_ERR for error logging and early returns; aim to continue cycles even after soft failures
- Cleanup should not escalate to fatal unless it indicates pool corruption

## Concurrency

- Single-threaded loop; no locking required

## Extensibility Points

- actions: add new action types (e.g., search, tool use)
- state: implement real paging strategy in agent_state_execute_paging
- http: auth headers, timeouts, retries
- prompt: richer message formats or function/tool schema
