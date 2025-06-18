#include "request_handler.h"
#include "http_parser.h"
#include "http_response.h"
#include "file_handler.h"
#include "json_parser.h"
#include "utils.h"
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 8192

static void handle_get_request(int client_fd, const HttpRequest *request, const ServerConfig *config) {
    char file_path[1024];
    
    if (file_resolve_path(config->document_root, request->uri, file_path, sizeof(file_path)) != 0) {
        http_send_error_response(client_fd, 400, "Bad Request");
        return;
    }
    
    if (!file_exists(file_path)) {
        http_send_error_response(client_fd, 404, "File Not Found");
        return;
    }
    
    FileContent content;
    if (file_read_content(file_path, &content) != 0) {
        http_send_error_response(client_fd, 500, "Internal Server Error");
        return;
    }
    
    http_send_file_response(client_fd, &content);
    file_content_free(&content);
}

static void handle_post_request(int client_fd, const HttpRequest *request, Database *db) {
    if (!request->body || request->body_length == 0) {
        http_send_error_response(client_fd, 400, "Empty POST body");
        return;
    }
    
    // Parse JSON events
    EventList *events = json_parse_events(request->body);
    if (!events || events->count == 0) {
        http_send_error_response(client_fd, 400, "Invalid JSON or no events");
        if (events) event_list_free(events);
        return;
    }
    
    // Store events in database
    int stored_count = 0;
    for (int i = 0; i < events->count; i++) {
        if (db_add_event(db, &events->events[i]) == 0) {
            stored_count++;
        }
    }
    
    // Send response
    char response_json[512];
    snprintf(response_json, sizeof(response_json),
             "{\"status\":\"success\",\"events_received\":%d,\"events_stored\":%d}",
             events->count, stored_count);
    
    http_send_json_response(client_fd, response_json);
    event_list_free(events);
    
    log_info("Processed POST request: %d events received, %d stored", events->count, stored_count);
}

void handle_client_request(int client_fd, const ServerConfig *config, Database *db) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = read(client_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_received <= 0) {
        log_error("Failed to read from client socket");
        return;
    }
    
    buffer[bytes_received] = '\0';
    
    HttpRequest request;
    if (http_parse_request(buffer, bytes_received, &request) != 0) {
        log_error("Failed to parse HTTP request");
        http_send_error_response(client_fd, 400, "Bad Request");
        return;
    }
    
    log_info("Request: %s %s %s", request.method, request.uri, request.version);
    
    if (strcmp(request.method, "GET") == 0) {
        handle_get_request(client_fd, &request, config);
    } else if (strcmp(request.method, "POST") == 0) {
        handle_post_request(client_fd, &request, db);
    } else {
        http_send_error_response(client_fd, 501, "Method Not Implemented");
    }
    
    http_request_cleanup(&request);
}
