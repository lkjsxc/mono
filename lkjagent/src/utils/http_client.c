/**
 * @file http_client.c
 * @brief HTTP client implementation for LKJAgent LLM communication
 * 
 * This module implements a robust HTTP client designed for reliable
 * communication with LMStudio and other LLM services. It provides
 * comprehensive error handling, retry mechanisms, and connection management.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../include/http_client.h"
#include "../lkjagent.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
 * @defgroup HTTP_Client_Internal Internal HTTP Client Functions
 * @{
 */

/**
 * @brief Parse URL into components
 * 
 * @param url URL to parse
 * @param host Buffer to store hostname
 * @param host_size Size of host buffer
 * @param port Pointer to store port number
 * @param path Buffer to store path
 * @param path_size Size of path buffer
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t parse_url(const char* url, char* host, size_t host_size, 
                         uint16_t* port, char* path, size_t path_size) {
    if (!url || !host || !port || !path) {
        RETURN_ERR("Invalid parameters for URL parsing");
        return RESULT_ERR;
    }
    
    /* Default values */
    *port = 80;
    strncpy(path, "/", path_size - 1);
    path[path_size - 1] = '\0';
    
    /* Parse protocol */
    const char* protocol_end = strstr(url, "://");
    if (!protocol_end) {
        RETURN_ERR("Invalid URL format - missing protocol");
        return RESULT_ERR;
    }
    
    /* Check for HTTPS */
    if (strncmp(url, "https://", 8) == 0) {
        *port = 443;
        /* Note: HTTPS support would require SSL/TLS implementation */
        RETURN_ERR("HTTPS not yet supported in this implementation");
        return RESULT_ERR;
    } else if (strncmp(url, "http://", 7) != 0) {
        RETURN_ERR("Unsupported protocol - only HTTP supported");
        return RESULT_ERR;
    }
    
    /* Parse host and port */
    const char* host_start = protocol_end + 3;
    const char* port_start = strchr(host_start, ':');
    const char* path_start = strchr(host_start, '/');
    
    /* Determine host end */
    const char* host_end = path_start;
    if (port_start && (!path_start || port_start < path_start)) {
        host_end = port_start;
    }
    if (!host_end) {
        host_end = host_start + strlen(host_start);
    }
    
    /* Extract host */
    size_t host_len = host_end - host_start;
    if (host_len >= host_size) {
        RETURN_ERR("Host name too long for buffer");
        return RESULT_ERR;
    }
    strncpy(host, host_start, host_len);
    host[host_len] = '\0';
    
    /* Extract port if present */
    if (port_start && (!path_start || port_start < path_start)) {
        *port = (uint16_t)strtoul(port_start + 1, NULL, 10);
        if (*port == 0) {
            *port = 80; /* Default fallback */
        }
    }
    
    /* Extract path if present */
    if (path_start) {
        strncpy(path, path_start, path_size - 1);
        path[path_size - 1] = '\0';
    }
    
    return RESULT_OK;
}

/**
 * @brief Create socket connection to host
 * 
 * @param host Hostname to connect to
 * @param port Port number
 * @param timeout_seconds Connection timeout
 * @return Socket file descriptor on success, -1 on failure
 */
static int create_connection(const char* host, uint16_t port, uint32_t timeout_seconds) {
    if (!host) {
        return -1;
    }
    
    /* Resolve hostname */
    struct hostent* he = gethostbyname(host);
    if (!he) {
        return -1;
    }
    
    /* Create socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }
    
    /* Set non-blocking for timeout control */
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    }
    
    /* Setup address */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    
    /* Attempt connection */
    int result = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    if (result < 0 && errno != EINPROGRESS) {
        close(sockfd);
        return -1;
    }
    
    /* Wait for connection with timeout */
    if (errno == EINPROGRESS) {
        struct pollfd pfd;
        pfd.fd = sockfd;
        pfd.events = POLLOUT;
        
        int poll_result = poll(&pfd, 1, timeout_seconds * 1000);
        if (poll_result <= 0) {
            close(sockfd);
            return -1;
        }
        
        /* Check if connection succeeded */
        int error = 0;
        socklen_t error_len = sizeof(error);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &error_len) < 0 || error != 0) {
            close(sockfd);
            return -1;
        }
    }
    
    /* Set back to blocking mode */
    if (flags >= 0) {
        fcntl(sockfd, F_SETFL, flags);
    }
    
    return sockfd;
}

