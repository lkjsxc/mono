#include "lkjagent.h"

/**
 * Read the entire contents of a file into a token
 * @param path The file path to read from
 * @param content The token to store the file contents
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result)) result_t file_read(const char* path, token_t* content) {
    if (!path || !content) {
        return RESULT_ERR;
    }

    FILE* file = fopen(path, "rb");
    if (!file) {
        return RESULT_ERR;
    }

    // Get file size
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return RESULT_ERR;
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        return RESULT_ERR;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return RESULT_ERR;
    }

    // Check if we have enough capacity in the token
    if ((size_t)file_size >= content->capacity) {
        fclose(file);
        return RESULT_ERR;
    }

    // Read the file contents
    size_t bytes_read = fread(content->data, 1, (size_t)file_size, file);
    fclose(file);

    if (bytes_read != (size_t)file_size) {
        return RESULT_ERR;
    }

    // Null-terminate the content
    content->data[bytes_read] = '\0';
    content->size = bytes_read;

    return RESULT_OK;
}

/**
 * Write the contents of a token to a file
 * @param path The file path to write to
 * @param content The token containing the data to write
 * @return RESULT_OK on success, RESULT_ERR on failure
 */
__attribute__((warn_unused_result)) result_t file_write(const char* path, const token_t* content) {
    if (!path || !content || !content->data) {
        return RESULT_ERR;
    }

    // Validate the token before writing
    if (token_validate(content) != RESULT_OK) {
        return RESULT_ERR;
    }

    FILE* file = fopen(path, "wb");
    if (!file) {
        return RESULT_ERR;
    }

    size_t bytes_written = fwrite(content->data, 1, content->size, file);
    fclose(file);

    if (bytes_written != content->size) {
        return RESULT_ERR;
    }

    return RESULT_OK;
}