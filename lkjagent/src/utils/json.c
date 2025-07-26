#include "utils/json.h"

result_t json_create_null(pool_t* pool, json_value_t** dst) {
    if (pool_json_value_alloc(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to allocate JSON value for null");
    }
    (*dst)->type = JSON_NULL;
    (*dst)->bool_value = 0;
    return RESULT_OK;
}

result_t json_create_bool(pool_t* pool, int b, json_value_t** dst) {
    if (pool_json_value_alloc(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to allocate JSON value for boolean");
    }
    (*dst)->type = JSON_BOOL;
    (*dst)->bool_value = b;
    return RESULT_OK;
}

result_t json_create_number(pool_t* pool, double num, json_value_t** dst) {
    if (pool_json_value_alloc(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to allocate JSON value for number");
    }
    (*dst)->type = JSON_NUMBER;
    (*dst)->number_value = num;
    return RESULT_OK;
}

result_t json_create_string(pool_t* pool, const string_t* src, json_value_t** dst) {
    if (pool_json_value_alloc(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to allocate JSON value for string");
    }
    (*dst)->type = JSON_STRING;
    if (string_create_string(pool, &(*dst)->string_value, src) != RESULT_OK) {
        if (pool_json_value_free(pool, *dst)) {
            RETURN_ERR("Failed to create string for JSON value");
        }
        RETURN_ERR("Failed to create string for JSON value");
    }
    return RESULT_OK;
}

result_t json_create_object(pool_t* pool, json_value_t** dst) {
    if (pool_json_value_alloc(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to allocate JSON value for object");
    }
    (*dst)->type = JSON_OBJECT;
    (*dst)->object.count = 0;
    (*dst)->object.elements = NULL;
    return RESULT_OK;
}

result_t json_create_array(pool_t* pool, json_value_t** dst) {
    if (pool_json_value_alloc(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to allocate JSON value for array");
    }
    (*dst)->type = JSON_ARRAY;
    (*dst)->array.count = 0;
    (*dst)->array.elements = NULL;
    return RESULT_OK;
}
