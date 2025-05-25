# C Language HTTP Server Core

## 1. Project Overview

This project aims to create a foundational, modular, and moderately feature-rich HTTP/1.1 server in C. The server should be designed with clarity, robustness, and adherence to good C programming practices in mind. It will focus on serving static files (with support for chunked transfer for large files), handling basic GET requests, and processing POST requests containing a list of events in JSON format. Event data will be efficiently managed using a B-Tree structure, keyed by `event_id`. Concurrency will be achieved using POSIX threads.

## 2. Core Features

*   **HTTP/1.1 Compliance (Basic):**
    *   Parsing of HTTP GET and POST requests.
    *   Support for common headers (Host, Content-Type, Content-Length, Connection).
    *   Generation of valid HTTP/1.1 responses.
*   **Request Handling:**
    *   Serving static files from a configurable document root.
    *   Large static files are sent in chunks for efficient transfer.
    *   Handling of `index.html` for directory requests.
    *   Basic MIME type detection based on file extension.
    *   Support for basic URL decoding of paths.
*   **POST Request Handling:**
    *   Ability to receive POST data (e.g., from forms).
    *   Data sent and received in POST requests will primarily be in JSON format.
    *   Specifically, data will be sent and received as a list of events, similar to the Nostr protocol, parsed as JSON by the server, and processed as events.
    *   Processed events are stored and managed in a B-Tree database, keyed by `event_id`.
    *   This enables structured event-based communication and efficient data management.
*   **Event Database Management (B-Tree):**
    *   Manage all events sent by clients in a B-Tree structure based on `event_id`.
    *   Support fast addition, search, update, and deletion operations for events.
    *   Database persistence ensures event data is retained after server restarts.
*   **Error Handling:**
    *   Graceful handling of errors (e.g., file not found, permission denied, bad request).
    *   Returning appropriate HTTP status codes (200, 400, 403, 404, 500, 501).
    *   Customizable error pages (e.g., `404.html`, `500.html`).
*   **Sequentiality:**
    *   Single-threaded.
*   **Configuration:**
    *   Server port (e.g., from command-line arguments or a simple configuration file).
*   **Logging:**
    *   Events are logged to individual JSON files.
*   **Security (Basic Considerations):**
    *   Prevention of directory traversal attacks (ensure requested path is within the document root).
    *   Basic input sanitization for paths.
    *   Be mindful of buffer overflows.

## 3. Project Structure

The project will be organized with the following file structure:

```
.
├── Makefile
├── .gitignore
├── README.md
├── include/
│   ├── http_parser.h
│   ├── json_parser.h
│   ├── http_response.h
│   ├── request_handler.h
│   ├── server_socket.h
│   ├── file_handler.h
│   ├── mime_types.h
│   ├── config.h
│   ├── utils.h
│   └── db.h
├── src/
│   ├── main.c
│   ├── json_parser.c
│   ├── http_parser.c
│   ├── http_response.c
│   ├── request_handler.c
│   ├── server_socket.c
│   ├── file_handler.c
│   ├── mime_types.c
│   ├── config.c
│   ├── utils.c
│   └── db.c
└── routes/
    ├── index.html
    ├── style.css
    ├── script.js
    ├── image.png
    ├── 404.html
    └── 500.html
```

### Module Responsibilities:

*   **`main.c`**:
    *   Parses command-line arguments (port, document root) or loads configuration.
    *   Initializes the server socket.
    *   Initializes the event database (`db.c`).
    *   Enters the main accept loop, dispatching new connections to handler threads.
    *   Properly closes the event database on shutdown.
*   **`server_socket.c/.h`**:
    *   Functions for creating, binding, and listening on the server socket.
    *   Function to accept new client connections.
*   **`http_parser.c/.h`**:
    *   Functions to parse incoming HTTP request strings from client sockets.
    *   Extracts method (GET, POST), URI, HTTP version, headers, and body.
    *   For POST requests, the body is expected to be a JSON list of events, to be parsed by the subsequent `json_parser.c`.
    *   Stores parsed information in a structured way (e.g., `HttpRequest` struct).
    *   Handles basic URL decoding of paths.
*   **`json_parser.c/.h`**:
    *   Responsible for parsing JSON-formatted event lists used in POST request bodies (and potentially response bodies), converting them into a data structure usable by the application (e.g., an array of event structures).
