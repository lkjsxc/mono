#include "http_parser.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>

int http_parse_request(const char *request_data, int data_len, HttpRequest *request) {
    if (!request_data || !request || data_len <= 0) {
        return -1;
    }
    
    // Initialize request structure
    memset(request, 0, sizeof(HttpRequest));
    
    // Find end of headers
    const char *header_end = strstr(request_data, "\r\n\r\n");
    if (!header_end) {
        header_end = strstr(request_data, "\n\n");
        if (!header_end) {
            log_error("Invalid HTTP request: no header termination found");
            return -1;
        }
    }
    
    // Copy header section for parsing
    size_t header_len = header_end - request_data;
    char *headers = safe_malloc(header_len + 1);
    memcpy(headers, request_data, header_len);
    headers[header_len] = '\0';
    
    // Parse request line
    char *line = strtok(headers, "\r\n");
    if (!line) {
        free(headers);
        return -1;
    }
    
    char *method = strtok(line, " ");
    char *uri = strtok(NULL, " ");
    char *version = strtok(NULL, " ");
    
    if (!method || !uri || !version) {
        free(headers);
        return -1;
    }
    
    safe_strcpy(request->method, method, sizeof(request->method));
    safe_strcpy(request->uri, uri, sizeof(request->uri));
    safe_strcpy(request->version, version, sizeof(request->version));
    
    // Parse headers
    while ((line = strtok(NULL, "\r\n")) != NULL && request->header_count < MAX_HEADERS) {
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            char *name = trim_whitespace(line);
            char *value = trim_whitespace(colon + 1);
            
            safe_strcpy(request->headers[request->header_count].name, name, 
                       sizeof(request->headers[request->header_count].name));
            safe_strcpy(request->headers[request->header_count].value, value,
                       sizeof(request->headers[request->header_count].value));
            request->header_count++;
        }
    }
    
    free(headers);
    
    // Parse body if present
    const char *content_length_str = http_get_header(request, "Content-Length");
    if (content_length_str) {
        request->content_length = atoi(content_length_str);
        if (request->content_length > 0 && request->content_length < MAX_BODY_SIZE) {
            const char *body_start = strstr(request_data, "\r\n\r\n");
            if (!body_start) {
                body_start = strstr(request_data, "\n\n");
                body_start += 2;
            } else {
                body_start += 4;
            }
            
            int available_body = data_len - (body_start - request_data);
            int body_len = (available_body < request->content_length) ? 
                          available_body : request->content_length;
            
            if (body_len > 0) {
                request->body = safe_malloc(body_len + 1);
                memcpy(request->body, body_start, body_len);
                request->body[body_len] = '\0';
                request->body_length = body_len;
            }
        }
    }
    
    return 0;
}

const char* http_get_header(const HttpRequest *request, const char *name) {
    if (!request || !name) return NULL;
    
    for (int i = 0; i < request->header_count; i++) {
        if (strcasecmp(request->headers[i].name, name) == 0) {
            return request->headers[i].value;
        }
    }
    return NULL;
}

void http_request_cleanup(HttpRequest *request) {
    if (request && request->body) {
        free(request->body);
        request->body = NULL;
    }
}
