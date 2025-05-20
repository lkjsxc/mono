# C HTTP Server Core

## 1. Project Overview

This project aims to create a foundational, modular, and reasonably feature-rich HTTP/1.1 server in C. The server should be designed with clarity, robustness, and adherence to good C programming practices. It will focus on serving static files and handling basic GET and POST requests. Concurrency will be achieved using POSIX threads.

## 2. Core Features

*   **HTTP/1.1 Compliance (Basic):**
    *   Parse HTTP GET and POST requests.
    *   Support common headers (Host, Content-Type, Content-Length, Connection).
    *   Generate valid HTTP/1.1 responses.
    *   Handle Keep-Alive connections.
*   **Request Handling:**
    *   Serve static files from a configurable document root.
    *   Handle `index.html` for directory requests.
    *   Basic MIME type detection based on file extension.
    *   Support for basic URL decoding for paths.
*   **POST Request Handling:**
    *   Ability to receive POST data (e.g., from a form).
    *   For simplicity, initially, it can just echo back the received POST data in the response or log it.
*   **Error Handling:**
    *   Graceful handling of errors (e.g., file not found, permission denied, bad request).
    *   Return appropriate HTTP status codes (200, 400, 403, 404, 500, 501).
    *   Customizable error pages (e.g., `404.html`, `500.html`).
*   **Concurrency:**
    *   Handle multiple client connections concurrently using POSIX threads (one thread per connection).
*   **Configuration:**
    *   Server port (e.g., from command-line argument or a simple config file).
    *   Document root directory (e.g., from command-line argument or a simple config file).
*   **Logging:**
    *   Basic logging to `stdout` or `stderr` (e.g., incoming requests, errors, server start/stop). Format: `[Timestamp] [Log Level] Message`.
*   **Security (Basic Considerations):**
    *   Prevent directory traversal attacks (ensure requested paths are within the document root).
    *   Basic input sanitization for paths.
    *   Be mindful of buffer overflows.

## 3. Project Structure

The project should be organized into the following file structure:

```
.
├── Makefile
├── .gitignore
├── README.md
├── include/
│   ├── http_parser.h
│   ├── http_response.h
│   ├── request_handler.h
│   ├── server_socket.h
│   ├── file_handler.h
│   ├── mime_types.h
│   ├── config.h
│   └── utils.h
├── src/
│   ├── main.c
│   ├── http_parser.c
│   ├── http_response.c
│   ├── request_handler.c
│   ├── server_socket.c
│   ├── file_handler.c
│   ├── mime_types.c
│   ├── config.c
│   └── utils.c
└── routes/             # Default document root
    ├── index.html      # Sample index page
    ├── style.css       # Sample CSS
    ├── script.js       # Sample JS
    ├── image.png       # Sample image
    ├── 404.html        # Custom 404 error page
    └── 500.html        # Custom 500 error page
```

### Module Responsibilities:

*   **`main.c`**:
    *   Parses command-line arguments (port, document root) or loads configuration.
    *   Initializes the server socket.
    *   Enters the main accept loop, dispatching new connections to handler threads.
    *   Handles graceful shutdown (e.g., on SIGINT).
*   **`server_socket.c/.h`**:
    *   Functions for creating, binding, and listening on a server socket.
    *   Function to accept new client connections.
*   **`http_parser.c/.h`**:
    *   Functions to parse an incoming HTTP request string from a client socket.
    *   Extracts method (GET, POST), URI, HTTP version, headers, and body.
    *   Stores parsed information in a structured way (e.g., a `HttpRequest` struct).
    *   Handles basic URL decoding for the path.
*   **`request_handler.c/.h`**:
    *   The core logic for handling a parsed HTTP request.
    *   This is where each client connection thread will start.
    *   Determines the type of request (e.g., GET static file, POST).
    *   Calls appropriate functions from `file_handler.c` or other modules.
    *   Orchestrates response generation using `http_response.c`.
*   **`http_response.c/.h`**:
    *   Functions to construct and send HTTP responses.
    *   Builds status lines, headers, and body.
    *   Sends data to the client socket.
    *   Helper function to send standard error responses (400, 403, 404, 500, 501).
