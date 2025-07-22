#define _GNU_SOURCE
#include "utils/http.h"

// Helper function to find case-insensitive substring
static const char* find_header(const char* haystack, const char* needle) {
    const char* pos = haystack;
    size_t needle_len = strlen(needle);

    while (*pos) {
        if (strncasecmp(pos, needle, needle_len) == 0) {
            return pos;
        }
        pos++;
    }
    return NULL;
}

static const char* http_method_strings[] = {
    "GET",
    "POST",
    "PUT",
    "DELETE",
    "HEAD",
    "OPTIONS",
    "PATCH"};

result_t http_parse_url(pool_t* pool, const char* url_str, http_url_t* url) {
    // Allocate strings
    if (pool_string4096_alloc(pool, &url->url) != RESULT_OK ||
        pool_string256_alloc(pool, &url->host) != RESULT_OK ||
        pool_string256_alloc(pool, &url->port) != RESULT_OK ||
        pool_string4096_alloc(pool, &url->path) != RESULT_OK ||
        pool_string4096_alloc(pool, &url->query) != RESULT_OK) {
        RETURN_ERR("Failed to allocate memory for URL components");
    }

    // Copy original URL
    if (string_assign(url->url, url_str) != RESULT_OK) {
        RETURN_ERR("URL string too long");
    }

    // Check for protocol
    const char* protocol_end = strstr(url_str, "://");
    if (!protocol_end) {
        RETURN_ERR("Invalid URL: missing protocol");
    }

    // Determine if HTTPS
    url->is_https = (strncmp(url_str, "https", 5) == 0);

    // Start parsing after protocol
    const char* start = protocol_end + 3;

    // Find host end (either ':', '/', or end of string)
    const char* host_end = start;
    while (*host_end && *host_end != ':' && *host_end != '/' && *host_end != '?') {
        host_end++;
    }

    // Extract host
    size_t host_len = host_end - start;
    if (host_len >= url->host->capacity) {
        RETURN_ERR("Host name too long");
    }
    strncpy(url->host->data, start, host_len);
    url->host->data[host_len] = '\0';
    url->host->size = host_len;

    // Default port
    if (url->is_https) {
        if (string_assign(url->port, HTTPS_DEFAULT_PORT_STR) != RESULT_OK) {
            RETURN_ERR("Failed to set default HTTPS port");
        }
    } else {
        if (string_assign(url->port, HTTP_DEFAULT_PORT_STR) != RESULT_OK) {
            RETURN_ERR("Failed to set default HTTP port");
        }
    }

    // Check for port
    if (*host_end == ':') {
        const char* port_start = host_end + 1;
        const char* port_end = port_start;
        while (*port_end && *port_end != '/' && *port_end != '?') {
            port_end++;
        }

        size_t port_len = port_end - port_start;
        if (port_len >= url->port->capacity) {
            RETURN_ERR("Port number too long");
        }
        strncpy(url->port->data, port_start, port_len);
        url->port->data[port_len] = '\0';
        url->port->size = port_len;

        host_end = port_end;
    }

    // Extract path
    if (*host_end == '/') {
        const char* path_start = host_end;
        const char* path_end = strchr(path_start, '?');
        if (!path_end) {
            path_end = path_start + strlen(path_start);
        }

        size_t path_len = path_end - path_start;
        if (path_len >= url->path->capacity) {
            RETURN_ERR("Path too long");
        }
        strncpy(url->path->data, path_start, path_len);
        url->path->data[path_len] = '\0';
        url->path->size = path_len;

        // Extract query if present
        if (*path_end == '?') {
            const char* query_start = path_end + 1;
            size_t query_len = strlen(query_start);
            if (query_len >= url->query->capacity) {
                RETURN_ERR("Query string too long");
            }
            strcpy(url->query->data, query_start);
            url->query->size = query_len;
        }
    } else {
        // Default path
        if (string_assign(url->path, "/") != RESULT_OK) {
            RETURN_ERR("Failed to set default path");
        }
    }

    return RESULT_OK;
}

result_t http_request_init(pool_t* pool, http_request_t* request, http_method_t method, const char* url_str) {
    request->method = method;
    request->header_count = 0;
    request->headers = NULL;
    request->body = NULL;
    request->timeout_seconds = 30;  // Default 30 second timeout

    return http_parse_url(pool, url_str, &request->url);
}

result_t http_request_add_header(pool_t* pool, http_request_t* request, const char* name, const char* value) {
    // For simplicity, we'll use a fixed array of headers
    // In a real implementation, you might want dynamic allocation
    static http_header_t header_storage[32];

    if (request->header_count >= 32) {
        RETURN_ERR("Too many headers");
    }

    if (!request->headers) {
        request->headers = header_storage;
    }

    http_header_t* header = &request->headers[request->header_count];

    if (pool_string256_alloc(pool, &header->name) != RESULT_OK ||
        pool_string4096_alloc(pool, &header->value) != RESULT_OK) {
        RETURN_ERR("Failed to allocate memory for header");
    }

    if (string_assign(header->name, name) != RESULT_OK) {
        RETURN_ERR("Header name too long");
    }

    if (string_assign(header->value, value) != RESULT_OK) {
        RETURN_ERR("Header value too long");
    }

    request->header_count++;
    return RESULT_OK;
}

