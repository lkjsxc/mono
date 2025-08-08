# LKJAgent Documentation Suite

This folder documents the `lkjagent` subsystem. The agent in `src/agent/*` has been redesigned and quality-hardened; these docs reflect the current implementation while keeping `src/utils/*` and `src/global/*` as stable runtime layers.

- architecture.md — High-level system overview, modules, data flow, and invariants
- state-machine.md — Agent states, transitions, prompts, and logs
- llm-protocol.md — Request/response JSON over HTTP and the XML-in-JSON content format
- data-models.md — Config schema, memory schema, core types and string/object pool contracts
- build-run.md — Build, run, container, and compose instructions
- refactor-plan.md — Historical requirements and guardrails from the previous redesign effort
