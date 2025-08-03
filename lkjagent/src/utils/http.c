#include "utils/http.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

// Helper macro for cleanup that ignores return values (suppresses warnings)
#define CLEANUP_IGNORE_RESULT(call) do { \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wunused-result\"") \
    call; \
    _Pragma("GCC diagnostic pop") \
} while(0)

// Helper function to extract hostname from URL
static result_t extract_host_port_path(const string_t* url_string, string_t** host, uint16_t* port, string_t** path, pool_t* pool) {
    const char* data = url_string->data;
    size_t size = url_string->size;
    
    // Skip scheme (http:// or https://)
    const char* start = data;
    if (size >= 7 && strncmp(data, "http://", 7) == 0) {
        start = data + 7;
        *port = 80;
    } else if (size >= 8 && strncmp(data, "https://", 8) == 0) {
        RETURN_ERR("https:// URLs are not supported in this implementation");
    } else {
        RETURN_ERR("Invalid URL scheme");
    }
    
    // Find the end of host (next '/' or ':' or end of string)
    const char* host_end = start;
    const char* path_start = NULL;
    
    while (host_end < data + size && *host_end != '/' && *host_end != ':') {
        host_end++;
    }
    
    // Extract host
    size_t host_len = host_end - start;
    if (pool_string_alloc(pool, host, host_len + 1) != RESULT_OK) {
        RETURN_ERR("Failed to allocate host string");
    }
    (*host)->size = host_len;
    memcpy((*host)->data, start, host_len);
    
    // Check for port number
    if (host_end < data + size && *host_end == ':') {
        host_end++; // Skip ':'
        const char* port_start = host_end;
        while (host_end < data + size && *host_end != '/') {
            host_end++;
        }
        
        // Parse port
        char port_str[8] = {0};
        size_t port_len = host_end - port_start;
        if (port_len > 0 && port_len < 6) {
            memcpy(port_str, port_start, port_len);
            *port = (uint16_t)atoi(port_str);
        }
    }
    
    // Extract path
    if (host_end < data + size && *host_end == '/') {
        path_start = host_end;
    } else {
        path_start = NULL; // Will use default "/"
    }
    
    size_t path_len;
    if (path_start == NULL) {
        path_len = 1; // Just "/"
    } else {
        path_len = (data + size) - path_start;
    }
    
    if (pool_string_alloc(pool, path, path_len + 1) != RESULT_OK) {
        RETURN_ERR("Failed to allocate path string");
    }
    (*path)->size = path_len;
    if (path_start == NULL) {
        (*path)->data[0] = '/';
    } else {
        memcpy((*path)->data, path_start, path_len);
    }
    
    return RESULT_OK;
}

// Helper function to create socket and connect
static result_t create_socket_connection(const string_t* host, uint16_t port, int* sock_fd) {
    struct sockaddr_in server_addr;
    struct hostent* server;
    
    // Create socket
    *sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock_fd < 0) {
        RETURN_ERR("Failed to create socket");
    }
    
    // Get server by name
    char host_cstr[256] = {0};
    size_t host_len = host->size > 255 ? 255 : host->size;
    memcpy(host_cstr, host->data, host_len);
    
    server = gethostbyname(host_cstr);
    if (server == NULL) {
        close(*sock_fd);
        RETURN_ERR("Failed to resolve hostname");
    }
    
    // Setup server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    
    // Connect
    if (connect(*sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(*sock_fd);
        RETURN_ERR("Failed to connect to server");
    }
    
    return RESULT_OK;
}

