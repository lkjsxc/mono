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
    
    if (sscanf(buffer, "POST %d", &content_length) != 1) {
        send(client_socket, "ERROR\n", 6, 0);
        return;
    }
    
    char* newline = strchr(buffer, '\n');
    if (!newline || content_length <= 0 || content_length >= MAX_CONTENT) {
        send(client_socket, "ERROR\n", 6, 0);
        return;
    }
    
    strncpy(content, newline + 1, content_length);
    content[content_length] = '\0';
    
    if (message_count >= MAX_MESSAGES) {
        send(client_socket, "ERROR\n", 6, 0);
        return;
    }
    
    strcpy(messages[message_count].content, content);
    messages[message_count].timestamp = time(NULL);
    message_count++;
    
    send(client_socket, "OK\n", 3, 0);
}

void handle_get(int client_socket) {
    char response[BUFFER_SIZE];
    int pos = 0;
    
    pos += sprintf(response + pos, "%d\n", message_count);
    
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
    
    if (buffer[bytes_received - 1] == '\n') {
        buffer[bytes_received - 1] = '\0';
    }
    
    if (strncmp(buffer, "POST", 4) == 0) {
        handle_post(client_socket, buffer);
    } else if (strncmp(buffer, "GET", 3) == 0) {
        handle_get(client_socket);
    } else {
        send(client_socket, "ERROR\n", 6, 0);
    }
    
    close(client_socket);
}

int run_server() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_socket, 5);
    
    printf("Server running on port %d\n", SERVER_PORT);
    
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        handle_client(client_socket);
    }
    
    close(server_socket);
    return 0;
}

// Client implementation
int connect_to_server() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    char* server_host = getenv("SERVER_HOST");
    if (!server_host) server_host = "127.0.0.1";
    
    struct hostent* host_entry = gethostbyname(server_host);
    if (host_entry) {
        memcpy(&server_addr.sin_addr, host_entry->h_addr_list[0], host_entry->h_length);
    } else {
        server_addr.sin_addr.s_addr = inet_addr(server_host);
        if (server_addr.sin_addr.s_addr == INADDR_NONE) {
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
    if (!file) return -1;
    
    size_t bytes_read = fread(html_content, 1, sizeof(html_content) - 1, file);
    html_content[bytes_read] = '\0';
    fclose(file);
    return 0;
}

void serve_html(int client_socket) {
    send_http_response(client_socket, 200, "text/html", html_content);
}

void handle_api_post(int client_socket, char* body) {
    char message[256] = {0};
    char* message_start = strstr(body, "\"message\":\"");
    if (!message_start) {
        send_http_response(client_socket, 400, "application/json", "{\"status\":\"error\"}");
        return;
    }
    
    message_start += 10;
    if (*message_start == '"') message_start++;
    char* message_end = strchr(message_start, '"');
    if (message_end) {
        int len = message_end - message_start;
        if (len > 0 && len < sizeof(message)) {
            strncpy(message, message_start, len);
            message[len] = '\0';
        }
    }
    
    int server_socket = connect_to_server();
    if (server_socket < 0) {
        send_http_response(client_socket, 500, "application/json", "{\"status\":\"error\"}");
        return;
    }
    
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "POST %d\n%s", (int)strlen(message), message);
    send(server_socket, command, strlen(command), 0);
    
    char response[BUFFER_SIZE];
    int bytes = recv(server_socket, response, sizeof(response) - 1, 0);
    close(server_socket);
    
    if (bytes > 0 && strncmp(response, "OK", 2) == 0) {
        send_http_response(client_socket, 200, "application/json", "{\"status\":\"success\"}");
    } else {
        send_http_response(client_socket, 400, "application/json", "{\"status\":\"error\"}");
    }
}

void handle_api_feed(int client_socket) {
    int server_socket = connect_to_server();
    if (server_socket < 0) {
        send_http_response(client_socket, 500, "application/json", "{\"status\":\"error\"}");
        return;
    }
    
    send(server_socket, "GET\n", 4, 0);
    
    char response[BUFFER_SIZE];
    int bytes = recv(server_socket, response, sizeof(response) - 1, 0);
    close(server_socket);
    
    if (bytes > 0) {
        response[bytes] = '\0';
        int count = atoi(response);
        char* line = strchr(response, '\n');
        if (!line) {
            send_http_response(client_socket, 500, "application/json", "{\"status\":\"error\"}");
            return;
        }
        line++;
        
        char json[BUFFER_SIZE] = "{\"status\":\"success\",\"messages\":[";
        int json_pos = strlen(json);
        
        for (int i = 0; i < count && line; i++) {
            char content[256];
            long timestamp;
            int length;
            
            if (sscanf(line, "%ld %d", &timestamp, &length) != 2) break;
            
            char* content_start = strchr(line, '\n');
            if (!content_start) break;
            content_start++;
            
            if (length > 0 && length < sizeof(content)) {
                strncpy(content, content_start, length);
                content[length] = '\0';
            } else {
                strcpy(content, "");
            }
            
            if (i > 0) json_pos += sprintf(json + json_pos, ",");
            json_pos += sprintf(json + json_pos, "{\"timestamp\":%ld,\"content\":\"%s\"}", timestamp, content);
            
            line = strchr(content_start + length, '\n');
            if (line) line++;
        }
        
        json_pos += sprintf(json + json_pos, "]}");
        send_http_response(client_socket, 200, "application/json", json);
    } else {
        send_http_response(client_socket, 500, "application/json", "{\"status\":\"error\"}");
    }
}

void send_http_response(int client_socket, int status, char* content_type, char* body) {
    char response[BUFFER_SIZE];
    int pos = 0;
    
    if (status == 200) {
        pos += sprintf(response + pos, "HTTP/1.1 200 OK\r\n");
    } else if (status == 400) {
        pos += sprintf(response + pos, "HTTP/1.1 400 Bad Request\r\n");
    } else {
        pos += sprintf(response + pos, "HTTP/1.1 500 Internal Server Error\r\n");
    }
    
    pos += sprintf(response + pos, "Content-Type: %s\r\n", content_type);
    pos += sprintf(response + pos, "Content-Length: %zu\r\n", strlen(body));
    pos += sprintf(response + pos, "Connection: close\r\n\r\n");
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
    
    if (strncmp(buffer, "GET / ", 6) == 0 || strncmp(buffer, "GET / HTTP", 10) == 0) {
        serve_html(client_socket);
    } else if (strncmp(buffer, "POST /api/post", 14) == 0) {
        char* body_start = strstr(buffer, "\r\n\r\n");
        if (body_start) {
            handle_api_post(client_socket, body_start + 4);
        } else {
            send_http_response(client_socket, 400, "application/json", "{\"status\":\"error\"}");
        }
    } else if (strncmp(buffer, "GET /api/feed", 13) == 0) {
        handle_api_feed(client_socket);
    } else {
        send_http_response(client_socket, 404, "text/plain", "Not Found");
    }
    
    close(client_socket);
}

int run_client() {
    if (load_html_file() < 0) {
        printf("Failed to load HTML file\n");
        exit(1);
    }
    
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(CLIENT_PORT);
    
    bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_socket, 5);
    
    printf("Client running on port %d\n", CLIENT_PORT);
    
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        serve_http_request(client_socket);
    }
    
    close(server_socket);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server|client>\n", argv[0]);
        exit(1);
    }
    
    if (strcmp(argv[1], "server") == 0) {
        return run_server();
    } else if (strcmp(argv[1], "client") == 0) {
        return run_client();
    } else {
        printf("Invalid argument: %s\n", argv[1]);
        exit(1);
    }
}
