#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum type_t type_t;
typedef union uni64_t uni64_t;
typedef struct token_t token_t;
typedef struct node_t node_t;
typedef struct parseresult_t parseresult_t;
typedef struct table_operator_t table_operator_t;

enum type_t {
    TYPE_NULL,
    TYPE_NONE,
    TYPE_ROOT,
    TYPE_BLOCK,
    TYPE_DECL_VAR,
    TYPE_DECL_FN,
    TYPE_CALL,
    TYPE_RETURN,
    TYPE_IF,
    TYPE_IFELSE,
    TYPE_LOOP_START,
    TYPE_LOOP_END,
    TYPE_LOOP_CONTINUE,
    TYPE_LOOP_BREAK,
    TYPE_INT,
    TYPE_STR,
    TYPE_IDENT,
    TYPE_ADD,
    TYPE_SUB,
    TYPE_MUL,
    TYPE_DIV,
    TYPE_MOD,
    TYPE_EQ,
    TYPE_NEQ,
    TYPE_LT,
    TYPE_LTE,
    TYPE_GT,
    TYPE_GTE,
    TYPE_AND,
    TYPE_OR,
    TYPE_BITAND,
    TYPE_BITOR,
    TYPE_BITXOR,
    TYPE_SHL,
    TYPE_SHR,
    TYPE_ASSIGN,
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
    type_t type;
    uni64_t value;
    node_t* next;
    node_t* parent;
    node_t* child_begin;
    node_t* child_rbegin;
};

typedef struct parseresult_t {
    token_t* token;
    node_t* node;
} parseresult_t;

struct table_operator_t {
    const char* data;
    type_t type;
};

table_operator_t table_operator[] = {
    {"+", TYPE_ADD},
    {"-", TYPE_SUB},
    {"*", TYPE_MUL},
    {"/", TYPE_DIV},
    {"%", TYPE_MOD},
    {"==", TYPE_EQ},
    {"!=", TYPE_NEQ},
    {"<", TYPE_LT},
    {"<=", TYPE_LTE},
    {">", TYPE_GT},
    {">=", TYPE_GTE},
    {"&&", TYPE_AND},
    {"||", TYPE_OR},
    {"&", TYPE_BITAND},
    {"|", TYPE_BITOR},
    {"^", TYPE_BITXOR},
    {"<<", TYPE_SHL},
    {">>", TYPE_SHR},
    {"=", TYPE_ASSIGN},
    {NULL, TYPE_NULL},
};

const char* table_sign[] = {"==", "!=", "<=", ">=", "&&", "||", "<<", ">>", ">", "<", "+", "-", "*", "/", "%", "&", "|", "^", "=", "(", ")", ",", ";", "\n", NULL};

parseresult_t parse_expr(token_t* token, node_t* parent);
parseresult_t parse_stmt(token_t* token, node_t* parent);

uint64_t random_u64() {
    static uint64_t x = 987234789;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return x;
}

bool token_equal(token_t* a, token_t* b) {
    if (a->size != b->size) {
        return false;
    }
    return strncmp(a->data, b->data, a->size) == 0;
}

bool token_equal_str(token_t* a, const char* b) {
    uint32_t b_size = strlen(b);
    if (a->size != b_size) {
        return false;
    }
    return strncmp(a->data, b, a->size) == 0;
}

token_t* token_skiplinebreak(token_t* token) {
    while (token != NULL && token_equal_str(token, "\n")) {
        token = token->next;
    }
    return token;
}

int64_t token_to_i64(token_t* token) {
    int64_t value = 0;
    for (uint32_t i = 0; i < token->size; i++) {
        value = value * 10 + (token->data[i] - '0');
    }
    return value;
}

node_t* node_new(type_t type, node_t* parent) {
    node_t* new_node = malloc(sizeof(node_t));
    *new_node = (node_t){.type = type, .value = {.u64 = 0}, .next = NULL, .parent = parent, .child_begin = NULL, .child_rbegin = NULL};
    return new_node;
}