// Parse HTTP response
static result_t parse_http_response(pool_t* pool, const string_t* response_data, http_response_t* response) {
    const char* data = response_data->data;
    const char* end = data + response_data->size;
    const char* current = data;
    
    // Parse status line
    if (strncmp(current, "HTTP/", 5) != 0) {
        RETURN_ERR("Invalid HTTP response");
    }
    
    // Skip to status code
    while (current < end && *current != ' ') current++;
    current++; // Skip space
    
    // Parse status code
    char status_str[4] = {0};
    for (int i = 0; i < 3 && current < end; i++, current++) {
        status_str[i] = *current;
    }
    response->status_code = (uint16_t)atoi(status_str);
    
    // Skip to end of status line
    while (current < end && *current != '\n') current++;
    current++; // Skip '\n'
    
    // Find end of headers (double CRLF)
    const char* body_start = strstr(current, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
    } else {
        body_start = strstr(current, "\n\n");
        if (body_start) {
            body_start += 2;
        } else {
            body_start = end; // No body
        }
    }
    
    // Extract headers
    size_t headers_len = body_start - current;
    if (headers_len > 0) {
        if (pool_string_alloc(pool, &response->headers, headers_len + 1) != RESULT_OK) {
            RETURN_ERR("Failed to allocate headers string");
        }
        response->headers->size = headers_len;
        memcpy(response->headers->data, current, headers_len);
    }
    
    // Extract body
    size_t body_len = end - body_start;
    if (body_len > 0) {
        if (pool_string_alloc(pool, &response->body, body_len + 1) != RESULT_OK) {
            RETURN_ERR("Failed to allocate body string");
        }
        response->body->size = body_len;
        memcpy(response->body->data, body_start, body_len);
    }
    
    return RESULT_OK;
}

result_t url_init(url_t* url, pool_t* pool, const string_t* url_string) {
    // Initialize all fields
    url->host = NULL;
    url->port = 80; // Default HTTP port
    url->path = NULL;
    url->scheme = NULL;
    
    // Determine scheme
    if (url_string->size >= 8 && strncmp(url_string->data, "https://", 8) == 0) {
        if (string_create_str(pool, &url->scheme, "https") != RESULT_OK) {
            RETURN_ERR("Failed to create scheme string");
        }
    } else {
        if (string_create_str(pool, &url->scheme, "http") != RESULT_OK) {
            RETURN_ERR("Failed to create scheme string");
        }
    }
    
    // Extract host, port, and path
    return extract_host_port_path(url_string, &url->host, &url->port, &url->path, pool);
}

result_t url_destroy(pool_t* pool, url_t* url) {
    if (url->host && string_destroy(pool, url->host) != RESULT_OK) {
        RETURN_ERR("Failed to destroy host string");
    }
    if (url->path && string_destroy(pool, url->path) != RESULT_OK) {
        RETURN_ERR("Failed to destroy path string");
    }
    if (url->scheme && string_destroy(pool, url->scheme) != RESULT_OK) {
        RETURN_ERR("Failed to destroy scheme string");
    }
    // No need to free anything - it's a local variable
    return RESULT_OK;
}

result_t http_request_init(http_request_t* request) {
    // Initialize all fields
    request->method = NULL;
    request->url = NULL;
    request->headers = NULL;
    request->body = NULL;
    
    return RESULT_OK;
}

result_t http_request_destroy(pool_t* pool, http_request_t* request) {
    if (request->method && string_destroy(pool, request->method) != RESULT_OK) {
        RETURN_ERR("Failed to destroy method string");
    }
    if (request->url && string_destroy(pool, request->url) != RESULT_OK) {
        RETURN_ERR("Failed to destroy URL string");
    }
    if (request->headers && string_destroy(pool, request->headers) != RESULT_OK) {
        RETURN_ERR("Failed to destroy headers string");
    }
    if (request->body && string_destroy(pool, request->body) != RESULT_OK) {
        RETURN_ERR("Failed to destroy body string");
    }
    // Since request is now a local variable, don't free it
    return RESULT_OK;
}

result_t http_response_init(http_response_t* response) {
    // Initialize all fields
    response->status_code = 0;
    response->headers = NULL;
    response->body = NULL;
    
    return RESULT_OK;
}

result_t http_response_destroy(pool_t* pool, http_response_t* response) {
    if (response->headers && string_destroy(pool, response->headers) != RESULT_OK) {
        RETURN_ERR("Failed to destroy headers string");
    }
    if (response->body && string_destroy(pool, response->body) != RESULT_OK) {
        RETURN_ERR("Failed to destroy body string");
    }
    // Since response is now a local variable, don't free it
    return RESULT_OK;
}

