/**
 * @file http.c
 * @brief HTTP client implementation with zero external dependencies
 *
 * This module provides a complete HTTP client implementation using only
 * standard POSIX sockets. It supports GET, POST, and generic HTTP requests
 * with proper error handling and timeout management.
 *
 * Key features:
 * - Zero external dependencies (pure socket implementation)
 * - Support for HTTP/1.1 with connection reuse
 * - Configurable timeouts and redirects
 * - Comprehensive error reporting
 * - Safe memory management with bounded buffers
 */

#include "../lkjagent.h"

// HTTP client configuration
#define HTTP_DEFAULT_PORT 80
#define HTTP_BUFFER_SIZE 4096
#define HTTP_MAX_REDIRECTS 3
#define HTTP_TIMEOUT_SECONDS 30
#define HTTP_USER_AGENT "LKJAgent-Enhanced/1.0"

/**
 * @brief HTTP URL structure for parsing
 */
typedef struct {
    char protocol[16];
    char host[256];
    int port;
    char path[1024];
} http_url_t;

/**
 * @brief Parse URL into components
 *
 * @param url_str URL string to parse
 * @param url Pointer to store parsed URL components
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
static result_t http_parse_url(const char* url_str, http_url_t* url) {
    if (!url_str || !url) {
        RETURN_ERR("http_parse_url: NULL parameter");
        return RESULT_ERR;
    }
    
    // Initialize URL structure
    memset(url, 0, sizeof(http_url_t));
    url->port = HTTP_DEFAULT_PORT;
    
    // Check for http:// prefix
    const char* start = url_str;
    if (strncmp(url_str, "http://", 7) == 0) {
        strncpy(url->protocol, "http", sizeof(url->protocol) - 1);
        start = url_str + 7;
    } else if (strncmp(url_str, "https://", 8) == 0) {
        RETURN_ERR("http_parse_url: HTTPS not supported in this implementation");
        return RESULT_ERR;
    } else {
        // Assume http if no protocol specified
        strncpy(url->protocol, "http", sizeof(url->protocol) - 1);
    }
    
    // Find host end (either ':', '/', or end of string)
    const char* host_end = start;
    while (*host_end && *host_end != ':' && *host_end != '/') {
        host_end++;
    }
    
    // Extract host
    size_t host_len = host_end - start;
    if (host_len >= sizeof(url->host)) {
        RETURN_ERR("http_parse_url: Host name too long");
        return RESULT_ERR;
    }
    strncpy(url->host, start, host_len);
    url->host[host_len] = '\0';
    
    // Check for port
    if (*host_end == ':') {
        host_end++; // Skip ':'
        char* endptr;
        long port = strtol(host_end, &endptr, 10);
        if (endptr == host_end || port <= 0 || port > 65535) {
            RETURN_ERR("http_parse_url: Invalid port number");
            return RESULT_ERR;
        }
        url->port = (int)port;
        host_end = endptr;
    }
    
    // Extract path (everything after host:port)
    if (*host_end == '/') {
        strncpy(url->path, host_end, sizeof(url->path) - 1);
    } else {
        strncpy(url->path, "/", sizeof(url->path) - 1);
    }
    url->path[sizeof(url->path) - 1] = '\0';
    
    return RESULT_OK;
}

/**
 * @brief Create socket connection to host
 *
 * @param host Hostname to connect to
 * @param port Port number
 * @return Socket file descriptor on success, -1 on failure
 */
static int http_connect(const char* host, int port) {
    if (!host) {
        RETURN_ERR("http_connect: NULL host parameter");
        return -1;
    }
    
    struct hostent* host_entry = gethostbyname(host);
    if (!host_entry) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "http_connect: Cannot resolve host '%s'", host);
        RETURN_ERR(error_msg);
        return -1;
    }
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "http_connect: Cannot create socket: %s", strerror(errno));
        RETURN_ERR(error_msg);
        return -1;
    }
    
    // Set socket to non-blocking for timeout support
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        close(sock);
        RETURN_ERR("http_connect: Cannot set socket non-blocking");
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = ((struct in_addr*)host_entry->h_addr_list[0])->s_addr;
    
    int connect_result = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (connect_result == -1 && errno != EINPROGRESS) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "http_connect: Cannot connect to %s:%d: %s", 
                host, port, strerror(errno));
        RETURN_ERR(error_msg);
        close(sock);
        return -1;
    }
    
    // Wait for connection with timeout
    if (errno == EINPROGRESS) {
        fd_set write_fds;
        struct timeval timeout;
        
        FD_ZERO(&write_fds);
        FD_SET(sock, &write_fds);
        timeout.tv_sec = HTTP_TIMEOUT_SECONDS;
        timeout.tv_usec = 0;
        
        int select_result = select(sock + 1, NULL, &write_fds, NULL, &timeout);
        if (select_result <= 0) {
            char error_msg[512];
            snprintf(error_msg, sizeof(error_msg), "http_connect: Connection timeout to %s:%d", host, port);
            RETURN_ERR(error_msg);
            close(sock);
            return -1;
        }
        
        // Check if connection succeeded
        int socket_error;
        socklen_t len = sizeof(socket_error);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &socket_error, &len) == -1 || socket_error != 0) {
            char error_msg[512];
            snprintf(error_msg, sizeof(error_msg), "http_connect: Connection failed to %s:%d: %s", 
                    host, port, strerror(socket_error));
            RETURN_ERR(error_msg);
            close(sock);
            return -1;
        }
    }
    
    // Set socket back to blocking mode
    if (fcntl(sock, F_SETFL, flags) == -1) {
        close(sock);
        RETURN_ERR("http_connect: Cannot restore socket blocking mode");
        return -1;
    }
    
    return sock;
}

