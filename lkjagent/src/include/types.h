#ifndef LKJAGENT_TYPES_H
#define LKJAGENT_TYPES_H

#include "std.h"
#include "const.h"

typedef enum {
    RESULT_OK = 0,
    RESULT_ERR = 1,
} result_t;

typedef struct {
    char* data;
    uint64_t size;
    uint64_t capacity;
} string_t;

typedef struct {
    char io_buf_data[IO_BUF_CAPACITY];
    string_t io_buf;
} lkjagent_t;

#endif
