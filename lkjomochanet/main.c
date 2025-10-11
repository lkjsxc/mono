#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>

#define MAX_MESSAGES 100
#define MAX_CONTENT 256
#define SERVER_PORT 8080
#define CLIENT_PORT 3000
#define BUFFER_SIZE 4096

typedef struct {
    char content[MAX_CONTENT];
    time_t timestamp;
} Message;

static Message messages[MAX_MESSAGES];
static int message_count = 0;
static char html_content[BUFFER_SIZE * 10]; // Buffer for HTML content

// Server functions
void handle_post(int client_socket, char* buffer);
void handle_get(int client_socket);
void handle_client(int client_socket);
int run_server();

// Client functions
int connect_to_server();
int load_html_file();
void serve_html(int client_socket);
void handle_api_post(int client_socket, char* body);
void handle_api_feed(int client_socket);
void send_http_response(int client_socket, int status, char* content_type, char* body);
void serve_http_request(int client_socket);
int run_client();

// Server implementation
void handle_post(int client_socket, char* buffer) {
    int content_length;
    char content[MAX_CONTENT];
    
    // Parse: POST <length>\n<content>
    if (sscanf(buffer, "POST %d", &content_length) != 1) {
        send(client_socket, "ERROR Invalid POST format\n", 26, 0);
        return;
    }
    
    // Find the newline and read content
    char* newline = strchr(buffer, '\n');
    if (!newline || content_length <= 0 || content_length >= MAX_CONTENT) {
        send(client_socket, "ERROR Invalid content length\n", 29, 0);
        return;
    }
    
    // Copy content (skip the newline)
    strncpy(content, newline + 1, content_length);
    content[content_length] = '\0';
    
    // Check if we have space for more messages
    if (message_count >= MAX_MESSAGES) {
        send(client_socket, "ERROR Server full\n", 18, 0);
        return;
    }
    
    // Add message
    strcpy(messages[message_count].content, content);
    messages[message_count].timestamp = time(NULL);
    message_count++;
    
    send(client_socket, "OK\n", 3, 0);
}

void handle_get(int client_socket) {
    char response[BUFFER_SIZE];
    int pos = 0;
    
    // Send count
    pos += sprintf(response + pos, "%d\n", message_count);
    
    // Send each message
    for (int i = 0; i < message_count; i++) {
        pos += sprintf(response + pos, "%ld %zu\n%s\n", 
                      messages[i].timestamp,
                      strlen(messages[i].content),
                      messages[i].content);
    }
    
    send(client_socket, response, pos, 0);
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_received <= 0) {
        close(client_socket);
        return;
    }
    
    buffer[bytes_received] = '\0';
    
    // Remove trailing newline if present
    if (buffer[bytes_received - 1] == '\n') {
        buffer[bytes_received - 1] = '\0';
    }
    
    // Parse command
    if (strncmp(buffer, "POST", 4) == 0) {
        handle_post(client_socket, buffer);
    } else if (strncmp(buffer, "GET", 3) == 0) {
        handle_get(client_socket);
    } else {
        send(client_socket, "ERROR Unknown command\n", 22, 0);
    }
    
    close(client_socket);
}

int run_server() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(1);
    }
    
    // Setup server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    
    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    // Listen for connections
    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        exit(1);
    }
    
    printf("Social Network Server running on port %d\n", SERVER_PORT);
    printf("Protocol: POST <length>\\n<content> or GET\\n\n");
    fflush(stdout);
    
    // Main server loop
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        printf("Client connected from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));
        
        handle_client(client_socket);
    }
    
    close(server_socket);
    return 0;
}

// Client implementation
int connect_to_server() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("Failed to create socket\n");
        return -1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    // Try to connect to server hostname first (for Docker), then localhost (for local dev)
    char* server_host = getenv("SERVER_HOST");
    if (!server_host) {
        server_host = "127.0.0.1";  // Default for local development
    }
    
    // Try to resolve hostname to IP address
    struct hostent* host_entry = gethostbyname(server_host);
    if (host_entry != NULL) {
        memcpy(&server_addr.sin_addr, host_entry->h_addr_list[0], host_entry->h_length);
    } else {
        // Fallback to direct IP address conversion
        server_addr.sin_addr.s_addr = inet_addr(server_host);
        if (server_addr.sin_addr.s_addr == INADDR_NONE) {
            // If hostname resolution failed, try localhost
            server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        }
    }
    
    if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_socket);
        return -1;
    }
    return server_socket;
}