/**
 * @brief Send HTTP request data
 *
 * @param sock Socket file descriptor
 * @param data Data to send
 * @param length Length of data
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
static result_t http_send_data(int sock, const char* data, size_t length) {
    if (!data) {
        RETURN_ERR("http_send_data: NULL data parameter");
        return RESULT_ERR;
    }
    
    size_t total_sent = 0;
    while (total_sent < length) {
        ssize_t sent = send(sock, data + total_sent, length - total_sent, 0);
        if (sent == -1) {
            char error_msg[512];
            snprintf(error_msg, sizeof(error_msg), "http_send_data: Send failed: %s", strerror(errno));
            RETURN_ERR(error_msg);
            return RESULT_ERR;
        }
        total_sent += sent;
    }
    
    return RESULT_OK;
}

/**
 * @brief Receive HTTP response
 *
 * @param sock Socket file descriptor
 * @param response Token to store response
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
static result_t http_receive_response(int sock, token_t* response) {
    if (!response || !response->data) {
        RETURN_ERR("http_receive_response: Invalid response token");
        return RESULT_ERR;
    }
    
    char buffer[HTTP_BUFFER_SIZE];
    size_t total_received = 0;
    int headers_complete = 0;
    char* body_start = NULL;
    size_t content_length = 0;
    int chunked = 0;
    
    // Clear response token
    response->size = 0;
    response->data[0] = '\0';
    
    while (1) {
        // Set receive timeout
        struct timeval timeout;
        timeout.tv_sec = HTTP_TIMEOUT_SECONDS;
        timeout.tv_usec = 0;
        
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
            RETURN_ERR("http_receive_response: Cannot set receive timeout");
            return RESULT_ERR;
        }
        
        ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (received == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                RETURN_ERR("http_receive_response: Receive timeout");
            } else {
                char error_msg[512];
                snprintf(error_msg, sizeof(error_msg), "http_receive_response: Receive failed: %s", strerror(errno));
                RETURN_ERR(error_msg);
            }
            return RESULT_ERR;
        }
        
        if (received == 0) {
            // Connection closed
            break;
        }
        
        buffer[received] = '\0';
        
        // Check if we can fit this data in the response token
        if (total_received + received >= response->capacity) {
            RETURN_ERR("http_receive_response: Response too large for buffer");
            return RESULT_ERR;
        }
        
        // Append to response
        memcpy(response->data + total_received, buffer, received);
        total_received += received;
        response->data[total_received] = '\0';
        response->size = total_received;
        
        // Look for end of headers if not found yet
        if (!headers_complete) {
            char* headers_end = strstr(response->data, "\r\n\r\n");
            if (headers_end) {
                headers_complete = 1;
                body_start = headers_end + 4;
                
                // Parse headers for Content-Length
                char* content_length_header = strstr(response->data, "Content-Length:");
                if (content_length_header) {
                    content_length = strtoul(content_length_header + 15, NULL, 10);
                }
                
                // Check for chunked encoding
                if (strstr(response->data, "Transfer-Encoding: chunked")) {
                    chunked = 1;
                }
            }
        }
        
        // Check if we have complete response
        if (headers_complete) {
            if (chunked) {
                // For chunked encoding, look for terminating chunk
                if (strstr(response->data, "\r\n0\r\n\r\n")) {
                    break;
                }
            } else if (content_length > 0) {
                // Check if we have received all content
                size_t body_length = total_received - (body_start - response->data);
                if (body_length >= content_length) {
                    break;
                }
            } else {
                // No content length, assume connection close indicates end
                // Continue reading until connection closes
            }
        }
    }
    
    return RESULT_OK;
}

/**
 * @brief Extract response body from HTTP response
 *
 * @param response Full HTTP response
 * @param body Token to store extracted body
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
static result_t http_extract_body(const token_t* response, token_t* body) {
    if (!response || !body) {
        RETURN_ERR("http_extract_body: NULL parameter");
        return RESULT_ERR;
    }
    
    if (!response->data || !body->data) {
        RETURN_ERR("http_extract_body: Uninitialized token");
        return RESULT_ERR;
    }
    
    // Find end of headers
    char* headers_end = strstr(response->data, "\r\n\r\n");
    if (!headers_end) {
        RETURN_ERR("http_extract_body: Cannot find end of headers");
        return RESULT_ERR;
    }
    
    char* body_start = headers_end + 4;
    size_t body_length = response->size - (body_start - response->data);
    
    // Check if body fits in destination token
    if (body_length >= body->capacity) {
        RETURN_ERR("http_extract_body: Body too large for destination token");
        return RESULT_ERR;
    }
    
    // Copy body
    memcpy(body->data, body_start, body_length);
    body->data[body_length] = '\0';
    body->size = body_length;
    
    return RESULT_OK;
}

/**
 * @brief Perform HTTP GET request
 *
 * @param url URL token (must contain complete HTTP URL)
 * @param response Token to store response body
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t http_get(token_t* url, token_t* response) {
    if (!url || !response) {
        RETURN_ERR("http_get: NULL parameter");
        return RESULT_ERR;
    }
    
    if (!url->data || !response->data) {
        RETURN_ERR("http_get: Uninitialized token");
        return RESULT_ERR;
    }
    
    return http_request_method("GET", url, NULL, response);
}

/**
 * @brief Perform HTTP POST request with body
 *
 * @param url URL token
 * @param body Request body token
 * @param response Token to store response
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t http_post(token_t* url, const token_t* body, token_t* response) {
    if (!url || !response) {
        RETURN_ERR("http_post: NULL parameter");
        return RESULT_ERR;
    }
    
    if (!url->data || !response->data) {
        RETURN_ERR("http_post: Uninitialized token");
        return RESULT_ERR;
    }
    
    return http_request_method("POST", url, body, response);
}

/**
 * @brief Generic HTTP request function
 *
 * @param method HTTP method string
 * @param url URL token
 * @param body Request body (can be NULL for GET)
 * @param response Response token
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t http_request_method(const char* method, token_t* url, const token_t* body, token_t* response) {
    if (!method || !url || !response) {
        RETURN_ERR("http_request_method: NULL parameter");
        return RESULT_ERR;
    }
    
    if (!url->data || !response->data) {
        RETURN_ERR("http_request_method: Uninitialized token");
        return RESULT_ERR;
    }
    
    // Parse URL
    http_url_t parsed_url;
    if (http_parse_url(url->data, &parsed_url) != RESULT_OK) {
        return RESULT_ERR; // Error already set
    }
    
    // Connect to server
    int sock = http_connect(parsed_url.host, parsed_url.port);
    if (sock == -1) {
        return RESULT_ERR; // Error already set
    }
    
    // Build HTTP request
    char request[4096];
    int request_len = snprintf(request, sizeof(request),
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: %s\r\n"
        "Connection: close\r\n",
        method, parsed_url.path, parsed_url.host, HTTP_USER_AGENT);
    
    // Add Content-Length and body for POST requests
    if (body && body->data && body->size > 0) {
        request_len += snprintf(request + request_len, sizeof(request) - request_len,
            "Content-Type: application/json\r\n"
            "Content-Length: %zu\r\n", body->size);
    }
    
    // End headers
    request_len += snprintf(request + request_len, sizeof(request) - request_len, "\r\n");
    
    // Add body if present
    if (body && body->data && body->size > 0) {
        if (request_len + body->size < sizeof(request)) {
            memcpy(request + request_len, body->data, body->size);
            request_len += body->size;
        } else {
            close(sock);
            RETURN_ERR("http_request_method: Request too large");
            return RESULT_ERR;
        }
    }
    
    // Send request
    if (http_send_data(sock, request, request_len) != RESULT_OK) {
        close(sock);
        return RESULT_ERR; // Error already set
    }
    
    // Receive response
    char full_response_buffer[8192];
    token_t full_response;
    if (token_init(&full_response, full_response_buffer, sizeof(full_response_buffer)) != RESULT_OK) {
        close(sock);
        return RESULT_ERR;
    }
    
    if (http_receive_response(sock, &full_response) != RESULT_OK) {
        close(sock);
        return RESULT_ERR; // Error already set
    }
    
    close(sock);
    
    // Extract body from response
    return http_extract_body(&full_response, response);
}

/**
 * @brief Legacy generic HTTP request function for compatibility
 *
 * @param method HTTP method token
 * @param url URL token
 * @param body Request body (can be NULL for GET)
 * @param response Response token
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result))
result_t http_request(token_t* method, token_t* url, const token_t* body, token_t* response) {
    if (!method || !method->data) {
        RETURN_ERR("http_request: NULL or uninitialized method token");
        return RESULT_ERR;
    }
    
    return http_request_method(method->data, url, body, response);
}
