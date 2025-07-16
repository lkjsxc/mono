#include "lkjagent.h"

// Helper function to parse URL and extract host, port, and path
static result_t parse_url(const char* url, char* host, int* port, char* path) {
    if (!url || !host || !port || !path) {
        return RESULT_ERR;
    }
    
    // Default values
    *port = 80;
    strcpy(path, "/");
    
    // Skip protocol if present
    const char* start = url;
    if (strncmp(url, "http://", 7) == 0) {
        start = url + 7;
        *port = 80;
    } else if (strncmp(url, "https://", 8) == 0) {
        return RESULT_ERR; // HTTPS not supported in this implementation
    }
    
    // Find host end (either ':' for port or '/' for path)
    const char* host_end = start;
    while (*host_end && *host_end != ':' && *host_end != '/') {
        host_end++;
    }
    
    // Copy host
    int host_len = host_end - start;
    if (host_len >= 256) { // Reasonable host length limit
        return RESULT_ERR;
    }
    strncpy(host, start, host_len);
    host[host_len] = '\0';
    
    // Parse port if present
    if (*host_end == ':') {
        host_end++; // Skip ':'
        *port = atoi(host_end);
        // Skip to path
        while (*host_end && *host_end != '/') {
            host_end++;
        }
    }
    
    // Copy path
    if (*host_end == '/') {
        strncpy(path, host_end, 16); // Reasonable path length limit
        path[1023] = '\0';
    }
    
    return RESULT_OK;
}

// Helper function to connect to host
static int connect_to_host(const char* host, int port) {
    struct hostent* server;
    struct sockaddr_in serv_addr;
    int sockfd;
    
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
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
    serv_addr.sin_port = htons(port);
    
    // Connect
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

// Helper function to send HTTP request
static result_t send_http_request(int sockfd, const char* method, const char* path, 
                                  const char* host, const char* body) {
    char request[8192];
    int content_length = body ? strlen(body) : 0;
    
    // Build HTTP request
    int len = snprintf(request, sizeof(request),
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: lkjagent/1.0\r\n"
        "Connection: close\r\n",
        method, path, host);
    
    if (content_length > 0) {
        len += snprintf(request + len, sizeof(request) - len,
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n",
            content_length);
    }
    
    // End headers
    len += snprintf(request + len, sizeof(request) - len, "\r\n");
    
    // Add body if present
    if (body && content_length > 0) {
        len += snprintf(request + len, sizeof(request) - len, "%s", body);
    }
    
    // Send request
    if (send(sockfd, request, len, 0) != len) {
        return RESULT_ERR;
    }
    
    return RESULT_OK;
}

// Helper function to receive HTTP response
static result_t receive_http_response(int sockfd, token_t* response) {
    char buffer[16384];
    int total_received = 0;
    int bytes_received;
    
    // Clear response token
    response->size = 0;
    
    // Receive data in chunks
    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        if (total_received + bytes_received >= response->capacity) {
            return RESULT_ERR; // Not enough space in response token
        }
        
        memcpy(response->data + total_received, buffer, bytes_received);
        total_received += bytes_received;
    }
    
    if (bytes_received < 0) {
        return RESULT_ERR; // Error receiving data
    }
    
    response->data[total_received] = '\0'; // Null-terminate the response
    response->size = total_received;
    
    return RESULT_OK;
}

__attribute__((warn_unused_result)) result_t http_request(token_t* method, token_t* url, token_t* body, token_t* response) {
    if (!method || !url || !response) {
        return RESULT_ERR;
    }
    
    if (!method->data || !url->data) {
        return RESULT_ERR;
    }
    
    char host[256];
    char path[16];
    int port;
    int sockfd = -1;
    result_t result = RESULT_ERR;
    
    // Parse URL
    if (parse_url(url->data, host, &port, path) == RESULT_ERR) {
        return RESULT_ERR;
    }
    
    // Connect to host
    sockfd = connect_to_host(host, port);
    if (sockfd < 0) {
        return RESULT_ERR;
    }
    
    // Send HTTP request
    const char* body_data = (body && body->data) ? body->data : NULL;
    if (send_http_request(sockfd, method->data, path, host, body_data) == RESULT_ERR) {
        close(sockfd);
        return RESULT_ERR;
    }
    
    // Receive response
    if (receive_http_response(sockfd, response) == RESULT_ERR) {
        close(sockfd);
        return RESULT_ERR;
    }
    
    close(sockfd);
    return RESULT_OK;
}