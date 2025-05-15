#ifndef LKJSCRIPT_H
#define LKJSCRIPT_H

#include <fcntl.h>
#include <unistd.h>

#define SRC_PATH "script/main.lkjscript"

typedef long long int int64_t;

typedef enum {
    OK,
    ERR
} result_t;

#endif