#include "lkjstring.h"

result_t string_init(string_t* string, char* data, uint64_t capacity) {
    string->data = data;
    string->capacity = capacity;
    string_clear(string);
    return RESULT_OK;
}

result_t string_assign(string_t* string, const char* str) {
    size_t str_len = strlen(str);
    if (str_len >= string->capacity) {
        return RESULT_ERR;
    }
    strcpy(string->data, str);
    string->size = str_len;
    return RESULT_OK;
}

result_t string_copy(string_t* dst, const string_t* src) {
    if (src->size >= dst->capacity) {
        return RESULT_ERR;
    }
    strcpy(dst->data, src->data);
    dst->size = src->size;
    return RESULT_OK;
}

void string_clear(string_t* string) {
    string->size = 0;
    string->data[0] = '\0';
}

result_t string_append(string_t* string, const char* str) {
    size_t str_len = strlen(str);
    if (string->size + str_len >= string->capacity) {
        return RESULT_ERR;
    }
    strcat(string->data, str);
    string->size += str_len;
    return RESULT_OK;
}

result_t string_append_data(string_t* string, const char* data, uint64_t size) {
    if (string->size + size >= string->capacity) {
        return RESULT_ERR;
    }
    memcpy(string->data + string->size, data, size);
    string->size += size;
    string->data[string->size] = '\0';
    return RESULT_OK;
}

result_t string_append_char(string_t* string, char c) {
    if (string->size + 1 >= string->capacity) {
        return RESULT_ERR;
    }
    string->data[string->size] = c;
    string->size++;
    string->data[string->size] = '\0';
    return RESULT_OK;
}

int string_equal(const string_t* string1, const string_t* string2) {
    if (string1->size != string2->size) {
        return 0;
    }
    return strcmp(string1->data, string2->data) == 0;
}

int string_equal_str(const string_t* string, const char* str) {
    return strcmp(string->data, str) == 0;
}

int64_t string_find(const string_t* string, const char* substr) {
    char* found = strstr(string->data, substr);
    if (found == NULL) {
        return -1;
    }
    return found - string->data;
}

int64_t string_find_char(const string_t* string, char c) {
    char* found = strchr(string->data, c);
    if (found == NULL) {
        return -1;
    }

    return found - string->data;
}

int64_t string_find_from(const string_t* string, const char* substr, uint64_t pos) {
    if (pos >= string->size) {
        return -1;
    }
    char* found = strstr(string->data + pos, substr);
    if (found == NULL) {
        return -1;
    }
    return found - string->data;
}

int64_t string_find_char_from(const string_t* string, char c, uint64_t pos) {
    if (pos >= string->size) {
        return -1;
    }
    char* found = strchr(string->data + pos, c);
    if (found == NULL) {
        return -1;
    }
    return found - string->data;
}