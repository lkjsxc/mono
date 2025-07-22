#include "utils/pool.h"

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

    // Initialize all strings in the 65536-byte pool
    for (uint64_t i = 0; i < COUNTOF(pool->pool_string65536); i++) {
        if (string_init(&pool->pool_string65536[i], pool->pool_string65536_data[i], sizeof(pool->pool_string65536_data[i])) != RESULT_OK) {
            return RESULT_ERR;
        }
        pool->pool_string65536_freelist_data[i] = &pool->pool_string65536[i];
    }

    // Initialize all strings in the 1048576-byte pool
    for (uint64_t i = 0; i < COUNTOF(pool->pool_string1048576); i++) {
        if (string_init(&pool->pool_string1048576[i], pool->pool_string1048576_data[i], sizeof(pool->pool_string1048576_data[i])) != RESULT_OK) {
            return RESULT_ERR;
        }
        pool->pool_string1048576_freelist_data[i] = &pool->pool_string1048576[i];
    }

    // Set initial freelist counts (all strings are available)
    pool->pool_string256_freelist_count = COUNTOF(pool->pool_string256);
    pool->pool_string4096_freelist_count = COUNTOF(pool->pool_string4096);
    pool->pool_string65536_freelist_count = COUNTOF(pool->pool_string65536);
    pool->pool_string1048576_freelist_count = COUNTOF(pool->pool_string1048576);

    // Initialize JSON value pool
    for (uint64_t i = 0; i < COUNTOF(pool->pool_json_value); i++) {
        pool->pool_json_value_freelist_data[i] = &pool->pool_json_value[i];
    }
    pool->pool_json_value_freelist_count = COUNTOF(pool->pool_json_value);

    // Initialize JSON object pool
    for (uint64_t i = 0; i < COUNTOF(pool->pool_json_object); i++) {
        pool->pool_json_object_freelist_data[i] = &pool->pool_json_object[i];
    }
    pool->pool_json_object_freelist_count = COUNTOF(pool->pool_json_object);

    // Initialize JSON array pool
    for (uint64_t i = 0; i < COUNTOF(pool->pool_json_array); i++) {
        pool->pool_json_array_freelist_data[i] = &pool->pool_json_array[i];
    }
    pool->pool_json_array_freelist_count = COUNTOF(pool->pool_json_array);

    // Initialize JSON object element pool
    for (uint64_t i = 0; i < COUNTOF(pool->pool_json_object_element); i++) {
        pool->pool_json_object_element_freelist_data[i] = &pool->pool_json_object_element[i];
    }
    pool->pool_json_object_element_freelist_count = COUNTOF(pool->pool_json_object_element);

    // Initialize JSON array element pool
    for (uint64_t i = 0; i < COUNTOF(pool->pool_json_array_element); i++) {
        pool->pool_json_array_element_freelist_data[i] = &pool->pool_json_array_element[i];
    }
    pool->pool_json_array_element_freelist_count = COUNTOF(pool->pool_json_array_element);

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

result_t pool_string65536_alloc(pool_t* pool, string_t** string) {
    if (pool->pool_string65536_freelist_count == 0) {
        return RESULT_ERR;  // Pool exhausted
    }

    // Get the last available string from the freelist
    pool->pool_string65536_freelist_count--;
    *string = pool->pool_string65536_freelist_data[pool->pool_string65536_freelist_count];

    // Clear the string before returning it
    string_clear(*string);

    return RESULT_OK;
}

result_t pool_string65536_free(pool_t* pool, string_t* string) {
    // Check if we have space in the freelist
    if (pool->pool_string65536_freelist_count >= POOL_STRING65536_MAXCOUNT) {
        return RESULT_ERR;  // Freelist full (should not happen in normal operation)
    }

    // Validate that this string belongs to our pool
    if (string < pool->pool_string65536 ||
        string >= pool->pool_string65536 + POOL_STRING65536_MAXCOUNT) {
        return RESULT_ERR;  // String doesn't belong to this pool
    }

    // Add the string back to the freelist
    pool->pool_string65536_freelist_data[pool->pool_string65536_freelist_count] = string;
    pool->pool_string65536_freelist_count++;

    return RESULT_OK;
}

