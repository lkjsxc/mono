#include "lkjlib.h"

// Pool
static void pool_data_init(char* data, data_t* datalist, data_t** freelist, uint64_t* freelist_count, uint64_t capacity, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        datalist[i].data = &data[i * capacity];
        datalist[i].capacity = capacity;
        freelist[i] = &datalist[i];
    }
    *freelist_count = count;
}
result_t pool_init(pool_t* pool) {
    pool_data_init(pool->data16_data, pool->data16, pool->data16_freelist_data, &pool->data16_freelist_count, 16, POOL_data16_MAXCOUNT);
    pool_data_init(pool->data256_data, pool->data256, pool->data256_freelist_data, &pool->data256_freelist_count, 256, POOL_data256_MAXCOUNT);
    pool_data_init(pool->data4096_data, pool->data4096, pool->data4096_freelist_data, &pool->data4096_freelist_count, 4096, POOL_data4096_MAXCOUNT);
    pool_data_init(pool->data65536_data, pool->data65536, pool->data65536_freelist_data, &pool->data65536_freelist_count, 65536, POOL_data65536_MAXCOUNT);
    pool_data_init(pool->data1048576_data, pool->data1048576, pool->data1048576_freelist_data, &pool->data1048576_freelist_count, 1048576, POOL_data1048576_MAXCOUNT);
    return RESULT_OK;
}
result_t pool_data16_alloc(pool_t* pool, data_t** data) {
    if (pool->data16_freelist_count == 0) {
        RETURN_ERR("No available data16 in pool");
    }
    *data = pool->data16_freelist_data[--pool->data16_freelist_count];
    return RESULT_OK;
}
result_t pool_data256_alloc(pool_t* pool, data_t** data) {
    if (pool->data256_freelist_count == 0) {
        RETURN_ERR("No available data256 in pool");
    }
    *data = pool->data256_freelist_data[--pool->data256_freelist_count];
    return RESULT_OK;
}
result_t pool_data4096_alloc(pool_t* pool, data_t** data) {
    if (pool->data4096_freelist_count == 0) {
        RETURN_ERR("No available data4096 in pool");
    }
    *data = pool->data4096_freelist_data[--pool->data4096_freelist_count];
    return RESULT_OK;
}
result_t pool_data65536_alloc(pool_t* pool, data_t** data) {
    if (pool->data65536_freelist_count == 0) {
        RETURN_ERR("No available data65536 in pool");
    }
    *data = pool->data65536_freelist_data[--pool->data65536_freelist_count];
    return RESULT_OK;
}
result_t pool_data1048576_alloc(pool_t* pool, data_t** data) {
    if (pool->data1048576_freelist_count == 0) {
        RETURN_ERR("No available data1048576 in pool");
    }
    *data = pool->data1048576_freelist_data[--pool->data1048576_freelist_count];
    return RESULT_OK;
}
result_t pool_data_alloc(pool_t* pool, data_t** data, uint64_t capacity) {
    if (capacity <= 16) {
        return pool_data16_alloc(pool, data);
    } else if (capacity <= 256) {
        return pool_data256_alloc(pool, data);
    } else if (capacity <= 4096) {
        return pool_data4096_alloc(pool, data);
    } else if (capacity <= 65536) {
        return pool_data65536_alloc(pool, data);
    } else if (capacity <= 1048576) {
        return pool_data1048576_alloc(pool, data);
    } else {
        RETURN_ERR("Invalid data size requested");
    }
}
result_t pool_data_free(pool_t* pool, data_t* data) {
    if (data->capacity == 16) {
        pool->data16_freelist_data[pool->data16_freelist_count++] = data;
        if (pool->data16_freelist_count > POOL_data16_MAXCOUNT) {
            RETURN_ERR("Freelist overflow for data16");
        }
        return RESULT_OK;
    } else if (data->capacity == 256) {
        pool->data256_freelist_data[pool->data256_freelist_count++] = data;
        if (pool->data256_freelist_count > POOL_data256_MAXCOUNT) {
            RETURN_ERR("Freelist overflow for data256");
        }
        return RESULT_OK;
    } else if (data->capacity == 4096) {
        pool->data4096_freelist_data[pool->data4096_freelist_count++] = data;
        if (pool->data4096_freelist_count > POOL_data4096_MAXCOUNT) {
            RETURN_ERR("Freelist overflow for data4096");
        }
        return RESULT_OK;
    } else if (data->capacity == 65536) {
        pool->data65536_freelist_data[pool->data65536_freelist_count++] = data;
        if (pool->data65536_freelist_count > POOL_data65536_MAXCOUNT) {
            RETURN_ERR("Freelist overflow for data65536");
        }
        return RESULT_OK;
    } else if (data->capacity == 1048576) {
        pool->data1048576_freelist_data[pool->data1048576_freelist_count++] = data;
        if (pool->data1048576_freelist_count > POOL_data1048576_MAXCOUNT) {
            RETURN_ERR("Freelist overflow for data1048576");
        }
        return RESULT_OK;
    } else {
        RETURN_ERR("Invalid data capacity requested");
    }
}
result_t pool_data_realloc(pool_t* pool, data_t** data, uint64_t capacity) {
    if (pool_data_free(pool, *data) != RESULT_OK) {
        RETURN_ERR("Failed to free existing data");
    }
    if (pool_data_alloc(pool, data, capacity) != RESULT_OK) {
        RETURN_ERR("Failed to allocate data with sufficient capacity");
    }
    return RESULT_OK;
}

