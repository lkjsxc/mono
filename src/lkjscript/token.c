#include "lkjscript.h"

int64_t token_eq(token_t* token1, token_t* token2) {
    if(token1 == NULL || token2 == NULL) {
        return 0;
    }
    if (token1->size != token2->size) {
        return 0;
    }
    for(int64_t i = 0; i < token1->size; i++) {
        if (token1->data[i] != token2->data[i]) {
            return 0;
        }
    }
    return 1;
}

int64_t token_eqstr(token_t* token, const char* str) {
    int64_t len = 0;
    while(str[len] != '\0') {
        len++;
    }
    token_t token2 = {.data = str, .size = len};
    return token_eq(token, &token2);
}

int64_t token_isdigit(token_t* token) {
    for(int64_t i = 0; i < token->size; i++) {
        if (token->data[i] < '0' || token->data[i] > '9') {
            return 0;
        }
    }
    return 1;
}

int64_t token_ischar(token_t* token) {
    if(token->data[0] == '"') {
        return 1;
    }
    return 0;
}