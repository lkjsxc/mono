#ifndef LKJSCRIPT_H
#define LKJSCRIPT_H

#include <fcntl.h>
#include <unistd.h>

#define MEM_SIZE (1024 * 1024)
#define SRC_PATH "script/main.lkjscript"

typedef long long int int64_t;

typedef enum {
    OK,
    ERR
} result_t;

typedef enum {
    TOKENTYPE_NUM,
    TOKENTYPE_STR,
    TOKENTYPE_IDENT,
} tokentype_t;

typedef struct {
    const char* data;
    int64_t size;
} token_t;

typedef struct {
    int64_t bin[MEM_SIZE / sizeof(int64_t) / 6];
    char src[MEM_SIZE / 6];
    token_t token[MEM_SIZE / sizeof(token_t) / 6];
} compile_t;

typedef union {
    int64_t i64[MEM_SIZE];
    compile_t compile;
} mem_t;

#endif