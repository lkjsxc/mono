// lkjlib version 0001

#ifndef LKJLIB_H
#define LKJLIB_H

// Standard Libraries
#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Constants
#define POOL_SIZE_BIAS 16

#define POOL_data16_MAXCOUNT (65536 * POOL_SIZE_BIAS)
#define POOL_data256_MAXCOUNT (4096 * POOL_SIZE_BIAS)
#define POOL_data4096_MAXCOUNT (256 * POOL_SIZE_BIAS)
#define POOL_data65536_MAXCOUNT (16 * POOL_SIZE_BIAS)
#define POOL_data1048576_MAXCOUNT (1 * POOL_SIZE_BIAS)
#define POOL_OBJECT_MAXCOUNT (4096 * POOL_SIZE_BIAS)

// Types
typedef enum result_t {
    RESULT_OK = 0,
    RESULT_ERR = 1,
} result_t;
typedef struct data_t {
    char* data;
    uint64_t capacity;
    uint64_t size;
} data_t;
// JSON-like object tree node
typedef struct object_t {
    data_t* data;
    struct object_t* child;
    struct object_t* next;
} object_t;
typedef struct pool_t {
    uint64_t data16_freelist_count;
    uint64_t data256_freelist_count;
    uint64_t data4096_freelist_count;
    uint64_t data65536_freelist_count;
    uint64_t data1048576_freelist_count;
    data_t data16[POOL_data16_MAXCOUNT];
    data_t data256[POOL_data256_MAXCOUNT];
    data_t data4096[POOL_data4096_MAXCOUNT];
    data_t data65536[POOL_data65536_MAXCOUNT];
    data_t data1048576[POOL_data1048576_MAXCOUNT];
    data_t* data16_freelist_data[POOL_data16_MAXCOUNT];
    data_t* data256_freelist_data[POOL_data256_MAXCOUNT];
    data_t* data4096_freelist_data[POOL_data4096_MAXCOUNT];
    data_t* data65536_freelist_data[POOL_data65536_MAXCOUNT];
    data_t* data1048576_freelist_data[POOL_data1048576_MAXCOUNT];
    char data16_data[POOL_data16_MAXCOUNT * 16];
    char data256_data[POOL_data256_MAXCOUNT * 256];
    char data4096_data[POOL_data4096_MAXCOUNT * 4096];
    char data65536_data[POOL_data65536_MAXCOUNT * 65536];
    char data1048576_data[POOL_data1048576_MAXCOUNT * 1048576];
    object_t object_data[POOL_OBJECT_MAXCOUNT];
    object_t* object_freelist_data[POOL_OBJECT_MAXCOUNT];
    uint64_t object_freelist_count;
} pool_t;

// Macros
#define COUNTOF(array) (sizeof(array) / sizeof((array)[0]))
#define RETURN_ERR3(n) #n
#define RETURN_ERR2(n) RETURN_ERR3(n)
#define RETURN_ERR(error_message)                                                   \
    {                                                                               \
        _Pragma("GCC diagnostic push");                                             \
        _Pragma("GCC diagnostic ignored \"-Wunused-result\"");                      \
        write(STDERR_FILENO, "Error: { file: \"", 17);                              \
        write(STDERR_FILENO, __FILE__, sizeof(__FILE__));                           \
        write(STDERR_FILENO, "\", func: \"", 11);                                   \
        write(STDERR_FILENO, __func__, sizeof(__func__));                           \
        write(STDERR_FILENO, "\", line: ", 10);                                     \
        write(STDERR_FILENO, RETURN_ERR2(__LINE__), sizeof(RETURN_ERR2(__LINE__))); \
        write(STDERR_FILENO, "\", message: \"", 13);                                \
        write(STDERR_FILENO, error_message, sizeof(error_message));                 \
        write(STDERR_FILENO, "\" }\n", 4);                                          \
        return RESULT_ERR;                                                          \
        _Pragma("GCC diagnostic pop");                                              \
    }

