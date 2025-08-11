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
    // Initialize data pools
    pool_data_init(pool->data16_data, pool->data16, pool->data16_freelist_data, &pool->data16_freelist_count, 16, POOL_data16_MAXCOUNT);
    pool_data_init(pool->data256_data, pool->data256, pool->data256_freelist_data, &pool->data256_freelist_count, 256, POOL_data256_MAXCOUNT);
    pool_data_init(pool->data4096_data, pool->data4096, pool->data4096_freelist_data, &pool->data4096_freelist_count, 4096, POOL_data4096_MAXCOUNT);
    pool_data_init(pool->data65536_data, pool->data65536, pool->data65536_freelist_data, &pool->data65536_freelist_count, 65536, POOL_data65536_MAXCOUNT);
    pool_data_init(pool->data1048576_data, pool->data1048576, pool->data1048576_freelist_data, &pool->data1048576_freelist_count, 1048576, POOL_data1048576_MAXCOUNT);
    // Initialize object freelist
    for (uint64_t i = 0; i < POOL_OBJECT_MAXCOUNT; i++) {
        pool->object_freelist_data[i] = &pool->object_data[i];
        pool->object_data[i].data = NULL;
        pool->object_data[i].child = NULL;
        pool->object_data[i].next = NULL;
    }
    pool->object_freelist_count = POOL_OBJECT_MAXCOUNT;
    return RESULT_OK;
}
result_t pool_data16_alloc(pool_t* pool, data_t** data) {
    if (pool->data16_freelist_count == 0) {
        RETURN_ERR("No available data16 in pool");
    }
    *data = pool->data16_freelist_data[--pool->data16_freelist_count];
    return RESULT_OK;
}
// Object pool helpers
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
    if (!data) {
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
    if (pool_data_free(pool, *data) != RESULT_OK) {
        RETURN_ERR("Failed to free existing data");
    }
    if (pool_data_alloc(pool, data, capacity) != RESULT_OK) {
        RETURN_ERR("Failed to allocate data with sufficient capacity");
    }
    return RESULT_OK;
}

