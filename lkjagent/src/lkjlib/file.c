#include "lkjlib.h"

// File
result_t file_read(pool_t* pool, const char* path, data_t** data) {
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
    if (pool_data_alloc(pool, data, file_size) != RESULT_OK) {
        fclose(file);
        RETURN_ERR("Failed to allocate data for file data");
    }
    size_t read_size = fread((*data)->data, 1, file_size, file);
    if (read_size != (uint64_t)file_size) {
        if (pool_data_free(pool, *data) != RESULT_OK) {
            fclose(file);
            RETURN_ERR("Failed to free data after partial read");
        }
        fclose(file);
        RETURN_ERR("Failed to read entire file");
    }
    (*data)->size = file_size;
    fclose(file);
    return RESULT_OK;
}

result_t file_write(const char* path, const data_t* data) {
    FILE* file = fopen(path, "w");
    if (!file) {
        RETURN_ERR("Failed to open file for writing");
    }
    size_t written_size = fwrite(data->data, 1, data->size, file);
    if (written_size != data->size) {
        fclose(file);
        RETURN_ERR("Failed to write entire data to file");
    }
    if (fclose(file) != 0) {
        RETURN_ERR("Failed to close file after writing");
    }
    return RESULT_OK;
}
