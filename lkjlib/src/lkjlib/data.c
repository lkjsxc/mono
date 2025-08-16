#include "lkjlib.h"

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
    if (pool_data_alloc(pool, data, 16) != RESULT_OK) {
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
        data_t* data_new = NULL;
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
        data_t* data_new = NULL;
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
        data_t* data_new = NULL;
        if (pool_data_alloc(pool, &data_new, (*data)->size + 1) != RESULT_OK) {
            RETURN_ERR("Failed to allocate data with sufficient capacity");
        }
        memcpy(data_new->data, data_old->data, data_old->size);
        data_new->data[data_old->size] = c;
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
