#include "utils/lkjstring.h"
#include "utils/lkjpool.h"

result_t string_init(string_t* string, char* data, uint64_t capacity) {
    if (!string || !data) {
        RETURN_ERR("Invalid parameters");
    }
    
    string->data = data;
    string->capacity = capacity;
    string_clear(string);
    return RESULT_OK;
}

result_t string_copy(pool_t* pool, string_t** dst, const string_t* src) {
    if (!pool || !dst || !src) {
        RETURN_ERR("Invalid parameters");
    }
    
    // If dst is NULL, allocate a new string
    if (*dst == NULL) {
        if (pool_string_alloc(pool, dst, src->size + 1) != RESULT_OK) {
            RETURN_ERR("Failed to allocate destination string");
        }
    } else {
        // Check if we need to reallocate to a larger size
        if (src->size >= (*dst)->capacity) {
            string_t* old_string = *dst;
            if (pool_string_alloc(pool, dst, src->size + 1) != RESULT_OK) {
                RETURN_ERR("Failed to reallocate destination string");
            }
            if (pool_string_free(pool, old_string) != RESULT_OK) {
                RETURN_ERR("Failed to free old string");
            }
        }
    }
    
    memcpy((*dst)->data, src->data, src->size);
    (*dst)->data[src->size] = '\0';
    (*dst)->size = src->size;
    return RESULT_OK;
}

result_t string_assign(pool_t* pool, string_t** string, const char* str) {
    if (!pool || !string || !str) {
        RETURN_ERR("Invalid parameters");
    }
    
    uint64_t str_len = (uint64_t)strlen(str);
    
    // If string is NULL, allocate a new string
    if (*string == NULL) {
        if (pool_string_alloc(pool, string, str_len + 1) != RESULT_OK) {
            RETURN_ERR("Failed to allocate string");
        }
    } else {
        // Check if we need to reallocate to a larger size
        if (str_len >= (*string)->capacity) {
            string_t* old_string = *string;
            if (pool_string_alloc(pool, string, str_len + 1) != RESULT_OK) {
                RETURN_ERR("Failed to reallocate string");
            }
            if (pool_string_free(pool, old_string) != RESULT_OK) {
                RETURN_ERR("Failed to free old string");
            }
        }
    }
    
    strcpy((*string)->data, str);
    (*string)->size = str_len;
    return RESULT_OK;
}

void string_clear(string_t* string) {
    if (!string) {
        return;
    }
    
    string->size = 0;
    if (string->data) {
        string->data[0] = '\0';
    }
}

result_t string_append(pool_t* pool, string_t** string, const string_t* src) {
    if (!pool || !string || !*string || !src) {
        RETURN_ERR("Invalid parameters");
    }
    
    uint64_t required_capacity = (*string)->size + src->size + 1; // +1 for null terminator
    
    // Check if we need to reallocate to a larger size
    if (required_capacity > (*string)->capacity) {
        string_t* old_string = *string;
        string_t* new_string;
        
        if (pool_string_alloc(pool, &new_string, required_capacity) != RESULT_OK) {
            RETURN_ERR("Failed to reallocate string for append");
        }
        
        // Copy existing data
        memcpy(new_string->data, old_string->data, old_string->size);
        new_string->size = old_string->size;
        new_string->data[new_string->size] = '\0';
        
        if (pool_string_free(pool, old_string) != RESULT_OK) {
            if (pool_string_free(pool, new_string) != RESULT_OK) {
                // Log error but continue with original error
            }
            RETURN_ERR("Failed to free old string");
        }
        
        *string = new_string;
    }
    
    // Perform the append
    memcpy((*string)->data + (*string)->size, src->data, src->size);
    (*string)->size += src->size;
    (*string)->data[(*string)->size] = '\0';
    
    return RESULT_OK;
}

result_t string_append_str(pool_t* pool, string_t** string, const char* str) {
    if (!pool || !string || !*string || !str) {
        RETURN_ERR("Invalid parameters");
    }
    
    uint64_t str_len = (uint64_t)strlen(str);
    uint64_t required_capacity = (*string)->size + str_len + 1; // +1 for null terminator
    
    // Check if we need to reallocate to a larger size
    if (required_capacity > (*string)->capacity) {
        string_t* old_string = *string;
        string_t* new_string;
        
        if (pool_string_alloc(pool, &new_string, required_capacity) != RESULT_OK) {
            RETURN_ERR("Failed to reallocate string for append");
        }
        
        // Copy existing data
        memcpy(new_string->data, old_string->data, old_string->size);
        new_string->size = old_string->size;
        new_string->data[new_string->size] = '\0';
        
        if (pool_string_free(pool, old_string) != RESULT_OK) {
            if (pool_string_free(pool, new_string) != RESULT_OK) {
                // Log error but continue with original error  
            }
            RETURN_ERR("Failed to free old string");
        }
        
        *string = new_string;
    }
    
    // Perform the append
    strcpy((*string)->data + (*string)->size, str);
    (*string)->size += str_len;
    
    return RESULT_OK;
}

