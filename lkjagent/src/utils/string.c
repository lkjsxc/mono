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
        if (pool_string_alloc(pool, string1, string2->size + 1) != RESULT_OK) {
            RETURN_ERR("Failed to allocate string with sufficient capacity");
        }
    }
    (*string1)->size = string2->size;
    memcpy((*string1)->data, string2->data, string2->size + 1);
    return RESULT_OK;
}

result_t string_copy_str(pool_t* pool, string_t** string, const char* str) {
    size_t len = strlen(str);
    if (len + 1 > (*string)->capacity) {
        if (pool_string_alloc(pool, string, len + 1) != RESULT_OK) {
            RETURN_ERR("Failed to allocate string with sufficient capacity");
        }
    }
    (*string)->size = len;
    memcpy((*string)->data, str, len + 1);
    return RESULT_OK;
}