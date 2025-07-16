#ifndef _LKJAGENT_H
#define _LKJAGENT_H

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

typedef enum {
    RESULT_OK = 0,
    RESULT_ERR = 1,
} result_t;

typedef struct {
    char* data;
    int size;
    int capacity;
} token_t;

typedef struct {
    token_t base_url;
    token_t model;
} config_t;

__attribute__((warn_unused_result)) result_t http_request(token_t* method, token_t* url, token_t* body, token_t* response);

#endif