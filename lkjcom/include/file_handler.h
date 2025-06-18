#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct {
    char *data;
    size_t size;
    const char *mime_type;
    int is_chunked;
} FileContent;

int file_resolve_path(const char *doc_root, const char *uri, char *resolved_path, size_t path_size);
int file_read_content(const char *file_path, FileContent *content);
int file_read_chunk(const char *file_path, off_t offset, size_t chunk_size, char *buffer, size_t *bytes_read);
void file_content_free(FileContent *content);
int file_exists(const char *path);
int file_is_directory(const char *path);

#endif
