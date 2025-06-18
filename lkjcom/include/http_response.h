#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "file_handler.h"
#include "json_parser.h"

void http_send_file_response(int client_fd, const FileContent *content);
void http_send_json_response(int client_fd, const char *json_data);
void http_send_error_response(int client_fd, int status_code, const char *message);
void http_send_chunked_file(int client_fd, const char *file_path, const char *mime_type);

#endif
