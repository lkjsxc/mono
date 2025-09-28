#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum node_type_t node_type_t;
typedef union uni64_t uni64_t;
typedef struct token_t token_t;
typedef struct node_t node_t;
typedef struct varinfo_t varinfo_t;
typedef struct fninfo_t fninfo_t;
typedef struct structinfo_t structinfo_t;
typedef struct object_t object_t;
typedef struct primitive_t primitive_t;

enum node_type_t {
    NODE_TYPE_NULL,
    NODE_TYPE_BLOCK,
    NODE_TYPE_DECL_VAR,
    NODE_TYPE_DECL_FN,
    NODE_TYPE_DECL_STRUCT,
    NODE_TYPE_CALL,
    NODE_TYPE_RETURN,
    NODE_TYPE_I64,
    NODE_TYPE_IDENT,
    NODE_TYPE_ADD,
    NODE_TYPE_SUB,
    NODE_TYPE_MUL,
    NODE_TYPE_DIV,
    NODE_TYPE_MOD,
    NODE_TYPE_EQ,
    NODE_TYPE_NEQ,
    NODE_TYPE_LT,
    NODE_TYPE_LTE,
    NODE_TYPE_GT,
    NODE_TYPE_GTE,
    NODE_TYPE_AND,
    NODE_TYPE_OR,
    NODE_TYPE_BITAND,
    NODE_TYPE_BITOR,
    NODE_TYPE_BITXOR,
    NODE_TYPE_SHL,
    NODE_TYPE_SHR,
    NODE_TYPE_ASSIGN,
};

union uni64_t {
    uint64_t u64;
    int64_t i64;
    double f64;
    const char* str;
    token_t* token;
    node_t* node;
    varinfo_t* var;
    fninfo_t* fn;
    structinfo_t* structinfo;
};

struct token_t {
    const char* data;
    uint32_t size;
    token_t* next;
};

struct node_t {
    node_type_t type;
    uni64_t value;
    node_t* next;
    node_t* parent;
    node_t* child_begin;
    node_t* child_rbegin;
};

struct varinfo_t {
    token_t* token;
    node_t* type;
    int64_t offset;
};

struct fninfo_t {
    token_t* token;
    node_t* type;
    node_t* arg_begin;
    node_t* arg_rbegin;
    node_t* body;
    int64_t size;
};

struct structinfo_t {
    token_t* token;
    node_t* type;
    node_t* member_begin;
    node_t* member_rbegin;
    int64_t size;
};

struct object_t {
    node_type_t type;
    uni64_t value;
};

struct primitive_t {
    const char* str;
    node_type_t type;
};

static primitive_t primitive_table[] = {
    {"return", NODE_TYPE_RETURN},
    {"add", NODE_TYPE_ADD},
    {"sub", NODE_TYPE_SUB},
    {"mul", NODE_TYPE_MUL},
    {"div", NODE_TYPE_DIV},
    {"mod", NODE_TYPE_MOD},
    {"eq", NODE_TYPE_EQ},
    {"neq", NODE_TYPE_NEQ},
    {"lt", NODE_TYPE_LT},
    {"lte", NODE_TYPE_LTE},
    {"gt", NODE_TYPE_GT},
    {"gte", NODE_TYPE_GTE},
    {"and", NODE_TYPE_AND},
    {"or", NODE_TYPE_OR},
    {"bitand", NODE_TYPE_BITAND},
    {"bitor", NODE_TYPE_BITOR},
    {"bitxor", NODE_TYPE_BITXOR},
    {"shl", NODE_TYPE_SHL},
    {"shr", NODE_TYPE_SHR},
    {"assign", NODE_TYPE_ASSIGN},
    {NULL, NODE_TYPE_NULL},
};

static bool token_equal(token_t* a, token_t* b) {
    if (a->size != b->size) {
        return false;
    }
    return strncmp(a->data, b->data, a->size) == 0;
}

static bool token_equal_str(token_t* token, const char* str) {
    uint32_t str_size = strlen(str);
    if (token->size != str_size) {
        return false;
    }
    return strncmp(token->data, str, str_size) == 0;
}

static int64_t token_to_i64(token_t* token) {
    int64_t value = 0;
    for (uint32_t i = 0; i < token->size; i++) {
        value = value * 10 + (token->data[i] - '0');
    }
    return value;
}

static const char* readsrc(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open %s\n", path);
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    long n = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buf = malloc(n + 4);
    buf[0] = '(';
    fread(buf + 1, 1, n, file);
    memcpy(buf + n + 1, ")\n\0", 3);
    fclose(file);
    return buf;
}

static void tokenize_pushback(token_t** token_rbegin, const char* data, uint32_t size) {
    token_t* token_new = malloc(sizeof(token_t));
    *token_new = (token_t){.data = data, .size = size, .next = NULL};
    (*token_rbegin)->next = token_new;
    *token_rbegin = token_new;
}