void node_addmember(node_t* parent, node_t* child) {
    child->parent = parent;
    if (parent->child_begin == NULL) {
        parent->child_begin = child;
    } else {
        parent->child_rbegin->next = child;
    }
    parent->child_rbegin = child;
    for (node_t* itr = child->next; itr != NULL; itr = itr->next) {
        itr->parent = parent;
        parent->child_rbegin->next = itr;
        parent->child_rbegin = itr;
    }
}

node_t* node_find(node_t* begin, type_t type) {
    node_t* itr = begin;
    if (itr == NULL) {
        return NULL;
    }
    while (1) {
        if (itr->type == type) {
            return itr;
        }
        if (itr->next != NULL) {
            itr = itr->next;
        } else if (itr->parent != NULL) {
            itr = itr->parent;
        } else {
            return NULL;
        }
    }
}

node_t* node_find_var(node_t* begin, token_t* token) {
    node_t* itr = begin;
    if (itr == NULL) {
        return NULL;
    }
    while (1) {
        if (itr->type == TYPE_DECL_VAR) {
            if (token_equal(itr->value.token, token)) {
                return itr;
            }
        }
        if (itr->next != NULL) {
            itr = itr->next;
        } else if (itr->parent != NULL) {
            itr = itr->parent;
        } else {
            return NULL;
        }
    }
}

node_t* node_find_fn(node_t* begin, token_t* token) {
    node_t* itr = begin;
    if (itr == NULL) {
        return NULL;
    }
    while (1) {
        if (itr->type == TYPE_DECL_FN) {
            if (token_equal(itr->value.token, token)) {
                return itr;
            }
        }
        if (itr->next != NULL) {
            itr = itr->next;
        } else if (itr->parent != NULL) {
            itr = itr->parent;
        } else {
            return NULL;
        }
    }
}

char* file_read(const char* filename) {
    FILE* f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* dst = malloc(n + 2);
    fread(dst, 1, n, f);
    dst[n + 0] = '\n';
    dst[n + 1] = '\0';
    fclose(f);
    return dst;
}

void tokenize_push(token_t** rbegin, const char* data, uint32_t size) {
    token_t* new_token = malloc(sizeof(token_t));
    *new_token = (token_t){.data = data, .size = size, .next = NULL};
    (*rbegin)->next = new_token;
    *rbegin = new_token;
}

token_t* tokenize(const char* src) {
    token_t* begin = malloc(sizeof(token_t));
    token_t* rbegin = begin;
    *begin = (token_t){.data = "\n", .size = 1, .next = NULL};
    const char* src_itr = src;
    while (*src_itr != '\0') {
        const char* sign_data = NULL;
        uint32_t sign_size = 0;
        for (const char** sign_itr = table_sign; *sign_itr != NULL; sign_itr++) {
            sign_size = strlen(*sign_itr);
            if (strncmp(src_itr, *sign_itr, sign_size) == 0) {
                sign_data = *sign_itr;
                sign_size = sign_size;
                break;
            }
        }
        if (*src_itr == '/' && *(src_itr + 1) == '/') {
            while (*src_itr != '\n') {
                src_itr++;
            }
            continue;
        } else if (sign_data != NULL) {
            tokenize_push(&rbegin, sign_data, sign_size);
            src_itr += sign_size;
            continue;
        } else if (*src_itr == ' ' || *src_itr == '\t') {
            while (*src_itr == ' ' || *src_itr == '\t') {
                src_itr++;
            }
            continue;
        } else if (strchr("abcdefghijklmnopqrstuvwxyz0123456789_", *src_itr) != NULL) {
            const char* id_begin = src_itr;
            while (strchr("abcdefghijklmnopqrstuvwxyz0123456789_", *src_itr) != NULL) {
                src_itr++;
            }
            tokenize_push(&rbegin, id_begin, src_itr - id_begin);
            continue;
        } else {
            fprintf(stderr, "Unknown token: '%c'\n", *src_itr);
            exit(1);
        }
    }
    return begin;
}