// Pool
__attribute__((warn_unused_result)) result_t pool_init(pool_t* pool);
__attribute__((warn_unused_result)) result_t pool_data16_alloc(pool_t* pool, data_t** data);
__attribute__((warn_unused_result)) result_t pool_data256_alloc(pool_t* pool, data_t** data);
__attribute__((warn_unused_result)) result_t pool_data4096_alloc(pool_t* pool, data_t** data);
__attribute__((warn_unused_result)) result_t pool_data65536_alloc(pool_t* pool, data_t** data);
__attribute__((warn_unused_result)) result_t pool_data1048576_alloc(pool_t* pool, data_t** data);
__attribute__((warn_unused_result)) result_t pool_data_alloc(pool_t* pool, data_t** data, uint64_t capacity);
__attribute__((warn_unused_result)) result_t pool_data_free(pool_t* pool, data_t* data);
__attribute__((warn_unused_result)) result_t pool_data_realloc(pool_t* pool, data_t** data, uint64_t capacity);
__attribute__((warn_unused_result)) result_t pool_object_alloc(pool_t* pool, object_t** obj);
__attribute__((warn_unused_result)) result_t pool_object_free(pool_t* pool, object_t* obj);

// data
__attribute__((warn_unused_result)) result_t data_create(pool_t* pool, data_t** data);
__attribute__((warn_unused_result)) result_t data_create_data(pool_t* pool, data_t** data1, const data_t* data2);
__attribute__((warn_unused_result)) result_t data_create_str(pool_t* pool, data_t** data, const char* str);
__attribute__((warn_unused_result)) result_t data_destroy(pool_t* pool, data_t* data);
__attribute__((warn_unused_result)) result_t data_clean(pool_t* pool, data_t** data);
__attribute__((warn_unused_result)) result_t data_copy_data(pool_t* pool, data_t** data1, const data_t* data2);
__attribute__((warn_unused_result)) result_t data_copy_str(pool_t* pool, data_t** data, const char* str);
__attribute__((warn_unused_result)) result_t data_append_data(pool_t* pool, data_t** data1, const data_t* data2);
__attribute__((warn_unused_result)) result_t data_append_str(pool_t* pool, data_t** data, const char* str);
__attribute__((warn_unused_result)) result_t data_append_char(pool_t* pool, data_t** data, char c);
__attribute__((warn_unused_result)) result_t data_escape(pool_t* pool, data_t** data);
__attribute__((warn_unused_result)) result_t data_unescape(pool_t* pool, data_t** data);
uint64_t data_equal_data(const data_t* data1, const data_t* data2);
uint64_t data_equal_str(const data_t* data, const char* str);
int64_t data_find_data(const data_t* data1, const data_t* data2, uint64_t index);
int64_t data_find_str(const data_t* data, const char* str, uint64_t index);
int64_t data_find_char(const data_t* data, char c, uint64_t index);

// File
__attribute__((warn_unused_result)) result_t file_read(pool_t* pool, data_t** data, const char* path);
__attribute__((warn_unused_result)) result_t file_write(const char* path, const data_t* data);

// Object
__attribute__((warn_unused_result)) result_t object_create(pool_t* pool, object_t** dst);
__attribute__((warn_unused_result)) result_t object_destroy(pool_t* pool, object_t* object);
__attribute__((warn_unused_result)) result_t object_parse_json(pool_t* pool, object_t** dst, const data_t* src);
__attribute__((warn_unused_result)) result_t object_todata_json(pool_t* pool, data_t** dst, const object_t* src);
__attribute__((warn_unused_result)) result_t object_parse_xml(pool_t* pool, object_t** dst, const data_t* src);
__attribute__((warn_unused_result)) result_t object_todata_xml(pool_t* pool, data_t** dst, const object_t* src);
__attribute__((warn_unused_result)) result_t object_provide_str(pool_t* pool, object_t** dst, const object_t* object, const char* path);

#endif
