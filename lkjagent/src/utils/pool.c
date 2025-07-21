#include "pool.h"
#include "lkjstring.h"

result_t pool_init(pool_t* pool) {
    // Initialize all strings in the 256-byte pool
    for (uint64_t i = 0; i < COUNTOF(pool->pool_string256); i++) {
        if (string_init(&pool->pool_string256[i], pool->pool_string256_data[i], sizeof(pool->pool_string256_data[i])) != RESULT_OK) {
            return RESULT_ERR;
        }
        pool->pool_string256_freelist_data[i] = &pool->pool_string256[i];
    }

    // Initialize all strings in the 4096-byte pool
    for (uint64_t i = 0; i < COUNTOF(pool->pool_string4096); i++) {
        if (string_init(&pool->pool_string4096[i], pool->pool_string4096_data[i], sizeof(pool->pool_string4096_data[i])) != RESULT_OK) {
            return RESULT_ERR;
        }
        pool->pool_string4096_freelist_data[i] = &pool->pool_string4096[i];
    }

    // Set initial freelist counts (all strings are available)
    pool->pool_string256_freelist_count = COUNTOF(pool->pool_string256);
    pool->pool_string4096_freelist_count = COUNTOF(pool->pool_string4096);

    return RESULT_OK;
}

result_t pool_string256_alloc(pool_t* pool, string_t** string) {
    if (pool->pool_string256_freelist_count == 0) {
        return RESULT_ERR;  // Pool exhausted
    }

    // Get the last available string from the freelist
    pool->pool_string256_freelist_count--;
    *string = pool->pool_string256_freelist_data[pool->pool_string256_freelist_count];

    // Clear the string before returning it
    string_clear(*string);

    return RESULT_OK;
}

result_t pool_string256_free(pool_t* pool, string_t* string) {
    // Check if we have space in the freelist
    if (pool->pool_string256_freelist_count >= POOL_STRING256_MAXCOUNT) {
        return RESULT_ERR;  // Freelist full (should not happen in normal operation)
    }

    // Validate that this string belongs to our pool
    if (string < pool->pool_string256 ||
        string >= pool->pool_string256 + POOL_STRING256_MAXCOUNT) {
        return RESULT_ERR;  // String doesn't belong to this pool
    }

    // Add the string back to the freelist
    pool->pool_string256_freelist_data[pool->pool_string256_freelist_count] = string;
    pool->pool_string256_freelist_count++;

    return RESULT_OK;
}

result_t pool_string4096_alloc(pool_t* pool, string_t** string) {
    if (pool->pool_string4096_freelist_count == 0) {
        return RESULT_ERR;  // Pool exhausted
    }

    // Get the last available string from the freelist
    pool->pool_string4096_freelist_count--;
    *string = pool->pool_string4096_freelist_data[pool->pool_string4096_freelist_count];

    // Clear the string before returning it
    string_clear(*string);

    return RESULT_OK;
}

result_t pool_string4096_free(pool_t* pool, string_t* string) {
    // Check if we have space in the freelist
    if (pool->pool_string4096_freelist_count >= POOL_STRING4096_MAXCOUNT) {
        return RESULT_ERR;  // Freelist full (should not happen in normal operation)
    }

    // Validate that this string belongs to our pool
    if (string < pool->pool_string4096 ||
        string >= pool->pool_string4096 + POOL_STRING4096_MAXCOUNT) {
        return RESULT_ERR;  // String doesn't belong to this pool
    }

    // Add the string back to the freelist
    pool->pool_string4096_freelist_data[pool->pool_string4096_freelist_count] = string;
    pool->pool_string4096_freelist_count++;

    return RESULT_OK;
}
