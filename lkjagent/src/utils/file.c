#include "utils/file.h"

result_t file_read(pool_t* pool, const char* path, string_t** string) {
    FILE* file = fopen(path, "r");
    if (!file) {
        RETURN_ERR("Failed to open file for reading");
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        RETURN_ERR("Failed to seek to end of file");
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        RETURN_ERR("Failed to get file size");
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        RETURN_ERR("Failed to seek to start of file");
    }

    if (pool_string_realloc(pool, string, file_size) != RESULT_OK) {
        fclose(file);
        RETURN_ERR("Failed to allocate string for file string");
    }

    size_t read_size = fread((*string)->data, 1, file_size, file);
    if (read_size != (uint64_t)file_size) {
        if (pool_string_free(pool, *string)) {
            fclose(file);
            RETURN_ERR("Failed to free string after partial read");
        }
        fclose(file);
        RETURN_ERR("Failed to read entire file");
    }

    (*string)->size = file_size;

    fclose(file);
    return RESULT_OK;
}

result_t file_write(const char* path, const string_t* string) {
    FILE* file = fopen(path, "w");
    if (!file) {
        RETURN_ERR("Failed to open file for writing");
    }

    size_t written_size = fwrite(string->data, 1, string->size, file);
    if (written_size != string->size) {
        fclose(file);
        RETURN_ERR("Failed to write entire string to file");
    }

    if (fclose(file) != 0) {
        RETURN_ERR("Failed to close file after writing");
    }

    return RESULT_OK;
}
