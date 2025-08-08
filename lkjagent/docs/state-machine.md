# State Machine

The agent is a simple state machine that cycles via LLM outputs. States drive prompt selection and post-processing.

## States

- thinking (default)
  - LLM returns: agent.next_state (thinking|executing|evaluating) and optional agent.thinking_log
- executing
  - LLM returns: agent.action { type, tags, [value] }
  - System logs an execution entry; auto-transition to evaluating
- evaluating
  - LLM returns: agent.next_state (always thinking) and optional agent.evaluation_log
- paging (planned)
  - Triggered by token-estimate >= paging_limit; LLM produces actions to archive/summarize

## Transitions

- core.c identifies if a response contains agent.action
  - yes => actions.dispatch + state.auto_transition() -> evaluating
  - no  => state.update_and_log() or state.handle_evaluation_transition() based on current state
- from evaluating
  - if paging required => state: paging -> execute_paging() -> thinking
  - else => next_state from response or fallback to thinking

## Logs

- thinking_log
  - Enabled by agent.thinking_log.enable
  - Rotation configured by max_entries and key_prefix (default thinking_log_)
  - Stored only in working_memory as key/value entries with sequential suffixes
- evaluation_log
  - Enabled by agent.evaluation_log.enable
  - Rotation similar to thinking_log (default evaluation_log_)
- execution_log
  - Enabled by agent.execution_log.enable
  - Written by agent_actions_log_result after every action attempt

## Working Memory and Storage

- working_memory: volatile kv space for current context
- storage: persistent kv space for long-term items; lives inside agent.data and is saved to disk

## Paging Hooks

- agent_state_estimate_tokens: serialize working_memory to JSON and divide by 4 for an approximate token count
- agent_state_check_memory_limits: compare with paging_limit.max_tokens
- agent_state_execute_paging: TODO placeholder; implement LLM-guided archival workflow