// Data
static const char* data_find(const char* str1, const char* str2, size_t size1, size_t size2) {
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
result_t data_create(pool_t* pool, data_t** data) {
    if (pool_data16_alloc(pool, data) != RESULT_OK) {
        RETURN_ERR("Failed to allocate data with capacity 16");
    }
    (*data)->size = 0;
    return RESULT_OK;
}
result_t data_create_data(pool_t* pool, data_t** data1, const data_t* data2) {
    if (pool_data_alloc(pool, data1, data2->capacity) != RESULT_OK) {
        RETURN_ERR("Failed to allocate data with sufficient capacity");
    }
    (*data1)->size = data2->size;
    memcpy((*data1)->data, data2->data, data2->size);
    return RESULT_OK;
}
result_t data_create_str(pool_t* pool, data_t** data, const char* str) {
    size_t len = strlen(str);
    if (pool_data_alloc(pool, data, len) != RESULT_OK) {
        RETURN_ERR("Failed to allocate data with sufficient capacity");
    }
    (*data)->size = len;
    memcpy((*data)->data, str, len);
    return RESULT_OK;
}
result_t data_clean(pool_t* pool, data_t** data) {
    if (pool_data_realloc(pool, data, 16) != RESULT_OK) {
        RETURN_ERR("Failed to reallocate data to clean it");
    }
    (*data)->size = 0;
    return RESULT_OK;
}
result_t data_copy_data(pool_t* pool, data_t** data1, const data_t* data2) {
    if ((*data1)->capacity != data2->capacity) {
        if (pool_data_realloc(pool, data1, data2->capacity) != RESULT_OK) {
            RETURN_ERR("Failed to reallocate data with sufficient capacity");
        }
    }
    (*data1)->size = data2->size;
    memcpy((*data1)->data, data2->data, data2->size);
    return RESULT_OK;
}
result_t data_copy_str(pool_t* pool, data_t** data, const char* str) {
    size_t len = strlen(str);
    if (pool_data_realloc(pool, data, len) != RESULT_OK) {
        RETURN_ERR("Failed to reallocate data with sufficient capacity");
    }
    (*data)->size = len;
    memcpy((*data)->data, str, len);
    return RESULT_OK;
}
result_t data_append_data(pool_t* pool, data_t** data1, const data_t* data2) {
    if ((*data1)->size + data2->size > (*data1)->capacity) {
        data_t* data_old = *data1;
        data_t* data_new;
        if (pool_data_alloc(pool, &data_new, data_old->size + data2->size) != RESULT_OK) {
            RETURN_ERR("Failed to allocate data with sufficient capacity");
        }
        memcpy(data_new->data, data_old->data, data_old->size);
        memcpy(data_new->data + data_old->size, data2->data, data2->size);
        data_new->size = data_old->size + data2->size;
        *data1 = data_new;
        if (pool_data_free(pool, data_old) != RESULT_OK) {
            RETURN_ERR("Failed to free old data");
        }
    } else {
        memcpy((*data1)->data + (*data1)->size, data2->data, data2->size);
        (*data1)->size += data2->size;
    }
    return RESULT_OK;
}
result_t data_append_str(pool_t* pool, data_t** data, const char* str) {
    size_t str_len = strlen(str);
    if ((*data)->size + str_len > (*data)->capacity) {
        data_t* data_old = *data;
        data_t* data_new;
        if (pool_data_alloc(pool, &data_new, (*data)->size + str_len) != RESULT_OK) {
            RETURN_ERR("Failed to allocate data with sufficient capacity");
        }
        memcpy(data_new->data, data_old->data, data_old->size);
        memcpy(data_new->data + data_old->size, str, str_len);
        data_new->size = data_old->size + str_len;
        *data = data_new;
        if (pool_data_free(pool, data_old) != RESULT_OK) {
            RETURN_ERR("Failed to free old data");
        }
    } else {
        memcpy((*data)->data + (*data)->size, str, str_len);
        (*data)->size += str_len;
    }
    return RESULT_OK;
}
result_t data_append_char(pool_t* pool, data_t** data, char c) {
    if ((*data)->size + 1 >= (*data)->capacity) {
        data_t* data_old = *data;
        data_t* data_new;
        if (pool_data_alloc(pool, &data_new, (*data)->size + 1) != RESULT_OK) {
            RETURN_ERR("Failed to allocate data with sufficient capacity");
        }
        memcpy(data_new->data, data_old->data, data_old->size);
        data_new->data[data_old->size] = c; // Append the new character
        data_new->size = data_old->size + 1;
        *data = data_new;
        if (pool_data_free(pool, data_old) != RESULT_OK) {
            RETURN_ERR("Failed to free old data");
        }
    } else {
        (*data)->data[(*data)->size++] = c;
    }
    return RESULT_OK;
}
result_t data_escape(pool_t* pool, data_t** data) {
    if (pool_data_realloc(pool, data, 114514) != RESULT_OK) {
        RETURN_ERR("data escaping not implemented yet");
    }
    return RESULT_OK;
}
result_t data_unescape(pool_t* pool, data_t** data) {
    if (pool_data_realloc(pool, data, 114514) != RESULT_OK) {
        RETURN_ERR("data unescaping not implemented yet");
    }
    return RESULT_OK;
}
uint64_t data_equal_data(const data_t* data1, const data_t* data2) {
    if (data1->size != data2->size) {
        return 0;
    }
    return memcmp(data1->data, data2->data, data1->size) == 0;
}
uint64_t data_equal_str(const data_t* data, const char* str) {
    size_t len = strlen(str);
    if (data->size != len) {
        return 0;
    }
    return memcmp(data->data, str, len) == 0;
}
int64_t data_find_data(const data_t* data1, const data_t* data2, uint64_t index) {
    if (index >= data1->size || data2->size == 0) {
        return -1;
    }
    const char* pos = data_find(data1->data + index, data2->data, data1->size - index, data2->size);
    if (!pos) {
        return -1;
    }
    return pos - data1->data;
}
int64_t data_find_str(const data_t* data, const char* str, uint64_t index) {
    if (index >= data->size || !str || *str == '\0') {
        return -1;
    }
    const char* pos = data_find(data->data + index, str, data->size - index, strlen(str));
    if (!pos) {
        return -1;
    }
    return pos - data->data;
}
int64_t data_find_char(const data_t* data, char c, uint64_t index) {
    if (index >= data->size) {
        return -1;
    }
    const char* pos = data_find(data->data + index, &c, data->size - index, 1);
    if (!pos) {
        return -1;
    }
    return pos - data->data;
}
result_t data_destroy(pool_t* pool, data_t* data) {
    if (pool_data_free(pool, data) != RESULT_OK) {
        RETURN_ERR("Failed to free data");
    }
    return RESULT_OK;
}

// ---------------- Object (JSON) ----------------

// Escape a JSON data
static result_t escape_json_data(pool_t* pool, const data_t* input, data_t** output) {
    // Worst case allocate 2x size
    uint64_t cap = (input ? input->size : 0) * 2 + 2;
    if (pool_data_alloc(pool, output, cap) != RESULT_OK) {
        RETURN_ERR("Failed to alloc escape buffer");
    }
    (*output)->size = 0;
    if (!input || input->size == 0) return RESULT_OK;
    for (uint64_t i = 0; i < input->size; i++) {
        unsigned char ch = (unsigned char)input->data[i];
        switch (ch) {
            case '"': if (data_append_str(pool, output, "\\\"") != RESULT_OK) RETURN_ERR("Failed to append escaped sequence while escaping JSON string"); break;
            case '\\': if (data_append_str(pool, output, "\\\\") != RESULT_OK) RETURN_ERR("Failed to append escaped sequence while escaping JSON string"); break;
            case '\b': if (data_append_str(pool, output, "\\b") != RESULT_OK) RETURN_ERR("Failed to append escaped sequence while escaping JSON string"); break;
            case '\f': if (data_append_str(pool, output, "\\f") != RESULT_OK) RETURN_ERR("Failed to append escaped sequence while escaping JSON string"); break;
            case '\n': if (data_append_str(pool, output, "\\n") != RESULT_OK) RETURN_ERR("Failed to append escaped sequence while escaping JSON string"); break;
            case '\r': if (data_append_str(pool, output, "\\r") != RESULT_OK) RETURN_ERR("Failed to append escaped sequence while escaping JSON string"); break;
            case '\t': if (data_append_str(pool, output, "\\t") != RESULT_OK) RETURN_ERR("Failed to append escaped sequence while escaping JSON string"); break;
            default:
                if (ch < 0x20) {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", ch);
                    if (data_append_str(pool, output, buf) != RESULT_OK) RETURN_ERR("Failed to append Unicode escape while escaping JSON string");
                } else {
                    if (data_append_char(pool, output, (char)ch) != RESULT_OK) RETURN_ERR("Failed to append character while escaping JSON string");
                }
        }
    }
    return RESULT_OK;
}

// JSON parsing helpers
static const char* skip_ws(const char* p, const char* end) {
    while (p < end && (*p==' '||*p=='\n'||*p=='\r'||*p=='\t')) p++;
    return p;
}
static result_t parse_json_data(pool_t* pool, const char** json, const char* end, data_t** out) {
    const char* p = *json;
    if (p >= end) RETURN_ERR("Unexpected end of JSON while parsing string");
    if (*p != '\"') RETURN_ERR("Expected '\"' to start JSON string");
    p++;
    const char* start = p;
    // find end quote, honoring escapes
    while (p < end) {
        if (*p == '"') break;
        if (*p == '\\' && p+1 < end) { p += 2; continue; }
        p++;
    }
    if (p >= end || *p != '"') RETURN_ERR("Unterminated JSON string");
    // decode escapes into output
    // First copy raw into temp and then unescape simple sequences
    size_t raw_len = (size_t)(p - start);
    if (pool_data_alloc(pool, out, raw_len + 1) != RESULT_OK) RETURN_ERR("Failed to allocate buffer for JSON string");
    (*out)->size = raw_len;
    memcpy((*out)->data, start, raw_len);
    // process backslash escapes in place into new buffer
    data_t* decoded;
    if (pool_data_alloc(pool, &decoded, raw_len + 1) != RESULT_OK) RETURN_ERR("Failed to allocate buffer for decoded JSON string");
    decoded->size = 0;
    for (size_t i = 0; i < raw_len; ) {
        char c = (*out)->data[i];
        if (c == '\\' && i + 1 < raw_len) {
            char e = (*out)->data[i+1];
            switch (e) {
                case '"': if (data_append_char(pool, &decoded, '"') != RESULT_OK) RETURN_ERR("Failed to append character while decoding JSON escape"); break;
                case '\\': if (data_append_char(pool, &decoded, '\\') != RESULT_OK) RETURN_ERR("Failed to append character while decoding JSON escape"); break;
                case '/': if (data_append_char(pool, &decoded, '/') != RESULT_OK) RETURN_ERR("Failed to append character while decoding JSON escape"); break;
                case 'b': if (data_append_char(pool, &decoded, '\b') != RESULT_OK) RETURN_ERR("Failed to append character while decoding JSON escape"); break;
                case 'f': if (data_append_char(pool, &decoded, '\f') != RESULT_OK) RETURN_ERR("Failed to append character while decoding JSON escape"); break;
                case 'n': if (data_append_char(pool, &decoded, '\n') != RESULT_OK) RETURN_ERR("Failed to append character while decoding JSON escape"); break;
                case 'r': if (data_append_char(pool, &decoded, '\r') != RESULT_OK) RETURN_ERR("Failed to append character while decoding JSON escape"); break;
                case 't': if (data_append_char(pool, &decoded, '\t') != RESULT_OK) RETURN_ERR("Failed to append character while decoding JSON escape"); break;
                default: if (data_append_char(pool, &decoded, e) != RESULT_OK) RETURN_ERR("Failed to append literal after backslash in JSON string"); break;
            }
            i += 2;
        } else {
            if (data_append_char(pool, &decoded, c) != RESULT_OK) RETURN_ERR("Failed to append character to decoded JSON string");
            i++;
        }
    }
    // move decoded into out
    data_t* tmp = *out;
    *out = decoded;
    if (pool_data_free(pool, tmp) != RESULT_OK) RETURN_ERR("Failed to free temporary JSON buffer");
    *json = p + 1;
    return RESULT_OK;
}

static result_t parse_json_value_local(pool_t* pool, const char** json, const char* end, object_t** out);

static result_t parse_json_array_local(pool_t* pool, const char** json, const char* end, object_t** out) {
    const char* p = *json; // points at '['
    if (p >= end || *p != '[') RETURN_ERR("Expected '[' at start of JSON array");
    p++; p = skip_ws(p, end);
    if (pool_object_alloc(pool, out) != RESULT_OK) RETURN_ERR("Failed to allocate array object from pool");
    (*out)->data = NULL; (*out)->child = NULL; (*out)->next = NULL;
    object_t* first = NULL; object_t* last = NULL;
    if (p < end && *p == ']') { *json = p + 1; return RESULT_OK; }
    while (p < end) {
        object_t* elem;
    if (parse_json_value_local(pool, &p, end, &elem) != RESULT_OK) RETURN_ERR("Failed to parse JSON array element");
        if (!first) { first = last = elem; } else { last->next = elem; last = elem; }
        p = skip_ws(p, end);
        if (p < end && *p == ',') { p++; p = skip_ws(p, end); continue; }
        if (p < end && *p == ']') break;
    RETURN_ERR("Expected ',' or ']' while parsing JSON array");
    }
    if (p >= end || *p != ']') RETURN_ERR("Unterminated JSON array");
    (*out)->child = first;
    *json = p + 1;
    return RESULT_OK;
}

static result_t parse_json_object_local(pool_t* pool, const char** json, const char* end, object_t** out) {
    const char* p = *json; // at '{'
    if (p >= end || *p != '{') RETURN_ERR("Expected '{' at start of JSON object");
    p++; p = skip_ws(p, end);
    if (pool_object_alloc(pool, out) != RESULT_OK) RETURN_ERR("Failed to allocate object from pool");
    (*out)->data = NULL; (*out)->child = NULL; (*out)->next = NULL;
    object_t* first = NULL; object_t* last = NULL;
    if (p < end && *p == '}') { *json = p + 1; return RESULT_OK; }
    while (p < end) {
        p = skip_ws(p, end);
        data_t* key;
    if (*p != '"') RETURN_ERR("Expected string key in JSON object");
    if (parse_json_data(pool, &p, end, &key) != RESULT_OK) RETURN_ERR("Failed to parse JSON object key");
    p = skip_ws(p, end); if (p >= end || *p != ':') RETURN_ERR("Expected ':' after object key"); p++; p = skip_ws(p, end);
        object_t* val;
    if (parse_json_value_local(pool, &p, end, &val) != RESULT_OK) RETURN_ERR("Failed to parse JSON object value");
    object_t* pair; if (pool_object_alloc(pool, &pair) != RESULT_OK) RETURN_ERR("Failed to allocate key-value node from pool");
        pair->data = key; pair->child = val; pair->next = NULL;
        if (!first) { first = last = pair; } else { last->next = pair; last = pair; }
        p = skip_ws(p, end);
        if (p < end && *p == ',') { p++; p = skip_ws(p, end); continue; }
        if (p < end && *p == '}') break;
    RETURN_ERR("Expected ',' or '}' while parsing JSON object");
    }
    if (p >= end || *p != '}') RETURN_ERR("Unterminated JSON object");
    (*out)->child = first;
    *json = p + 1;
    return RESULT_OK;
}

static result_t parse_primitive_local(pool_t* pool, const char** json, const char* end, data_t** out) {
    const char* p = *json;
    const char* start = p;
    while (p < end && *p != ',' && *p != '}' && *p != ']' && *p!=' ' && *p!='\t' && *p!='\n' && *p!='\r') p++;
    if (p == start) RETURN_ERR("Invalid JSON primitive literal");
    size_t len = (size_t)(p - start);
    if (pool_data_alloc(pool, out, len) != RESULT_OK) RETURN_ERR("Failed to allocate buffer for JSON primitive");
    (*out)->size = len; memcpy((*out)->data, start, len);
    *json = p;
    return RESULT_OK;
}

static result_t parse_json_value_local(pool_t* pool, const char** json, const char* end, object_t** out) {
    const char* p = skip_ws(*json, end);
    if (p >= end) RETURN_ERR("Unexpected end of JSON input");
    if (*p == '"') {
    if (pool_object_alloc(pool, out) != RESULT_OK) RETURN_ERR("Failed to allocate object from pool");
    if (parse_json_data(pool, &p, end, &((*out)->data)) != RESULT_OK) RETURN_ERR("Failed to parse JSON string value");
        (*out)->child = NULL; (*out)->next = NULL; *json = p; return RESULT_OK;
    } else if (*p == '{') {
        *json = p; return parse_json_object_local(pool, json, end, out);
    } else if (*p == '[') {
        *json = p; return parse_json_array_local(pool, json, end, out);
    } else {
    if (pool_object_alloc(pool, out) != RESULT_OK) RETURN_ERR("Failed to allocate object from pool");
    if (parse_primitive_local(pool, &p, end, &((*out)->data)) != RESULT_OK) RETURN_ERR("Failed to parse JSON primitive value");
        (*out)->child = NULL; (*out)->next = NULL; *json = p; return RESULT_OK;
    }
}

result_t object_create(pool_t* pool, object_t** dst) {
    if (pool_object_alloc(pool, dst) != RESULT_OK) RETURN_ERR("Failed to allocate object from pool");
    (*dst)->data = NULL; (*dst)->child = NULL; (*dst)->next = NULL;
    return RESULT_OK;
}

static result_t object_destroy_recursive(pool_t* pool, object_t* obj) {
    if (!obj) return RESULT_OK;
    if (obj->data) {
        if (data_destroy(pool, obj->data) != RESULT_OK) RETURN_ERR("free str");
        obj->data = NULL;
    }
    if (obj->child) {
        object_t* c = obj->child;
        while (c) {
            object_t* nxt = c->next;
            if (object_destroy_recursive(pool, c) != RESULT_OK) RETURN_ERR("free child");
            c = nxt;
        }
        obj->child = NULL;
    }
    if (pool_object_free(pool, obj) != RESULT_OK) RETURN_ERR("free obj");
    return RESULT_OK;
}

result_t object_destroy(pool_t* pool, object_t* object) {
    if (!object) return RESULT_OK;
    return object_destroy_recursive(pool, object);
}

result_t object_parse_json(pool_t* pool, object_t** dst, const data_t* src) {
    if (!src || src->size == 0) RETURN_ERR("Empty JSON data");
    const char* json = src->data;
    const char* end = src->data + src->size;
    const char* p = skip_ws(json, end);
    if (parse_json_value_local(pool, &p, end, dst) != RESULT_OK) RETURN_ERR("parse json");
    return RESULT_OK;
}

static int is_json_primitive_local(const data_t* s) {
    if (!s || s->size == 0) return 0;
    if (data_equal_str(s, "null") || data_equal_str(s, "true") || data_equal_str(s, "false")) return 1;
    // number simple check
    const char* d = s->data; size_t n = s->size; size_t i = 0;
    if (d[i] == '-' ) i++;
    int has_digit = 0;
    while (i < n && isdigit((unsigned char)d[i])) { has_digit = 1; i++; }
    if (i < n && d[i] == '.') { i++; while (i < n && isdigit((unsigned char)d[i])) { has_digit = 1; i++; } }
    if (!has_digit) return 0;
    if (i < n && (d[i] == 'e' || d[i] == 'E')) {
        i++; if (i < n && (d[i]=='+'||d[i]=='-')) i++; while (i < n && isdigit((unsigned char)d[i])) i++;
    }
    return i == n;
}

static result_t object_to_json_recursive_local(pool_t* pool, data_t** dst, const object_t* obj) {
    if (!obj) return data_append_str(pool, dst, "null");
    if (obj->data && !obj->child) {
        // leaf
        if (is_json_primitive_local(obj->data)) {
            return data_append_data(pool, dst, obj->data);
        } else {
            data_t* esc; if (escape_json_data(pool, obj->data, &esc) != RESULT_OK) RETURN_ERR("Failed to escape string for JSON output");
            if (data_append_char(pool, dst, '"') != RESULT_OK) RETURN_ERR("Failed to append to JSON output");
            if (data_append_data(pool, dst, esc) != RESULT_OK) RETURN_ERR("Failed to append to JSON output");
            if (data_destroy(pool, esc) != RESULT_OK) RETURN_ERR("Failed to free escaped buffer");
            if (data_append_char(pool, dst, '"') != RESULT_OK) RETURN_ERR("Failed to append to JSON output");
            return RESULT_OK;
        }
    }
    // container: determine if object or array
    if (!obj->data && obj->child && obj->child->data && obj->child->child) {
        // This node is an object (children are key-value pairs)
    if (data_append_char(pool, dst, '{') != RESULT_OK) RETURN_ERR("Failed to append '{' to JSON output");
        const object_t* ch = obj->child; int first = 1;
        while (ch) {
            if (!first) { if (data_append_char(pool, dst, ',') != RESULT_OK) RETURN_ERR("Failed to append ',' to JSON output"); }
            first = 0;
            data_t* esc_key; if (escape_json_data(pool, ch->data, &esc_key) != RESULT_OK) RETURN_ERR("Failed to escape object key for JSON output");
            if (data_append_char(pool, dst, '"') != RESULT_OK) RETURN_ERR("Failed to append to JSON output");
            if (data_append_data(pool, dst, esc_key) != RESULT_OK) RETURN_ERR("Failed to append to JSON output");
            if (data_destroy(pool, esc_key) != RESULT_OK) RETURN_ERR("Failed to free escaped key buffer");
            if (data_append_str(pool, dst, "\":") != RESULT_OK) RETURN_ERR("Failed to append ':' to JSON output");
            if (object_to_json_recursive_local(pool, dst, ch->child) != RESULT_OK) RETURN_ERR("Failed to serialize JSON value");
            ch = ch->next;
        }
    if (data_append_char(pool, dst, '}') != RESULT_OK) RETURN_ERR("Failed to append '}' to JSON output");
        return RESULT_OK;
    } else if (!obj->data && obj->child) {
        // array
    if (data_append_char(pool, dst, '[') != RESULT_OK) RETURN_ERR("Failed to append '[' to JSON output");
        const object_t* ch = obj->child; int first = 1;
        while (ch) {
            if (!first) { if (data_append_char(pool, dst, ',') != RESULT_OK) RETURN_ERR("Failed to append ',' to JSON output"); }
            first = 0;
            if (object_to_json_recursive_local(pool, dst, ch) != RESULT_OK) RETURN_ERR("Failed to serialize JSON array element");
            ch = ch->next;
        }
    if (data_append_char(pool, dst, ']') != RESULT_OK) RETURN_ERR("Failed to append ']' to JSON output");
        return RESULT_OK;
    }
    // empty or null
    return data_append_str(pool, dst, "null");
}

result_t object_todata_json(pool_t* pool, data_t** dst, const object_t* src) {
    if (!*dst) {
    if (data_create(pool, dst) != RESULT_OK) RETURN_ERR("Failed to create destination data buffer");
    } else {
    if (data_clean(pool, dst) != RESULT_OK) RETURN_ERR("Failed to clear destination data buffer");
    }
    return object_to_json_recursive_local(pool, dst, src);
}

// dot-path traversal supporting object keys and array indices (e.g., "a.b.0.c")
result_t object_provide_str(pool_t* pool, object_t** dst, const object_t* object, const char* path) {
    (void)pool; // pool unused except for signature symmetry
    if (!object || !path) RETURN_ERR("Invalid arguments: object and path are required");
    const object_t* cur = object;
    // if root is a container, dive into its child list
    if (cur->data == NULL && cur->child) cur = cur; // no-op
    const char* p = path;
    while (*p) {
        // extract segment until '.'
        char seg[256]; size_t si = 0;
        while (*p && *p != '.' && si + 1 < sizeof(seg)) seg[si++] = *p++;
        seg[si] = '\0';
        if (*p == '.') p++;
        // decide if index
        int is_index = 1; for (size_t i = 0; i < si; i++) { if (!isdigit((unsigned char)seg[i])) { is_index = 0; break; } }
        if (is_index && si > 0) {
            // array index
            size_t idx = (size_t)strtoull(seg, NULL, 10);
            const object_t* child = cur->child; size_t k = 0;
            while (child && k < idx) { child = child->next; k++; }
            if (!child) RETURN_ERR("Array index out of range in path traversal");
            cur = child;
        } else {
            // find key in object
            const object_t* child = cur->child; int found = 0;
            while (child) {
                if (child->data && child->child) {
                    if (child->data->size == si && memcmp(child->data->data, seg, si) == 0) { cur = child->child; found = 1; break; }
                }
                child = child->next;
            }
            if (!found) RETURN_ERR("Key not found in object during path traversal");
        }
    }
    *dst = (object_t*)cur; // cast away const for API
    return RESULT_OK;
}

// File
result_t file_read(pool_t* pool, data_t** data, const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        RETURN_ERR("Failed to open file for reading");
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        RETURN_ERR("Failed to seek to end of file");
    }
    long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        RETURN_ERR("Failed to get file size");
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        RETURN_ERR("Failed to seek to start of file");
    }
    if (pool_data_realloc(pool, data, file_size) != RESULT_OK) {
        fclose(file);
        RETURN_ERR("Failed to allocate data for file data");
    }
    size_t read_size = fread((*data)->data, 1, file_size, file);
    if (read_size != (uint64_t)file_size) {
        if (pool_data_free(pool, *data)) {
            fclose(file);
            RETURN_ERR("Failed to free data after partial read");
        }
        fclose(file);
        RETURN_ERR("Failed to read entire file");
    }
    (*data)->size = file_size;
    fclose(file);
    return RESULT_OK;
}
result_t file_write(const char* path, const data_t* data) {
    FILE* file = fopen(path, "w");
    if (!file) {
        RETURN_ERR("Failed to open file for writing");
    }
    size_t written_size = fwrite(data->data, 1, data->size, file);
    if (written_size != data->size) {
        fclose(file);
        RETURN_ERR("Failed to write entire data to file");
    }
    if (fclose(file) != 0) {
        RETURN_ERR("Failed to close file after writing");
    }
    return RESULT_OK;
}

