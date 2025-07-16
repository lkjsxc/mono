#include "lkjagent.h"

// HTTP configuration constants
#define HTTP_MAX_HOST_LEN 256
#define HTTP_MAX_PATH_LEN 1024
#define HTTP_MAX_REQUEST_LEN 8192
#define HTTP_MAX_RESPONSE_CHUNK 4096
#define HTTP_DEFAULT_PORT 80
#define HTTP_HTTPS_PORT 443
#define HTTP_USER_AGENT "lkjagent/1.0"
#define HTTP_PROTOCOL_HTTP "http://"
#define HTTP_PROTOCOL_HTTPS "https://"
#define HTTP_TIMEOUT_SECONDS 30

// HTTP status codes for better error handling
typedef enum {
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_CREATED = 201,
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_UNAUTHORIZED = 401,
    HTTP_STATUS_FORBIDDEN = 403,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
    HTTP_STATUS_BAD_GATEWAY = 502,
    HTTP_STATUS_SERVICE_UNAVAILABLE = 503
} http_status_t;

// URL parsing structure for better organization
typedef struct {
    char host[HTTP_MAX_HOST_LEN];
    char path[HTTP_MAX_PATH_LEN];
    int port;
    int is_https;
} url_info_t;

static result_t parse_url(const token_t* url_token, url_info_t* info) {
    if (token_validate(url_token) != RESULT_OK || !info || token_is_empty(url_token)) {
        return RESULT_ERR;
    }
    
    const char* url = url_token->data;
    
    // Initialize with defaults
    info->port = HTTP_DEFAULT_PORT;
    info->is_https = 0;
    strncpy(info->path, "/", sizeof(info->path) - 1);
    info->path[sizeof(info->path) - 1] = '\0';
    memset(info->host, 0, sizeof(info->host));
    
    // Determine protocol
    const char* start = url;
    if (strncmp(url, HTTP_PROTOCOL_HTTP, strlen(HTTP_PROTOCOL_HTTP)) == 0) {
        start = url + strlen(HTTP_PROTOCOL_HTTP);
        info->port = HTTP_DEFAULT_PORT;
        info->is_https = 0;
    } else if (strncmp(url, HTTP_PROTOCOL_HTTPS, strlen(HTTP_PROTOCOL_HTTPS)) == 0) {
        // HTTPS not supported in this basic implementation
        return RESULT_ERR;
    }
    
    // Find host end (either ':' for port or '/' for path)
    const char* host_end = start;
    while (*host_end && *host_end != ':' && *host_end != '/') {
        host_end++;
    }
    
    // Validate and copy host
    size_t host_len = host_end - start;
    if (host_len == 0 || host_len >= HTTP_MAX_HOST_LEN) {
        return RESULT_ERR;
    }
    
    strncpy(info->host, start, host_len);
    info->host[host_len] = '\0';
    
    // Parse port if present
    if (*host_end == ':') {
        host_end++; // Skip ':'
        char* end_ptr;
        long port = strtol(host_end, &end_ptr, 10);
        
        if (port <= 0 || port > 65535 || end_ptr == host_end) {
            return RESULT_ERR; // Invalid port
        }
        
        info->port = (int)port;
        host_end = end_ptr;
    }
    
    // Copy path if present
    if (*host_end == '/') {
        size_t path_len = strlen(host_end);
        if (path_len >= sizeof(info->path)) {
            return RESULT_ERR; // Path too long
        }
        strncpy(info->path, host_end, sizeof(info->path) - 1);
        info->path[sizeof(info->path) - 1] = '\0';
    }
    
    return RESULT_OK;
}