result_t pool_string1048576_alloc(pool_t* pool, string_t** string) {
    if (pool->pool_string1048576_freelist_count == 0) {
        return RESULT_ERR;  // Pool exhausted
    }

    // Get the last available string from the freelist
    pool->pool_string1048576_freelist_count--;
    *string = pool->pool_string1048576_freelist_data[pool->pool_string1048576_freelist_count];

    // Clear the string before returning it
    string_clear(*string);

    return RESULT_OK;
}

result_t pool_string1048576_free(pool_t* pool, string_t* string) {
    // Check if we have space in the freelist
    if (pool->pool_string1048576_freelist_count >= POOL_STRING1048576_MAXCOUNT) {
        return RESULT_ERR;  // Freelist full (should not happen in normal operation)
    }

    // Validate that this string belongs to our pool
    if (string < pool->pool_string1048576 ||
        string >= pool->pool_string1048576 + POOL_STRING1048576_MAXCOUNT) {
        return RESULT_ERR;  // String doesn't belong to this pool
    }

    // Add the string back to the freelist
    pool->pool_string1048576_freelist_data[pool->pool_string1048576_freelist_count] = string;
    pool->pool_string1048576_freelist_count++;

    return RESULT_OK;
}

result_t pool_json_value_alloc(pool_t* pool, json_value_t** value) {
    if (pool->pool_json_value_freelist_count == 0) {
        return RESULT_ERR;  // Pool exhausted
    }

    pool->pool_json_value_freelist_count--;
    *value = pool->pool_json_value_freelist_data[pool->pool_json_value_freelist_count];
    
    // Clear the value
    memset(*value, 0, sizeof(json_value_t));
    
    return RESULT_OK;
}

result_t pool_json_value_free(pool_t* pool, json_value_t* value) {
    if (pool->pool_json_value_freelist_count >= POOL_JSON_VALUE_MAXCOUNT) {
        return RESULT_ERR;
    }

    if (value < pool->pool_json_value ||
        value >= pool->pool_json_value + POOL_JSON_VALUE_MAXCOUNT) {
        return RESULT_ERR;
    }

    pool->pool_json_value_freelist_data[pool->pool_json_value_freelist_count] = value;
    pool->pool_json_value_freelist_count++;

    return RESULT_OK;
}

result_t pool_json_object_alloc(pool_t* pool, json_object_t** object) {
    if (pool->pool_json_object_freelist_count == 0) {
        return RESULT_ERR;
    }

    pool->pool_json_object_freelist_count--;
    *object = pool->pool_json_object_freelist_data[pool->pool_json_object_freelist_count];
    
    // Clear the object
    (*object)->head = NULL;
    (*object)->length = 0;
    
    return RESULT_OK;
}

result_t pool_json_object_free(pool_t* pool, json_object_t* object) {
    if (pool->pool_json_object_freelist_count >= POOL_JSON_OBJECT_MAXCOUNT) {
        return RESULT_ERR;
    }

    if (object < pool->pool_json_object ||
        object >= pool->pool_json_object + POOL_JSON_OBJECT_MAXCOUNT) {
        return RESULT_ERR;
    }

    pool->pool_json_object_freelist_data[pool->pool_json_object_freelist_count] = object;
    pool->pool_json_object_freelist_count++;

    return RESULT_OK;
}

result_t pool_json_array_alloc(pool_t* pool, json_array_t** array) {
    if (pool->pool_json_array_freelist_count == 0) {
        return RESULT_ERR;
    }

    pool->pool_json_array_freelist_count--;
    *array = pool->pool_json_array_freelist_data[pool->pool_json_array_freelist_count];
    
    // Clear the array
    (*array)->head = NULL;
    (*array)->length = 0;
    
    return RESULT_OK;
}

