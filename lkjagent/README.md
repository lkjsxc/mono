# lkjagent (TypeScript)

A functional TypeScript rewrite of the lkjagent autonomous reasoning system. The agent maintains a dual-memory architecture (working memory plus persistent storage), builds structured prompts for large-context LLMs, and interprets XML responses to plan and execute actions.

## Project layout

```
src/
  index.ts               # Entrypoint that loads config/memory and runs the loop
  agent/runner.ts        # Iteration orchestrator and persistence handling
  actions/               # Pure action pipelines for working memory and storage
  config/                # Config + memory loaders and path helpers
  domain/                # Shared types and tag utilities
  io/                    # File, HTTP, and timing side-effect helpers
  llm/                   # Payload builder + HTTP client
  memory/                # Working-memory, storage, and paging logic
  process/               # LLM JSON content extraction and XML interpretation
  prompt/                # Prompt composer and Qwen parameter selection
```

## Getting started

```bash
npm install --ignore-scripts
npm run build
npm start
```

> **Note:** When installing dependencies on Windows/WSL, ensure you are using a Node.js binary that supports UNC paths (e.g., install Node directly inside WSL). If you rely on the Windows `npm`, use `npm install --ignore-scripts` to skip native binaries and run `npm rebuild esbuild --platform=linux --arch=x64` inside WSL afterwards.

### Working memory auto-cleanup

Set `agent.memory_system.working_memory.auto_cleanup_limit` in `data/config.json` to automatically trim older working-memory actions after each iteration. The agent keeps only the most recent _N_ entries, encouraging important knowledge to be promoted into long-term storage.

## Testing

```bash
npm test
```

The Vitest suite covers tag normalization, storage behaviors, and XML interpretation to guard the agent’s core transformations.

## Docker

Build and run the agent in a container:

```bash
docker compose build
docker compose up
```

The container mounts `./data` into `/app/data`, preserving configuration and long-term storage across restarts.