static result_t set_socket_timeout(int sockfd, int timeout_seconds) {
    if (sockfd < 0 || timeout_seconds <= 0) {
        return RESULT_ERR;
    }
    
    struct timeval timeout;
    timeout.tv_sec = timeout_seconds;
    timeout.tv_usec = 0;
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0 ||
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

static int connect_to_host(const char* host, int port) {
    if (!host || port <= 0 || port > 65535) {
        return -1;
    }
    
    struct hostent* server;
    struct sockaddr_in serv_addr;
    int sockfd;
    
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }
    
    // Set socket timeout
    if (set_socket_timeout(sockfd, HTTP_TIMEOUT_SECONDS) != RESULT_OK) {
        // Continue without timeout if setting fails
    }
    
    // Get server info
    server = gethostbyname(host);
    if (server == NULL) {
        close(sockfd);
        return -1;
    }
    
    // Setup server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons((uint16_t)port);
    
    // Connect
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

static result_t build_http_headers(token_t* request_token, const char* method, 
                                   const char* path, const char* host, size_t content_length) {
    if (token_validate(request_token) != RESULT_OK || !method || !path || !host) {
        return RESULT_ERR;
    }
    
    // Clear request token
    if (token_clear(request_token) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Start with request line
    if (token_append(request_token, method) != RESULT_OK ||
        token_append(request_token, " ") != RESULT_OK ||
        token_append(request_token, path) != RESULT_OK ||
        token_append(request_token, " HTTP/1.1\r\n") != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Add required headers
    if (token_append(request_token, "Host: ") != RESULT_OK ||
        token_append(request_token, host) != RESULT_OK ||
        token_append(request_token, "\r\n") != RESULT_OK ||
        token_append(request_token, "User-Agent: " HTTP_USER_AGENT "\r\n") != RESULT_OK ||
        token_append(request_token, "Connection: close\r\n") != RESULT_OK ||
        token_append(request_token, "Accept: */*\r\n") != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Add content headers if body is present
    if (content_length > 0) {
        char content_length_str[32];
        int len = snprintf(content_length_str, sizeof(content_length_str), "%zu", content_length);
        if (len < 0 || (size_t)len >= sizeof(content_length_str)) {
            return RESULT_ERR;
        }
        
        if (token_append(request_token, "Content-Type: application/json\r\n") != RESULT_OK ||
            token_append(request_token, "Content-Length: ") != RESULT_OK ||
            token_append(request_token, content_length_str) != RESULT_OK ||
            token_append(request_token, "\r\n") != RESULT_OK) {
            return RESULT_ERR;
        }
    }
    
    // End headers
    if (token_append(request_token, "\r\n") != RESULT_OK) {
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

static result_t send_data(int sockfd, const char* data, size_t length) {
    if (sockfd < 0 || !data || length == 0) {
        return RESULT_ERR;
    }
    
    size_t bytes_sent = 0;
    while (bytes_sent < length) {
        ssize_t sent = send(sockfd, data + bytes_sent, length - bytes_sent, 0);
        if (sent <= 0) {
            return RESULT_ERR;
        }
        bytes_sent += (size_t)sent;
    }
    
    return RESULT_OK;
}

static result_t send_http_request(int sockfd, const token_t* method_token, const char* path, 
                                  const char* host, const token_t* body_token) {
    if (sockfd < 0 || token_validate(method_token) != RESULT_OK || !path || !host) {
        return RESULT_ERR;
    }
    
    // Validate body token if provided
    size_t body_length = 0;
    if (body_token && body_token->data) {
        if (token_validate(body_token) != RESULT_OK) {
            return RESULT_ERR;
        }
        body_length = body_token->size;
    }
    
    // Create temporary buffer for request headers
    char request_buffer[HTTP_MAX_REQUEST_LEN];
    token_t request_token;
    
    if (token_init(&request_token, request_buffer, sizeof(request_buffer)) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Build headers
    if (build_http_headers(&request_token, method_token->data, path, host, body_length) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Send headers
    if (send_data(sockfd, request_token.data, request_token.size) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Send body if present
    if (body_length > 0) {
        if (send_data(sockfd, body_token->data, body_length) != RESULT_OK) {
            return RESULT_ERR;
        }
    }
    
    return RESULT_OK;
}

static result_t receive_http_response(int sockfd, token_t* response) {
    if (sockfd < 0 || token_validate(response) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    char buffer[HTTP_MAX_RESPONSE_CHUNK];
    ssize_t bytes_received;
    
    // Clear response token
    if (token_clear(response) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Receive data in chunks
    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        // Check if we have enough space
        if (token_available_space(response) < (int)bytes_received) {
            return RESULT_ERR; // Not enough space in response token
        }
        
        // Append received data to response token
        if (token_append_length(response, buffer, (size_t)bytes_received) != RESULT_OK) {
            return RESULT_ERR;
        }
    }
    
    if (bytes_received < 0) {
        return RESULT_ERR; // Error receiving data
    }
    
    return RESULT_OK;
}

__attribute__((warn_unused_result)) result_t http_request(token_t* method, token_t* url, token_t* body, token_t* response) {
    // Validate required parameters
    if (token_validate(method) != RESULT_OK || 
        token_validate(url) != RESULT_OK || 
        token_validate(response) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Check for empty method or URL
    if (token_is_empty(method) || token_is_empty(url)) {
        return RESULT_ERR;
    }
    
    // Validate body token if provided
    if (body && body->data && token_validate(body) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    url_info_t url_info;
    int sockfd = -1;
    result_t result = RESULT_ERR;
    
    // Parse URL
    if (parse_url(url, &url_info) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    // Connect to host
    sockfd = connect_to_host(url_info.host, url_info.port);
    if (sockfd < 0) {
        return RESULT_ERR;
    }
    
    // Send HTTP request
    if (send_http_request(sockfd, method, url_info.path, url_info.host, body) != RESULT_OK) {
        goto cleanup;
    }
    
    // Receive response
    if (receive_http_response(sockfd, response) != RESULT_OK) {
        goto cleanup;
    }
    
    result = RESULT_OK;

cleanup:
    if (sockfd >= 0) {
        close(sockfd);
    }
    
    return result;
}

__attribute__((warn_unused_result)) result_t http_get(token_t* url, token_t* response) {
    char method_buffer[16];
    token_t method;
    
    if (token_init(&method, method_buffer, sizeof(method_buffer)) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (token_set(&method, "GET") != RESULT_OK) {
        return RESULT_ERR;
    }
    
    return http_request(&method, url, NULL, response);
}

__attribute__((warn_unused_result)) result_t http_post(token_t* url, token_t* body, token_t* response) {
    char method_buffer[16];
    token_t method;
    
    if (token_init(&method, method_buffer, sizeof(method_buffer)) != RESULT_OK) {
        return RESULT_ERR;
    }
    
    if (token_set(&method, "POST") != RESULT_OK) {
        return RESULT_ERR;
    }
    
    return http_request(&method, url, body, response);
}