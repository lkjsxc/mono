#include "utils/string.h"

result_t string_create(pool_t* pool, string_t** string) {
    if (pool_string16_alloc(pool, string) != RESULT_OK) {
        RETURN_ERR("Failed to allocate string with capacity 16");
    }
    (*string)->size = 0;
    (*string)->data[0] = '\0';
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