#include "lkjlib.h"

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
    if (pool_data_alloc(pool, data, 16) != RESULT_OK) {
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
        data_t* data_new = NULL;
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
        data_t* data_new = NULL;
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
        data_t* data_new = NULL;
        if (pool_data_alloc(pool, &data_new, (*data)->size + 1) != RESULT_OK) {
            RETURN_ERR("Failed to allocate data with sufficient capacity");
        }
        memcpy(data_new->data, data_old->data, data_old->size);
        data_new->data[data_old->size] = c;
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

result_t data_toint(const data_t* data, int64_t* dst) {
    int64_t value = 0;
    int neg;
    uint64_t i;
    uint64_t size = data->size;
    if (data->data[0] == '-') {
        neg = 1;
        i = 1;
    } else {
        neg = 0;
        i = 0;
    }
    while(i < size) {
        char c = data->data[i];
        if(c < '0' || c > '9') {
            RETURN_ERR("Invalid character in data");
        }
        value = value * 10 + (c - '0');
        i++;
    }
    if (neg) {
        value = -value;
    }
    *dst = value;
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

result_t data_escape_json(pool_t* pool, data_t** data) {
    data_t* result = NULL;
    if (data_create(pool, &result) != RESULT_OK) {
        RETURN_ERR("Failed to create result data");
    }
    
    for (uint64_t i = 0; i < (*data)->size; i++) {
        char c = (*data)->data[i];
        switch (c) {
            case '"':
                if (data_append_str(pool, &result, "\\\"") != RESULT_OK) {
                    if (data_destroy(pool, result) != RESULT_OK) {
                        PRINT_ERR("Failed to destroy result data during cleanup");
                    }
                    RETURN_ERR("Failed to append escaped quote");
                }
                break;
            case '\\':
                if (data_append_str(pool, &result, "\\\\") != RESULT_OK) {
                    if (data_destroy(pool, result) != RESULT_OK) {
                        PRINT_ERR("Failed to destroy result data during cleanup");
                    }
                    RETURN_ERR("Failed to append escaped backslash");
                }
                break;
            case '\b':
                if (data_append_str(pool, &result, "\\b") != RESULT_OK) {
                    if (data_destroy(pool, result) != RESULT_OK) {
                        PRINT_ERR("Failed to destroy result data during cleanup");
                    }
                    RETURN_ERR("Failed to append escaped backspace");
                }
                break;
            case '\f':
                if (data_append_str(pool, &result, "\\f") != RESULT_OK) {
                    if (data_destroy(pool, result) != RESULT_OK) {
                        PRINT_ERR("Failed to destroy result data during cleanup");
                    }
                    RETURN_ERR("Failed to append escaped form feed");
                }
                break;
            case '\n':
                if (data_append_str(pool, &result, "\\n") != RESULT_OK) {
                    if (data_destroy(pool, result) != RESULT_OK) {
                        PRINT_ERR("Failed to destroy result data during cleanup");
                    }
                    RETURN_ERR("Failed to append escaped newline");
                }
                break;
            case '\r':
                if (data_append_str(pool, &result, "\\r") != RESULT_OK) {
                    if (data_destroy(pool, result) != RESULT_OK) {
                        PRINT_ERR("Failed to destroy result data during cleanup");
                    }
                    RETURN_ERR("Failed to append escaped carriage return");
                }
                break;
            case '\t':
                if (data_append_str(pool, &result, "\\t") != RESULT_OK) {
                    if (data_destroy(pool, result) != RESULT_OK) {
                        PRINT_ERR("Failed to destroy result data during cleanup");
                    }
                    RETURN_ERR("Failed to append escaped tab");
                }
                break;
            default:
                if ((unsigned char)c < 0x20) {
                    // Escape control characters as \uXXXX
                    char hex_buffer[7];
                    snprintf(hex_buffer, sizeof(hex_buffer), "\\u%04x", (unsigned char)c);
                    if (data_append_str(pool, &result, hex_buffer) != RESULT_OK) {
                        if (data_destroy(pool, result) != RESULT_OK) {
                            PRINT_ERR("Failed to destroy result data during cleanup");
                        }
                        RETURN_ERR("Failed to append unicode escape");
                    }
                } else {
                    if (data_append_char(pool, &result, c) != RESULT_OK) {
                        if (data_destroy(pool, result) != RESULT_OK) {
                            PRINT_ERR("Failed to destroy result data during cleanup");
                        }
                        RETURN_ERR("Failed to append regular character");
                    }
                }
                break;
        }
    }
    
    data_t* old_data = *data;
    *data = result;
    if (data_destroy(pool, old_data) != RESULT_OK) {
        RETURN_ERR("Failed to destroy old data");
    }
    
    return RESULT_OK;
}

static int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

result_t data_unescape_json(pool_t* pool, data_t** data) {
    data_t* result = NULL;
    if (data_create(pool, &result) != RESULT_OK) {
        RETURN_ERR("Failed to create result data");
    }
    
    for (uint64_t i = 0; i < (*data)->size; i++) {
        char c = (*data)->data[i];
        if (c == '\\' && i + 1 < (*data)->size) {
            char next = (*data)->data[i + 1];
            switch (next) {
                case '"':
                    if (data_append_char(pool, &result, '"') != RESULT_OK) {
                        if (data_destroy(pool, result) != RESULT_OK) {
                            PRINT_ERR("Failed to destroy result data during cleanup");
                        }
                        RETURN_ERR("Failed to append unescaped quote");
                    }
                    i++; // Skip the next character
                    break;
                case '\\':
                    if (data_append_char(pool, &result, '\\') != RESULT_OK) {
                        if (data_destroy(pool, result) != RESULT_OK) {
                            PRINT_ERR("Failed to destroy result data during cleanup");
                        }
                        RETURN_ERR("Failed to append unescaped backslash");
                    }
                    i++; // Skip the next character
                    break;
                case '/':
                    if (data_append_char(pool, &result, '/') != RESULT_OK) {
                        if (data_destroy(pool, result) != RESULT_OK) {
                            PRINT_ERR("Failed to destroy result data during cleanup");
                        }
                        RETURN_ERR("Failed to append unescaped slash");
                    }
                    i++; // Skip the next character
                    break;
                case 'b':
                    if (data_append_char(pool, &result, '\b') != RESULT_OK) {
                        if (data_destroy(pool, result) != RESULT_OK) {
                            PRINT_ERR("Failed to destroy result data during cleanup");
                        }
                        RETURN_ERR("Failed to append unescaped backspace");
                    }
                    i++; // Skip the next character
                    break;
                case 'f':
                    if (data_append_char(pool, &result, '\f') != RESULT_OK) {
                        if (data_destroy(pool, result) != RESULT_OK) {
                            PRINT_ERR("Failed to destroy result data during cleanup");
                        }
                        RETURN_ERR("Failed to append unescaped form feed");
                    }
                    i++; // Skip the next character
                    break;
                case 'n':
                    if (data_append_char(pool, &result, '\n') != RESULT_OK) {
                        if (data_destroy(pool, result) != RESULT_OK) {
                            PRINT_ERR("Failed to destroy result data during cleanup");
                        }
                        RETURN_ERR("Failed to append unescaped newline");
                    }
                    i++; // Skip the next character
                    break;
                case 'r':
                    if (data_append_char(pool, &result, '\r') != RESULT_OK) {
                        if (data_destroy(pool, result) != RESULT_OK) {
                            PRINT_ERR("Failed to destroy result data during cleanup");
                        }
                        RETURN_ERR("Failed to append unescaped carriage return");
                    }
                    i++; // Skip the next character
                    break;
                case 't':
                    if (data_append_char(pool, &result, '\t') != RESULT_OK) {
                        if (data_destroy(pool, result) != RESULT_OK) {
                            PRINT_ERR("Failed to destroy result data during cleanup");
                        }
                        RETURN_ERR("Failed to append unescaped tab");
                    }
                    i++; // Skip the next character
                    break;
                case 'u':
                    // Unicode escape sequence \uXXXX
                    if (i + 5 < (*data)->size) {
                        int hex1 = hex_char_to_int((*data)->data[i + 2]);
                        int hex2 = hex_char_to_int((*data)->data[i + 3]);
                        int hex3 = hex_char_to_int((*data)->data[i + 4]);
                        int hex4 = hex_char_to_int((*data)->data[i + 5]);
                        
                        if (hex1 >= 0 && hex2 >= 0 && hex3 >= 0 && hex4 >= 0) {
                            int unicode_value = (hex1 << 12) | (hex2 << 8) | (hex3 << 4) | hex4;
                            if (unicode_value < 0x80) {
                                // ASCII character
                                if (data_append_char(pool, &result, (char)unicode_value) != RESULT_OK) {
                                    if (data_destroy(pool, result) != RESULT_OK) {
                                        PRINT_ERR("Failed to destroy result data during cleanup");
                                    }
                                    RETURN_ERR("Failed to append unicode character");
                                }
                            } else {
                                // For simplicity, we'll handle only ASCII range characters
                                // In a full implementation, you'd need proper UTF-8 encoding
                                if (data_append_char(pool, &result, '?') != RESULT_OK) {
                                    if (data_destroy(pool, result) != RESULT_OK) {
                                        PRINT_ERR("Failed to destroy result data during cleanup");
                                    }
                                    RETURN_ERR("Failed to append replacement character");
                                }
                            }
                            i += 5; // Skip the entire \uXXXX sequence
                        } else {
                            // Invalid unicode escape, treat as literal
                            if (data_append_char(pool, &result, c) != RESULT_OK) {
                                if (data_destroy(pool, result) != RESULT_OK) {
                                    PRINT_ERR("Failed to destroy result data during cleanup");
                                }
                                RETURN_ERR("Failed to append literal backslash");
                            }
                        }
                    } else {
                        // Incomplete unicode escape, treat as literal
                        if (data_append_char(pool, &result, c) != RESULT_OK) {
                            if (data_destroy(pool, result) != RESULT_OK) {
                                PRINT_ERR("Failed to destroy result data during cleanup");
                            }
                            RETURN_ERR("Failed to append literal backslash");
                        }
                    }
                    break;
                default:
                    // Unknown escape sequence, treat as literal
                    if (data_append_char(pool, &result, c) != RESULT_OK) {
                        if (data_destroy(pool, result) != RESULT_OK) {
                            PRINT_ERR("Failed to destroy result data during cleanup");
                        }
                        RETURN_ERR("Failed to append literal backslash");
                    }
                    break;
            }
        } else {
            if (data_append_char(pool, &result, c) != RESULT_OK) {
                if (data_destroy(pool, result) != RESULT_OK) {
                    PRINT_ERR("Failed to destroy result data during cleanup");
                }
                RETURN_ERR("Failed to append regular character");
            }
        }
    }
    
    data_t* old_data = *data;
    *data = result;
    if (data_destroy(pool, old_data) != RESULT_OK) {
        RETURN_ERR("Failed to destroy old data");
    }
    
    return RESULT_OK;
}

// **JSON ESCAPE FUNCTION** - Properly escape JSON strings
result_t data_append_json_escaped(pool_t* pool, data_t** dst, const data_t* src) {
    if (!pool || !dst || !*dst || !src) {
        RETURN_ERR("Invalid input parameters for JSON escape");
    }
    
    // Reserve extra space for escaping (worst case: every char needs escaping)
    uint64_t estimated_size = src->size * 2;
    if ((*dst)->capacity < (*dst)->size + estimated_size) {
        if (pool_data_realloc(pool, dst, (*dst)->size + estimated_size) != RESULT_OK) {
            RETURN_ERR("Failed to allocate space for JSON escaping");
        }
    }
    
    // Escape JSON characters
    for (uint64_t i = 0; i < src->size; i++) {
        char c = src->data[i];
        
        switch (c) {
            case '"':
                if (data_append_str(pool, dst, "\\\"") != RESULT_OK) {
                    RETURN_ERR("Failed to escape quote");
                }
                break;
            case '\\':
                if (data_append_str(pool, dst, "\\\\") != RESULT_OK) {
                    RETURN_ERR("Failed to escape backslash");
                }
                break;
            case '\n':
                if (data_append_str(pool, dst, "\\n") != RESULT_OK) {
                    RETURN_ERR("Failed to escape newline");
                }
                break;
            case '\r':
                if (data_append_str(pool, dst, "\\r") != RESULT_OK) {
                    RETURN_ERR("Failed to escape carriage return");
                }
                break;
            case '\t':
                if (data_append_str(pool, dst, "\\t") != RESULT_OK) {
                    RETURN_ERR("Failed to escape tab");
                }
                break;
            default:
                if (data_append_char(pool, dst, c) != RESULT_OK) {
                    RETURN_ERR("Failed to append character");
                }
                break;
        }
    }
    
    return RESULT_OK;
}

result_t data_destroy(pool_t* pool, data_t* data) {
    if (pool_data_free(pool, data) != RESULT_OK) {
        RETURN_ERR("Failed to free data");
    }
    return RESULT_OK;
}
