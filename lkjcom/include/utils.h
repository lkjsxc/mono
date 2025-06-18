#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

// Logging levels
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

// Memory management
void* safe_malloc(size_t size);
void* safe_realloc(void *ptr, size_t size);
char* safe_strdup(const char *str);

// Logging
void log_message(LogLevel level, const char *format, ...);
void log_debug(const char *format, ...);
void log_info(const char *format, ...);
void log_warn(const char *format, ...);
void log_error(const char *format, ...);

// String utilities
char* url_decode(const char *encoded);
char* trim_whitespace(char *str);
int safe_strcpy(char *dest, const char *src, size_t dest_size);

#endif