*   **`file_handler.c/.h`**:
    *   Functions for serving static files.
    *   Resolves file paths relative to the document root.
    *   Performs security checks (e.g., prevent directory traversal).
    *   Reads file content and determines its size.
    *   Uses `mime_types.c` to get the `Content-Type`.
    *   Handles `index.html` if a directory is requested.
*   **`mime_types.c/.h`**:
    *   Function to map file extensions to MIME types (e.g., ".html" -> "text/html").
    *   Can use a simple hardcoded lookup table.
*   **`config.c/.h`**:
    *   Functions to load server configuration (port, document root).
    *   Can start with command-line argument parsing, potentially extendable to a simple config file later.
    *   Defines a struct to hold configuration values.
*   **`utils.c/.h`**:
    *   Utility functions:
        *   String manipulation helpers (if needed beyond standard library).
        *   Logging functions (e.g., `log_info()`, `log_error()`, `log_debug()`).
        *   Safe memory allocation wrappers (e.g., `safe_malloc`, `safe_realloc`).
        *   URL decoding.

## 4. Build Instructions

*   A `Makefile` should be provided.
*   It should compile all `.c` files in the `src/` directory and link them into an executable (e.g., `c_http_server`).
*   Compiler flags: `-Wall -Wextra -pedantic -std=c11 -g` (for development). Link with `-pthread`.
*   A `clean` target to remove compiled files.
*   An `all` target (default) to build the project.

## 5. Usage

The server should be runnable from the command line:

```bash
./c_http_server [port] [document_root]
```
Example:
```bash
./c_http_server 8080 ./routes
```
If arguments are not provided, use sensible defaults (e.g., port 8080, document root `./routes`).

## 6. Dependencies

*   Standard C Library (`stdio.h`, `stdlib.h`, `string.h`, `errno.h`, etc.)
*   POSIX Libraries:
    *   `sys/socket.h`, `netinet/in.h`, `arpa/inet.h` (for socket programming)
    *   `unistd.h` (for `read`, `write`, `close`, `fork` - though we use threads)
    *   `pthread.h` (for threading)
    *   `sys/stat.h`, `fcntl.h` (for file operations)
    *   `dirent.h` (for directory listing, if implemented beyond `index.html`)
    *   `signal.h` (for graceful shutdown)

## 7. Non-Functional Requirements

*   **Code Clarity:** Well-commented code, especially for complex logic. Meaningful variable and function names.
*   **Error Handling:** Robust error checking for all system calls and library functions.
*   **Memory Management:** No memory leaks. All dynamically allocated memory must be freed.
*   **Portability:** Aim for POSIX compliance. Avoid platform-specific extensions where possible.
*   **Modularity:** Each module should have a clear responsibility and a well-defined interface (header file).

## 8. Sample Files in `routes/`

Create simple placeholder files:

*   **`routes/index.html`**:
    ```html
    <!DOCTYPE html>
    <html>
    <head><title>Welcome!</title><link rel="stylesheet" href="style.css"></head>
    <body><h1>Hello from C HTTP Server!</h1><p>This is a test page.</p>
    <img src="image.png" alt="sample image" width="100">
    <form action="/submit_form" method="post">
        <label for="name">Name:</label><input type="text" id="name" name="name"><br>
        <label for="data">Data:</label><input type="text" id="data" name="data"><br>
        <input type="submit" value="Submit POST">
    </form>
    <script src="script.js"></script>
    </body></html>
    ```
*   **`routes/style.css`**:
    ```css
    body { font-family: sans-serif; background-color: #f0f0f0; color: #333; }
    h1 { color: #007bff; }
    ```
*   **`routes/script.js`**:
    ```javascript
    console.log("Sample JavaScript loaded!");
    alert("Hello from JavaScript on the C HTTP Server!");
    ```
*   **`routes/404.html`**:
    ```html
    <!DOCTYPE html><html><head><title>404 Not Found</title></head>
    <body><h1>404 - Page Not Found</h1><p>Sorry, the page you are looking for does not exist.</p></body></html>
    ```
*   **`routes/500.html`**:
    ```html
    <!DOCTYPE html><html><head><title>500 Internal Server Error</title></head>
    <body><h1>500 - Internal Server Error</h1><p>Sorry, something went wrong on our end.</p></body></html>
    ```
*   Create a small dummy `routes/image.png` (any small PNG file will do).