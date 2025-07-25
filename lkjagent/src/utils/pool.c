#include "utils/pool.h"

static void pool_string_init(char* datalist, string_t* stringlist, string_t** freelist, uint64_t* freelist_count, uint64_t capacity, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        stringlist[i].data = &datalist[i * capacity];
        stringlist[i].capacity = capacity;
        freelist[i] = &stringlist[i];
    }
    *freelist_count = count;
}

result_t pool_init(pool_t* pool) {
    // Initialize string pools
    pool_string_init(pool->string16_data, pool->string16, pool->string16_freelist_data, &pool->string16_freelist_count, 16, POOL_STRING16_MAXCOUNT);
    pool_string_init(pool->string256_data, pool->string256, pool->string256_freelist_data, &pool->string256_freelist_count, 256, POOL_STRING256_MAXCOUNT);
    pool_string_init(pool->string4096_data, pool->string4096, pool->string4096_freelist_data, &pool->string4096_freelist_count, 4096, POOL_STRING4096_MAXCOUNT);
    pool_string_init(pool->string65536_data, pool->string65536, pool->string65536_freelist_data, &pool->string65536_freelist_count, 65536, POOL_STRING65536_MAXCOUNT);
    pool_string_init(pool->string1048576_data, pool->string1048576, pool->string1048576_freelist_data, &pool->string1048576_freelist_count, 1048576, POOL_STRING1048576_MAXCOUNT);

    // Initialize freelists for json types
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

// String pool alloc/free
static string_t* pool_string_alloc(string_t** freelist, uint64_t* freelist_count) {
    if (*freelist_count == 0)
        return NULL;
    string_t* s = freelist[--(*freelist_count)];
    s->size = 0;
    return s;
}

static void pool_string_free(string_t** freelist, uint64_t* freelist_count, string_t* s) {
    freelist[(*freelist_count)++] = s;
}

string_t* pool_alloc_string16(pool_t* pool) {
    return pool_string_alloc(pool->string16_freelist_data, &pool->string16_freelist_count);
}

void pool_free_string16(pool_t* pool, string_t* s) {
    pool_string_free(pool->string16_freelist_data, &pool->string16_freelist_count, s);
}

string_t* pool_alloc_string256(pool_t* pool) {
    return pool_string_alloc(pool->string256_freelist_data, &pool->string256_freelist_count);
}

void pool_free_string256(pool_t* pool, string_t* s) {
    pool_string_free(pool->string256_freelist_data, &pool->string256_freelist_count, s);
}

string_t* pool_alloc_string4096(pool_t* pool) {
    return pool_string_alloc(pool->string4096_freelist_data, &pool->string4096_freelist_count);
}

void pool_free_string4096(pool_t* pool, string_t* s) {
    pool_string_free(pool->string4096_freelist_data, &pool->string4096_freelist_count, s);
}

string_t* pool_alloc_string65536(pool_t* pool) {
    return pool_string_alloc(pool->string65536_freelist_data, &pool->string65536_freelist_count);
}

void pool_free_string65536(pool_t* pool, string_t* s) {
    pool_string_free(pool->string65536_freelist_data, &pool->string65536_freelist_count, s);
}

string_t* pool_alloc_string1048576(pool_t* pool) {
    return pool_string_alloc(pool->string1048576_freelist_data, &pool->string1048576_freelist_count);
}

void pool_free_string1048576(pool_t* pool, string_t* s) {
    pool_string_free(pool->string1048576_freelist_data, &pool->string1048576_freelist_count, s);
}

// JSON value pool alloc/free
json_value_t* pool_alloc_json_value(pool_t* pool) {
    if (pool->json_value_freelist_count == 0)
        return NULL;
    return pool->json_value_freelist_data[--pool->json_value_freelist_count];
}

void pool_free_json_value(pool_t* pool, json_value_t* v) {
    pool->json_value_freelist_data[pool->json_value_freelist_count++] = v;
}

json_object_t* pool_alloc_json_object(pool_t* pool) {
    if (pool->json_object_freelist_count == 0)
        return NULL;
    return pool->json_object_freelist_data[--pool->json_object_freelist_count];
}

void pool_free_json_object(pool_t* pool, json_object_t* o) {
    pool->json_object_freelist_data[pool->json_object_freelist_count++] = o;
}

json_array_t* pool_alloc_json_array(pool_t* pool) {
    if (pool->json_array_freelist_count == 0)
        return NULL;
    return pool->json_array_freelist_data[--pool->json_array_freelist_count];
}

void pool_free_json_array(pool_t* pool, json_array_t* a) {
    pool->json_array_freelist_data[pool->json_array_freelist_count++] = a;
}

json_object_element_t* pool_alloc_json_object_element(pool_t* pool) {
    if (pool->json_object_element_freelist_count == 0)
        return NULL;
    return pool->json_object_element_freelist_data[--pool->json_object_element_freelist_count];
}

void pool_free_json_object_element(pool_t* pool, json_object_element_t* e) {
    pool->json_object_element_freelist_data[pool->json_object_element_freelist_count++] = e;
}

json_array_element_t* pool_alloc_json_array_element(pool_t* pool) {
    if (pool->json_array_element_freelist_count == 0)
        return NULL;
    return pool->json_array_element_freelist_data[--pool->json_array_element_freelist_count];
}

void pool_free_json_array_element(pool_t* pool, json_array_element_t* e) {
    pool->json_array_element_freelist_data[pool->json_array_element_freelist_count++] = e;
}
