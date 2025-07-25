#include "utils/string.h"

result_t string_create(pool_t* pool, string_t** string) {
    if (pool_string16_alloc(pool, string) != RESULT_OK) {
        RETURN_ERR("Failed to allocate string with capacity 16");
    }
    (*string)->size = 0;
    (*string)->data[0] = '\0';
    return RESULT_OK;
}

result_t string_create_string(pool_t* pool, string_t** string1, const string_t* string2) {
    if (pool_string_alloc(pool, string1, string2->capacity) != RESULT_OK) {
        RETURN_ERR("Failed to allocate string with sufficient capacity");
    }
    (*string1)->size = string2->size;
    memcpy((*string1)->data, string2->data, string2->size + 1);
    return RESULT_OK;
}

result_t string_create_str(pool_t* pool, string_t** string, const char* str) {
    size_t len = strlen(str);
    if (pool_string_alloc(pool, string, len + 1) != RESULT_OK) {
        RETURN_ERR("Failed to allocate string with sufficient capacity");
    }
    (*string)->size = len;
    memcpy((*string)->data, str, len + 1);
    return RESULT_OK;
}

result_t string_copy_string(pool_t* pool, string_t** string1, const string_t* string2) {
    if (string2->size + 1 > (*string1)->capacity) {
        if (pool_string_realloc(pool, string1, string2->size + 1) != RESULT_OK) {
            RETURN_ERR("Failed to reallocate string with sufficient capacity");
        }
    }
    (*string1)->size = string2->size;
    memcpy((*string1)->data, string2->data, string2->size + 1);
    return RESULT_OK;
}

result_t string_copy_str(pool_t* pool, string_t** string, const char* str) {
    size_t len = strlen(str);
    if (len + 1 > (*string)->capacity) {
        if (pool_string_realloc(pool, string, len + 1) != RESULT_OK) {
            RETURN_ERR("Failed to reallocate string with sufficient capacity");
        }
    }
    (*string)->size = len;
    memcpy((*string)->data, str, len + 1);
    return RESULT_OK;
}

result_t string_append_string(pool_t* pool, string_t** string1, const string_t* string2) {
    if ((*string1)->size + string2->size + 1 > (*string1)->capacity) {
        if (pool_string_realloc(pool, string1, (*string1)->size + string2->size + 1) != RESULT_OK) {
            RETURN_ERR("Failed to reallocate string with sufficient capacity");
        }
    }
    memcpy((*string1)->data + (*string1)->size, string2->data, string2->size + 1);
    (*string1)->size += string2->size;
    return RESULT_OK;
}

result_t string_append_str(pool_t* pool, string_t** string, const char* str) {
    size_t len = strlen(str);
    if ((*string)->size + len + 1 > (*string)->capacity) {
        if (pool_string_realloc(pool, string, (*string)->size + len + 1) != RESULT_OK) {
            RETURN_ERR("Failed to reallocate string with sufficient capacity");
        }
    }
    memcpy((*string)->data + (*string)->size, str, len + 1);
    (*string)->size += len;
    return RESULT_OK;
}

result_t string_append_char(pool_t* pool, string_t** string, char c) {
    if ((*string)->size + 1 >= (*string)->capacity) {
        if (pool_string_realloc(pool, string, (*string)->size + 2) != RESULT_OK) {
            RETURN_ERR("Failed to reallocate string with sufficient capacity");
        }
    }
    (*string)->data[(*string)->size++] = c;
    (*string)->data[(*string)->size] = '\0';
    return RESULT_OK;
}

uint64_t string_equal_string(const string_t* string1, const string_t* string2) {
    if (string1->size != string2->size) {
        return 0;
    }
    return memcmp(string1->data, string2->data, string1->size) == 0;
}

uint64_t string_equal_str(const string_t* string, const char* str) {
    size_t len = strlen(str);
    if (string->size != len) {
        return 0;
    }
    return memcmp(string->data, str, len) == 0;
}

int64_t string_find_string(const string_t* string1, const string_t* string2, uint64_t index) {
    if (index >= string1->size || string2->size == 0) {
        return -1;
    }
    const char* pos = strstr(string1->data + index, string2->data);
    if (!pos) {
        return -1;
    }
    return pos - string1->data;
}

int64_t string_find_str(const string_t* string, const char* str, uint64_t index) {
    if (index >= string->size || !str || *str == '\0') {
        return -1;
    }
    const char* pos = strstr(string->data + index, str);
    if (!pos) {
        return -1;
    }
    return pos - string->data;
}

int64_t string_find_char(const string_t* string, char c, uint64_t index) {
    if (index >= string->size) {
        return -1;
    }
    const char* pos = strchr(string->data + index, c);
    if (!pos) {
        return -1;
    }
    return pos - string->data;
}

result_t string_destroy(pool_t* pool, string_t* string) {
    if (pool_string_free(pool, string) != RESULT_OK) {
        RETURN_ERR("Failed to free string");
    }
    return RESULT_OK;
}