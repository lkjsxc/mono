#include "utils/pool.h"

static void pool_string_init(char* datalist, string_t* stringlist, string_t** freelist, uint64_t* freelist_count, uint64_t capacity, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        stringlist[i].data = &datalist[i * capacity];
        stringlist[i].capacity = capacity;
        freelist[i] = &stringlist[i];
    }
    *freelist_count = count;
}

result_t pool_string_alloc(pool_t* pool, string_t** string, uint64_t capacity) {
    if (capacity <= 16) {
        if (pool->string16_freelist_count == 0) {
            RETURN_ERR("No available string16 in pool");
        }
        *string = pool->string16_freelist_data[--pool->string16_freelist_count];
        (*string)->size = 0;
        (*string)->data[0] = '\0';
        return RESULT_OK;
    } else if (capacity <= 256) {
        if (pool->string256_freelist_count == 0) {
            RETURN_ERR("No available string256 in pool");
        }
        *string = pool->string256_freelist_data[--pool->string256_freelist_count];
        (*string)->size = 0;
        (*string)->data[0] = '\0';
        return RESULT_OK;
    } else if (capacity <= 4096) {
        if (pool->string4096_freelist_count == 0) {
            RETURN_ERR("No available string4096 in pool");
        }
        *string = pool->string4096_freelist_data[--pool->string4096_freelist_count];
        (*string)->size = 0;
        (*string)->data[0] = '\0';
        return RESULT_OK;
    } else if (capacity <= 65536) {
        if (pool->string65536_freelist_count == 0) {
            RETURN_ERR("No available string65536 in pool");
        }
        *string = pool->string65536_freelist_data[--pool->string65536_freelist_count];
        (*string)->size = 0;
        (*string)->data[0] = '\0';
        return RESULT_OK;
    } else if (capacity <= 1048576) {
        if (pool->string1048576_freelist_count == 0) {
            RETURN_ERR("No available string1048576 in pool");
        }
        *string = pool->string1048576_freelist_data[--pool->string1048576_freelist_count];
        (*string)->size = 0;
        (*string)->data[0] = '\0';
        return RESULT_OK;
    } else {
        RETURN_ERR("Invalid string capacity requested");
    }
}

result_t pool_string_free(pool_t* pool, string_t* string) {
    if (string->capacity == 16) {
        if (pool->string16_freelist_count >= POOL_STRING16_MAXCOUNT) {
            RETURN_ERR("String16 freelist is full");
        }
        pool->string16_freelist_data[pool->string16_freelist_count++] = string;
        return RESULT_OK;
    } else if (string->capacity == 256) {
        if (pool->string256_freelist_count >= POOL_STRING256_MAXCOUNT) {
            RETURN_ERR("String256 freelist is full");
        }
        pool->string256_freelist_data[pool->string256_freelist_count++] = string;
        return RESULT_OK;
    } else if (string->capacity == 4096) {
        if (pool->string4096_freelist_count >= POOL_STRING4096_MAXCOUNT) {
            RETURN_ERR("String4096 freelist is full");
        }
        pool->string4096_freelist_data[pool->string4096_freelist_count++] = string;
        return RESULT_OK;
    } else if (string->capacity == 65536) {
        if (pool->string65536_freelist_count >= POOL_STRING65536_MAXCOUNT) {
            RETURN_ERR("String65536 freelist is full");
        }
        pool->string65536_freelist_data[pool->string65536_freelist_count++] = string;
        return RESULT_OK;
    } else if (string->capacity == 1048576) {
        if (pool->string1048576_freelist_count >= POOL_STRING1048576_MAXCOUNT) {
            RETURN_ERR("String1048576 freelist is full");
        }
        pool->string1048576_freelist_data[pool->string1048576_freelist_count++] = string;
        return RESULT_OK;
    } else {
        RETURN_ERR("Invalid string capacity requested");
    }
}

result_t pool_init(pool_t* pool) {
    pool_string_init(pool->string16_data, pool->string16, pool->string16_freelist_data, &pool->string16_freelist_count, 16, POOL_STRING16_MAXCOUNT);
    pool_string_init(pool->string256_data, pool->string256, pool->string256_freelist_data, &pool->string256_freelist_count, 256, POOL_STRING256_MAXCOUNT);
    pool_string_init(pool->string4096_data, pool->string4096, pool->string4096_freelist_data, &pool->string4096_freelist_count, 4096, POOL_STRING4096_MAXCOUNT);
    pool_string_init(pool->string65536_data, pool->string65536, pool->string65536_freelist_data, &pool->string65536_freelist_count, 65536, POOL_STRING65536_MAXCOUNT);
    pool_string_init(pool->string1048576_data, pool->string1048576, pool->string1048576_freelist_data, &pool->string1048576_freelist_count, 1048576, POOL_STRING1048576_MAXCOUNT);

    for (uint64_t i = 0; i < POOL_JSON_VALUE_MAXCOUNT; i++) {
        pool->json_value_freelist_data[i] = &pool->json_value[i];
    }
    pool->json_value_freelist_count = POOL_JSON_VALUE_MAXCOUNT;

    for (uint64_t i = 0; i < POOL_JSON_OBJECT_MAXCOUNT; i++) {
        pool->json_object_freelist_data[i] = &pool->json_object[i];
    }
    pool->json_object_freelist_count = POOL_JSON_OBJECT_MAXCOUNT;

    for (uint64_t i = 0; i < POOL_JSON_ARRAY_MAXCOUNT; i++) {
        pool->json_array_freelist_data[i] = &pool->json_array[i];
    }
    pool->json_array_freelist_count = POOL_JSON_ARRAY_MAXCOUNT;

    for (uint64_t i = 0; i < POOL_JSON_OBJECT_ELEMENT_MAXCOUNT; i++) {
        pool->json_object_element_freelist_data[i] = &pool->json_object_element[i];
    }
    pool->json_object_element_freelist_count = POOL_JSON_OBJECT_ELEMENT_MAXCOUNT;

    for (uint64_t i = 0; i < POOL_JSON_ARRAY_ELEMENT_MAXCOUNT; i++) {
        pool->json_array_element_freelist_data[i] = &pool->json_array_element[i];
    }
    pool->json_array_element_freelist_count = POOL_JSON_ARRAY_ELEMENT_MAXCOUNT;
    return RESULT_OK;
}