/**
 * @brief Send HTTP request through socket
 * 
 * @param sockfd Socket file descriptor
 * @param method HTTP method
 * @param path Request path
 * @param host Host header value
 * @param custom_headers Additional headers
 * @param body Request body (can be NULL)
 * @param timeout_seconds Send timeout
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t send_http_request(int sockfd, http_method_t method, const char* path,
                                 const char* host, const char* custom_headers, 
                                 const char* body, uint32_t timeout_seconds) {
    if (sockfd < 0 || !path || !host) {
        RETURN_ERR("Invalid parameters for HTTP request");
        return RESULT_ERR;
    }
    
    /* Build request */
    data_t request;
    if (data_init(&request, 2048) != RESULT_OK) {
        RETURN_ERR("Failed to initialize request buffer");
        return RESULT_ERR;
    }
    
    /* Request line */
    const char* method_str = (method == HTTP_METHOD_POST) ? "POST" : "GET";
    char request_line[512];
    snprintf(request_line, sizeof(request_line), "%s %s HTTP/1.1\r\n", method_str, path);
    
    if (data_set(&request, request_line, 0) != RESULT_OK) {
        { result_t _r = data_clear(&request); (void)_r; }
        RETURN_ERR("Failed to set request line");
        return RESULT_ERR;
    }
    
    /* Host header */
    char host_header[512];
    snprintf(host_header, sizeof(host_header), "Host: %s\r\n", host);
    if (data_append(&request, host_header, 0) != RESULT_OK) {
        { result_t _r = data_clear(&request); (void)_r; }
        RETURN_ERR("Failed to append host header");
        return RESULT_ERR;
    }
    
    /* User agent */
    if (data_append(&request, "User-Agent: LKJAgent/1.0\r\n", 0) != RESULT_OK) {
        { result_t _r = data_clear(&request); (void)_r; }
        RETURN_ERR("Failed to append user agent");
        return RESULT_ERR;
    }
    
    /* Connection header */
    if (data_append(&request, "Connection: close\r\n", 0) != RESULT_OK) {
        { result_t _r = data_clear(&request); (void)_r; }
        RETURN_ERR("Failed to append connection header");
        return RESULT_ERR;
    }
    
    /* Content type for POST */
    if (method == HTTP_METHOD_POST) {
        if (data_append(&request, "Content-Type: application/json\r\n", 0) != RESULT_OK) {
            { result_t _r = data_clear(&request); (void)_r; }
            RETURN_ERR("Failed to append content type");
            return RESULT_ERR;
        }
        
        /* Content length */
        if (body) {
            char content_length[64];
            snprintf(content_length, sizeof(content_length), "Content-Length: %zu\r\n", strlen(body));
            if (data_append(&request, content_length, 0) != RESULT_OK) {
                { result_t _r = data_clear(&request); (void)_r; }
                RETURN_ERR("Failed to append content length");
                return RESULT_ERR;
            }
        } else {
            if (data_append(&request, "Content-Length: 0\r\n", 0) != RESULT_OK) {
                { result_t _r = data_clear(&request); (void)_r; }
                RETURN_ERR("Failed to append zero content length");
                return RESULT_ERR;
            }
        }
    }
    
    /* Custom headers */
    if (custom_headers && custom_headers[0] != '\0') {
        if (data_append(&request, custom_headers, 0) != RESULT_OK) {
            { result_t _r = data_clear(&request); (void)_r; }
            RETURN_ERR("Failed to append custom headers");
            return RESULT_ERR;
        }
    }
    
    /* End headers */
    if (data_append(&request, "\r\n", 0) != RESULT_OK) {
        { result_t _r = data_clear(&request); (void)_r; }
        RETURN_ERR("Failed to append header separator");
        return RESULT_ERR;
    }
    
    /* Body for POST */
    if (method == HTTP_METHOD_POST && body) {
        if (data_append(&request, body, 0) != RESULT_OK) {
            { result_t _r = data_clear(&request); (void)_r; }
            RETURN_ERR("Failed to append request body");
            return RESULT_ERR;
        }
    }
    
    /* Send request with timeout */
    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLOUT;
    
    size_t sent = 0;
    while (sent < request.size) {
        int poll_result = poll(&pfd, 1, timeout_seconds * 1000);
        if (poll_result <= 0) {
            { result_t _r = data_clear(&request); (void)_r; }
            RETURN_ERR("Send timeout or poll error");
            return RESULT_ERR;
        }
        
        ssize_t bytes_sent = send(sockfd, request.data + sent, request.size - sent, 0);
        if (bytes_sent < 0) {
            { result_t _r = data_clear(&request); (void)_r; }
            RETURN_ERR("Failed to send request data");
            return RESULT_ERR;
        }
        
        sent += bytes_sent;
    }
    
    { result_t _r = data_clear(&request); (void)_r; }
    return RESULT_OK;
}