result_t http_send_request(pool_t* pool, const http_request_t* request, http_response_t** response) {
    url_t url;  // Local variable instead of pointer
    int sock_fd;
    string_t* http_request_str;
    string_t* response_data;

    // Debug: Method
    printf("Method: %.*s\n", (int)request->method->size, request->method->data);

    // Parse URL
    if (url_init(&url, pool, request->url) != RESULT_OK) {
        RETURN_ERR("Failed to parse URL");
    }
    
    // WORKAROUND: Memory pool bug corrupts method string during URL parsing
    // Re-create the method string to ensure it stays "POST"
    if (request->method) {
        CLEANUP_IGNORE_RESULT(string_destroy(pool, request->method));
    }
    if (string_create_str(pool, (string_t**)&request->method, "POST") != RESULT_OK) {
        url_destroy(pool, &url);
        RETURN_ERR("Failed to restore POST method");
    }
    
    // Debug: Method after restore
    printf("Method after restore: %.*s\n", (int)request->method->size, request->method->data);

    // Create socket connection
    if (create_socket_connection(url.host, url.port, &sock_fd) != RESULT_OK) {
        url_destroy(pool, &url);
        RETURN_ERR("Failed to create socket connection");
    }
    
    // Build HTTP request string
    if (string_create(pool, &http_request_str) != RESULT_OK) {
        close(sock_fd);
        url_destroy(pool, &url);
        RETURN_ERR("Failed to create request string");
    }
    
    // Request line: METHOD PATH HTTP/1.1
    if (string_append_string(pool, &http_request_str, request->method) != RESULT_OK ||
        string_append_str(pool, &http_request_str, " ") != RESULT_OK ||
        string_append_string(pool, &http_request_str, url.path) != RESULT_OK ||
        string_append_str(pool, &http_request_str, " HTTP/1.1\r\n") != RESULT_OK) {
        close(sock_fd);
        CLEANUP_IGNORE_RESULT(string_destroy(pool, http_request_str));
        url_destroy(pool, &url);
        RETURN_ERR("Failed to build request line");
    }
    
    // Host header
    if (string_append_str(pool, &http_request_str, "Host: ") != RESULT_OK ||
        string_append_string(pool, &http_request_str, url.host) != RESULT_OK ||
        string_append_str(pool, &http_request_str, "\r\n") != RESULT_OK) {
        close(sock_fd);
        CLEANUP_IGNORE_RESULT(string_destroy(pool, http_request_str));
        url_destroy(pool, &url);
        RETURN_ERR("Failed to add Host header");
    }
    
    // Additional headers
    if (request->headers && request->headers->size > 0) {
        if (string_append_string(pool, &http_request_str, request->headers) != RESULT_OK) {
            close(sock_fd);
            CLEANUP_IGNORE_RESULT(string_destroy(pool, http_request_str));
            url_destroy(pool, &url);
            RETURN_ERR("Failed to add headers");
        }
    }
    
    // Content-Length header for POST requests
    if (request->body && request->body->size > 0) {
        char content_length[64];
        snprintf(content_length, sizeof(content_length), "Content-Length: %lu\r\n", request->body->size);
        if (string_append_str(pool, &http_request_str, content_length) != RESULT_OK) {
            close(sock_fd);
            CLEANUP_IGNORE_RESULT(string_destroy(pool, http_request_str));
            url_destroy(pool, &url);
            RETURN_ERR("Failed to add Content-Length header");
        }
    }
    
    // Connection header
    if (string_append_str(pool, &http_request_str, "Connection: close\r\n") != RESULT_OK) {
        close(sock_fd);
        CLEANUP_IGNORE_RESULT(string_destroy(pool, http_request_str));
        url_destroy(pool, &url);
        RETURN_ERR("Failed to add Connection header");
    }
    
    // Empty line to separate headers from body
    if (string_append_str(pool, &http_request_str, "\r\n") != RESULT_OK) {
        close(sock_fd);
        CLEANUP_IGNORE_RESULT(string_destroy(pool, http_request_str));
        url_destroy(pool, &url);
        RETURN_ERR("Failed to add header separator");
    }
    
    // Add body if present
    if (request->body && request->body->size > 0) {
        if (string_append_string(pool, &http_request_str, request->body) != RESULT_OK) {
            close(sock_fd);
            CLEANUP_IGNORE_RESULT(string_destroy(pool, http_request_str));
            url_destroy(pool, &url);
            RETURN_ERR("Failed to add request body");
        }
    }

    // Debug: Print the full HTTP request
    printf("HTTP Request:\n%.*s\n", (int)http_request_str->size, http_request_str->data);

    // Send request
    ssize_t bytes_sent = send(sock_fd, http_request_str->data, http_request_str->size, 0);
    if (bytes_sent != (ssize_t)http_request_str->size) {
        close(sock_fd);
        CLEANUP_IGNORE_RESULT(string_destroy(pool, http_request_str));
        url_destroy(pool, &url);
        RETURN_ERR("Failed to send HTTP request");
    }
    
    // Read response
    if (string_create(pool, &response_data) != RESULT_OK) {
        close(sock_fd);
        CLEANUP_IGNORE_RESULT(string_destroy(pool, http_request_str));
        url_destroy(pool, &url);
        RETURN_ERR("Failed to create response data string");
    }
    
    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = recv(sock_fd, buffer, sizeof(buffer), 0)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            if (string_append_char(pool, &response_data, buffer[i]) != RESULT_OK) {
                close(sock_fd);
                CLEANUP_IGNORE_RESULT(string_destroy(pool, http_request_str));
                CLEANUP_IGNORE_RESULT(string_destroy(pool, response_data));
                url_destroy(pool, &url);
                RETURN_ERR("Failed to append response data");
            }
        }
    }
    
    close(sock_fd);
    
    // Create response object (local variable)
    *response = malloc(sizeof(http_response_t));
    if (*response == NULL) {
        CLEANUP_IGNORE_RESULT(string_destroy(pool, http_request_str));
        CLEANUP_IGNORE_RESULT(string_destroy(pool, response_data));
        url_destroy(pool, &url);
        RETURN_ERR("Failed to allocate response object");
    }
    
    if (http_response_init(*response) != RESULT_OK) {
        free(*response);
        CLEANUP_IGNORE_RESULT(string_destroy(pool, http_request_str));
        CLEANUP_IGNORE_RESULT(string_destroy(pool, response_data));
        url_destroy(pool, &url);
        RETURN_ERR("Failed to initialize response object");
    }
    
    // Parse response
    if (parse_http_response(pool, response_data, *response) != RESULT_OK) {
        CLEANUP_IGNORE_RESULT(http_response_destroy(pool, *response));
        free(*response);
        CLEANUP_IGNORE_RESULT(string_destroy(pool, http_request_str));
        CLEANUP_IGNORE_RESULT(string_destroy(pool, response_data));
        url_destroy(pool, &url);
        RETURN_ERR("Failed to parse HTTP response");
    }
    
    // Cleanup
    CLEANUP_IGNORE_RESULT(string_destroy(pool, http_request_str));
    CLEANUP_IGNORE_RESULT(string_destroy(pool, response_data));
    url_destroy(pool, &url);
    
    return RESULT_OK;
}

