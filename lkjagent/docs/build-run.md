# Build and Run

This project is designed for Linux containers; local builds may require a Unix-like environment.

## Build (containerized)

- Dockerfile builds a static-linked binary
- docker-compose.yml defines `lkjagent` service mounting `lkjagent/data`

### Compose

- Ensure an LLM endpoint is reachable at `config.llm.endpoint` (default uses host.docker.internal:1234)
- Start services:

```powershell
# From repo root on Windows (PowerShell)
docker compose up --build lkjagent
```

- Container runs `/app/build/lkjagent`

## Build (local Linux)

- Requirements: gcc, make
- From `lkjagent/`:

```bash
make
./build/lkjagent
```

## Data Files

- lkjagent/data/config.json — configuration
- lkjagent/data/memory.json — agent state persisted each cycle

## Troubleshooting

- If HTTP fails, check LLM endpoint and network access from container
- Increase logging by inspecting stdout; errors use RETURN_ERR structured prints
- Freelist stats printed on shutdown help detect leaks (non-zero is okay due to pool reuse)
