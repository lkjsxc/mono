# lkjagent TypeScript Rewrite Plan

## Goals
- Preserve the behavioral semantics of the original C implementation while adopting a functional TypeScript architecture.
- Maintain compatibility with existing `config.json` and `memory.json` data formats.
- Provide composable, side-effect-free core logic and isolate all I/O at the edges.
- Support XML-based interaction with the LLM endpoint and JSON for all internal persistence.

## Proposed Module Layout

```
src/
  index.ts                 # Program entrypoint (bootstraps runtime loop)
  agent/
    runner.ts              # Single step execution orchestration
  config/
    paths.ts               # Constants for config/memory paths
    load.ts                # Pure loader + validation helpers
    types.ts               # Shared config type definitions
  domain/
    types.ts               # AgentMemory, WorkingMemoryEntry, Action types
    tags.ts                # Tag normalization, sorting, deduplication
    state.ts               # Helpers for state transitions and guards
  io/
    jsonFile.ts            # Persist/restore JSON from disk
    http.ts                # fetch wrapper with retry/backoff
    time.ts                # sleep helper
  memory/
    working.ts             # Working-memory CRUD operations (pure functions)
    storage.ts             # Storage CRUD and subset matching utilities
    paging.ts              # Automatic paging strategy (returns updated memory)
    cleanup.ts             # Config-driven working memory retention controls
  actions/
    execute.ts             # Dispatch table for agent actions
    workingMemory.ts       # working_memory_add/remove implementations
    storage.ts             # storage_save/load/search implementations
  prompt/
    builder.ts             # Assemble prompt sections and context budget tracking
    qwen.ts                # State-aware parameter selection for Qwen models
  llm/
    client.ts              # Build request payload and invoke HTTP call
  process/
    parser.ts              # Parse LLM JSON, strip <think>, parse XML
    interpreter.ts         # Convert parsed XML into Action + next state
```

Each module exports pure functions (data in → data out). Only `index.ts`, `jsonFile.ts`, and `http.ts` perform side effects.

## Data Model
- `AgentConfig` mirrors `config.json` with typed helpers for nested role/state definitions.
- `AgentMemory` retains the shape `{ state: string; workingMemory: Record<string, string>; storage: Record<string, string>; }`.
- `SortedTags` is represented as a canonical comma-separated `string` produced by `normalizeTags`.
- `AgentAction` is a discriminated union for the five supported action types (working-memory add/remove, storage save/load/search).
- `PromptContext` captures extracted role information, working memory snapshot, storage excerpts, and token budgeting metadata.

## Execution Flow
1. `index.ts` loads config & memory via `jsonFile.ts` (ensuring defaults when absent).
2. `agent/runner.ts` performs an iteration:
   - Build prompt and payload via `prompt/builder.ts` + `llm/client.ts`.
   - Send request using `io/http.ts`.
   - Parse and interpret the XML response (`process/parser.ts` + `process/interpreter.ts`).
   - Execute the resulting action through `actions/execute.ts`, returning a new `AgentMemory` snapshot.
   - Apply paging policy (`memory/paging.ts`) and persist the updated memory.
3. Loop with exponential backoff on failure while respecting iteration limits defined in config.

## Outstanding Tasks
1. Scaffold the TypeScript project (`package.json`, `tsconfig.json`, lint/test tooling, Docker updates).
2. Implement the modules listed above using a functional style.
3. Port the action semantics (working memory + storage operations) to pure functions.
4. Implement prompt building and request assembly faithful to the existing behavior.
5. Implement response parsing, XML interpretation, and paging heuristics.
6. Add automated tests covering tag utilities, action logic, paging thresholds, and prompt assembly.
7. Update documentation and container orchestration for the Node.js runtime.
```