/**
 * @brief Receive HTTP response from socket
 * 
 * @param sockfd Socket file descriptor
 * @param response Response structure to populate
 * @param timeout_seconds Receive timeout
 * @param max_size Maximum response size
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
static result_t receive_http_response(int sockfd, http_response_t* response, 
                                     uint32_t timeout_seconds, size_t max_size) {
    if (sockfd < 0 || !response) {
        RETURN_ERR("Invalid parameters for HTTP response");
        return RESULT_ERR;
    }
    
    /* Initialize response timing */
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    /* Read response */
    data_t buffer;
    if (data_init(&buffer, 1024) != RESULT_OK) {
        RETURN_ERR("Failed to initialize response buffer");
        return RESULT_ERR;
    }
    
    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;
    
    bool headers_complete = false;
    size_t content_length = 0;
    size_t headers_end = 0;
    
    while (true) {
        int poll_result = poll(&pfd, 1, timeout_seconds * 1000);
        if (poll_result <= 0) {
            { result_t _r = data_clear(&buffer); (void)_r; }
            RETURN_ERR("Receive timeout or poll error");
            return RESULT_ERR;
        }
        
        char temp_buffer[1024];
        ssize_t bytes_received = recv(sockfd, temp_buffer, sizeof(temp_buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            break; /* Connection closed or error */
        }
        
        temp_buffer[bytes_received] = '\0';
        
        if (data_append(&buffer, temp_buffer, 0) != RESULT_OK) {
            { result_t _r = data_clear(&buffer); (void)_r; }
            RETURN_ERR("Failed to append received data");
            return RESULT_ERR;
        }
        
        /* Check if headers are complete */
        if (!headers_complete) {
            const char* headers_end_marker = strstr(buffer.data, "\r\n\r\n");
            if (headers_end_marker) {
                headers_complete = true;
                headers_end = headers_end_marker - buffer.data + 4;
                
                /* Parse content length */
                const char* content_length_header = strcasestr(buffer.data, "content-length:");
                if (content_length_header) {
                    content_length = strtoul(content_length_header + 15, NULL, 10);
                }
            }
        }
        
        /* Check if we have complete response */
        if (headers_complete) {
            size_t body_received = buffer.size - headers_end;
            if (content_length == 0 || body_received >= content_length) {
                break;
            }
        }
        
        /* Check size limit */
        if (buffer.size >= max_size) {
            { result_t _r = data_clear(&buffer); (void)_r; }
            RETURN_ERR("Response size exceeds maximum limit");
            return RESULT_ERR;
        }
    }
    
    /* Calculate response time */
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    response->response_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                             (end_time.tv_nsec - start_time.tv_nsec) / 1000000;
    
    /* Parse status code */
    if (buffer.size > 12) {
        response->status_code = (http_status_t)strtoul(buffer.data + 9, NULL, 10);
    } else {
        response->status_code = HTTP_STATUS_BAD_REQUEST;
    }
    
    /* Extract headers */
    if (headers_complete && headers_end > 0) {
        char* headers_text = strndup(buffer.data, headers_end - 4);
        if (headers_text) {
            if (data_set(&response->headers, headers_text, 0) != RESULT_OK) {
                free(headers_text);
                { result_t _r = data_clear(&buffer); (void)_r; }
                RETURN_ERR("Failed to set response headers");
                return RESULT_ERR;
            }
            free(headers_text);
        }
        
        /* Extract body */
        if (buffer.size > headers_end) {
            if (data_set(&response->body, buffer.data + headers_end, 0) != RESULT_OK) {
                { result_t _r = data_clear(&buffer); (void)_r; }
                RETURN_ERR("Failed to set response body");
                return RESULT_ERR;
            }
        }
    } else {
        /* No proper headers, treat entire response as body */
        if (data_set(&response->body, buffer.data, 0) != RESULT_OK) {
            { result_t _r = data_clear(&buffer); (void)_r; }
            RETURN_ERR("Failed to set response body fallback");
            return RESULT_ERR;
        }
    }
    
    { result_t _r = data_clear(&buffer); (void)_r; }
    return RESULT_OK;
}

