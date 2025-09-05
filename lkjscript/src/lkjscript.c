#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LKJSCRIPT_SIZE (1024 * 1024 * 16)
#define LKJSCRIPT_SRCPATH "/data/main.lkjscript"

typedef enum result_t result_t;
typedef enum node_type_t node_type_t;
typedef union uni64_t uni64_t;
typedef struct token_t token_t;
typedef struct node_t node_t;
typedef struct parse_stat_t parse_stat_t;
typedef struct lkjscript_t lkjscript_t;

enum result_t {
    RESULT_OK,
    RESULT_ERROR
};

enum node_type_t {
    NODE_TYPE_NULL,
    NODE_TYPE_BLOCK,
    NODE_TYPE_NUMBER,
    NODE_TYPE_IDENT,
    NODE_TYPE_STR,
    NODE_TYPE_ADD
};

union uni64_t {
    uint64_t u64;
    int64_t i64;
    double f64;
    node_type_t type;
    char* str;
};

struct token_t {
    char* data;
    uint64_t size;
};

struct node_t {
    uni64_t type;
    uni64_t value;
    node_t* next;
    node_t* parent;
    node_t* child_begin;
    node_t* child_rbegin;
};

struct parse_stat_t {
    result_t result;
    char* token_itr;
    node_t* node_itr;
    node_t* parent;
};

struct lkjscript_t {
    int8_t data[LKJSCRIPT_SIZE];
};

static lkjscript_t lkjscript;

__attribute__((warn_unused_result)) static result_t compile_readsrc(int8_t** itr) {
    FILE* f = fopen(LKJSCRIPT_SRCPATH, "r");
    if (f == NULL) {
        fprintf(stderr, "Failed to open source file: %s\n", LKJSCRIPT_SRCPATH);
        return RESULT_ERROR;
    }
    fseek(f, 0, SEEK_END);
    int64_t n = ftell(f);
    fseek(f, 0, SEEK_SET);
    fread(*itr, 1, n, f);
    (*itr)[n] = '\n';
    (*itr)[n + 1] = '\0';
    fclose(f);
    *itr += n + 2;
    return RESULT_OK;
}

__attribute__((warn_unused_result)) static uint64_t compile_tokenize_isspace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

__attribute__((warn_unused_result)) static uint64_t compile_tokenize_isdigit(char c) {
    return c >= '0' && c <= '9';
}

__attribute__((warn_unused_result)) static uint64_t compile_tokenize_isalpha(char c) {
    // return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    return (c >= 'a' && c <= 'z') || c == '_';
}

__attribute__((warn_unused_result)) static uint64_t compile_tokenize_issymbol(char c) {
    // return strchr("+-*/&><=!%^|;(){},[]~.#", c) != NULL;
    return strchr("+-*/&><=!%^|;(),~.", c) != NULL;
}

__attribute__((warn_unused_result)) static result_t compile_tokenize_next(char** itr) {
    if ((**itr & 0x80) == 0) {
        (*itr)++;
    } else if ((**itr & 0xE0) == 0xC0) {
        (*itr) += 2;
    } else if ((**itr & 0xF0) == 0xE0) {
        (*itr) += 3;
    } else if ((**itr & 0xF8) == 0xF0) {
        (*itr) += 4;
    } else {
        fprintf(stderr, "Invalid UTF-8 sequence\n");
        return RESULT_ERROR;
    }
    return RESULT_OK;
}

void compile_tokenize_push(int8_t** itr, char* data, uint64_t size) {
    token_t* token = (token_t*)(*itr);
    token->data = data;
    token->size = size;
    *itr += sizeof(token_t);
}

__attribute__((warn_unused_result)) static result_t compile_tokenize(int8_t** itr, char* src) {
    char* src_itr = src;
    while (*src_itr != '\0') {
        if (compile_tokenize_isspace(*src_itr)) {
            while (compile_tokenize_isspace(*src_itr)) {
                if (*src_itr == '\n') {
                    compile_tokenize_push(itr, src_itr, 1);
                }
                src_itr++;
            }
        } else if (strncmp(src_itr, "// ", 3) == 0) {
            while (*src_itr != '\n') {
                if (compile_tokenize_next(&src_itr) != RESULT_OK) {
                    fprintf(stderr, "Invalid UTF-8 sequence in comment\n");
                    return RESULT_ERROR;
                }
            }
        } else if (*src_itr == '\"' || *src_itr == '\'') {
            char quote = *src_itr;
            char* start = src_itr;
            src_itr++;
            while (*src_itr != quote) {
                if (*src_itr == '\0') {
                    fprintf(stderr, "Unterminated string literal\n");
                    return RESULT_ERROR;
                }
                if (compile_tokenize_next(&src_itr) != RESULT_OK) {
                    fprintf(stderr, "Invalid UTF-8 sequence in string literal\n");
                    return RESULT_ERROR;
                }
            }
            src_itr++;
            compile_tokenize_push(itr, start, src_itr - start);
        } else if (compile_tokenize_issymbol(*src_itr)) {
            const char* symbol[] = {"<<=", ">>=", "==", "!=", "<=", ">=", "->", "+=",
                                    "-=", "*=", "/=", "%=", "&=", "|=", "^=", "&&",
                                    "||", "<<", ">>", "+", "-", "*", "/", "&", ">", "<", "=", "!", "%", "^", "|", ";", "(", ")", ",", "~", "."};
            for (uint64_t i = 0; i < sizeof(symbol) / sizeof(symbol[0]); i++) {
                uint64_t len = strlen(symbol[i]);
                if (strncmp(src_itr, symbol[i], len) == 0) {
                    compile_tokenize_push(itr, src_itr, len);
                    src_itr += len;
                    break;
                }
            }
        } else if (compile_tokenize_isdigit(*src_itr)) {
            char* start = src_itr;
            while (compile_tokenize_isdigit(*src_itr)) {
                src_itr++;
            }
            compile_tokenize_push(itr, start, src_itr - start);
        } else if (compile_tokenize_isalpha(*src_itr)) {
            char* start = src_itr;
            while (compile_tokenize_isalpha(*src_itr) || compile_tokenize_isdigit(*src_itr)) {
                src_itr++;
            }
            compile_tokenize_push(itr, start, src_itr - start);
        } else {
            fprintf(stderr, "Unknown character: %c\n", *src_itr);
            return RESULT_ERROR;
        }
    }
    compile_tokenize_push(itr, NULL, 0);
    return RESULT_OK;
}

__attribute__((warn_unused_result)) static result_t lkjscript_init() {
    int8_t* itr = lkjscript.data;
    char* src = (char*)itr;
    if (compile_readsrc(&itr) != RESULT_OK) {
        fprintf(stderr, "Failed to read source file: %s\n", LKJSCRIPT_SRCPATH);
        return RESULT_ERROR;
    }
    token_t* token = (token_t*)itr;
    if (compile_tokenize(&itr, src) != RESULT_OK) {
        fprintf(stderr, "Failed to tokenize source code\n");
        return RESULT_ERROR;
    }

    for (token_t* t = token; t->data != NULL; t++) {
        printf("TOKEN: %.*s\n", (int)t->size, t->data);
    }

    // node_t* root = NULL;
    // if (compile_parse(&itr, token, &root) != RESULT_OK) {
    //     fprintf(stderr, "Failed to parse source code\n");
    //     return RESULT_ERROR;
    // }
    return RESULT_OK;
}

int main() {
    if (lkjscript_init() != RESULT_OK) {
        fprintf(stderr, "Failed to initialize lkjscript\n");
        return 1;
    }
    fflush(stdout);
    return 0;
}
