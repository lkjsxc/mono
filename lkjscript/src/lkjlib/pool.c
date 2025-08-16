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
    for (uint64_t i = 0; i < POOL_OBJECT_MAXCOUNT; i++) {
        pool->object_freelist_data[i] = &pool->object_data[i];
        pool->object_data[i].data = NULL;
        pool->object_data[i].child = NULL;
        pool->object_data[i].next = NULL;
    }
    pool->object_freelist_count = POOL_OBJECT_MAXCOUNT;
    return RESULT_OK;
}

result_t pool_object_alloc(pool_t* pool, object_t** obj) {
    if (pool->object_freelist_count == 0) {
        RETURN_ERR("No available object in pool");
    }
    *obj = pool->object_freelist_data[--pool->object_freelist_count];
    (*obj)->data = NULL;
    (*obj)->child = NULL;
    (*obj)->next = NULL;
    return RESULT_OK;
}

result_t pool_object_free(pool_t* pool, object_t* obj) {
    pool->object_freelist_data[pool->object_freelist_count++] = obj;
    if (pool->object_freelist_count > POOL_OBJECT_MAXCOUNT) {
        RETURN_ERR("Freelist overflow for object");
    }
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
    if (*data != NULL) {
        RETURN_ERR("Data pointer is not NULL");
    }
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
    if (data == NULL) {
        RETURN_ERR("Cannot free null data");
    }
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
    data_t* new_data = NULL;
    if (pool_data_free(pool, *data) != RESULT_OK) {
        RETURN_ERR("Failed to free existing data");
    }
    if (pool_data_alloc(pool, &new_data, capacity) != RESULT_OK) {
        RETURN_ERR("Failed to allocate data with sufficient capacity");
    }
    *data = new_data;
    return RESULT_OK;
}
