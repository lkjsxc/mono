#include "lkjlib.h"

// HTTP implementation

// Helper function to extract host, port, and path from URL
static result_t extract_url_components(const data_t* url_data, data_t** host, uint16_t* port, data_t** path, pool_t* pool) {
    const char* data = url_data->data;
    uint64_t size = url_data->size;

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
    uint64_t host_len = host_end - start;
    if (host_len == 0) {
        RETURN_ERR("Empty hostname in URL");
    }

    if (pool_data_alloc(pool, host, host_len + 1) != RESULT_OK) {
        RETURN_ERR("Failed to allocate host data");
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
                if (data_destroy(pool, *host) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy host data");
                }
                RETURN_ERR("Invalid port number in URL");
            }
            host_end++;
        }

        // Parse port
        char port_str[8] = {0};
        uint64_t port_len = host_end - port_start;
        if (port_len > 0 && port_len < 6) {
            memcpy(port_str, port_start, port_len);
            int parsed_port = atoi(port_str);
            if (parsed_port <= 0 || parsed_port > 65535) {
                if (data_destroy(pool, *host) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy host data");
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

    uint64_t path_len;
    if (path_start == NULL) {
        path_len = 1;  // Just "/"
    } else {
        path_len = (data + size) - path_start;
    }

    if (pool_data_alloc(pool, path, path_len + 1) != RESULT_OK) {
        if (data_destroy(pool, *host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host data");
        }
        RETURN_ERR("Failed to allocate path data");
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
static result_t create_connection(const data_t* host, uint16_t port, int* sock_fd) {
    struct sockaddr_in server_addr;
    struct hostent* server;

    // Create socket
    *sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock_fd < 0) {
        RETURN_ERR("Failed to create TCP socket");
    }

    // Resolve hostname
    char host_cstr[256] = {0};
    uint64_t host_len = host->size > 255 ? 255 : host->size;
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
static result_t send_http_request(pool_t* pool, int sock_fd, const data_t* request, data_t** response) {
    // Send the request
    ssize_t bytes_sent = send(sock_fd, request->data, request->size, 0);
    if (bytes_sent != (ssize_t)request->size) {
        RETURN_ERR("Failed to send complete HTTP request");
    }

    // Create response data
    if (data_create(pool, response) != RESULT_OK) {
        RETURN_ERR("Failed to create response data");
    }

    // Read response in chunks
    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = recv(sock_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        if (data_append_str(pool, response, buffer) != RESULT_OK) {
            if (data_destroy(pool, *response) != RESULT_OK) {
                RETURN_ERR("Failed to destroy response data");
            }
            RETURN_ERR("Failed to append response data");
        }
    }

    if (bytes_read < 0) {
        if (data_destroy(pool, *response) != RESULT_OK) {
            RETURN_ERR("Failed to destroy response data");
        }
        RETURN_ERR("Error reading HTTP response");
    }

    return RESULT_OK;
}

// Helper function to extract HTTP response body
static result_t extract_response_body(pool_t* pool, const data_t* raw_response, data_t** body) {
    // Find the end of headers (double CRLF or double LF)
    int64_t body_start_crlf = data_find_str(raw_response, "\r\n\r\n", 0);
    int64_t body_start_lf = data_find_str(raw_response, "\n\n", 0);
    int64_t body_start = -1;

    if (data_create(pool, body) != RESULT_OK) {
        RETURN_ERR("Failed to create response body");
    }

    if (body_start_crlf >= 0) {
        body_start = body_start_crlf + 4;
    } else if (body_start_lf >= 0) {
        body_start = body_start_lf + 2;
    } else {
        // No body separator found - return empty body
        if (data_clean(pool, body) != RESULT_OK) {
            RETURN_ERR("Failed to clean empty response body");
        }
        return RESULT_OK;
    }

    // Check status code before extracting body
    if (raw_response->size < 5 || strncmp(raw_response->data, "HTTP/", 5) != 0) {
        RETURN_ERR("Invalid HTTP response format");
    }

    // Find first space (after HTTP version) to locate status code
    int64_t first_space = data_find_char(raw_response, ' ', 0);
    if (first_space < 0 || first_space + 4 >= (int64_t)raw_response->size) {
        RETURN_ERR("Invalid HTTP response - no status code found");
    }

    // Extract and parse status code (3 digits after the space)
    char status_str[4] = {0};
    for (int i = 0; i < 3 && (first_space + 1 + i) < (int64_t)raw_response->size; i++) {
        char c = raw_response->data[first_space + 1 + i];
        if (c < '0' || c > '9') {
            RETURN_ERR("Invalid status code format");
        }
        status_str[i] = c;
    }
    int status_code = atoi(status_str);

    // Check for successful status codes (2xx)
    if (status_code < 200 || status_code >= 300) {
        RETURN_ERR("HTTP request failed with non-2xx status code");
    }

    // Extract body content using data utilities
    if (body_start < (int64_t)raw_response->size) {
        uint64_t body_len = raw_response->size - body_start;
        if (body_len > 0) {
            // Create a temporary data to hold the body data
            data_t* temp_body = NULL;
            if (data_create(pool, &temp_body) != RESULT_OK) {
                RETURN_ERR("Failed to create temporary body data");
            }

            // Allocate enough space for the body
            if (pool_data_realloc(pool, &temp_body, body_len + 1) != RESULT_OK) {
                if (data_destroy(pool, temp_body) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy temporary body data");
                }
                RETURN_ERR("Failed to allocate space for response body");
            }

            // Copy the body data
            memcpy(temp_body->data, raw_response->data + body_start, body_len);
            temp_body->data[body_len] = '\0';
            temp_body->size = body_len;

            // Copy the body to the output parameter using data utilities
            if (data_copy_data(pool, body, temp_body) != RESULT_OK) {
                if (data_destroy(pool, temp_body) != RESULT_OK) {
                    RETURN_ERR("Failed to destroy temporary body data");
                }
                RETURN_ERR("Failed to copy body to output data");
            }

            if (data_destroy(pool, temp_body) != RESULT_OK) {
                RETURN_ERR("Failed to destroy temporary body data");
            }
        } else {
            // Empty body
            if (data_clean(pool, body) != RESULT_OK) {
                RETURN_ERR("Failed to clean empty response body");
            }
        }
    } else {
        // Body start is beyond the response size - empty body
        if (data_clean(pool, body) != RESULT_OK) {
            RETURN_ERR("Failed to clean empty response body");
        }
    }

    return RESULT_OK;
}

// Public API implementation

result_t http_get(pool_t* pool, const data_t* url, data_t** response) {
    data_t* host = NULL;
    data_t* path = NULL;
    data_t* request = NULL;
    data_t* raw_response = NULL;
    uint16_t port;
    int sock_fd = -1;

    // Parse URL components
    if (extract_url_components(url, &host, &port, &path, pool) != RESULT_OK) {
        RETURN_ERR("Failed to extract URL components");
    }

    // Create HTTP GET request
    if (data_create(pool, &request) != RESULT_OK) {
        if (data_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host data after request creation failure");
        }
        if (data_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path data after request creation failure");
        }
        RETURN_ERR("Failed to create HTTP request data");
    }

    // Build request: "GET /path HTTP/1.1\r\nHost: hostname\r\nConnection: close\r\n\r\n"
    if (data_append_str(pool, &request, "GET ") != RESULT_OK ||
        data_append_data(pool, &request, path) != RESULT_OK ||
        data_append_str(pool, &request, " HTTP/1.1\r\nHost: ") != RESULT_OK ||
        data_append_data(pool, &request, host) != RESULT_OK ||
        data_append_str(pool, &request, "\r\nConnection: close\r\n\r\n") != RESULT_OK) {
        if (data_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host data after request build failure");
        }
        if (data_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path data after request build failure");
        }
        if (data_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request data after request build failure");
        }
        RETURN_ERR("Failed to build HTTP GET request");
    }

    // Create connection
    if (create_connection(host, port, &sock_fd) != RESULT_OK) {
        if (data_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host data after connection failure");
        }
        if (data_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path data after connection failure");
        }
        if (data_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request data after connection failure");
        }
        RETURN_ERR("Failed to create connection");
    }

    // Send request and receive response
    if (send_http_request(pool, sock_fd, request, &raw_response) != RESULT_OK) {
        close(sock_fd);
        if (data_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host data after request send failure");
        }
        if (data_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path data after request send failure");
        }
        if (data_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request data after request send failure");
        }
        RETURN_ERR("Failed to send HTTP request and receive response");
    }

    // Extract response body
    if (extract_response_body(pool, raw_response, response) != RESULT_OK) {
        close(sock_fd);
        if (data_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host data after body extraction failure");
        }
        if (data_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path data after body extraction failure");
        }
        if (data_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request data after body extraction failure");
        }
        if (data_destroy(pool, raw_response) != RESULT_OK) {
            RETURN_ERR("Failed to destroy raw response data after body extraction failure");
        }
        RETURN_ERR("Failed to extract response body");
    }

    // Clean up all resources
    close(sock_fd);
    if (data_destroy(pool, host) != RESULT_OK) {
        RETURN_ERR("Failed to destroy host data");
    }
    if (data_destroy(pool, path) != RESULT_OK) {
        RETURN_ERR("Failed to destroy path data");
    }
    if (data_destroy(pool, request) != RESULT_OK) {
        RETURN_ERR("Failed to destroy request data");
    }
    if (data_destroy(pool, raw_response) != RESULT_OK) {
        RETURN_ERR("Failed to destroy raw response data");
    }

    return RESULT_OK;
}

result_t http_post(pool_t* pool, const data_t* url, const data_t* content_type, const data_t* body, data_t** response) {
    data_t* host = NULL;
    data_t* path = NULL;
    data_t* request = NULL;
    data_t* raw_response = NULL;
    uint16_t port;
    int sock_fd = -1;

    // Parse URL components
    if (extract_url_components(url, &host, &port, &path, pool) != RESULT_OK) {
        RETURN_ERR("Failed to extract URL components");
    }

    // Create HTTP POST request
    if (data_create(pool, &request) != RESULT_OK) {
        if (data_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host data after request creation failure");
        }
        if (data_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path data after request creation failure");
        }
        RETURN_ERR("Failed to create HTTP request data");
    }

    // Build request line
    if (data_append_str(pool, &request, "POST ") != RESULT_OK ||
        data_append_data(pool, &request, path) != RESULT_OK ||
        data_append_str(pool, &request, " HTTP/1.1\r\n") != RESULT_OK) {
        if (data_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host data after request line build failure");
        }
        if (data_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path data after request line build failure");
        }
        if (data_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request data after request line build failure");
        }
        RETURN_ERR("Failed to build HTTP POST request line");
    }

    // Add Host header
    if (data_append_str(pool, &request, "Host: ") != RESULT_OK ||
        data_append_data(pool, &request, host) != RESULT_OK ||
        data_append_str(pool, &request, "\r\n") != RESULT_OK) {
        if (data_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host data after host header failure");
        }
        if (data_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path data after host header failure");
        }
        if (data_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request data after host header failure");
        }
        RETURN_ERR("Failed to add Host header");
    }

    // Add Content-Type header
    if (data_append_str(pool, &request, "Content-Type: ") != RESULT_OK ||
        data_append_data(pool, &request, content_type) != RESULT_OK ||
        data_append_str(pool, &request, "\r\n") != RESULT_OK) {
        if (data_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host data after content type header failure");
        }
        if (data_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path data after content type header failure");
        }
        if (data_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request data after content type header failure");
        }
        RETURN_ERR("Failed to add Content-Type header");
    }

    // Add Content-Length header
    char content_length[64];
    snprintf(content_length, sizeof(content_length), "Content-Length: %lu\r\n", body->size);
    if (data_append_str(pool, &request, content_length) != RESULT_OK) {
        if (data_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host data after content length header failure");
        }
        if (data_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path data after content length header failure");
        }
        if (data_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request data after content length header failure");
        }
        RETURN_ERR("Failed to add Content-Length header");
    }

    // Add Connection header and end headers
    if (data_append_str(pool, &request, "Connection: close\r\n\r\n") != RESULT_OK) {
        if (data_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host data after connection header failure");
        }
        if (data_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path data after connection header failure");
        }
        if (data_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request data after connection header failure");
        }
        RETURN_ERR("Failed to add Connection header");
    }

    // Add body
    if (data_append_data(pool, &request, body) != RESULT_OK) {
        if (data_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host data after body append failure");
        }
        if (data_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path data after body append failure");
        }
        if (data_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request data after body append failure");
        }
        RETURN_ERR("Failed to add request body");
    }

    // Create connection
    if (create_connection(host, port, &sock_fd) != RESULT_OK) {
        if (data_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host data after connection failure");
        }
        if (data_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path data after connection failure");
        }
        if (data_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request data after connection failure");
        }
        RETURN_ERR("Failed to create connection");
    }

    // Send request and receive response
    if (send_http_request(pool, sock_fd, request, &raw_response) != RESULT_OK) {
        close(sock_fd);
        if (data_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host data after request send failure");
        }
        if (data_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path data after request send failure");
        }
        if (data_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request data after request send failure");
        }
        RETURN_ERR("Failed to send HTTP request and receive response");
    }

    // Extract response body
    if (extract_response_body(pool, raw_response, response) != RESULT_OK) {
        close(sock_fd);
        if (data_destroy(pool, host) != RESULT_OK) {
            RETURN_ERR("Failed to destroy host data after body extraction failure");
        }
        if (data_destroy(pool, path) != RESULT_OK) {
            RETURN_ERR("Failed to destroy path data after body extraction failure");
        }
        if (data_destroy(pool, request) != RESULT_OK) {
            RETURN_ERR("Failed to destroy request data after body extraction failure");
        }
        if (data_destroy(pool, raw_response) != RESULT_OK) {
            RETURN_ERR("Failed to destroy raw response data after body extraction failure");
        }
        RETURN_ERR("Failed to extract response body");
    }

    // Clean up all resources
    close(sock_fd);
    if (data_destroy(pool, host) != RESULT_OK) {
        RETURN_ERR("Failed to destroy host data");
    }
    if (data_destroy(pool, path) != RESULT_OK) {
        RETURN_ERR("Failed to destroy path data");
    }
    if (data_destroy(pool, request) != RESULT_OK) {
        RETURN_ERR("Failed to destroy request data");
    }
    if (data_destroy(pool, raw_response) != RESULT_OK) {
        RETURN_ERR("Failed to destroy raw response data");
    }

    return RESULT_OK;
}