// ---------------- Object (XML) ----------------
// Minimal helpers for XML using data_t
static const char* skip_xml_ws_local(const char* p, const char* end) {
    while (p < end && (*p==' '||*p=='\t'||*p=='\n'||*p=='\r')) p++;
    return p;
}
static result_t parse_xml_tag_name_local(pool_t* pool, const char** xml, const char* end, data_t** name) {
    const char* p = *xml;
    if (p >= end || (!isalpha((unsigned char)*p) && *p != '_')) RETURN_ERR("bad tag start");
    const char* start = p;
    while (p < end && (isalnum((unsigned char)*p) || *p=='-'||*p=='_'||*p=='.'||*p==':')) p++;
    size_t len = (size_t)(p - start);
    if (pool_data_alloc(pool, name, len) != RESULT_OK) RETURN_ERR("alloc tag");
    (*name)->size = len; memcpy((*name)->data, start, len);
    *xml = p;
    return RESULT_OK;
}
static result_t parse_xml_text_local(pool_t* pool, const char** xml, const char* end, data_t** out) {
    const char* p = *xml; const char* start = p;
    while (p < end && *p != '<') p++;
    size_t len = (size_t)(p - start);
    // trim
    while (len > 0 && (*start==' '||*start=='\t'||*start=='\n'||*start=='\r')) { start++; len--; }
    while (len > 0 && (start[len-1]==' '||start[len-1]=='\t'||start[len-1]=='\n'||start[len-1]=='\r')) { len--; }
    if (len == 0) { *out = NULL; *xml = p; return RESULT_OK; }
    if (pool_data_alloc(pool, out, len) != RESULT_OK) RETURN_ERR("alloc text");
    (*out)->size = len; memcpy((*out)->data, start, len);
    *xml = p; return RESULT_OK;
}

