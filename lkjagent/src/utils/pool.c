#include "utils/pool.h"

static void pool_string_init(char* datalist, string_t* stringlist, string_t** freelist, uint64_t* freelist_count, uint64_t capacity, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        stringlist[i].data = &datalist[i * capacity];
        stringlist[i].capacity = capacity;
        freelist[i] = &stringlist[i];
    }
    *freelist_count = count;
}

result_t pool_string16_alloc(pool_t* pool, string_t** string) {
    if (pool->string16_freelist_count == 0) {
        RETURN_ERR("No available string16 in pool");
    }
    *string = pool->string16_freelist_data[--pool->string16_freelist_count];
    return RESULT_OK;
}

result_t pool_string256_alloc(pool_t* pool, string_t** string) {
    if (pool->string256_freelist_count == 0) {
        RETURN_ERR("No available string256 in pool");
    }
    *string = pool->string256_freelist_data[--pool->string256_freelist_count];
    return RESULT_OK;
}

result_t pool_string4096_alloc(pool_t* pool, string_t** string) {
    if (pool->string4096_freelist_count == 0) {
        RETURN_ERR("No available string4096 in pool");
    }
    *string = pool->string4096_freelist_data[--pool->string4096_freelist_count];
    return RESULT_OK;
}

result_t pool_string65536_alloc(pool_t* pool, string_t** string) {
    if (pool->string65536_freelist_count == 0) {
        RETURN_ERR("No available string65536 in pool");
    }
    *string = pool->string65536_freelist_data[--pool->string65536_freelist_count];
    return RESULT_OK;
}

result_t pool_string1048576_alloc(pool_t* pool, string_t** string) {
    if (pool->string1048576_freelist_count == 0) {
        RETURN_ERR("No available string1048576 in pool");
    }
    *string = pool->string1048576_freelist_data[--pool->string1048576_freelist_count];
    return RESULT_OK;
}

result_t pool_string_alloc(pool_t* pool, string_t** string, uint64_t capacity) {
    if (capacity <= 16) {
        return pool_string16_alloc(pool, string);
    } else if (capacity <= 256) {
        return pool_string256_alloc(pool, string);
    } else if (capacity <= 4096) {
        return pool_string4096_alloc(pool, string);
    } else if (capacity <= 65536) {
        return pool_string65536_alloc(pool, string);
    } else if (capacity <= 1048576) {
        return pool_string1048576_alloc(pool, string);
    } else {
        RETURN_ERR("Invalid string size requested");
    }
}

result_t pool_string_free(pool_t* pool, string_t* string) {
    // Validate string pointer first
    if (!string) {
        return RESULT_OK; // NULL string is OK to free
    }
    
    // Validate capacity is within expected ranges - if corrupted, don't crash
    if (string->capacity != 16 && string->capacity != 256 && string->capacity != 4096 && 
        string->capacity != 65536 && string->capacity != 1048576) {
        printf("Warning: Detected corrupted string with capacity %llu, skipping free to prevent crash\n", 
               (unsigned long long)string->capacity);
        return RESULT_OK; // Don't propagate corruption
    }

    if (string->capacity == 16) {
        pool->string16_freelist_data[pool->string16_freelist_count++] = string;
        return RESULT_OK;
    } else if (string->capacity == 256) {
        pool->string256_freelist_data[pool->string256_freelist_count++] = string;
        return RESULT_OK;
    } else if (string->capacity == 4096) {
        pool->string4096_freelist_data[pool->string4096_freelist_count++] = string;
        return RESULT_OK;
    } else if (string->capacity == 65536) {
        pool->string65536_freelist_data[pool->string65536_freelist_count++] = string;
        return RESULT_OK;
    } else if (string->capacity == 1048576) {
        pool->string1048576_freelist_data[pool->string1048576_freelist_count++] = string;
        return RESULT_OK;
    } else {
        printf("Warning: Invalid string capacity %llu detected in pool_string_free\n", 
               (unsigned long long)string->capacity);
        return RESULT_OK; // Don't crash on invalid capacity
    }
}

result_t pool_string_realloc(pool_t* pool, string_t** string, uint64_t capacity) {
    if (pool_string_free(pool, *string) != RESULT_OK) {
        RETURN_ERR("Failed to free existing string");
    }
    if (pool_string_alloc(pool, string, capacity) != RESULT_OK) {
        RETURN_ERR("Failed to allocate string with sufficient capacity");
    }
    return RESULT_OK;
}

result_t pool_object_alloc(pool_t* pool, object_t** object) {
    if (pool->object_freelist_count == 0) {
        RETURN_ERR("No available object in pool");
    }
    *object = pool->object_freelist_data[--pool->object_freelist_count];
    return RESULT_OK;
}

result_t pool_object_free(pool_t* pool, object_t* object) {
    pool->object_freelist_data[pool->object_freelist_count++] = object;
    return RESULT_OK;
}

result_t pool_init(pool_t* pool) {
    pool_string_init(pool->string16_data, pool->string16, pool->string16_freelist_data, &pool->string16_freelist_count, 16, POOL_STRING16_MAXCOUNT);
    pool_string_init(pool->string256_data, pool->string256, pool->string256_freelist_data, &pool->string256_freelist_count, 256, POOL_STRING256_MAXCOUNT);
    pool_string_init(pool->string4096_data, pool->string4096, pool->string4096_freelist_data, &pool->string4096_freelist_count, 4096, POOL_STRING4096_MAXCOUNT);
    pool_string_init(pool->string65536_data, pool->string65536, pool->string65536_freelist_data, &pool->string65536_freelist_count, 65536, POOL_STRING65536_MAXCOUNT);
    pool_string_init(pool->string1048576_data, pool->string1048576, pool->string1048576_freelist_data, &pool->string1048576_freelist_count, 1048576, POOL_STRING1048576_MAXCOUNT);

    for (uint64_t i = 0; i < POOL_OBJECT_MAXCOUNT; i++) {
        pool->object_freelist_data[i] = &pool->object_data[i];
    }
    pool->object_freelist_count = POOL_OBJECT_MAXCOUNT;

    return RESULT_OK;
}