result_t http_request_set_body(pool_t* pool, http_request_t* request, const char* body) {
    if (pool_string1048576_alloc(pool, &request->body) != RESULT_OK) {
        RETURN_ERR("Failed to allocate memory for request body");
    }

    return string_assign(request->body, body);
}

result_t http_response_init(pool_t* pool, http_response_t* response) {
    response->status_code = 0;
    response->header_count = 0;
    response->headers = NULL;

    if (pool_string256_alloc(pool, &response->status_message) != RESULT_OK ||
        pool_string1048576_alloc(pool, &response->body) != RESULT_OK) {
        RETURN_ERR("Failed to allocate memory for response");
    }

    string_clear(response->status_message);
    string_clear(response->body);

    return RESULT_OK;
}

static int create_socket_connection(const char* host, const char* port) {
    struct addrinfo hints, *result, *rp;
    int sockfd = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(host, port, &hints, &result);
    if (status != 0) {
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) {
            continue;
        }

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;  // Success
        }

        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(result);
    return sockfd;
}

static result_t send_all(int sockfd, const char* data, size_t len) {
    size_t total_sent = 0;

    while (total_sent < len) {
        ssize_t sent = send(sockfd, data + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                continue;  // Retry
            }
            return RESULT_ERR;
        }
        total_sent += sent;
    }

    return RESULT_OK;
}

static result_t receive_response(int sockfd, string_t* response_buffer) {
    char buffer[HTTP_BUFFER_SIZE];
    ssize_t bytes_received;

    string_clear(response_buffer);

    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        if (string_append_data(response_buffer, buffer, bytes_received) != RESULT_OK) {
            RETURN_ERR("Response buffer overflow");
        }

        // Check if we have complete headers
        if (strstr(response_buffer->data, "\r\n\r\n")) {
            // We have complete headers, check if we need to read more body
            const char* body_start = strstr(response_buffer->data, "\r\n\r\n") + 4;
            const char* content_length_header = find_header(response_buffer->data, "content-length:");

            if (content_length_header) {
                content_length_header += 15;  // Skip "content-length:"
                while (*content_length_header == ' ')
                    content_length_header++;  // Skip spaces

                int content_length = atoi(content_length_header);
                int body_received = response_buffer->size - (body_start - response_buffer->data);

                if (body_received >= content_length) {
                    break;  // We have all the content
                }
                // Continue reading if we need more content
            } else {
                // No content-length header, check for chunked encoding or connection close
                if (find_header(response_buffer->data, "transfer-encoding: chunked")) {
                    // Handle chunked encoding (simplified)
                    if (strstr(body_start, "\r\n0\r\n\r\n")) {
                        break;  // End of chunked response
                    }
                } else {
                    // For simplicity, assume connection close indicates end of response
                    // In a real implementation, you'd want better handling
                    break;
                }
            }
        }
    }

    if (bytes_received < 0) {
        RETURN_ERR("Failed to receive response");
    }

    return RESULT_OK;
}

static result_t parse_response(const string_t* response_buffer, http_response_t* response) {
    const char* data = response_buffer->data;

    // Parse status line
    const char* status_line_end = strstr(data, "\r\n");
    if (!status_line_end) {
        RETURN_ERR("Invalid HTTP response: no status line");
    }

    // Extract status code
    const char* status_code_start = strchr(data, ' ');
    if (!status_code_start) {
        RETURN_ERR("Invalid HTTP response: no status code");
    }
    status_code_start++;  // Skip space

    response->status_code = atoi(status_code_start);

    // Extract status message
    const char* status_msg_start = strchr(status_code_start, ' ');
    if (status_msg_start) {
        status_msg_start++;  // Skip space
        size_t msg_len = status_line_end - status_msg_start;
        if (msg_len < response->status_message->capacity) {
            strncpy(response->status_message->data, status_msg_start, msg_len);
            response->status_message->data[msg_len] = '\0';
            response->status_message->size = msg_len;
        }
    }

    // Find body start
    const char* body_start = strstr(data, "\r\n\r\n");
    if (body_start) {
        body_start += 4;  // Skip "\r\n\r\n"
        size_t body_len = response_buffer->size - (body_start - data);

        if (body_len < response->body->capacity) {
            memcpy(response->body->data, body_start, body_len);
            response->body->data[body_len] = '\0';
            response->body->size = body_len;
        } else {
            RETURN_ERR("Response body too large");
        }
    }

    return RESULT_OK;
}