static result_t parse_xml_element_local(pool_t* pool, const char** xml, const char* end, object_t** out);

static result_t parse_xml_content_local(pool_t* pool, const char** xml, const char* end, const data_t* tag_name, object_t** content) {
    const char* p = *xml; p = skip_xml_ws_local(p, end);
    // self-closing
    if (p < end && *p == '/') {
        p++; p = skip_xml_ws_local(p, end);
        if (p >= end || *p != '>') RETURN_ERR("expected > after /");
        *xml = p + 1; return RESULT_OK;
    }
    if (p >= end || *p != '>') RETURN_ERR("expected > after tag name");
    p++;
    object_t* first = NULL; object_t* last = NULL; data_t* text_acc = NULL;
    while (p < end) {
        p = skip_xml_ws_local(p, end);
        if (p >= end) RETURN_ERR("unexpected end xml");
        if (*p == '<') {
            p++;
            if (p < end && *p == '/') {
                // closing
                p++; p = skip_xml_ws_local(p, end);
                data_t* closing;
                if (parse_xml_tag_name_local(pool, &p, end, &closing) != RESULT_OK) RETURN_ERR("close name");
                // compare
                if (closing->size != tag_name->size || memcmp(closing->data, tag_name->data, tag_name->size) != 0) {
                    if (pool_data_free(pool, closing) != RESULT_OK) RETURN_ERR("free closing");
                    RETURN_ERR("closing mismatch");
                }
                if (pool_data_free(pool, closing) != RESULT_OK) RETURN_ERR("free closing");
                p = skip_xml_ws_local(p, end);
                if (p >= end || *p != '>') RETURN_ERR("expected > after closing");
                p++; break;
            } else {
                // child element
                p--; // include '<'
                object_t* child;
                if (parse_xml_element_local(pool, &p, end, &child) != RESULT_OK) RETURN_ERR("child elem");
                if (!first) { first = last = child; } else { last->next = child; last = child; }
            }
        } else {
            // text
            data_t* text;
            if (parse_xml_text_local(pool, &p, end, &text) != RESULT_OK) RETURN_ERR("text");
            if (text && text->size > 0) {
                if (!text_acc) { text_acc = text; }
                else {
                    if (data_append_data(pool, &text_acc, text) != RESULT_OK) {
                        if (pool_data_free(pool, text) != RESULT_OK) RETURN_ERR("free text");
                        RETURN_ERR("append text");
                    }
                    if (pool_data_free(pool, text) != RESULT_OK) RETURN_ERR("free text");
                }
            } else if (text) {
                if (pool_data_free(pool, text) != RESULT_OK) RETURN_ERR("free text");
            }
        }
    }
    if (text_acc && first) RETURN_ERR("mixed content unsupported");
    if (text_acc) { (*content)->data = text_acc; }
    else if (first) { (*content)->child = first; }
    *xml = p; return RESULT_OK;
}