/** @} */

result_t http_client_init(http_client_t* client, const http_client_config_t* config) {
    if (!client) {
        RETURN_ERR("HTTP client pointer is NULL");
        return RESULT_ERR;
    }
    
    /* Initialize structure */
    memset(client, 0, sizeof(http_client_t));
    
    /* Apply configuration or defaults */
    if (config) {
        client->config = *config;
    } else {
        /* Default configuration */
        client->config.connect_timeout = 10;
        client->config.request_timeout = 30;
        client->config.max_retries = 3;
        client->config.retry_delay = 1000;
        client->config.max_response_size = 1024 * 1024; /* 1MB */
        strcpy(client->config.user_agent, "LKJAgent/1.0");
        client->config.enable_keepalive = false;
    }
    
    /* Initialize internal state */
    client->connection_fd = -1;
    client->is_connected = false;
    client->current_port = 0;
    
    /* Initialize custom headers buffer */
    if (data_init(&client->custom_headers, 512) != RESULT_OK) {
        RETURN_ERR("Failed to initialize custom headers buffer");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t http_client_post(http_client_t* client, const char* url, const char* json_payload, http_response_t* response) {
    if (!client || !url || !json_payload || !response) {
        RETURN_ERR("Invalid parameters for HTTP POST");
        return RESULT_ERR;
    }
    
    /* Parse URL */
    char host[256];
    uint16_t port;
    char path[512];
    
    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) != RESULT_OK) {
        RETURN_ERR("Failed to parse URL for POST request");
        return RESULT_ERR;
    }
    
    /* Retry loop */
    for (uint32_t attempt = 0; attempt <= client->config.max_retries; attempt++) {
        /* Create connection */
        int sockfd = create_connection(host, port, client->config.connect_timeout);
        if (sockfd < 0) {
            if (attempt < client->config.max_retries) {
                usleep(client->config.retry_delay * 1000);
                continue;
            }
            RETURN_ERR("Failed to establish connection for POST request");
            return RESULT_ERR;
        }
        
        /* Send request */
        result_t send_result = send_http_request(sockfd, HTTP_METHOD_POST, path, host,
                                               client->custom_headers.data, json_payload,
                                               client->config.request_timeout);
        
        if (send_result != RESULT_OK) {
            close(sockfd);
            if (attempt < client->config.max_retries) {
                usleep(client->config.retry_delay * 1000);
                continue;
            }
            RETURN_ERR("Failed to send POST request");
            return RESULT_ERR;
        }
        
        /* Receive response */
        result_t receive_result = receive_http_response(sockfd, response,
                                                      client->config.request_timeout,
                                                      client->config.max_response_size);
        
        close(sockfd);
        
        if (receive_result != RESULT_OK) {
            if (attempt < client->config.max_retries) {
                usleep(client->config.retry_delay * 1000);
                continue;
            }
            RETURN_ERR("Failed to receive POST response");
            return RESULT_ERR;
        }
        
        /* Check if we should retry based on status code */
        bool should_retry = false;
        char error_message[256];
        
        if (http_client_handle_errors(response->status_code, &should_retry, 
                                     error_message, sizeof(error_message)) != RESULT_OK) {
            if (should_retry && attempt < client->config.max_retries) {
                usleep(client->config.retry_delay * 1000);
                continue;
            }
            RETURN_ERR(error_message);
            return RESULT_ERR;
        }
        
        /* Success */
        return RESULT_OK;
    }
    
    RETURN_ERR("Maximum retry attempts exceeded for POST request");
    return RESULT_ERR;
}

