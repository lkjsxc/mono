# Agent Rewrite Plan (src/agent/*)

Objective: Produce high-quality, robust code for `src/agent/*` while keeping `src/utils/*` and `src/global/*` unchanged. Preserve public contracts and behaviors.

## Requirements Checklist

- Keep utils/global APIs stable; do not change function signatures
- Maintain end-to-end flow: prompt -> http -> parse/act -> state/logs -> persist
- Be resilient to malformed LLM responses; never crash the loop
- Use RETURN_ERR consistently; prefer soft-fail and continue next cycle
- Ensure working_memory always exists before logging
- Store logs only under working_memory with rotation and prefixes from config
- Respect config: iterate.max_iterations, log enables, paging_limit
- Implement TODO: real paging strategy is optional; keep placeholder if scope-limited
- Do not introduce malloc/free inside agent code; use pool_* APIs via utils modules
- Keep code warning-free under CFLAGS in Makefile

## Suggested Improvements

- Strengthen parsing: tolerate whitespace/newlines; case-insensitive tags if needed
- Harden string handling: guard copies, cap buffers, always terminate
- Add small helpers for repeated path strings ("state", "working_memory")
- Centralize config accessors (already present in state.c)
- Add structured execution log format (already present; could add timestamps later)

## Test Scenarios

- thinking -> thinking with think_log
- thinking -> executing (working_memory_add)
- executing -> evaluating (auto) -> thinking
- storage_save + storage_load round-trip
- paging_limit reached triggers paging state path (still placeholder)
- malformed response: parser sets default next_state and no actions

## Non-Goals

- Changes to pool allocator or utils APIs
- Network timeouts/retries beyond current http_post
- Multi-threading
