#include "utils/http.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

// Helper function to extract host, port, and path from URL
static result_t extract_url_components(const string_t* url_string, string_t** host, uint16_t* port, string_t** path, pool_t* pool) {
    const char* data = url_string->data;
    size_t size = url_string->size;

    // Skip scheme (http:// or https://)
    const char* start = data;
    if (size >= 7 && strncmp(data, "http://", 7) == 0) {
        start = data + 7;
        *port = 80;
    } else if (size >= 8 && strncmp(data, "https://", 8) == 0) {
        RETURN_ERR("HTTPS URLs are not supported in this implementation");
    } else {
        RETURN_ERR("Invalid URL scheme - only HTTP is supported");
    }

    // Find the end of host (next '/' or ':' or end of string)
    const char* host_end = start;
    const char* path_start = NULL;

    while (host_end < data + size && *host_end != '/' && *host_end != ':') {
        host_end++;
    }

    // Extract host
    size_t host_len = host_end - start;
    if (host_len == 0) {
        RETURN_ERR("Empty hostname in URL");
    }

    if (pool_string_alloc(pool, host, host_len + 1) != RESULT_OK) {
        RETURN_ERR("Failed to allocate host string");
    }
    (*host)->size = host_len;
    memcpy((*host)->data, start, host_len);
    (*host)->data[host_len] = '\0';

    // Check for port number
    if (host_end < data + size && *host_end == ':') {
        host_end++;  // Skip ':'
        const char* port_start = host_end;
        while (host_end < data + size && *host_end != '/') {
            if (!isdigit(*host_end)) {
                if (string_destroy(pool, *host) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy host string");
                }
                RETURN_ERR("Invalid port number in URL");
            }
            host_end++;
        }

        // Parse port
        char port_str[8] = {0};
        size_t port_len = host_end - port_start;
        if (port_len > 0 && port_len < 6) {
            memcpy(port_str, port_start, port_len);
            int parsed_port = atoi(port_str);
            if (parsed_port <= 0 || parsed_port > 65535) {
                if (string_destroy(pool, *host) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy host string");
                }
                RETURN_ERR("Port number out of valid range");
            }
            *port = (uint16_t)parsed_port;
        }
    }

    // Extract path
    if (host_end < data + size && *host_end == '/') {
        path_start = host_end;
    }

    size_t path_len;
    if (path_start == NULL) {
        path_len = 1;  // Just "/"
    } else {
        path_len = (data + size) - path_start;
    }

    if (pool_string_alloc(pool, path, path_len + 1) != RESULT_OK) {
        if (string_destroy(pool, *host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host string");
        }
        RETURN_ERR("Failed to allocate path string");
    }
    (*path)->size = path_len;
    if (path_start == NULL) {
        (*path)->data[0] = '/';
        (*path)->data[1] = '\0';
    } else {
        memcpy((*path)->data, path_start, path_len);
        (*path)->data[path_len] = '\0';
    }

    return RESULT_OK;
}

// Helper function to create socket and connect to server
static result_t create_connection(const string_t* host, uint16_t port, int* sock_fd) {
    struct sockaddr_in server_addr;
    struct hostent* server;

    // Create socket
    *sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock_fd < 0) {
        RETURN_ERR("Failed to create TCP socket");
    }

    // Resolve hostname
    char host_cstr[256] = {0};
    size_t host_len = host->size > 255 ? 255 : host->size;
    memcpy(host_cstr, host->data, host_len);

    server = gethostbyname(host_cstr);
    if (server == NULL) {
        close(*sock_fd);
        RETURN_ERR("Failed to resolve hostname");
    }

    // Setup server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);

    // Connect to server
    if (connect(*sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(*sock_fd);
        RETURN_ERR("Failed to connect to server");
    }

    return RESULT_OK;
}

// Helper function to send HTTP request and receive response
static result_t send_http_request(pool_t* pool, int sock_fd, const string_t* request, string_t** response) {
    // Send the request
    ssize_t bytes_sent = send(sock_fd, request->data, request->size, 0);
    if (bytes_sent != (ssize_t)request->size) {
        RETURN_ERR("Failed to send complete HTTP request");
    }

    // Create response string
    if (string_create(pool, response) != RESULT_OK) {
        RETURN_ERR("Failed to create response string");
    }

    // Read response in chunks
    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = recv(sock_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        if (string_append_str(pool, response, buffer) != RESULT_OK) {
            if (string_destroy(pool, *response) != RESULT_OK) {
                RETURN_ERR("Failed to destroy response string");
            }
            RETURN_ERR("Failed to append response data");
        }
    }

    if (bytes_read < 0) {
        if (string_destroy(pool, *response) != RESULT_OK) {
            RETURN_ERR("Failed to destroy response string");
        }
        RETURN_ERR("Error reading HTTP response");
    }

    return RESULT_OK;
}

// Helper function to extract HTTP response body
static result_t extract_response_body(pool_t* pool, const string_t* raw_response, string_t** body) {
    const char* data = raw_response->data;
    const char* end = data + raw_response->size;

    // Find the end of headers (double CRLF or double LF)
    const char* body_start = strstr(data, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
    } else {
        body_start = strstr(data, "\n\n");
        if (body_start) {
            body_start += 2;
        } else {
            // No body separator found - return empty body
            if (string_create(pool, body) != RESULT_OK) {
                RETURN_ERR("Failed to create empty response body");
            }
            return RESULT_OK;
        }
    }

    // Check status code before extracting body
    if (strncmp(data, "HTTP/", 5) != 0) {
        RETURN_ERR("Invalid HTTP response format");
    }

    // Skip to status code
    const char* status_pos = data;
    while (status_pos < end && *status_pos != ' ') {
        status_pos++;
    }
    if (status_pos >= end) {
        RETURN_ERR("Invalid HTTP response - no status code");
    }
    status_pos++; // Skip space

    // Parse status code
    char status_str[4] = {0};
    for (int i = 0; i < 3 && status_pos < end && isdigit(*status_pos); i++, status_pos++) {
        status_str[i] = *status_pos;
    }
    int status_code = atoi(status_str);

    // Check for successful status codes (2xx)
    if (status_code < 200 || status_code >= 300) {
        RETURN_ERR("HTTP request failed with non-2xx status code");
    }

    // Extract body content
    size_t body_len = end - body_start;
    if (body_len > 0) {
        if (pool_string_alloc(pool, body, body_len + 1) != RESULT_OK) {
            RETURN_ERR("Failed to allocate response body string");
        }
        memcpy((*body)->data, body_start, body_len);
        (*body)->data[body_len] = '\0';
        (*body)->size = body_len;
    } else {
        // Empty body
        if (string_create(pool, body) != RESULT_OK) {
            RETURN_ERR("Failed to create empty response body");
        }
    }

    return RESULT_OK;
}

// Public API implementation

result_t http_get(pool_t* pool, const string_t* url, string_t** response) {
    string_t* host = NULL;
    string_t* path = NULL;
    string_t* request = NULL;
    string_t* raw_response = NULL;
    uint16_t port;
    int sock_fd = -1;

    // Parse URL components
    if (extract_url_components(url, &host, &port, &path, pool) != RESULT_OK) {
        RETURN_ERR("Failed to extract URL components");
    }

    // Create HTTP GET request
    if (string_create(pool, &request) != RESULT_OK) {
        if (string_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host string after request creation failure");
        }
        if (string_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path string after request creation failure");
        }
        RETURN_ERR("Failed to create HTTP request string");
    }

    // Build request: "GET /path HTTP/1.1\r\nHost: hostname\r\nConnection: close\r\n\r\n"
    if (string_append_str(pool, &request, "GET ") != RESULT_OK ||
        string_append_string(pool, &request, path) != RESULT_OK ||
        string_append_str(pool, &request, " HTTP/1.1\r\nHost: ") != RESULT_OK ||
        string_append_string(pool, &request, host) != RESULT_OK ||
        string_append_str(pool, &request, "\r\nConnection: close\r\n\r\n") != RESULT_OK) {
        
        if (string_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host string after request build failure");
        }
        if (string_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path string after request build failure");
        }
        if (string_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request string after request build failure");
        }
        RETURN_ERR("Failed to build HTTP GET request");
    }

    // Create connection
    if (create_connection(host, port, &sock_fd) != RESULT_OK) {
        if (string_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host string after connection failure");
        }
        if (string_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path string after connection failure");
        }
        if (string_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request string after connection failure");
        }
        RETURN_ERR("Failed to create connection");
    }

    // Send request and receive response
    if (send_http_request(pool, sock_fd, request, &raw_response) != RESULT_OK) {
        close(sock_fd);
        if (string_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host string after request send failure");
        }
        if (string_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path string after request send failure");
        }
        if (string_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request string after request send failure");
        }
        RETURN_ERR("Failed to send HTTP request and receive response");
    }

    // Extract response body
    if (extract_response_body(pool, raw_response, response) != RESULT_OK) {
        close(sock_fd);
        if (string_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host string after body extraction failure");
        }
        if (string_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path string after body extraction failure");
        }
        if (string_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request string after body extraction failure");
        }
        if (string_destroy(pool, raw_response) != RESULT_OK) {
            RETURN_ERR("Failed to destroy raw response string after body extraction failure");
        }
        RETURN_ERR("Failed to extract response body");
    }

    // Clean up all resources
    close(sock_fd);
    if (string_destroy(pool, host) != RESULT_OK) {
        RETURN_ERR("Failed to destroy host string");
    }
    if (string_destroy(pool, path) != RESULT_OK) {
        RETURN_ERR("Failed to destroy path string");
    }
    if (string_destroy(pool, request) != RESULT_OK) {
        RETURN_ERR("Failed to destroy request string");
    }
    if (string_destroy(pool, raw_response) != RESULT_OK) {
        RETURN_ERR("Failed to destroy raw response string");
    }

    return RESULT_OK;
}

result_t http_post(pool_t* pool, const string_t* url, const string_t* content_type, const string_t* body, string_t** response) {
    string_t* host = NULL;
    string_t* path = NULL;
    string_t* request = NULL;
    string_t* raw_response = NULL;
    uint16_t port;
    int sock_fd = -1;

    // Parse URL components
    if (extract_url_components(url, &host, &port, &path, pool) != RESULT_OK) {
        RETURN_ERR("Failed to extract URL components");
    }

    // Create HTTP POST request
    if (string_create(pool, &request) != RESULT_OK) {
        if (string_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host string after request creation failure");
        }
        if (string_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path string after request creation failure");
        }
        RETURN_ERR("Failed to create HTTP request string");
    }

    // Build request line
    if (string_append_str(pool, &request, "POST ") != RESULT_OK ||
        string_append_string(pool, &request, path) != RESULT_OK ||
        string_append_str(pool, &request, " HTTP/1.1\r\n") != RESULT_OK) {
        
        if (string_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host string after request line build failure");
        }
        if (string_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path string after request line build failure");
        }
        if (string_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request string after request line build failure");
        }
        RETURN_ERR("Failed to build HTTP POST request line");
    }

    // Add Host header
    if (string_append_str(pool, &request, "Host: ") != RESULT_OK ||
        string_append_string(pool, &request, host) != RESULT_OK ||
        string_append_str(pool, &request, "\r\n") != RESULT_OK) {
        
        if (string_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host string after host header failure");
        }
        if (string_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path string after host header failure");
        }
        if (string_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request string after host header failure");
        }
        RETURN_ERR("Failed to add Host header");
    }

    // Add Content-Type header
    if (string_append_str(pool, &request, "Content-Type: ") != RESULT_OK ||
        string_append_string(pool, &request, content_type) != RESULT_OK ||
        string_append_str(pool, &request, "\r\n") != RESULT_OK) {
        
        if (string_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host string after content type header failure");
        }
        if (string_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path string after content type header failure");
        }
        if (string_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request string after content type header failure");
        }
        RETURN_ERR("Failed to add Content-Type header");
    }

    // Add Content-Length header
    char content_length[64];
    snprintf(content_length, sizeof(content_length), "Content-Length: %lu\r\n", body->size);
    if (string_append_str(pool, &request, content_length) != RESULT_OK) {
        if (string_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host string after content length header failure");
        }
        if (string_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path string after content length header failure");
        }
        if (string_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request string after content length header failure");
        }
        RETURN_ERR("Failed to add Content-Length header");
    }

    // Add Connection header and end headers
    if (string_append_str(pool, &request, "Connection: close\r\n\r\n") != RESULT_OK) {
        if (string_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host string after connection header failure");
        }
        if (string_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path string after connection header failure");
        }
        if (string_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request string after connection header failure");
        }
        RETURN_ERR("Failed to add Connection header");
    }

    // Add body
    if (string_append_string(pool, &request, body) != RESULT_OK) {
        if (string_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host string after body append failure");
        }
        if (string_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path string after body append failure");
        }
        if (string_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request string after body append failure");
        }
        RETURN_ERR("Failed to add request body");
    }

    // Create connection
    if (create_connection(host, port, &sock_fd) != RESULT_OK) {
        if (string_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host string after connection failure");
        }
        if (string_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path string after connection failure");
        }
        if (string_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request string after connection failure");
        }
        RETURN_ERR("Failed to create connection");
    }

    // Send request and receive response
    if (send_http_request(pool, sock_fd, request, &raw_response) != RESULT_OK) {
        close(sock_fd);
        if (string_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host string after request send failure");
        }
        if (string_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path string after request send failure");
        }
        if (string_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request string after request send failure");
        }
        RETURN_ERR("Failed to send HTTP request and receive response");
    }

    // Extract response body
    if (extract_response_body(pool, raw_response, response) != RESULT_OK) {
        close(sock_fd);
        if (string_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host string after body extraction failure");
        }
        if (string_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path string after body extraction failure");
        }
        if (string_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request string after body extraction failure");
        }
        if (string_destroy(pool, raw_response) != RESULT_OK) {
            RETURN_ERR("Failed to destroy raw response string after body extraction failure");
        }
        RETURN_ERR("Failed to extract response body");
    }

    // Clean up all resources
    close(sock_fd);
    if (string_destroy(pool, host) != RESULT_OK) {
        RETURN_ERR("Failed to destroy host string");
    }
    if (string_destroy(pool, path) != RESULT_OK) {
        RETURN_ERR("Failed to destroy path string");
    }
    if (string_destroy(pool, request) != RESULT_OK) {
        RETURN_ERR("Failed to destroy request string");
    }
    if (string_destroy(pool, raw_response) != RESULT_OK) {
        RETURN_ERR("Failed to destroy raw response string");
    }

    return RESULT_OK;
}
