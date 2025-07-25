#include "utils/pool.h"

static void pool_string_init(pool_t* pool, char** datalist, string_t* stringlist, string_t** freelist, uint64_t* freelist_count, uint64_t capacity, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        stringlist[i].data = datalist[i];
        stringlist[i].capacity = capacity;
        freelist[i] = &stringlist[i];
    }
    *freelist_count = count;
}

result_t pool_init(pool_t* pool) {
    pool_string_init(pool, pool->pool_string16_data, pool->pool_string16, pool->pool_string16_freelist_data, &pool->pool_string16_freelist_count, 16, POOL_STRING16_MAXCOUNT);
    pool_string_init(pool, pool->pool_string256_data, pool->pool_string256, pool->pool_string256_freelist_data, &pool->pool_string256_freelist_count, 256, POOL_STRING256_MAXCOUNT);
    pool_string_init(pool, pool->pool_string4096_data, pool->pool_string4096, pool->pool_string4096_freelist_data, &pool->pool_string4096_freelist_count, 4096, POOL_STRING4096_MAXCOUNT);
    pool_string_init(pool, pool->pool_string65536_data, pool->pool_string65536, pool->pool_string65536_freelist_data, &pool->pool_string65536_freelist_count, 65536, POOL_STRING65536_MAXCOUNT);
    pool_string_init(pool, pool->pool_string1048576_data, pool->pool_string1048576, pool->pool_string1048576_freelist_data, &pool->pool_string1048576_freelist_count, 1048576, POOL_STRING1048576_MAXCOUNT);
    return RESULT_OK;
}