result_t string_append_data(pool_t* pool, string_t** string, const char* data, uint64_t size) {
    if (!pool || !string || !*string || !data) {
        RETURN_ERR("Invalid parameters");
    }
    
    uint64_t required_capacity = (*string)->size + size + 1; // +1 for null terminator
    
    // Check if we need to reallocate to a larger size
    if (required_capacity > (*string)->capacity) {
        string_t* old_string = *string;
        string_t* new_string;
        
        if (pool_string_alloc(pool, &new_string, required_capacity) != RESULT_OK) {
            RETURN_ERR("Failed to reallocate string for append data");
        }
        
        // Copy existing data
        memcpy(new_string->data, old_string->data, old_string->size);
        new_string->size = old_string->size;
        new_string->data[new_string->size] = '\0';
        
        if (pool_string_free(pool, old_string) != RESULT_OK) {
            if (pool_string_free(pool, new_string) != RESULT_OK) {
                // Log error but continue with original error
            }
            RETURN_ERR("Failed to free old string");
        }
        
        *string = new_string;
    }
    
    // Perform the append
    memcpy((*string)->data + (*string)->size, data, size);
    (*string)->size += size;
    (*string)->data[(*string)->size] = '\0';
    
    return RESULT_OK;
}

result_t string_append_char(pool_t* pool, string_t** string, char c) {
    if (!pool || !string || !*string) {
        RETURN_ERR("Invalid parameters");
    }
    
    uint64_t required_capacity = (*string)->size + 2; // +1 for char, +1 for null terminator
    
    // Check if we need to reallocate to a larger size
    if (required_capacity > (*string)->capacity) {
        string_t* old_string = *string;
        string_t* new_string;
        
        if (pool_string_alloc(pool, &new_string, required_capacity) != RESULT_OK) {
            RETURN_ERR("Failed to reallocate string for append char");
        }
        
        // Copy existing data
        memcpy(new_string->data, old_string->data, old_string->size);
        new_string->size = old_string->size;
        new_string->data[new_string->size] = '\0';
        
        if (pool_string_free(pool, old_string) != RESULT_OK) {
            if (pool_string_free(pool, new_string) != RESULT_OK) {
                // Log error but continue with original error
            }
            RETURN_ERR("Failed to free old string");
        }
        
        *string = new_string;
    }
    
    // Perform the append
    (*string)->data[(*string)->size] = c;
    (*string)->size++;
    (*string)->data[(*string)->size] = '\0';
    
    return RESULT_OK;
}

int string_equal(const string_t* string1, const string_t* string2) {
    if (!string1 || !string2) {
        return 0;
    }
    
    if (string1->size != string2->size) {
        return 0;
    }
    
    return memcmp(string1->data, string2->data, string1->size) == 0;
}

int string_equal_str(const string_t* string, const char* str) {
    if (!string || !str) {
        return 0;
    }
    
    uint64_t str_len = (uint64_t)strlen(str);
    if (string->size != str_len) {
        return 0;
    }
    
    return memcmp(string->data, str, str_len) == 0;
}

int64_t string_find(const string_t* string, const char* substr) {
    if (!string || !substr || !string->data) {
        return -1;
    }
    
    char* found = strstr(string->data, substr);
    if (found == NULL) {
        return -1;
    }
    
    return found - string->data;
}

int64_t string_find_char(const string_t* string, char c) {
    if (!string || !string->data) {
        return -1;
    }
    
    char* found = strchr(string->data, c);
    if (found == NULL) {
        return -1;
    }
    
    return found - string->data;
}

int64_t string_find_from(const string_t* string, const char* substr, uint64_t pos) {
    if (!string || !substr || !string->data || pos >= string->size) {
        return -1;
    }
    
    char* found = strstr(string->data + pos, substr);
    if (found == NULL) {
        return -1;
    }
    
    return found - string->data;
}

int64_t string_find_char_from(const string_t* string, char c, uint64_t pos) {
    if (!string || !string->data || pos >= string->size) {
        return -1;
    }
    
    char* found = strchr(string->data + pos, c);
    if (found == NULL) {
        return -1;
    }
    
    return found - string->data;
}