static result_t parse_xml_element_local(pool_t* pool, const char** xml, const char* end, object_t** out) {
    const char* p = *xml; p = skip_xml_ws_local(p, end);
    if (p >= end || *p != '<') RETURN_ERR("expected <");
    p++;
    data_t* tag;
    if (parse_xml_tag_name_local(pool, &p, end, &tag) != RESULT_OK) RETURN_ERR("tag");
    object_t* content; if (pool_object_alloc(pool, &content) != RESULT_OK) { if (pool_data_free(pool, tag) != RESULT_OK) RETURN_ERR("free tag"); RETURN_ERR("alloc content"); }
    content->data = NULL; content->child = NULL; content->next = NULL;
    if (parse_xml_content_local(pool, &p, end, tag, &content) != RESULT_OK) { if (pool_data_free(pool, tag) != RESULT_OK) RETURN_ERR("free tag"); RETURN_ERR("content"); }
    if (pool_object_alloc(pool, out) != RESULT_OK) { if (pool_data_free(pool, tag) != RESULT_OK) RETURN_ERR("free tag"); RETURN_ERR("alloc pair"); }
    (*out)->data = tag; (*out)->child = content; (*out)->next = NULL; *xml = p; return RESULT_OK;
}

result_t object_parse_xml(pool_t* pool, object_t** dst, const data_t* src) {
    if (!pool || !dst || !src) RETURN_ERR("bad args");
    if (src->size == 0) RETURN_ERR("empty xml");
    const char* xml = src->data; const char* end = src->data + src->size; const char* p = skip_xml_ws_local(xml, end);
    // skip declaration <? ... ?>
    if (p < end && *p == '<' && p + 1 < end && p[1] == '?') {
        while (p < end && !(p[0]=='?' && p + 1 < end && p[1]=='>')) p++;
        if (p < end) p += 2;
        p = skip_xml_ws_local(p, end);
    }
    if (pool_object_alloc(pool, dst) != RESULT_OK) RETURN_ERR("alloc root");
    (*dst)->data = NULL; (*dst)->child = NULL; (*dst)->next = NULL;
    object_t* first = NULL; object_t* last = NULL;
    while (p < end) {
        p = skip_xml_ws_local(p, end);
        if (p >= end) break;
        if (*p == '<') {
            // skip comments <!-- ... --> and declarations <! ...>
            if (p + 4 <= end && strncmp(p, "<!--", 4) == 0) {
                p += 4; while (p + 3 <= end && strncmp(p, "-->", 3) != 0) p++; if (p + 3 <= end) p += 3; continue;
            }
            if (p + 2 <= end && p[1] == '!') { while (p < end && *p != '>') p++; if (p < end) p++; continue; }
            object_t* elem; if (parse_xml_element_local(pool, &p, end, &elem) != RESULT_OK) RETURN_ERR("elem");
            if (!first) { first = last = elem; } else { last->next = elem; last = elem; }
        } else {
            while (p < end && *p != '<') p++;
        }
    }
    (*dst)->child = first; return RESULT_OK;
}

