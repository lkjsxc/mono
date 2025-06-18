#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>

void* safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        log_error("Memory allocation failed for size %zu", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void* safe_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        log_error("Memory reallocation failed for size %zu", size);
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

char* safe_strdup(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char *copy = safe_malloc(len);
    memcpy(copy, str, len);
    return copy;
}

void log_message(LogLevel level, const char *format, ...) {
    const char *level_strings[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    printf("[%s] [%s] ", time_str, level_strings[level]);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
}

void log_debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_DEBUG, format, args);
    va_end(args);
}

void log_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_INFO, format, args);
    va_end(args);
}

void log_warn(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_WARN, format, args);
    va_end(args);
}

void log_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_ERROR, format, args);
    va_end(args);
}

char* url_decode(const char *encoded) {
    if (!encoded) return NULL;
    
    size_t len = strlen(encoded);
    char *decoded = safe_malloc(len + 1);
    size_t i = 0, j = 0;
    
    while (i < len) {
        if (encoded[i] == '%' && i + 2 < len) {
            int hex_value;
            if (sscanf(&encoded[i + 1], "%2x", &hex_value) == 1) {
                decoded[j++] = (char)hex_value;
                i += 3;
            } else {
                decoded[j++] = encoded[i++];
            }
        } else if (encoded[i] == '+') {
            decoded[j++] = ' ';
            i++;
        } else {
            decoded[j++] = encoded[i++];
        }
    }
    
    decoded[j] = '\0';
    return decoded;
}

char* trim_whitespace(char *str) {
    if (!str) return NULL;
    
    // Trim leading whitespace
    while (isspace(*str)) str++;
    
    if (*str == '\0') return str;
    
    // Trim trailing whitespace
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    end[1] = '\0';
    
    return str;
}

int safe_strcpy(char *dest, const char *src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) return -1;
    
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        strncpy(dest, src, dest_size - 1);
        dest[dest_size - 1] = '\0';
        return -1; // Truncated
    }
    
    strcpy(dest, src);
    return 0;
}
