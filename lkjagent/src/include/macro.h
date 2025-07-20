#ifndef LKJAGENT_MACRO_H
#define LKJAGENT_MACRO_H

#include "std.h"

#define RETURN_ERR3(n) #n
#define RETURN_ERR2(n) RETURN_ERR3(n)
#define RETURN_ERR(error_message)                                                   \
    {                                                                               \
        _Pragma("GCC diagnostic push")                                              \
            _Pragma("GCC diagnostic ignored \"-Wunused-result\"")                   \
                write(STDERR_FILENO, "Error: { file: \"", 17);                      \
        write(STDERR_FILENO, __FILE__, sizeof(__FILE__));                           \
        write(STDERR_FILENO, "\", func: \"", 11);                                   \
        write(STDERR_FILENO, __func__, sizeof(__func__));                           \
        write(STDERR_FILENO, "\", line: ", 10);                                     \
        write(STDERR_FILENO, RETURN_ERR2(__LINE__), sizeof(RETURN_ERR2(__LINE__))); \
        write(STDERR_FILENO, "\", message: \"", 13);                                \
        write(STDERR_FILENO, error_message, sizeof(error_message));                 \
        write(STDERR_FILENO, "\" }\n", 4);                                          \
        _Pragma("GCC diagnostic pop") return RESULT_ERR;                            \
    }

#endif