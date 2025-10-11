# Simple Educational Social Network

A minimal social networking system built in C with standard libraries only. This educational project demonstrates basic networking concepts, protocol design, and web interface integration.

## Architecture

- **Single Program** (`main.c`): Unified C program that can run as either server or client
- **Server Mode**: TCP server on port 8080 that handles message posting and retrieval
- **Client Mode**: HTTP server on port 3000 that serves a web interface and proxies requests to the server
- **Protocol**: Simple text-based protocol over TCP
- **Storage**: In-memory only (data lost on restart)

## Features

- Post text messages
- View global message feed
- Simple web interface
- Real-time message display

## Quick Start with Docker

```bash
docker compose up -d --build
```

Then open http://localhost:3000 in your browser.

## Manual Compilation and Running

### Prerequisites

- GCC compiler
- Standard C libraries (no external dependencies)

### Compile and Run

```bash
# Compile the program
gcc -o social-network main.c

# Run as server (backend)
./social-network server

# In another terminal, run as client (web interface)
./social-network client
```

The server will start on port 8080, and the client will start on port 3000. Open http://localhost:3000 in your browser.

## Protocol Specification

The server uses a simple text-based protocol:

### POST Message
```
POST <message_length>
<message_content>
```

Response:
```
OK
```
or
```
ERROR <reason>
```

### GET Messages
```
GET
```

Response:
```
<count>
<timestamp1> <length1>
<content1>
<timestamp2> <length2>
<content2>
...
```

## API Endpoints

The client provides these HTTP endpoints:

- `GET /` - Serves the web interface
- `POST /api/post` - Post a new message (JSON body: `{"message":"content"}`)
- `GET /api/feed` - Get all messages (returns JSON array)

## Technical Details

### Implementation
- Single ~400 line C program with both server and client functionality
- Server mode: Uses `socket()`, `bind()`, `listen()`, `accept()`
- Client mode: HTTP server using raw sockets, proxies requests to backend
- Stores up to 100 messages in memory
- Handles one client at a time (simple iterative server)
- Serves embedded HTML with JavaScript

### Limitations
- No persistent storage (messages lost on restart)
- Single-threaded (one client at a time)
- No authentication or user management
- No usernames (anonymous messages only)
- Basic error handling
- No message validation beyond length limits

## Educational Value

This project demonstrates:
- Socket programming in C
- HTTP protocol implementation
- Simple protocol design
- Client-server architecture
- Web interface integration
- Docker containerization

## File Structure

```
lkjomochanet/
├── main.c              # Unified server and client implementation
├── Dockerfile          # Single container definition
├── docker-compose.yml  # Multi-container setup
└── README.md          # This file
```

## Stopping the Services

```bash
docker compose down
```

Or manually stop the processes with Ctrl+C if running directly.