// escape data bytes for XML text/element names
static result_t escape_xml_data(pool_t* pool, const data_t* in, data_t** out) {
    size_t est = in ? in->size * 6 + 1 : 1;
    if (pool_data_alloc(pool, out, est) != RESULT_OK) RETURN_ERR("alloc esc xml");
    (*out)->size = 0; if (!in) return RESULT_OK;
    for (uint64_t i = 0; i < in->size; i++) {
        char ch = in->data[i];
        switch (ch) {
            case '<': if (data_append_str(pool, out, "&lt;") != RESULT_OK) RETURN_ERR("esc"); break;
            case '>': if (data_append_str(pool, out, "&gt;") != RESULT_OK) RETURN_ERR("esc"); break;
            case '&': if (data_append_str(pool, out, "&amp;") != RESULT_OK) RETURN_ERR("esc"); break;
            case '"': if (data_append_str(pool, out, "&quot;") != RESULT_OK) RETURN_ERR("esc"); break;
            case '\'': if (data_append_str(pool, out, "&apos;") != RESULT_OK) RETURN_ERR("esc"); break;
            default:
                if ((unsigned char)ch < 0x20 && ch != '\t' && ch != '\n' && ch != '\r') {
                    // skip control
                } else {
                    if (data_append_char(pool, out, ch) != RESULT_OK) RETURN_ERR("esc");
                }
        }
    }
    return RESULT_OK;
}

