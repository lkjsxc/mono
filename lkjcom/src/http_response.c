#include "http_response.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define CHUNK_SIZE (64 * 1024)

static void send_response_headers(int client_fd, int status_code, const char *mime_type, 
                                size_t content_length, int is_chunked) {
    char response[1024];
    char date_str[128];
    time_t now = time(NULL);
    
    strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    
    const char *status_text = "OK";
    switch (status_code) {
        case 400: status_text = "Bad Request"; break;
        case 403: status_text = "Forbidden"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
        case 501: status_text = "Not Implemented"; break;
    }
    
    int len = snprintf(response, sizeof(response),
                      "HTTP/1.1 %d %s\r\n"
                      "Date: %s\r\n"
                      "Server: C-HTTP-Server/1.0\r\n"
                      "Content-Type: %s\r\n",
                      status_code, status_text, date_str, mime_type ? mime_type : "text/plain");
    
    if (is_chunked) {
        len += snprintf(response + len, sizeof(response) - len,
                       "Transfer-Encoding: chunked\r\n");
    } else if (content_length > 0) {
        len += snprintf(response + len, sizeof(response) - len,
                       "Content-Length: %zu\r\n", content_length);
    }
    
    len += snprintf(response + len, sizeof(response) - len, "\r\n");
    
    write(client_fd, response, len);
}

void http_send_file_response(int client_fd, const FileContent *content) {
    if (!content) {
        http_send_error_response(client_fd, 500, "Internal Server Error");
        return;
    }
    
    if (content->is_chunked) {
        send_response_headers(client_fd, 200, content->mime_type, 0, 1);
        // Chunked response would be handled differently
        // For now, send error
        http_send_error_response(client_fd, 501, "Chunked transfer not fully implemented");
    } else {
        send_response_headers(client_fd, 200, content->mime_type, content->size, 0);
        if (content->data && content->size > 0) {
            write(client_fd, content->data, content->size);
        }
    }
}

void http_send_json_response(int client_fd, const char *json_data) {
    if (!json_data) {
        http_send_error_response(client_fd, 500, "Internal Server Error");
        return;
    }
    
    size_t json_len = strlen(json_data);
    send_response_headers(client_fd, 200, "application/json", json_len, 0);
    write(client_fd, json_data, json_len);
}

void http_send_error_response(int client_fd, int status_code, const char *message) {
    char error_html[1024];
    int len = snprintf(error_html, sizeof(error_html),
                      "<html><head><title>Error %d</title></head>"
                      "<body><h1>Error %d</h1><p>%s</p></body></html>",
                      status_code, status_code, message ? message : "Unknown Error");
    
    send_response_headers(client_fd, status_code, "text/html", len, 0);
    write(client_fd, error_html, len);
}

void http_send_chunked_file(int client_fd, const char *file_path, const char *mime_type) {
    // Simplified chunked implementation
    log_warn("Chunked file transfer not fully implemented");
    http_send_error_response(client_fd, 501, "Chunked transfer not implemented");
}