result_t http_send_request(pool_t* pool, const http_request_t* request, http_response_t* response) {
    if (request->url.is_https) {
        RETURN_ERR("HTTPS not supported in this implementation");
    }

    // Create socket connection
    int sockfd = create_socket_connection(request->url.host->data, request->url.port->data);
    if (sockfd < 0) {
        RETURN_ERR("Failed to connect to server");
    }

    // Build HTTP request
    string_t* request_str;
    if (pool_string1048576_alloc(pool, &request_str) != RESULT_OK) {
        close(sockfd);
        RETURN_ERR("Failed to allocate memory for request string");
    }

    // Request line
    if (string_assign(request_str, http_method_strings[request->method]) != RESULT_OK ||
        string_append(request_str, " ") != RESULT_OK ||
        string_append(request_str, request->url.path->data) != RESULT_OK) {
        close(sockfd);
        RETURN_ERR("Failed to build request line");
    }

    if (request->url.query->size > 0) {
        if (string_append(request_str, "?") != RESULT_OK ||
            string_append(request_str, request->url.query->data) != RESULT_OK) {
            close(sockfd);
            RETURN_ERR("Failed to add query string");
        }
    }

    if (string_append(request_str, " HTTP/1.1\r\n") != RESULT_OK) {
        close(sockfd);
        RETURN_ERR("Failed to add HTTP version");
    }

    // Host header
    if (string_append(request_str, "Host: ") != RESULT_OK ||
        string_append(request_str, request->url.host->data) != RESULT_OK ||
        string_append(request_str, "\r\n") != RESULT_OK) {
        close(sockfd);
        RETURN_ERR("Failed to add Host header");
    }

    // User-Agent header
    if (string_append(request_str, "User-Agent: lkjagent/1.0\r\n") != RESULT_OK) {
        close(sockfd);
        RETURN_ERR("Failed to add User-Agent header");
    }

    // Connection header
    if (string_append(request_str, "Connection: close\r\n") != RESULT_OK) {
        close(sockfd);
        RETURN_ERR("Failed to add Connection header");
    }

    // Add custom headers
    for (uint64_t i = 0; i < request->header_count; i++) {
        if (string_append(request_str, request->headers[i].name->data) != RESULT_OK ||
            string_append(request_str, ": ") != RESULT_OK ||
            string_append(request_str, request->headers[i].value->data) != RESULT_OK ||
            string_append(request_str, "\r\n") != RESULT_OK) {
            close(sockfd);
            RETURN_ERR("Failed to add custom header");
        }
    }

    // Content-Length header if we have a body
    if (request->body && request->body->size > 0) {
        char content_length_str[64];  // Increased buffer size
        snprintf(content_length_str, sizeof(content_length_str), "Content-Length: %lu\r\n", request->body->size);
        if (string_append(request_str, content_length_str) != RESULT_OK) {
            close(sockfd);
            RETURN_ERR("Failed to add Content-Length header");
        }
    }

    // End headers
    if (string_append(request_str, "\r\n") != RESULT_OK) {
        close(sockfd);
        RETURN_ERR("Failed to add header terminator");
    }

    // Add body if present
    if (request->body && request->body->size > 0) {
        if (string_append_data(request_str, request->body->data, request->body->size) != RESULT_OK) {
            close(sockfd);
            RETURN_ERR("Failed to add request body");
        }
    }

    // Send request
    result_t send_result = send_all(sockfd, request_str->data, request_str->size);
    if (send_result != RESULT_OK) {
        close(sockfd);
        RETURN_ERR("Failed to send HTTP request");
    }

    // Receive response
    string_t* response_buffer;
    if (pool_string1048576_alloc(pool, &response_buffer) != RESULT_OK) {
        close(sockfd);
        RETURN_ERR("Failed to allocate memory for response buffer");
    }

    result_t receive_result = receive_response(sockfd, response_buffer);
    close(sockfd);

    if (receive_result != RESULT_OK) {
        RETURN_ERR("Failed to receive HTTP response");
    }

    // Parse response
    return parse_response(response_buffer, response);
}

result_t http_get(pool_t* pool, const char* url, http_response_t* response) {
    http_request_t request;

    if (http_request_init(pool, &request, HTTP_METHOD_GET, url) != RESULT_OK) {
        RETURN_ERR("Failed to initialize HTTP request");
    }

    if (http_response_init(pool, response) != RESULT_OK) {
        RETURN_ERR("Failed to initialize HTTP response");
    }

    return http_send_request(pool, &request, response);
}

result_t http_post_json(pool_t* pool, const char* url, const char* json_body, http_response_t* response) {
    http_request_t request;

    if (http_request_init(pool, &request, HTTP_METHOD_POST, url) != RESULT_OK) {
        RETURN_ERR("Failed to initialize HTTP request");
    }

    // Add Content-Type header for JSON
    if (http_request_add_header(pool, &request, "Content-Type", "application/json") != RESULT_OK) {
        RETURN_ERR("Failed to add Content-Type header");
    }

    // Set body
    if (http_request_set_body(pool, &request, json_body) != RESULT_OK) {
        RETURN_ERR("Failed to set request body");
    }

    if (http_response_init(pool, response) != RESULT_OK) {
        RETURN_ERR("Failed to initialize HTTP response");
    }

    return http_send_request(pool, &request, response);
}