result_t pool_json_array_free(pool_t* pool, json_array_t* array) {
    if (pool->pool_json_array_freelist_count >= POOL_JSON_ARRAY_MAXCOUNT) {
        return RESULT_ERR;
    }

    if (array < pool->pool_json_array ||
        array >= pool->pool_json_array + POOL_JSON_ARRAY_MAXCOUNT) {
        return RESULT_ERR;
    }

    pool->pool_json_array_freelist_data[pool->pool_json_array_freelist_count] = array;
    pool->pool_json_array_freelist_count++;

    return RESULT_OK;
}

result_t pool_json_object_element_alloc(pool_t* pool, json_object_element_t** element) {
    if (pool->pool_json_object_element_freelist_count == 0) {
        return RESULT_ERR;
    }

    pool->pool_json_object_element_freelist_count--;
    *element = pool->pool_json_object_element_freelist_data[pool->pool_json_object_element_freelist_count];
    
    // Clear the element
    (*element)->key = NULL;
    (*element)->value = NULL;
    (*element)->next = NULL;
    
    return RESULT_OK;
}

result_t pool_json_object_element_free(pool_t* pool, json_object_element_t* element) {
    if (pool->pool_json_object_element_freelist_count >= POOL_JSON_OBJECT_ELEMENT_MAXCOUNT) {
        return RESULT_ERR;
    }

    if (element < pool->pool_json_object_element ||
        element >= pool->pool_json_object_element + POOL_JSON_OBJECT_ELEMENT_MAXCOUNT) {
        return RESULT_ERR;
    }

    pool->pool_json_object_element_freelist_data[pool->pool_json_object_element_freelist_count] = element;
    pool->pool_json_object_element_freelist_count++;

    return RESULT_OK;
}

result_t pool_json_array_element_alloc(pool_t* pool, json_array_element_t** element) {
    if (pool->pool_json_array_element_freelist_count == 0) {
        return RESULT_ERR;
    }

    pool->pool_json_array_element_freelist_count--;
    *element = pool->pool_json_array_element_freelist_data[pool->pool_json_array_element_freelist_count];
    
    // Clear the element
    (*element)->value = NULL;
    (*element)->next = NULL;
    
    return RESULT_OK;
}

result_t pool_json_array_element_free(pool_t* pool, json_array_element_t* element) {
    if (pool->pool_json_array_element_freelist_count >= POOL_JSON_ARRAY_ELEMENT_MAXCOUNT) {
        return RESULT_ERR;
    }

    if (element < pool->pool_json_array_element ||
        element >= pool->pool_json_array_element + POOL_JSON_ARRAY_ELEMENT_MAXCOUNT) {
        return RESULT_ERR;
    }

    pool->pool_json_array_element_freelist_data[pool->pool_json_array_element_freelist_count] = element;
    pool->pool_json_array_element_freelist_count++;

    return RESULT_OK;
}

result_t pool_string_alloc(pool_t* pool, uint64_t size, string_t** string) {
    // Select the smallest pool that can accommodate the requested size
    if (size <= 256) {
        return pool_string256_alloc(pool, string);
    } else if (size <= 4096) {
        return pool_string4096_alloc(pool, string);
    } else if (size <= 65536) {
        return pool_string65536_alloc(pool, string);
    } else if (size <= 1048576) {
        return pool_string1048576_alloc(pool, string);
    } else {
        RETURN_ERR("Size too large for any pool");
    }
}

result_t pool_string_free(pool_t* pool, string_t* string) {
    // Determine which pool this string belongs to based on its capacity
    if (string->capacity == 256) {
        return pool_string256_free(pool, string);
    } else if (string->capacity == 4096) {
        return pool_string4096_free(pool, string);
    } else if (string->capacity == 65536) {
        return pool_string65536_free(pool, string);
    } else if (string->capacity == 1048576) {
        return pool_string1048576_free(pool, string);
    } else {
        RETURN_ERR("String does not belong to any known pool");
    }
}