result_t http_client_get(http_client_t* client, const char* url, http_response_t* response) {
    if (!client || !url || !response) {
        RETURN_ERR("Invalid parameters for HTTP GET");
        return RESULT_ERR;
    }
    
    /* Parse URL */
    char host[256];
    uint16_t port;
    char path[512];
    
    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) != RESULT_OK) {
        RETURN_ERR("Failed to parse URL for GET request");
        return RESULT_ERR;
    }
    
    /* Retry loop */
    for (uint32_t attempt = 0; attempt <= client->config.max_retries; attempt++) {
        /* Create connection */
        int sockfd = create_connection(host, port, client->config.connect_timeout);
        if (sockfd < 0) {
            if (attempt < client->config.max_retries) {
                usleep(client->config.retry_delay * 1000);
                continue;
            }
            RETURN_ERR("Failed to establish connection for GET request");
            return RESULT_ERR;
        }
        
        /* Send request */
        result_t send_result = send_http_request(sockfd, HTTP_METHOD_GET, path, host,
                                               client->custom_headers.data, NULL,
                                               client->config.request_timeout);
        
        if (send_result != RESULT_OK) {
            close(sockfd);
            if (attempt < client->config.max_retries) {
                usleep(client->config.retry_delay * 1000);
                continue;
            }
            RETURN_ERR("Failed to send GET request");
            return RESULT_ERR;
        }
        
        /* Receive response */
        result_t receive_result = receive_http_response(sockfd, response,
                                                      client->config.request_timeout,
                                                      client->config.max_response_size);
        
        close(sockfd);
        
        if (receive_result != RESULT_OK) {
            if (attempt < client->config.max_retries) {
                usleep(client->config.retry_delay * 1000);
                continue;
            }
            RETURN_ERR("Failed to receive GET response");
            return RESULT_ERR;
        }
        
        /* Check if we should retry based on status code */
        bool should_retry = false;
        char error_message[256];
        
        if (http_client_handle_errors(response->status_code, &should_retry, 
                                     error_message, sizeof(error_message)) != RESULT_OK) {
            if (should_retry && attempt < client->config.max_retries) {
                usleep(client->config.retry_delay * 1000);
                continue;
            }
            RETURN_ERR(error_message);
            return RESULT_ERR;
        }
        
        /* Success */
        return RESULT_OK;
    }
    
    RETURN_ERR("Maximum retry attempts exceeded for GET request");
    return RESULT_ERR;
}