static int data_lexcmp_local(const data_t* a, const data_t* b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    uint64_t min = a->size < b->size ? a->size : b->size;
    int cmp = memcmp(a->data, b->data, min);
    if (cmp != 0) return cmp;
    if (a->size < b->size) return -1;
    if (a->size > b->size) return 1;
    return 0;
}

static result_t object_to_xml_recursive_local(pool_t* pool, data_t** dst, const object_t* src, const char* element_name) {
    if (!src) {
        if (data_append_str(pool, dst, "<") != RESULT_OK) RETURN_ERR("xml");
        if (data_append_str(pool, dst, element_name) != RESULT_OK) RETURN_ERR("xml");
        if (data_append_str(pool, dst, "/>") != RESULT_OK) RETURN_ERR("xml");
        return RESULT_OK;
    }
    if (src->data && !src->child) {
        data_t* esc; if (escape_xml_data(pool, src->data, &esc) != RESULT_OK) RETURN_ERR("esc");
        if (data_append_str(pool, dst, "<") != RESULT_OK) RETURN_ERR("xml");
        if (data_append_str(pool, dst, element_name) != RESULT_OK) RETURN_ERR("xml");
    if (data_append_str(pool, dst, ">") != RESULT_OK) RETURN_ERR("xml");
    if (data_append_data(pool, dst, esc) != RESULT_OK) RETURN_ERR("xml");
    if (data_append_str(pool, dst, "</") != RESULT_OK) RETURN_ERR("xml");
    if (data_append_str(pool, dst, element_name) != RESULT_OK) RETURN_ERR("xml");
    if (data_append_str(pool, dst, ">") != RESULT_OK) RETURN_ERR("xml");
    if (pool_data_free(pool, esc) != RESULT_OK) RETURN_ERR("free esc");
    return RESULT_OK;
    }
    // container
    object_t* first = src->child;
    if (first && first->data) {
        // object
        if (data_append_str(pool, dst, "<") != RESULT_OK) RETURN_ERR("xml");
        if (data_append_str(pool, dst, element_name) != RESULT_OK) RETURN_ERR("xml");
        if (data_append_str(pool, dst, ">") != RESULT_OK) RETURN_ERR("xml");
        const data_t* prev_key = NULL; const object_t* prev_child = NULL; size_t emitted = 0; size_t count = 0;
        { for (object_t* c = first; c; c = c->next) if (c->data) count++; }
        while (emitted < count) {
            object_t* best = NULL;
            for (object_t* c = first; c; c = c->next) {
                if (!c->data) continue;
                if (!prev_key) {
                    int rel = data_lexcmp_local(c->data, best ? best->data : NULL);
                    if (!best || rel < 0 || (rel == 0 && c < best)) best = c;
                } else {
                    int cmp_prev = data_lexcmp_local(c->data, prev_key);
                    if (cmp_prev < 0) continue;
                    if (cmp_prev == 0 && prev_child && c <= prev_child) continue;
                    int rel = data_lexcmp_local(c->data, best ? best->data : NULL);
                    if (!best || rel < 0 || (rel == 0 && c < best)) best = c;
                }
            }
            if (!best) break;
            data_t* key_esc; if (escape_xml_data(pool, best->data, &key_esc) != RESULT_OK) RETURN_ERR("esk");
            data_t* key_cstr; if (pool_data_alloc(pool, &key_cstr, key_esc->size + 1) != RESULT_OK) { if (pool_data_free(pool, key_esc) != RESULT_OK) RETURN_ERR("free key_esc"); RETURN_ERR("alloc"); }
            memcpy(key_cstr->data, key_esc->data, key_esc->size); key_cstr->data[key_esc->size] = '\0'; key_cstr->size = key_esc->size;
            if (object_to_xml_recursive_local(pool, dst, best->child, key_cstr->data) != RESULT_OK) {
                if (pool_data_free(pool, key_cstr) != RESULT_OK) RETURN_ERR("free key_cstr");
                if (pool_data_free(pool, key_esc) != RESULT_OK) RETURN_ERR("free key_esc");
                RETURN_ERR("child");
            }
            if (pool_data_free(pool, key_cstr) != RESULT_OK) RETURN_ERR("free key_cstr");
            if (pool_data_free(pool, key_esc) != RESULT_OK) RETURN_ERR("free key_esc");
            prev_key = best->data; prev_child = best; emitted++;
        }
        if (data_append_str(pool, dst, "</") != RESULT_OK) RETURN_ERR("xml");
        if (data_append_str(pool, dst, element_name) != RESULT_OK) RETURN_ERR("xml");
        if (data_append_str(pool, dst, ">") != RESULT_OK) RETURN_ERR("xml");
        return RESULT_OK;
    } else if (first) {
        // array
        if (data_append_str(pool, dst, "<") != RESULT_OK) RETURN_ERR("xml");
        if (data_append_str(pool, dst, element_name) != RESULT_OK) RETURN_ERR("xml");
        if (data_append_str(pool, dst, ">") != RESULT_OK) RETURN_ERR("xml");
        int index = 0; for (object_t* c = first; c; c = c->next, index++) {
            char item_name[32]; snprintf(item_name, sizeof(item_name), "item%d", index);
            if (object_to_xml_recursive_local(pool, dst, c, item_name) != RESULT_OK) RETURN_ERR("elem");
        }
        if (data_append_str(pool, dst, "</") != RESULT_OK) RETURN_ERR("xml");
        if (data_append_str(pool, dst, element_name) != RESULT_OK) RETURN_ERR("xml");
        if (data_append_str(pool, dst, ">") != RESULT_OK) RETURN_ERR("xml");
        return RESULT_OK;
    } else {
        if (data_append_str(pool, dst, "<") != RESULT_OK) RETURN_ERR("xml");
        if (data_append_str(pool, dst, element_name) != RESULT_OK) RETURN_ERR("xml");
        if (data_append_str(pool, dst, "/>") != RESULT_OK) RETURN_ERR("xml");
        return RESULT_OK;
    }
}