static token_t* tokenize(const char* src) {
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
        } else if (strchr("().,", *src_itr) != NULL) {
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

static node_t* parse_node_new(node_type_t type) {
    node_t* node = malloc(sizeof(node_t));
    *node = (node_t){.type = type, .value = {.u64 = 0}, .next = NULL, .parent = NULL, .child_begin = NULL, .child_rbegin = NULL};
    return node;
}

static node_t* parse_var_new(token_t* token) {
    node_t* node = parse_node_new(NODE_TYPE_DECL_VAR);
    node->value.var = malloc(sizeof(varinfo_t));
    *node->value.var = (varinfo_t){.token = token, .type = NULL, .offset = 0};
    return node;
}

static node_t* parse_fn_new(token_t* token) {
    node_t* node = parse_node_new(NODE_TYPE_DECL_FN);
    node->value.fn = malloc(sizeof(fninfo_t));
    *node->value.fn = (fninfo_t){.token = token, .type = NULL, .arg_begin = NULL, .arg_rbegin = NULL, .body = NULL, .size = 0};
    return node;
}

static node_t* parse_struct_new(token_t* token) {
    node_t* node = parse_node_new(NODE_TYPE_DECL_STRUCT);
    node->value.structinfo = malloc(sizeof(structinfo_t));
    *node->value.structinfo = (structinfo_t){.token = token, .type = NULL, .member_begin = NULL, .member_rbegin = NULL, .size = 0};
    return node;
}

static void parse_node_addmember(node_t* parent, node_t* node) {
    node->parent = parent;
    if (parent->child_begin == NULL) {
        parent->child_begin = node;
    } else {
        parent->child_rbegin->next = node;
    }
    parent->child_rbegin = node;
}

static void parse_expr(token_t** token_itr, node_t* parent) {
    primitive_t* primitive_found = NULL;
    varinfo_t* varinfo_found = NULL;
    fninfo_t* fninfo_found = NULL;
    structinfo_t* structinfo_found = NULL;
    for (primitive_t* primitive_itr = primitive_table; primitive_itr->str != NULL; primitive_itr++) {
        if (token_equal_str(*token_itr, primitive_itr->str)) {
            primitive_found = primitive_itr;
            break;
        }
    }
    for (node_t* parent_itr = parent; parent_itr != NULL; parent_itr = parent_itr->parent) {
        for (node_t* node_itr = parent_itr->child_begin; node_itr != NULL; node_itr = node_itr->next) {
            if(node_itr->type == NODE_TYPE_DECL_VAR || node_itr->type == NODE_TYPE_DECL_FN || node_itr->type == NODE_TYPE_DECL_STRUCT) {
                if(varinfo_found == NULL && token_equal(node_itr->value.var->token, *token_itr)) {
                    varinfo_found = node_itr->value.var;
                }
                if(fninfo_found == NULL && token_equal(node_itr->value.fn->token, *token_itr)) {
                    fninfo_found = node_itr->value.fn;
                }
                if(structinfo_found == NULL && token_equal(node_itr->value.structinfo->token, *token_itr)) {
                    structinfo_found = node_itr->value.structinfo;
                }
            }
        }
    }
    if (primitive_found != NULL) {
        node_t* node = parse_node_new(primitive_found->type);
        parse_node_addmember(parent, node);
        *token_itr = (*token_itr)->next;
        parse_expr(token_itr, node);
    } else if (varinfo_found != NULL) {
        // Placeholder for variable
    } else if (fninfo_found != NULL) {
        // Placeholder for function
    } else if (structinfo_found != NULL) {
        // Placeholder for struct
    } else if (token_equal_str(*token_itr, "(")) {
        node_t* node = parse_node_new(NODE_TYPE_BLOCK);
        parse_node_addmember(parent, node);
        *token_itr = (*token_itr)->next;
        while (!token_equal_str(*token_itr, ")")) {
            parse_expr(token_itr, node);
            *token_itr = (*token_itr)->next;
        }
        *token_itr = (*token_itr)->next;
        if(*token_itr == NULL) {
            return;
        }
    } else if (token_equal_str(*token_itr, "var")) {
        // Placeholder for variable
        node_t* node = parse_var_new(*token_itr);
        parse_node_addmember(parent, node);
        *token_itr = (*token_itr)->next;
    } else if (token_equal_str(*token_itr, "fn")) {
        // Placeholder for function
        node_t* node = parse_fn_new(*token_itr);
        parse_node_addmember(parent, node);
        *token_itr = (*token_itr)->next;
    } else if (token_equal_str(*token_itr, "struct")) {
        // Placeholder for struct
        node_t* node = parse_struct_new(*token_itr);
        parse_node_addmember(parent, node);
        *token_itr = (*token_itr)->next;
    } else if (strchr("0123456789", (*token_itr)->data[0]) != NULL) {
        node_t* node = parse_node_new(NODE_TYPE_I64);
        node->value.i64 = token_to_i64(*token_itr);
        parse_node_addmember(parent, node);
        *token_itr = (*token_itr)->next;
    } else {
        fprintf(stderr, "Expected expression but got '%.*s'\n", (*token_itr)->size, (*token_itr)->data);
        exit(1);
    }
}

static node_t* parse(token_t* token) {
    node_t* root = parse_node_new(NODE_TYPE_BLOCK);
    parse_expr(&token, root);
    return root;
}

static object_t eval(node_t* node) {
    switch (node->type) {
        case NODE_TYPE_BLOCK: {
            object_t result = {.type = NODE_TYPE_NULL, .value = {.u64 = 0}};
            for (node_t* child = node->child_begin; child != NULL; child = child->next) {
                result = eval(child);
            }
            return result;
        }
        case NODE_TYPE_I64: {
            return (object_t){.type = NODE_TYPE_I64, .value = {.i64 = node->value.i64}};
        }
        case NODE_TYPE_RETURN: {
            // Placeholder for return value
            return eval(node->child_begin);
        }
        case NODE_TYPE_ADD: {
            return (object_t){.type = NODE_TYPE_I64, .value = {.i64 = eval(node->child_begin).value.i64 + eval(node->child_rbegin).value.i64}};
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
    return 0;
    object_t result = eval(node);
    printf("result: %ld\n", result.value.i64);
    return 0;
}