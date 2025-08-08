# State Machine

The agent is a simple state machine that cycles via LLM outputs. States drive prompt selection and post-processing.

## States

- thinking (default)
  - LLM may return: `<next_state>` (thinking|commanding|evaluating) and optional `<think_log>`
- commanding
  - LLM returns: `<action>` with `<type>`, `<tags>`, and optional `<value>`
  - System logs an command entry and auto-transitions to `evaluating`
- evaluating
  - LLM may return: `<next_state>` and optional `<evaluation_log>`
  - If paging required, transitions to `paging` briefly, commands paging, then returns to `thinking`
- paging (hook)
  - Triggered when token-estimate >= `agent.paging_limit.max_tokens`; placeholder implementation

## Transitions

- core.c checks for `<action>` in the parsed response
  - present => `agent_actions_dispatch` then `agent_state_auto_transition()` -> evaluating
  - absent  => `agent_state_update_and_log()` unless current state is evaluating
    - when current state is evaluating => `agent_state_handle_evaluation_transition()`
- from evaluating
  - if paging required => set `paging` -> `agent_state_command_paging()` -> set `thinking`
  - else => set `next_state` from response if provided; otherwise fallback to `thinking`

## Logs

- think_log
  - Controlled by `agent.think_log.enable`
  - Rotation configured by `max_entries` and `key_prefix` (default `think_log_`)
  - Stored in `working_memory` as key/value entries with zero-padded numeric suffixes
- evaluation_log
  - Controlled by `agent.evaluation_log.enable`; same rotation scheme (default `evaluation_log_`)
- command_log
  - Controlled by `agent.command_log.enable`
  - Written by `agent_actions_log_result` after every action attempt (success or failure)

## Working Memory and Storage

- working_memory: volatile kv space for current context
- storage: persistent kv space for long-term items; lives inside agent.data and is saved to disk

## Paging Hooks

- `agent_state_estimate_tokens`: serialize `working_memory` to JSON and divide by 4 for an approximate token count
- `agent_state_check_memory_limits`: compare against `agent.paging_limit.max_tokens` when enabled
- `agent_state_command_paging`: placeholder; implement archival/summarization workflow later