result_t object_todata_xml(pool_t* pool, data_t** dst, const object_t* src) {
    if (!*dst) { if (data_create(pool, dst) != RESULT_OK) RETURN_ERR("dst"); }
    else { if (data_clean(pool, dst) != RESULT_OK) RETURN_ERR("clear"); }
    if (src && src->child) {
        object_t* first = src->child;
        if (first && first->data) {
            const data_t* prev_key = NULL; const object_t* prev_child = NULL; size_t emitted = 0; size_t count = 0;
            { for (object_t* c = first; c; c = c->next) if (c->data) count++; }
            while (emitted < count) {
                object_t* best = NULL;
                for (object_t* c = first; c; c = c->next) {
                    if (!c->data) continue;
                    if (!prev_key) { int rel = data_lexcmp_local(c->data, best ? best->data : NULL); if (!best || rel < 0 || (rel==0 && c < best)) best = c; }
                    else {
                        int cmp_prev = data_lexcmp_local(c->data, prev_key); if (cmp_prev < 0) continue; if (cmp_prev == 0 && prev_child && c <= prev_child) continue;
                        int rel = data_lexcmp_local(c->data, best ? best->data : NULL); if (!best || rel < 0 || (rel==0 && c < best)) best = c;
                    }
                }
                if (!best) break;
                data_t* key_esc; if (escape_xml_data(pool, best->data, &key_esc) != RESULT_OK) RETURN_ERR("esk");
                data_t* key_cstr; if (pool_data_alloc(pool, &key_cstr, key_esc->size + 1) != RESULT_OK) { if (pool_data_free(pool, key_esc) != RESULT_OK) RETURN_ERR("free key_esc"); RETURN_ERR("alloc"); }
                memcpy(key_cstr->data, key_esc->data, key_esc->size); key_cstr->data[key_esc->size] = '\0'; key_cstr->size = key_esc->size;
                if (object_to_xml_recursive_local(pool, dst, best->child, key_cstr->data) != RESULT_OK) {
                    if (pool_data_free(pool, key_cstr) != RESULT_OK) RETURN_ERR("free key_cstr");
                    if (pool_data_free(pool, key_esc) != RESULT_OK) RETURN_ERR("free key_esc");
                    RETURN_ERR("child");
                }
                if (pool_data_free(pool, key_cstr) != RESULT_OK) RETURN_ERR("free key_cstr");
                if (pool_data_free(pool, key_esc) != RESULT_OK) RETURN_ERR("free key_esc");
                prev_key = best->data; prev_child = best; emitted++;
            }
            return RESULT_OK;
        } else {
            int index = 0; for (object_t* c = first; c; c = c->next, index++) {
                char item_name[32]; snprintf(item_name, sizeof(item_name), "item%d", index);
                if (object_to_xml_recursive_local(pool, dst, c, item_name) != RESULT_OK) RETURN_ERR("elem");
            }
            return RESULT_OK;
        }
    } else {
        return object_to_xml_recursive_local(pool, dst, src, "value");
    }
}