result_t http_client_set_headers(http_client_t* client, const char* headers) {
    if (!client || !headers) {
        RETURN_ERR("Invalid parameters for setting headers");
        return RESULT_ERR;
    }
    
    if (data_append(&client->custom_headers, headers, 0) != RESULT_OK) {
        RETURN_ERR("Failed to append custom headers");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t http_client_set_timeout(http_client_t* client, uint32_t connect_timeout, uint32_t request_timeout) {
    if (!client) {
        RETURN_ERR("HTTP client pointer is NULL");
        return RESULT_ERR;
    }
    
    if (connect_timeout == 0 || request_timeout == 0) {
        RETURN_ERR("Timeout values must be greater than 0");
        return RESULT_ERR;
    }
    
    client->config.connect_timeout = connect_timeout;
    client->config.request_timeout = request_timeout;
    
    return RESULT_OK;
}

result_t http_client_handle_errors(http_status_t status_code, bool* should_retry, char* error_message, size_t message_size) {
    if (!should_retry || !error_message) {
        RETURN_ERR("Invalid parameters for error handling");
        return RESULT_ERR;
    }
    
    *should_retry = false;
    
    switch (status_code) {
        case HTTP_STATUS_OK:
            strncpy(error_message, "Success", message_size - 1);
            error_message[message_size - 1] = '\0';
            return RESULT_OK;
            
        case HTTP_STATUS_BAD_REQUEST:
            strncpy(error_message, "Bad Request (400) - Client error", message_size - 1);
            break;
            
        case HTTP_STATUS_UNAUTHORIZED:
            strncpy(error_message, "Unauthorized (401) - Authentication required", message_size - 1);
            break;
            
        case HTTP_STATUS_FORBIDDEN:
            strncpy(error_message, "Forbidden (403) - Access denied", message_size - 1);
            break;
            
        case HTTP_STATUS_NOT_FOUND:
            strncpy(error_message, "Not Found (404) - Resource not found", message_size - 1);
            break;
            
        case HTTP_STATUS_INTERNAL_SERVER_ERROR:
            strncpy(error_message, "Internal Server Error (500) - Server error", message_size - 1);
            *should_retry = true;
            break;
            
        case HTTP_STATUS_BAD_GATEWAY:
            strncpy(error_message, "Bad Gateway (502) - Proxy error", message_size - 1);
            *should_retry = true;
            break;
            
        case HTTP_STATUS_SERVICE_UNAVAILABLE:
            strncpy(error_message, "Service Unavailable (503) - Service temporarily unavailable", message_size - 1);
            *should_retry = true;
            break;
            
        case HTTP_STATUS_GATEWAY_TIMEOUT:
            strncpy(error_message, "Gateway Timeout (504) - Proxy timeout", message_size - 1);
            *should_retry = true;
            break;
            
        default:
            snprintf(error_message, message_size, "HTTP Error (%d) - Unknown status code", (int)status_code);
            if (status_code >= 500) {
                *should_retry = true;
            }
            break;
    }
    
    error_message[message_size - 1] = '\0';
    return RESULT_ERR;
}

result_t http_client_test_connectivity(http_client_t* client, const char* host, uint16_t port, uint64_t* response_time) {
    if (!client || !host || !response_time) {
        RETURN_ERR("Invalid parameters for connectivity test");
        return RESULT_ERR;
    }
    
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    int sockfd = create_connection(host, port, client->config.connect_timeout);
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    *response_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                     (end_time.tv_nsec - start_time.tv_nsec) / 1000000;
    
    if (sockfd < 0) {
        RETURN_ERR("Connectivity test failed - unable to connect");
        return RESULT_ERR;
    }
    
    close(sockfd);
    return RESULT_OK;
}

result_t http_client_cleanup(http_client_t* client) {
    if (!client) {
        RETURN_ERR("HTTP client pointer is NULL");
        return RESULT_ERR;
    }
    
    /* Close connection if open */
    if (client->connection_fd >= 0) {
        close(client->connection_fd);
        client->connection_fd = -1;
    }
    
    /* Clean up custom headers */
    { result_t _r = data_clear(&client->custom_headers); (void)_r; }
    
    /* Reset state */
    memset(client, 0, sizeof(http_client_t));
    client->connection_fd = -1;
    
    return RESULT_OK;
}

result_t http_response_init(http_response_t* response, size_t body_capacity) {
    if (!response) {
        RETURN_ERR("HTTP response pointer is NULL");
        return RESULT_ERR;
    }
    
    /* Initialize structure */
    memset(response, 0, sizeof(http_response_t));
    
    /* Initialize buffers */
    if (data_init(&response->headers, 512) != RESULT_OK) {
        RETURN_ERR("Failed to initialize response headers buffer");
        return RESULT_ERR;
    }
    
    if (data_init(&response->body, body_capacity) != RESULT_OK) {
        { result_t _r = data_clear(&response->headers); (void)_r; }
        RETURN_ERR("Failed to initialize response body buffer");
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

result_t http_response_cleanup(http_response_t* response) {
    if (!response) {
        RETURN_ERR("HTTP response pointer is NULL");
        return RESULT_ERR;
    }
    
    /* Clean up buffers */
    { result_t _r = data_clear(&response->headers); (void)_r; }
    { result_t _r = data_clear(&response->body); (void)_r; }
    
    /* Reset structure */
    memset(response, 0, sizeof(http_response_t));
    
    return RESULT_OK;
}