// Data
static const char* data_find(const char* str1, const char* str2, size_t size1, size_t size2) {
    if (size1 < size2) {
        return NULL;
    }
    for (size_t i = 0; i <= size1 - size2; i++) {
        if (str1[i] == *str2 && memcmp(str1 + i, str2, size2) == 0) {
            return str1 + i;
        }
    }
    return NULL;
}
result_t data_create(pool_t* pool, data_t** data) {
    if (pool_data16_alloc(pool, data) != RESULT_OK) {
        RETURN_ERR("Failed to allocate data with capacity 16");
    }
    (*data)->size = 0;
    return RESULT_OK;
}
result_t data_create_data(pool_t* pool, data_t** data1, const data_t* data2) {
    if (pool_data_alloc(pool, data1, data2->capacity) != RESULT_OK) {
        RETURN_ERR("Failed to allocate data with sufficient capacity");
    }
    (*data1)->size = data2->size;
    memcpy((*data1)->data, data2->data, data2->size);
    return RESULT_OK;
}
result_t data_create_str(pool_t* pool, data_t** data, const char* str) {
    size_t len = strlen(str);
    if (pool_data_alloc(pool, data, len) != RESULT_OK) {
        RETURN_ERR("Failed to allocate data with sufficient capacity");
    }
    (*data)->size = len;
    memcpy((*data)->data, str, len);
    return RESULT_OK;
}
result_t data_clean(pool_t* pool, data_t** data) {
    if (pool_data_realloc(pool, data, 16) != RESULT_OK) {
        RETURN_ERR("Failed to reallocate data to clean it");
    }
    (*data)->size = 0;
    return RESULT_OK;
}
result_t data_copy_data(pool_t* pool, data_t** data1, const data_t* data2) {
    if ((*data1)->capacity != data2->capacity) {
        if (pool_data_realloc(pool, data1, data2->capacity) != RESULT_OK) {
            RETURN_ERR("Failed to reallocate data with sufficient capacity");
        }
    }
    (*data1)->size = data2->size;
    memcpy((*data1)->data, data2->data, data2->size);
    return RESULT_OK;
}
result_t data_copy_str(pool_t* pool, data_t** data, const char* str) {
    size_t len = strlen(str);
    if (pool_data_realloc(pool, data, len) != RESULT_OK) {
        RETURN_ERR("Failed to reallocate data with sufficient capacity");
    }
    (*data)->size = len;
    memcpy((*data)->data, str, len);
    return RESULT_OK;
}
result_t data_append_data(pool_t* pool, data_t** data1, const data_t* data2) {
    if ((*data1)->size + data2->size > (*data1)->capacity) {
        data_t* data_old = *data1;
        data_t* data_new;
        if (pool_data_alloc(pool, &data_new, data_old->size + data2->size) != RESULT_OK) {
            RETURN_ERR("Failed to allocate data with sufficient capacity");
        }
        memcpy(data_new->data, data_old->data, data_old->size);
        memcpy(data_new->data + data_old->size, data2->data, data2->size);
        data_new->size = data_old->size + data2->size;
        *data1 = data_new;
        if (pool_data_free(pool, data_old) != RESULT_OK) {
            RETURN_ERR("Failed to free old data");
        }
    } else {
        memcpy((*data1)->data + (*data1)->size, data2->data, data2->size);
        (*data1)->size += data2->size;
    }
    return RESULT_OK;
}
result_t data_append_str(pool_t* pool, data_t** data, const char* str) {
    size_t str_len = strlen(str);
    if ((*data)->size + str_len > (*data)->capacity) {
        data_t* data_old = *data;
        data_t* data_new;
        if (pool_data_alloc(pool, &data_new, (*data)->size + str_len) != RESULT_OK) {
            RETURN_ERR("Failed to allocate data with sufficient capacity");
        }
        memcpy(data_new->data, data_old->data, data_old->size);
        memcpy(data_new->data + data_old->size, str, str_len);
        data_new->size = data_old->size + str_len;
        *data = data_new;
        if (pool_data_free(pool, data_old) != RESULT_OK) {
            RETURN_ERR("Failed to free old data");
        }
    } else {
        memcpy((*data)->data + (*data)->size, str, str_len);
        (*data)->size += str_len;
    }
    return RESULT_OK;
}
result_t data_append_char(pool_t* pool, data_t** data, char c) {
    if ((*data)->size + 1 >= (*data)->capacity) {
        data_t* data_old = *data;
        data_t* data_new;
        if (pool_data_alloc(pool, &data_new, (*data)->size + 1) != RESULT_OK) {
            RETURN_ERR("Failed to allocate data with sufficient capacity");
        }
        memcpy(data_new->data, data_old->data, data_old->size);
        data_new->data[data_old->size] = c; // Append the new character
        data_new->size = data_old->size + 1;
        *data = data_new;
        if (pool_data_free(pool, data_old) != RESULT_OK) {
            RETURN_ERR("Failed to free old data");
        }
    } else {
        (*data)->data[(*data)->size++] = c;
    }
    return RESULT_OK;
}
result_t data_escape(pool_t* pool, data_t** data) {
    if (pool_data_realloc(pool, data, 114514) != RESULT_OK) {
        RETURN_ERR("data escaping not implemented yet");
    }
    return RESULT_OK;
}
result_t data_unescape(pool_t* pool, data_t** data) {
    if (pool_data_realloc(pool, data, 114514) != RESULT_OK) {
        RETURN_ERR("data unescaping not implemented yet");
    }
    return RESULT_OK;
}
uint64_t data_equal_data(const data_t* data1, const data_t* data2) {
    if (data1->size != data2->size) {
        return 0;
    }
    return memcmp(data1->data, data2->data, data1->size) == 0;
}
uint64_t data_equal_str(const data_t* data, const char* str) {
    size_t len = strlen(str);
    if (data->size != len) {
        return 0;
    }
    return memcmp(data->data, str, len) == 0;
}
int64_t data_find_data(const data_t* data1, const data_t* data2, uint64_t index) {
    if (index >= data1->size || data2->size == 0) {
        return -1;
    }
    const char* pos = data_find(data1->data + index, data2->data, data1->size - index, data2->size);
    if (!pos) {
        return -1;
    }
    return pos - data1->data;
}
int64_t data_find_str(const data_t* data, const char* str, uint64_t index) {
    if (index >= data->size || !str || *str == '\0') {
        return -1;
    }
    const char* pos = data_find(data->data + index, str, data->size - index, strlen(str));
    if (!pos) {
        return -1;
    }
    return pos - data->data;
}
int64_t data_find_char(const data_t* data, char c, uint64_t index) {
    if (index >= data->size) {
        return -1;
    }
    const char* pos = data_find(data->data + index, &c, data->size - index, 1);
    if (!pos) {
        return -1;
    }
    return pos - data->data;
}
result_t data_destroy(pool_t* pool, data_t* data) {
    if (pool_data_free(pool, data) != RESULT_OK) {
        RETURN_ERR("Failed to free data");
    }
    return RESULT_OK;
}

// File
result_t file_read(pool_t* pool, data_t** data, const char* path) {
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
    if (pool_data_realloc(pool, data, file_size) != RESULT_OK) {
        fclose(file);
        RETURN_ERR("Failed to allocate data for file data");
    }
    size_t read_size = fread((*data)->data, 1, file_size, file);
    if (read_size != (uint64_t)file_size) {
        if (pool_data_free(pool, *data)) {
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
