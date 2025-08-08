# LKJAgent Documentation Suite

This folder contains structured documentation for the `lkjagent` subsystem, intended to guide both contributors and AI-codegen agents that will rewrite `src/agent/*` while preserving `src/utils/*` and `src/global/*`.

- architecture.md — High-level system overview, modules, data flow, and invariants
- state-machine.md — Agent states, transitions, prompts, and logs
- llm-protocol.md — Request/response JSON over HTTP and the XML-in-JSON content format
- data-models.md — Config schema, memory schema, core types and string/object pool contracts
- build-run.md — Build, run, container, and compose instructions
- refactor-plan.md — Requirements and guardrails for the agent rewrite
