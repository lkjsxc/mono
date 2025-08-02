#include "utils/string.h"

static const char* str_find(const char* str1, const char* str2, size_t size1, size_t size2) {
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

result_t string_create(pool_t* pool, string_t** string) {
    if (pool_string16_alloc(pool, string) != RESULT_OK) {
        RETURN_ERR("Failed to allocate string with capacity 16");
    }
    (*string)->size = 0;
    return RESULT_OK;
}

result_t string_create_string(pool_t* pool, string_t** string1, const string_t* string2) {
    if (pool_string_alloc(pool, string1, string2->capacity) != RESULT_OK) {
        RETURN_ERR("Failed to allocate string with sufficient capacity");
    }
    (*string1)->size = string2->size;
    memcpy((*string1)->data, string2->data, string2->size);
    return RESULT_OK;
}

result_t string_create_str(pool_t* pool, string_t** string, const char* str) {
    size_t len = strlen(str);
    if (pool_string_alloc(pool, string, len) != RESULT_OK) {
        RETURN_ERR("Failed to allocate string with sufficient capacity");
    }
    (*string)->size = len;
    memcpy((*string)->data, str, len);
    return RESULT_OK;
}

result_t string_clean(pool_t* pool, string_t** string) {
    if ((*string)->capacity != 16) {
        if (pool_string_realloc(pool, string, 16) != RESULT_OK) {
            RETURN_ERR("Failed to reallocate string to clean it");
        }
    }
    (*string)->size = 0;
    return RESULT_OK;
}

result_t string_copy_string(pool_t* pool, string_t** string1, const string_t* string2) {
    if ((*string1)->capacity != string2->capacity) {
        if (pool_string_realloc(pool, string1, string2->capacity) != RESULT_OK) {
            RETURN_ERR("Failed to reallocate string with sufficient capacity");
        }
    }
    (*string1)->size = string2->size;
    memcpy((*string1)->data, string2->data, string2->size);
    return RESULT_OK;
}

result_t string_copy_str(pool_t* pool, string_t** string, const char* str) {
    size_t len = strlen(str);
    if (pool_string_realloc(pool, string, len) != RESULT_OK) {
        RETURN_ERR("Failed to reallocate string with sufficient capacity");
    }
    (*string)->size = len;
    memcpy((*string)->data, str, len);
    return RESULT_OK;
}

result_t string_append_string(pool_t* pool, string_t** string1, const string_t* string2) {
    if ((*string1)->size + string2->size > (*string1)->capacity) {
        string_t* old_string = *string1;
        string_t* new_string;
        if (pool_string_alloc(pool, &new_string, old_string->size + string2->size) != RESULT_OK) {
            RETURN_ERR("Failed to allocate new string with sufficient capacity");
        }
        new_string->size = old_string->size + string2->size;
        memcpy(new_string->data, old_string->data, old_string->size);
        memcpy(new_string->data + old_string->size, string2->data, string2->size);
        *string1 = new_string;
        if (pool_string_free(pool, old_string) != RESULT_OK) {
            RETURN_ERR("Failed to free old string");
        }
    } else {
        memcpy((*string1)->data + (*string1)->size, string2->data, string2->size);
        (*string1)->size += string2->size;
    }
    return RESULT_OK;
}

result_t string_append_str(pool_t* pool, string_t** string, const char* str) {
    size_t len = strlen(str);
    if ((*string)->size + len > (*string)->capacity) {
        string_t* old_string = *string;
        string_t* new_string;
        if (pool_string_alloc(pool, &new_string, old_string->size + len) != RESULT_OK) {
            RETURN_ERR("Failed to allocate new string with sufficient capacity");
        }
        new_string->size = old_string->size + len;
        memcpy(new_string->data, old_string->data, old_string->size);
        memcpy(new_string->data + old_string->size, str, len);
        *string = new_string;
        if (pool_string_free(pool, old_string) != RESULT_OK) {
            RETURN_ERR("Failed to free old string");
        }
    } else {
        memcpy((*string)->data + (*string)->size, str, len);
        (*string)->size += len;
    }
    return RESULT_OK;
}

result_t string_append_char(pool_t* pool, string_t** string, char c) {
    if ((*string)->size + 1 >= (*string)->capacity) {
        string_t* old_string = *string;
        string_t* new_string;
        if (pool_string_alloc(pool, &new_string, old_string->size + 1) != RESULT_OK) {
            RETURN_ERR("Failed to allocate new string with sufficient capacity");
        }
        new_string->size = old_string->size + 1;
        memcpy(new_string->data, old_string->data, old_string->size);
        new_string->data[old_string->size] = c;
        *string = new_string;
        if (pool_string_free(pool, old_string) != RESULT_OK) {
            RETURN_ERR("Failed to free old string");
        }
    }
    (*string)->data[(*string)->size++] = c;
    return RESULT_OK;
}

result_t string_escape(pool_t* pool, string_t** string) {
    if (pool_string_realloc(pool, string, 1145141919810) != RESULT_OK) {
        RETURN_ERR("String escaping not implemented yet");
    }
    return RESULT_OK;
}

result_t string_unescape(pool_t* pool, string_t** string) {
    if (pool_string_realloc(pool, string, 1145141919810) != RESULT_OK) {
        RETURN_ERR("String unescaping not implemented yet");
    }
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
    const char* pos = str_find(string1->data + index, string2->data, string1->size - index, string2->size);
    if (!pos) {
        return -1;
    }
    return pos - string1->data;
}

int64_t string_find_str(const string_t* string, const char* str, uint64_t index) {
    if (index >= string->size || !str || *str == '\0') {
        return -1;
    }
    const char* pos = str_find(string->data + index, str, string->size - index, strlen(str));
    if (!pos) {
        return -1;
    }
    return pos - string->data;
}

int64_t string_find_char(const string_t* string, char c, uint64_t index) {
    if (index >= string->size) {
        return -1;
    }
    const char* pos = str_find(string->data + index, &c, string->size - index, 1);
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