int load_html_file() {
    FILE* file = fopen("index.html", "r");
    if (!file) {
        printf("Error: Could not open index.html\n");
        return -1;
    }
    
    size_t bytes_read = fread(html_content, 1, sizeof(html_content) - 1, file);
    html_content[bytes_read] = '\0';
    fclose(file);
    
    printf("Loaded HTML file (%zu bytes)\n", bytes_read);
    return 0;
}

void serve_html(int client_socket) {
    send_http_response(client_socket, 200, "text/html", html_content);
}

void handle_api_post(int client_socket, char* body) {
    // Parse JSON: {"message":"content"}
    char message[256] = {0};
    
    // Simple JSON parsing (find message value)
    char* message_start = strstr(body, "\"message\":\"");
    
    if (!message_start) {
        send_http_response(client_socket, 400, "application/json", 
                          "{\"status\":\"error\",\"error\":\"Invalid JSON format\"}");
        return;
    }
    
    // Extract message
    message_start += 10; // Skip "message":"
    
    // Skip the opening quote of the message value
    if (*message_start == '"') {
        message_start++;
    }
    
    char* message_end = strchr(message_start, '"');
    if (message_end) {
        int len = message_end - message_start;
        if (len > 0 && len < sizeof(message)) {
            strncpy(message, message_start, len);
            message[len] = '\0'; // Ensure null termination
        }
    }
    
    // Connect to server and send POST command
    int server_socket = connect_to_server();
    if (server_socket < 0) {
        send_http_response(client_socket, 500, "application/json", 
                          "{\"status\":\"error\",\"error\":\"Cannot connect to server\"}");
        return;
    }
    
    // Format command: POST <length>\n<content>
    char command[BUFFER_SIZE];
    int content_len = strlen(message);
    snprintf(command, sizeof(command), "POST %d\n%s", content_len, message);
    
    send(server_socket, command, strlen(command), 0);
    
    // Read response
    char response[BUFFER_SIZE];
    int bytes = recv(server_socket, response, sizeof(response) - 1, 0);
    close(server_socket);
    
    if (bytes > 0) {
        response[bytes] = '\0';
        if (strncmp(response, "OK", 2) == 0) {
            send_http_response(client_socket, 200, "application/json", 
                              "{\"status\":\"success\"}");
        } else {
            send_http_response(client_socket, 400, "application/json", 
                              "{\"status\":\"error\",\"error\":\"Server error\"}");
        }
    } else {
        send_http_response(client_socket, 500, "application/json", 
                          "{\"status\":\"error\",\"error\":\"No response from server\"}");
    }
}

void handle_api_feed(int client_socket) {
    int server_socket = connect_to_server();
    if (server_socket < 0) {
        send_http_response(client_socket, 500, "application/json", 
                          "{\"status\":\"error\",\"error\":\"Cannot connect to server\"}");
        return;
    }
    
    // Send GET command
    send(server_socket, "GET\n", 4, 0);
    
    // Read response
    char response[BUFFER_SIZE];
    int bytes = recv(server_socket, response, sizeof(response) - 1, 0);
    close(server_socket);
    
    if (bytes > 0) {
        response[bytes] = '\0';
        
        // Parse response: <count>\n<timestamp1> <length1>\n<content1>\n...
        int count = atoi(response);
        char* line = strchr(response, '\n');
        if (!line) {
            send_http_response(client_socket, 500, "application/json", 
                              "{\"status\":\"error\",\"error\":\"Invalid server response\"}");
            return;
        }
        line++; // Skip first newline
        
        char json[BUFFER_SIZE] = "{\"status\":\"success\",\"messages\":[";
        int json_pos = strlen(json);
        
        for (int i = 0; i < count && line; i++) {
            char content[256];
            long timestamp;
            int length;
            
            // Parse: <timestamp> <length>\n<content>\n
            if (sscanf(line, "%ld %d", &timestamp, &length) != 2) {
                break;
            }
            
            // Find content
            char* content_start = strchr(line, '\n');
            if (!content_start) break;
            content_start++;
            
            if (length > 0 && length < sizeof(content)) {
                strncpy(content, content_start, length);
                content[length] = '\0';
            } else {
                strcpy(content, "");
            }
            
            // Add to JSON
            if (i > 0) json_pos += sprintf(json + json_pos, ",");
            json_pos += sprintf(json + json_pos, 
                "{\"timestamp\":%ld,\"content\":\"%s\"}", 
                timestamp, content);
            
            // Move to next message
            line = strchr(content_start + length, '\n');
            if (line) line++;
        }
        
        json_pos += sprintf(json + json_pos, "]}");
        send_http_response(client_socket, 200, "application/json", json);
    } else {
        send_http_response(client_socket, 500, "application/json", 
                          "{\"status\":\"error\",\"error\":\"No response from server\"}");
    }
}

