#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum nodetype_t nodetype_t;
typedef union uni64_t uni64_t;
typedef struct token_t token_t;
typedef struct node_t node_t;
typedef struct object_t object_t;
typedef struct keyword_t keyword_t;

enum nodetype_t {
    NODETYPE_NULL,
    NODETYPE_BLOCK,
    NODETYPE_DECL_VAR,
    NODETYPE_DECL_FN,
    NODETYPE_CALL,
    NODETYPE_RETURN,
    NODETYPE_INT,
    NODETYPE_IDENT,
};

union uni64_t {
    uint64_t u64;
    int64_t i64;
    double f64;
    const char* str;
    token_t* token;
    node_t* node;
};

struct token_t {
    const char* data;
    uint32_t size;
    token_t* next;
};

struct node_t {
    nodetype_t type;
    uni64_t value;
    node_t* next;
    node_t* parent;
    node_t* child_begin;
    node_t* child_rbegin;
};

struct object_t {
    nodetype_t type;
    uni64_t value;
};

struct keyword_t {
    const char* str;
    nodetype_t type;
};

keyword_t keyword_table[] = {
    {"return", NODETYPE_RETURN},
    {NULL, NODETYPE_NULL},
};

void parse_expr(token_t** token_itr, node_t* parent);

bool token_equal(token_t* token, token_t* other) {
    if (token->size != other->size) {
        return false;
    }
    return strncmp(token->data, other->data, token->size) == 0;
}

bool token_equal_str(token_t* token, const char* str) {
    uint32_t str_size = strlen(str);
    if (token->size != str_size) {
        return false;
    }
    return strncmp(token->data, str, str_size) == 0;
}

int64_t token_to_i64(token_t* token) {
    int64_t value = 0;
    for (uint32_t i = 0; i < token->size; i++) {
        value = value * 10 + (token->data[i] - '0');
    }
    return value;
}

const char* readsrc(const char* path) {
    FILE* file = fopen(path, "r");
    fseek(file, 0, SEEK_END);
    long n = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buf = malloc(n + 2);
    fread(buf, 1, n, file);
    buf[n + 0] = '\n';
    buf[n + 1] = '\0';
    fclose(file);
    return buf;
}

void tokenize_pushback(token_t** token_rbegin, const char* data, uint32_t size) {
    token_t* token_new = malloc(sizeof(token_t));
    *token_new = (token_t){.data = data, .size = size, .next = NULL};
    (*token_rbegin)->next = token_new;
    *token_rbegin = token_new;
}

token_t* tokenize(const char* src) {
    token_t token_tmp = {.data = NULL, .size = 0, .next = NULL};
    token_t* token_rbegin = &token_tmp;
    const char* src_itr = src;
    while (*src_itr != '\0') {
        if (*src_itr == ' ' || *src_itr == '\t' || *src_itr == '\n' || *src_itr == '\r') {
            src_itr += 1;
        } else if (*src_itr == '/' && *(src_itr + 1) == '/') {
            while (*src_itr != '\n') {
                src_itr += 1;
            }
        } else if (*src_itr == '(' || *src_itr == ')' || *src_itr == ',' || *src_itr == '.') {
            tokenize_pushback(&token_rbegin, src_itr, 1);
            src_itr += 1;
        } else if (strchr("0123456789abcdefghijklmnopqrstuvwxyz_", *src_itr) != NULL) {
            const char* ident_begin = src_itr;
            while (strchr("0123456789abcdefghijklmnopqrstuvwxyz_", *src_itr) != NULL) {
                src_itr += 1;
            }
            tokenize_pushback(&token_rbegin, ident_begin, src_itr - ident_begin);
        } else {
            fprintf(stderr, "Unknown token: '%c'\n", *src_itr);
            exit(1);
        }
    }
    return token_tmp.next;
}

node_t* parse_new(nodetype_t type) {
    node_t* node = malloc(sizeof(node_t));
    *node = (node_t){.type = type, .value = {.u64 = 0}, .next = NULL, .parent = NULL, .child_begin = NULL, .child_rbegin = NULL};
    return node;
}

void parse_pushback(node_t* parent, node_t* node) {
    node->parent = parent;
    if (parent->child_begin == NULL) {
        parent->child_begin = node;
    } else {
        parent->child_rbegin->next = node;
    }
    parent->child_rbegin = node;
}

void parse_primary(token_t** token_itr, node_t* parent) {
    keyword_t* keyword_found = NULL;
    for (keyword_t* keyword_itr = keyword_table; keyword_itr->str != NULL; keyword_itr++) {
        if (token_equal_str(*token_itr, keyword_itr->str)) {
            keyword_found = keyword_itr;
            break;
        }
    }
    if (token_equal_str(*token_itr, "(")) {
        node_t* node = parse_new(NODETYPE_BLOCK);
        parse_pushback(parent, node);
        *token_itr = (*token_itr)->next;
        while (!token_equal_str(*token_itr, ")")) {
            parse_expr(token_itr, node);
        }
        *token_itr = (*token_itr)->next;
    } else if (keyword_found != NULL) {
        node_t* node = parse_new(keyword_found->type);
        parse_pushback(parent, node);
        *token_itr = (*token_itr)->next;
        parse_expr(token_itr, node);
    } else if (strchr("0123456789", (*token_itr)->data[0]) != NULL) {
        node_t* node = parse_new(NODETYPE_INT);
        node->value.i64 = token_to_i64(*token_itr);
        parse_pushback(parent, node);
        *token_itr = (*token_itr)->next;
    } else {
        fprintf(stderr, "Expected primary expression but got '%.*s'\n", (*token_itr)->size, (*token_itr)->data);
        exit(1);
    }
}

void parse_expr(token_t** token_itr, node_t* parent) {
    if (token_equal_str(*token_itr, "(")) {
        node_t* node = parse_new(NODETYPE_BLOCK);
        parse_pushback(parent, node);
        *token_itr = (*token_itr)->next;
        while (!token_equal_str(*token_itr, ")")) {
            parse_expr(token_itr, node);
        }
        *token_itr = (*token_itr)->next;
    } else {
        parse_primary(token_itr, parent);
    }
}

node_t* parse(token_t* token) {
    node_t* node_root = parse_new(NODETYPE_BLOCK);
    token_t* token_itr = token;
    while (token_itr != NULL) {
        parse_expr(&token_itr, node_root);
    }
    return node_root;
}

object_t eval(node_t* node) {
    switch (node->type) {
        case NODETYPE_BLOCK: {
            object_t result = {.type = NODETYPE_NULL, .value = {.u64 = 0}};
            for (node_t* child = node->child_begin; child != NULL; child = child->next) {
                result = eval(child);
            }
            return result;
        }
        case NODETYPE_INT: {
            return (object_t){.type = NODETYPE_INT, .value = {.i64 = node->value.i64}};
        }
        case NODETYPE_RETURN: {
            return eval(node->child_begin);
        }
        default: {
            fprintf(stderr, "Evaluation not implemented for node type %d\n", node->type);
            exit(1);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        return 1;
    }
    const char* src = readsrc(argv[1]);
    token_t* token = tokenize(src);
    node_t* node = parse(token);
    object_t result = eval(node);
    printf("result: %ld\n", result.value.i64);
    return 0;
}