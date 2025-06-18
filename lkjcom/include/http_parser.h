#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#define MAX_METHOD_LEN 16
#define MAX_URI_LEN 2048
#define MAX_VERSION_LEN 16
#define MAX_HEADER_NAME_LEN 128
#define MAX_HEADER_VALUE_LEN 1024
#define MAX_HEADERS 50
#define MAX_BODY_SIZE (1024 * 1024)  // 1MB

typedef struct {
    char name[MAX_HEADER_NAME_LEN];
    char value[MAX_HEADER_VALUE_LEN];
} HttpHeader;

typedef struct {
    char method[MAX_METHOD_LEN];
    char uri[MAX_URI_LEN];
    char version[MAX_VERSION_LEN];
    HttpHeader headers[MAX_HEADERS];
    int header_count;
    char *body;
    int body_length;
    int content_length;
} HttpRequest;

int http_parse_request(const char *request_data, int data_len, HttpRequest *request);
const char* http_get_header(const HttpRequest *request, const char *name);
void http_request_cleanup(HttpRequest *request);

#endif