void send_http_response(int client_socket, int status, char* content_type, char* body) {
    char response[BUFFER_SIZE];
    int pos = 0;
    
    // HTTP status line
    if (status == 200) {
        pos += sprintf(response + pos, "HTTP/1.1 200 OK\r\n");
    } else if (status == 400) {
        pos += sprintf(response + pos, "HTTP/1.1 400 Bad Request\r\n");
    } else {
        pos += sprintf(response + pos, "HTTP/1.1 500 Internal Server Error\r\n");
    }
    
    // Headers
    pos += sprintf(response + pos, "Content-Type: %s\r\n", content_type);
    pos += sprintf(response + pos, "Content-Length: %zu\r\n", strlen(body));
    pos += sprintf(response + pos, "Connection: close\r\n");
    pos += sprintf(response + pos, "\r\n");
    
    // Body
    strcpy(response + pos, body);
    
    send(client_socket, response, pos + strlen(body), 0);
}

void serve_http_request(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_received <= 0) {
        close(client_socket);
        return;
    }
    
    buffer[bytes_received] = '\0';
    
    // Parse HTTP request
    if (strncmp(buffer, "GET / ", 6) == 0 || strncmp(buffer, "GET / HTTP", 10) == 0) {
        serve_html(client_socket);
    } else if (strncmp(buffer, "POST /api/post", 14) == 0) {
        // Find body (after \r\n\r\n)
        char* body_start = strstr(buffer, "\r\n\r\n");
        if (body_start) {
            body_start += 4;
            handle_api_post(client_socket, body_start);
        } else {
            send_http_response(client_socket, 400, "application/json", 
                              "{\"status\":\"error\",\"error\":\"No request body\"}");
        }
    } else if (strncmp(buffer, "GET /api/feed", 13) == 0) {
        handle_api_feed(client_socket);
    } else {
        send_http_response(client_socket, 404, "text/plain", "Not Found");
    }
    
    close(client_socket);
}

int run_client() {
    // Load HTML file at startup
    if (load_html_file() < 0) {
        printf("Failed to load HTML file. Exiting.\n");
        exit(1);
    }
    
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(1);
    }
    
    // Setup server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(CLIENT_PORT);
    
    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    // Listen for connections
    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        exit(1);
    }
    
    printf("HTTP Client Server running on port %d\n", CLIENT_PORT);
    printf("Open http://localhost:%d in your browser\n", CLIENT_PORT);
    
    // Main server loop
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        serve_http_request(client_socket);
    }
    
    close(server_socket);
    return 0;
}

// Main function
int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server|client>\n", argv[0]);
        printf("  server - Run the social network backend server (port %d)\n", SERVER_PORT);
        printf("  client - Run the HTTP client server (port %d)\n", CLIENT_PORT);
        exit(1);
    }
    
    if (strcmp(argv[1], "server") == 0) {
        return run_server();
    } else if (strcmp(argv[1], "client") == 0) {
        return run_client();
    } else {
        printf("Error: Invalid argument '%s'\n", argv[1]);
        printf("Use 'server' or 'client'\n");
        exit(1);
    }
}
