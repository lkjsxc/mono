#include "file_handler.h"
#include "mime_types.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define CHUNK_SIZE (64 * 1024)  // 64KB chunks

int file_resolve_path(const char *doc_root, const char *uri, char *resolved_path, size_t path_size) {
    if (!doc_root || !uri || !resolved_path) return -1;
    
    // Decode URI
    char *decoded_uri = url_decode(uri);
    if (!decoded_uri) return -1;
    
    // Security check - prevent directory traversal
    if (strstr(decoded_uri, "..") || strstr(decoded_uri, "//")) {
        log_warn("Directory traversal attempt: %s", decoded_uri);
        free(decoded_uri);
        return -1;
    }
    
    // Build full path
    int ret = snprintf(resolved_path, path_size, "%s%s", doc_root, decoded_uri);
    free(decoded_uri);
    
    if (ret >= (int)path_size) {
        log_error("Path too long");
        return -1;
    }
    
    // If path ends with '/', try index.html
    if (resolved_path[strlen(resolved_path) - 1] == '/') {
        if (strlen(resolved_path) + strlen("index.html") < path_size) {
            strcat(resolved_path, "index.html");
        }
    }
    
    return 0;
}

int file_read_content(const char *file_path, FileContent *content) {
    if (!file_path || !content) return -1;
    
    memset(content, 0, sizeof(FileContent));
    
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        log_error("Cannot open file: %s - %s", file_path, strerror(errno));
        return -1;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < 0) {
        fclose(file);
        return -1;
    }
    
    // For large files, mark for chunked transfer
    if (file_size > CHUNK_SIZE * 2) {
        content->is_chunked = 1;
        content->size = file_size;
        content->data = NULL;
    } else {
        // Read entire file for small files
        content->data = safe_malloc(file_size + 1);
        size_t bytes_read = fread(content->data, 1, file_size, file);
        content->data[bytes_read] = '\0';
        content->size = bytes_read;
        content->is_chunked = 0;
    }
    
    content->mime_type = mime_get_type(file_path);
    fclose(file);
    
    return 0;
}

int file_read_chunk(const char *file_path, off_t offset, size_t chunk_size, char *buffer, size_t *bytes_read) {
    if (!file_path || !buffer || !bytes_read) return -1;
    
    FILE *file = fopen(file_path, "rb");
    if (!file) return -1;
    
    if (fseek(file, offset, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }
    
    *bytes_read = fread(buffer, 1, chunk_size, file);
    fclose(file);
    
    return 0;
}

void file_content_free(FileContent *content) {
    if (content && content->data) {
        free(content->data);
        content->data = NULL;
    }
}

int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

int file_is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return 0;
}