parseresult_t parse_primary(token_t* token, node_t* parent) {
    if (token_equal_str(token, "(")) {
        parseresult_t expr = parse_expr(token->next, parent);
        return (parseresult_t){.token = expr.token->next, .node = expr.node};
    } else if (strchr("0123456789", token->data[0]) != NULL) {
        node_t* node = node_new(TYPE_INT, parent);
        node->value.i64 = token_to_i64(token);
        return (parseresult_t){.token = token->next, .node = node};
    } else if (strchr("abcdefghijklmnopqrstuvwxyz_", token->data[0]) != NULL) {
        node_t* node = node_new(TYPE_IDENT, parent);
        node->value.node = node_find_var(parent, token);
        return (parseresult_t){.token = token->next, .node = node};
    } else {
        fprintf(stderr, "Expected primary expression but got '%.*s'\n next: '%.*s'\n", token->size, token->data, token->next != NULL ? token->next->size : 0, token->next != NULL ? token->next->data : "");
        exit(1);
    }
}

parseresult_t parse_binary(token_t* token, node_t* parent) {
    parseresult_t left = parse_primary(token, parent);
    for (table_operator_t* operator_itr = table_operator; operator_itr->data != NULL; operator_itr++) {
        if (token_equal_str(left.token, operator_itr->data)) {
            parseresult_t right = parse_primary(left.token->next, parent);
            node_t* operator = node_new(operator_itr->type, parent);
            node_addmember(operator, left.node);
            node_addmember(operator, right.node);
            return (parseresult_t){.token = right.token, .node = operator};
        }
    }
    return left;
}

parseresult_t parse_expr(token_t* token, node_t* parent) {
    return parse_binary(token, parent);
}

parseresult_t parse_stmt(token_t* token, node_t* parent) {
    if (token_equal_str(token, "(")) {
        node_t* node = node_new(TYPE_BLOCK, parent);
        token = token_skiplinebreak(token->next);
        while (!token_equal_str(token, ")")) {
            parseresult_t stmt = parse_stmt(token, node);
            node_addmember(node, stmt.node);
            token = token_skiplinebreak(stmt.token);
        }
        return (parseresult_t){.token = token->next, .node = node};
    } else if (token_equal_str(token, "let")) {
        node_t* decl = node_new(TYPE_DECL_VAR, parent);
        token = token->next;
        decl->value.token = token;
        if (token_equal_str(token->next, "=")) {
            node_t* assign = node_new(TYPE_ASSIGN, parent);
            parseresult_t lhs = parse_primary(token, assign);
            parseresult_t rhs = parse_expr(token->next->next, assign);
            decl->next = assign;
            node_addmember(assign, lhs.node);
            node_addmember(assign, rhs.node);
        }
        return (parseresult_t){.token = token->next, .node = decl};
    } else if (token_equal_str(token, "return")) {
        node_t* node = node_new(TYPE_RETURN, parent);
        parseresult_t expr = parse_expr(token->next, node);
        node_addmember(node, expr.node);
        return (parseresult_t){.token = expr.token, .node = node};
    } else {
        return parse_expr(token, parent);
    }
}

node_t* parse(token_t* token) {
    node_t* root = malloc(sizeof(node_t));
    *root = (node_t){.type = TYPE_ROOT, .value = {.u64 = 0}, .next = NULL, .parent = NULL, .child_begin = NULL, .child_rbegin = NULL};
    token_t* token_itr = token;
    token_itr = token_skiplinebreak(token_itr);
    while (token_itr != NULL) {
        parseresult_t result = parse_stmt(token_itr, root);
        node_addmember(root, result.node);
        token_itr = token_skiplinebreak(result.token);
    }
    return root;
}

int64_t eval_node(node_t* node) {
    switch (node->type) {
        case TYPE_ROOT:
        case TYPE_BLOCK: {
            int64_t last_value = 0;
            node_t* child = node->child_begin;
            while (child != NULL) {
                last_value = eval_node(child);
                child = child->next;
            }
            return last_value;
        }
        default:{
            fprintf(stderr, "Evaluation not implemented for node type %d\n", node->type);
            exit(1);
        }
    }
}

int64_t eval(node_t* node) {
    return eval_node(node);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <name>\n", argv[0]);
        return 1;
    }
    char* src = file_read(argv[1]);
    token_t* token = tokenize(src);
    node_t* node = parse(token);
    eval(node);
    return 0;
}