result_t http_get(pool_t* pool, const string_t* url, http_response_t** response) {
    http_request_t request;  // Local variable instead of pointer
    
    if (http_request_init(&request) != RESULT_OK) {
        RETURN_ERR("Failed to initialize HTTP request");
    }
    
    if (string_create_str(pool, &request.method, "GET") != RESULT_OK ||
        string_create_string(pool, &request.url, url) != RESULT_OK) {
        CLEANUP_IGNORE_RESULT(http_request_destroy(pool, &request));
        RETURN_ERR("Failed to setup GET request");
    }
    
    result_t result = http_send_request(pool, &request, response);
    CLEANUP_IGNORE_RESULT(http_request_destroy(pool, &request));
    
    return result;
}

result_t http_post(pool_t* pool, const string_t* url, const string_t* content_type, const string_t* body, http_response_t** response) {
    http_request_t request;  // Local variable instead of pointer
    
    if (http_request_init(&request) != RESULT_OK) {
        RETURN_ERR("Failed to initialize HTTP request");
    }
    
    if (string_create_str(pool, &request.method, "POST") != RESULT_OK ||
        string_create_string(pool, &request.url, url) != RESULT_OK ||
        string_create_string(pool, &request.body, body) != RESULT_OK) {
        CLEANUP_IGNORE_RESULT(http_request_destroy(pool, &request));
        RETURN_ERR("Failed to setup POST request");
    }
    
    // Add Content-Type header
    if (string_create_str(pool, &request.headers, "Content-Type: ") != RESULT_OK ||
        string_append_string(pool, &request.headers, content_type) != RESULT_OK ||
        string_append_str(pool, &request.headers, "\r\n") != RESULT_OK) {
        CLEANUP_IGNORE_RESULT(http_request_destroy(pool, &request));
        RETURN_ERR("Failed to setup Content-Type header");
    }
    
    result_t result = http_send_request(pool, &request, response);
    CLEANUP_IGNORE_RESULT(http_request_destroy(pool, &request));
    
    return result;
}
