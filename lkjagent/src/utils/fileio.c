#include "utils/lkjfileio.h"

result_t file_read(const char* path, string_t* string) {
    if (!path || !string) {
        RETURN_ERR("Invalid parameters");
    }

    FILE* file = fopen(path, "rb");
    if (!file) {
        RETURN_ERR("Failed to open file for reading");
    }

    // Get file size
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        RETURN_ERR("Failed to seek to end of file");
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        RETURN_ERR("Failed to get file size");
    }

    if ((uint64_t)file_size >= string->capacity) {
        fclose(file);
        RETURN_ERR("File too large for buffer");
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        RETURN_ERR("Failed to seek to beginning of file");
    }

    // Read file string
    size_t bytes_read = fread(string->data, 1, file_size, file);
    if (bytes_read != (size_t)file_size) {
        fclose(file);
        RETURN_ERR("Failed to read complete file");
    }

    string->data[file_size] = '\0';
    string->size = file_size;

    fclose(file);
    return RESULT_OK;
}

result_t file_write(const char* path, const string_t* string) {
    if (!path || !string) {
        RETURN_ERR("Invalid parameters");
    }

    FILE* file = fopen(path, "wb");
    if (!file) {
        RETURN_ERR("Failed to open file for writing");
    }

    size_t bytes_written = fwrite(string->data, 1, string->size, file);
    if (bytes_written != string->size) {
        fclose(file);
        RETURN_ERR("Failed to write complete content");
    }

    if (fclose(file) != 0) {
        RETURN_ERR("Failed to close file");
    }

    return RESULT_OK;
}