*   **`request_handler.c/.h`**:
    *   Core logic for processing parsed HTTP requests.
    *   Each client connection thread starts here.
    *   Determines the request type (e.g., GET for static file, POST).
    *   For POST requests, it retrieves the list of events parsed by `json_parser.c` and stores/processes each event in the B-Tree database using the `db.c` module.
    *   When serving large static files, it coordinates with `file_handler.c` and `http_response.c` to control chunked transfer.
    *   Calls appropriate functions from `file_handler.c` and other modules.
    *   Coordinates response generation using `http_response.c`.
*   **`http_response.c/.h`**:
    *   Functions to construct and send HTTP responses.
    *   When generating responses for large files, provides functionality to support chunked transfer (e.g., HTTP chunked encoding).
    *   Builds status line, headers, and body.
    *   Sends data to the client socket.
    *   Helper functions to send standard error responses (400, 403, 404, 500, 501).
*   **`file_handler.c/.h`**:
    *   Functions for serving static files.
    *   Resolves file paths relative to the document root.
    *   Performs security checks (e.g., prevention of directory traversal).
    *   Reads file content and determines its size.
    *   When handling large files, provides functionality to read files in chunks for efficient segmented transmission.
    *   Uses `mime_types.c` to get the `Content-Type`.
    *   Handles `index.html` if a directory is requested.
*   **`mime_types.c/.h`**:
    *   Function to map file extensions to MIME types (e.g., ".html" -> "text/html").
    *   Can use a simple hardcoded lookup table.
*   **`config.c/.h`**:
    *   Functions to load server configuration (port, document root, database file path, etc.).
    *   Can start with command-line argument parsing and later be extended to a simple configuration file.
    *   Defines a struct to hold configuration values.
*   **`utils.c/.h`**:
    *   Utility functions:
        *   String manipulation helpers (if needed beyond standard library).
        *   Logging functions (e.g., `log_info()`, `log_error()`, `log_debug()`).
        *   Safe memory allocation wrappers (e.g., `safe_malloc`, `safe_realloc`).
        *   URL decoding.
*   **`db.c/.h`**:
    *   Database module for persistent management of events sent by clients.
    *   Events are efficiently stored and retrieved using a B-Tree structure, keyed by a unique `event_id`.
    *   **Key Features:**
        *   Database initialization (`db_init`) and closing (`db_close`).
        *   Adding an event (`db_add_event`).
        *   Finding an event by `event_id` (`db_find_event_by_id`).
        *   (Optional) Updating an event (`db_update_event`).
        *   (Optional) Deleting an event by `event_id` (`db_delete_event_by_id`).
        *   (Optional) Range search for events based on specific criteria.
    *   The B-Tree implementation enables fast search, insertion, and deletion operations (O(log n) complexity) even for a large number of events.
    *   Handles data persistence (e.g., writing to and reading from a dedicated database file) to retain event information after server restarts.

## 4. Build Instructions

*   A `Makefile` should be provided.
*   It should compile all `.c` files in the `src/` directory and link them into an executable (e.g., `c_http_server`).
*   Compiler flags: `-Wall -Wextra -pedantic -std=c11 -g` (for development). Link with `-pthread`.
*   A `clean` target to remove compiled files.
*   An `all` target (default) to build the project.

## 5. Usage

The server should be executable from the command line:

```bash
./c_http_server [port] [document_root] [database_file_path]
```
Example:
```bash
./c_http_server 8080 ./routes ./events.db
```
If arguments are not specified, appropriate default values (e.g., port 8080, document root `./routes`, database file `./events.db`) should be used.

## 6. Dependencies

*   Standard C Library (`stdio.h`, `stdlib.h`, `string.h`, `errno.h`, etc.)
*   POSIX Libraries:
    *   `sys/socket.h`, `netinet/in.h`, `arpa/inet.h` (for socket programming)
    *   `unistd.h` (for `read`, `write`, `close`)
    *   `pthread.h` (for threading)
    *   `sys/stat.h`, `fcntl.h` (for file operations)
    *   `dirent.h` (for directory listing, if implemented beyond `index.html`)
    *   `signal.h` (for graceful shutdown)

## 7. Non-Functional Requirements

*   **Code Clarity:** Sufficient comments, especially for complex logic. Meaningful variable and function names.
*   **Error Handling:** Robust error checking for all system calls and library functions.
*   **Memory Management:** No memory leaks. All dynamically allocated memory must be freed.
*   **Portability:** Aim for POSIX compliance. Avoid platform-specific extensions where possible.
*   **Modularity:** Each module should have clear responsibilities and a well-defined interface (header file).
*   **Data Persistence:** Event data should not be lost after server shutdown/restart.
*   **Efficiency:** Efficient event search, insertion, and deletion through the use of a B-Tree.