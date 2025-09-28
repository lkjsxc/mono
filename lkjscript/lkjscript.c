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
typedef struct primitive_t primitive_t;

enum nodetype_t {
    NODETYPE_NULL,
    NODETYPE_BLOCK,
    NODETYPE_DECL_VAR,
    NODETYPE_DECL_FN,
    NODETYPE_CALL,
    NODETYPE_RETURN,
    NODETYPE_INT,
    NODETYPE_IDENT,
    NODETYPE_ADD,
    NODETYPE_SUB,
    NODETYPE_MUL,
    NODETYPE_DIV,
    NODETYPE_MOD,
    NODETYPE_EQ,
    NODETYPE_NEQ,
    NODETYPE_LT,
    NODETYPE_LTE,
    NODETYPE_GT,
    NODETYPE_GTE,
    NODETYPE_AND,
    NODETYPE_OR,
    NODETYPE_BITAND,
    NODETYPE_BITOR,
    NODETYPE_BITXOR,
    NODETYPE_SHL,
    NODETYPE_SHR,
    NODETYPE_ASSIGN,
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

struct primitive_t {
    const char* str;
    nodetype_t type;
};

primitive_t primitive_binary_table[] = {
    {"add", NODETYPE_ADD},
    {"sub", NODETYPE_SUB},
    {"mul", NODETYPE_MUL},
    {"div", NODETYPE_DIV},
    {"mod", NODETYPE_MOD},
    {"eq", NODETYPE_EQ},
    {"neq", NODETYPE_NEQ},
    {"lt", NODETYPE_LT},
    {"lte", NODETYPE_LTE},
    {"gt", NODETYPE_GT},
    {"gte", NODETYPE_GTE},
    {"and", NODETYPE_AND},
    {"or", NODETYPE_OR},
    {"bitand", NODETYPE_BITAND},
    {"bitor", NODETYPE_BITOR},
    {"bitxor", NODETYPE_BITXOR},
    {"shl", NODETYPE_SHL},
    {"shr", NODETYPE_SHR},
    {"assign", NODETYPE_ASSIGN},
    {NULL, NODETYPE_NULL},
};

void parse_lkjxml(token_t** token_itr, node_t* parent);

bool token_equal(token_t* a, token_t* b) {
    if (a->size != b->size) {
        return false;
    }
    return strncmp(a->data, b->data, a->size) == 0;
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
        } else if (strchr("<>/.,", *src_itr) != NULL) {
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

bool parse_lkjxmltag_equal(token_t* a, token_t* b) {
    if (a == NULL || !token_equal_str(a, "<")) {
        fprintf(stderr, "parse_lkjxmltag_equal: a is NULL or not <");
        exit(1);
    }
    a = a->next;
    if (b == NULL || !token_equal_str(b, "<")) {
        fprintf(stderr, "parse_lkjxmltag_equal: b is <");
        exit(1);
    }
    b = b->next;
    while (a != NULL && b != NULL) {
        if (!token_equal(a, b)) {
            return false;
        } else if (token_equal_str(a, ">")) {
            return true;
        } else {
            a = a->next;
            b = b->next;
        }
    }
    return false;
}

bool parse_lkjxmltag_equal_str(token_t* a, const char* b) {
    if (a == NULL || !token_equal_str(a, "<")) {
        return false;
    }
    a = a->next;
    return token_equal_str(a, b);
}

bool parse_lkjxmltag_ispair(token_t* a, token_t* b) {
    if (a == NULL) {
        return false;
    }
    a = a->next;
    if (b == NULL) {
        return false;
    }
    b = b->next;
    if (b == NULL) {
        return false;
    }
    b = b->next;
    while (a != NULL && b != NULL) {
        if (!token_equal(a, b)) {
            return false;
        } else if (token_equal_str(a, ">")) {
            return true;
        } else {
            a = a->next;
            b = b->next;
        }
    }
    return false;
}

void parse_primary(token_t** token_itr, node_t* parent) {
    if (strchr("0123456789", (*token_itr)->data[0]) != NULL) {
        node_t* node = parse_new(NODETYPE_INT);
        node->value.i64 = token_to_i64(*token_itr);
        parse_pushback(parent, node);
        *token_itr = (*token_itr)->next;
    } else {
        fprintf(stderr, "Expected primary expression but got '%.*s'\n next: '%.*s'\n", (*token_itr)->size, (*token_itr)->data, (*token_itr)->next != NULL ? (*token_itr)->next->size : 0, (*token_itr)->next != NULL ? (*token_itr)->next->data : "");
        exit(1);
    }
}

void parse_skip_lkjxmltag(token_t** token_itr) {
    while (!token_equal_str(*token_itr, ">")) {
        *token_itr = (*token_itr)->next;
    }
    *token_itr = (*token_itr)->next;
}

void parse_lkjxml(token_t** token_itr, node_t* parent) {
    if (!token_equal_str(*token_itr, "<")) {
        parse_primary(token_itr, parent);
        return;
    }
    token_t* base = *token_itr;
    primitive_t* primitive_found = NULL;
    for (primitive_t* primitive_itr = primitive_binary_table; primitive_itr->str != NULL; primitive_itr++) {
        if (parse_lkjxmltag_equal_str(*token_itr, primitive_itr->str)) {
            primitive_found = primitive_itr;
            break;
        }
    }
    if (parse_lkjxmltag_equal_str(*token_itr, "return")) {
        node_t* node = parse_new(NODETYPE_RETURN);
        parse_pushback(parent, node);
        parse_skip_lkjxmltag(token_itr);
        parse_lkjxml(token_itr, node);
        if (!parse_lkjxmltag_ispair(base, *token_itr)) {
            fprintf(stderr, "Expected </%.*s> but got '%.*s'\n", base->next->size, base->next->data, (*token_itr)->size, (*token_itr)->data);
            exit(1);
        }
        parse_skip_lkjxmltag(token_itr);
    } else if (parse_lkjxmltag_equal_str(*token_itr, "let")) {
        fprintf(stderr, "let tag not implemented\n");
        exit(1);
    } else if (primitive_found != NULL) {
        node_t* node = parse_new(primitive_found->type);
        parse_pushback(parent, node);
        parse_skip_lkjxmltag(token_itr);
        parse_lkjxml(token_itr, node);
        parse_lkjxml(token_itr, node);
        if (!parse_lkjxmltag_ispair(base, *token_itr)) {
            fprintf(stderr, "Expected </%.*s> but got '%.*s'\n", base->next->size, base->next->data, (*token_itr)->size, (*token_itr)->data);
            exit(1);
        }
        parse_skip_lkjxmltag(token_itr);
    } else {
        fprintf(stderr, "Unknown tag: '%.*s'\n", base->size, base->data);
        exit(1);
    }
}

node_t* parse(token_t* token) {
    node_t* node_root = parse_new(NODETYPE_BLOCK);
    token_t* token_itr = token;
    parse_lkjxml(&token_itr, node_root);
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
            // Placeholder for return value
            return eval(node->child_begin);
        }
        case NODETYPE_ADD: {
            return (object_t){.type = NODETYPE_INT, .value = {.i64 = eval(node->child_begin).value.i64 + eval(node->child_rbegin).value.i64}};
        }
        default:
            fprintf(stderr, "TODO: Eval\n");
            exit(1);
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