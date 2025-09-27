#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum nodetype_t nodetype_t;
typedef union uni64_t uni64_t;
typedef struct token_t token_t;
typedef struct node_t node_t;
typedef struct val_t val_t;

enum nodetype_t {
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
    uint64_t size;
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

struct val_t {
    nodetype_t type;
    uni64_t value;
};

const char* table_sign[] = {"==", "!=", "<=", ">=", "&&", "||", "<<", ">>", ">", "<", "+", "-", "*", "/", "%", "&", "|", "^", "=", "(", ")", ",", ";", NULL};

void parse_expr(token_t** token, node_t* parent);

bool token_equal(token_t* a, token_t* b) {
    if (a->size != b->size) {
        return false;
    }
    return strncmp(a->data, b->data, a->size) == 0;
}

bool token_equal_str(token_t* a, const char* b) {
    uint64_t b_size = strlen(b);
    if (a->size != b_size) {
        return false;
    }
    return strncmp(a->data, b, a->size) == 0;
}

token_t* token_skiplf(token_t* token) {
    while (token != NULL && token_equal_str(token, "\n")) {
        token = token->next;
    }
    return token;
}

int64_t token_to_i64(token_t* token) {
    int64_t value = 0;
    for (uint64_t i = 0; i < token->size; i++) {
        value = value * 10 + (token->data[i] - '0');
    }
    return value;
}

const char* file_read(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buffer = (char*)malloc(size + 2);
    fread(buffer, 1, size, file);
    buffer[size] = '\n';
    buffer[size + 1] = '\0';
    fclose(file);
    return buffer;
}

const char* tokenize_find_sign(const char* src_itr) {
    for (const char** sign_itr = table_sign; *sign_itr != NULL; sign_itr++) {
        uint64_t sign_size = strlen(*sign_itr);
        if (strncmp(src_itr, *sign_itr, sign_size) == 0) {
            return *sign_itr;
        }
    }
    return NULL;
}

void tokenize_push(token_t** token_rbegin, const char* data, uint64_t size) {
    token_t* new_token = malloc(sizeof(token_t));
    *new_token = (token_t){.data = data, .size = size, .next = NULL};
    (*token_rbegin)->next = new_token;
    *token_rbegin = new_token;
}

token_t* tokenize(const char* src) {
    token_t* token_begin = malloc(sizeof(token_t));
    token_t* token_rbegin = token_begin;
    *token_begin = (token_t){.data = NULL, .size = 0, .next = NULL};
    const char* src_itr = src;
    while (*src_itr != '\0') {
        const char* sign_data = tokenize_find_sign(src_itr);
        if (*src_itr == '\n') {
            tokenize_push(&token_rbegin, src_itr, 1);
            src_itr++;
            continue;
        } else if (*src_itr == '/' && *(src_itr + 1) == '/') {
            while (*src_itr != '\n') {
                src_itr++;
            }
            continue;
        } else if (*src_itr == ' ' || *src_itr == '\t' || *src_itr == '\r') {
            src_itr++;
        } else if (sign_data != NULL) {
            tokenize_push(&token_rbegin, sign_data, strlen(sign_data));
            src_itr += strlen(sign_data);
        } else if (strchr("abcdefghijklmnopqrstuvwxyz0123456789_", *src_itr) != NULL) {
            const char* ident_begin = src_itr;
            while (strchr("abcdefghijklmnopqrstuvwxyz0123456789_", *src_itr) != NULL) {
                src_itr++;
            }
            tokenize_push(&token_rbegin, ident_begin, src_itr - ident_begin);
            continue;
        } else {
            fprintf(stderr, "Unknown token: '%c'\n", *src_itr);
            exit(1);
        }
    }
    return token_begin->next;
}

node_t* parse_new(nodetype_t type, node_t* parent) {
    node_t* new_node = malloc(sizeof(node_t));
    *new_node = (node_t){.type = type, .value = {.u64 = 0}, .next = NULL, .parent = parent, .child_begin = NULL, .child_rbegin = NULL};
    return new_node;
}

void parse_addmember(node_t* parent, node_t* child) {
    child->parent = parent;
    if (parent->child_begin == NULL) {
        parent->child_begin = child;
    } else {
        parent->child_rbegin->next = child;
    }
    parent->child_rbegin = child;
}

void parse_primary(token_t** token, node_t* parent) {
    node_t* node_var = NULL;
    node_t* node_fn = NULL;
    for (node_t* parent_itr = parent; parent_itr != NULL && node_var == NULL && node_fn == NULL; parent_itr = parent_itr->parent) {
        for (node_t* child_itr = parent_itr->child_begin; child_itr != NULL; child_itr = child_itr->next) {
            if (child_itr->type == TYPE_DECL_VAR) {
                if (token_equal(child_itr->value.token, *token)) {
                    node_var = child_itr;
                    break;
                }
            } else if (child_itr->type == TYPE_DECL_FN) {
                if (token_equal(child_itr->value.token, *token)) {
                    node_fn = child_itr;
                    break;
                }
            }
        }
    }
    if (node_var != NULL) {
        node_t* node = parse_new(TYPE_IDENT, parent);
        node->value.node = node_var;
        parse_addmember(parent, node);
        *token = (*token)->next;
    } else if (node_fn != NULL) {
        node_t* node = parse_new(TYPE_CALL, parent);
        node->value.node = node_fn;
        parse_addmember(parent, node);
        *token = (*token)->next;
        if (!token_equal_str(*token, "(")) {
            fprintf(stderr, "Expected '(' but got '%.*s'\n", (int)((*token)->size), (*token)->data);
            exit(1);
        }
        *token = (*token)->next;
        parse_expr(token, node);
        if (!token_equal_str(*token, ")")) {
            fprintf(stderr, "Expected ')' but got '%.*s'\n", (int)((*token)->size), (*token)->data);
            exit(1);
        }
        *token = (*token)->next;
    } else if (token_equal_str(*token, "(")) {
        *token = (*token)->next;
        parse_expr(token, parent);
        if (!token_equal_str(*token, ")")) {
            fprintf(stderr, "Expected ')' but got '%.*s'\n", (int)((*token)->size), (*token)->data);
            exit(1);
        }
        *token = (*token)->next;
    } else if (strchr("0123456789", (*token)->data[0]) != NULL) {
        node_t* node = parse_new(TYPE_INT, parent);
        node->value.i64 = token_to_i64(*token);
        parse_addmember(parent, node);
        *token = (*token)->next;
    } else if (strchr("abcdefghijklmnopqrstuvwxyz_", (*token)->data[0]) != NULL) {
        node_t* node = parse_new(TYPE_IDENT, parent);
        node->value.token = (*token);
        parse_addmember(parent, node);
        *token = (*token)->next;
    } else {
        fprintf(stderr, "Expected primary expression but got '%.*s'\n next: '%.*s'\n", (int)((*token)->size), (*token)->data, (*token)->next != NULL ? (int)((*token)->next->size) : 0, (*token)->next != NULL ? (*token)->next->data : "");
        exit(1);
    }
}

void parse_expr_pre(token_t* token, node_t* parent) {
    uint64_t nest_level = 0;
    while (token != NULL) {
        if (token_equal_str(token, "(")) {
            nest_level++;
        } else if (token_equal_str(token, ")")) {
            nest_level--;
        } else if (token_equal_str(token, "fn")) {
            token = token->next;
            node_t* node = parse_new(TYPE_DECL_FN, parent);
            node->value.token = token;
            parse_addmember(parent, node);
        }
        token = token->next;
    }
}

void parse_expr(token_t** token, node_t* parent) {
    parse_expr_pre(*token, parent);
    if (token_equal_str(*token, "(")) {
        node_t* node = parse_new(TYPE_BLOCK, parent);
        parse_addmember(parent, node);
        *token = token_skiplf((*token)->next);
        while (!token_equal_str(*token, ")")) {
            parse_expr(token, node);
            *token = token_skiplf((*token)->next);
        }
        *token = token_skiplf((*token)->next);
    } else if (token_equal_str(*token, "return")) {
        node_t* node = parse_new(TYPE_RETURN, parent);
        parse_addmember(parent, node);
        *token = (*token)->next;
        parse_expr(token, node);
    } else {
        parse_primary(token, parent);
    }
}

node_t* parse(token_t* token) {
    node_t* root = parse_new(TYPE_ROOT, NULL);
    token_t* token_itr = token;
    token_itr = token_skiplf(token_itr);
    while (token_itr != NULL) {
        parse_expr(&token_itr, root);
        token_itr = token_skiplf(token_itr->next);
    }
    return root;
}

val_t eval(node_t* node) {
    switch (node->type) {
        case TYPE_ROOT:
        case TYPE_BLOCK: {
            val_t result = {.type = TYPE_NULL, .value = {.u64 = 0}};
            for (node_t* child = node->child_begin; child != NULL; child = child->next) {
                result = eval(child);
            }
            return result;
        }
        case TYPE_INT: {
            return (val_t){.type = TYPE_INT, .value = {.i64 = node->value.i64}};
        }
        case TYPE_IDENT: {
            return (val_t){.type = TYPE_IDENT, .value = {.node = node->value.node}};
        }
        case TYPE_CALL: {
            return (val_t){.type = TYPE_CALL, .value = {.node = node->value.node}};
        }
        case TYPE_RETURN: {
            return eval(node->child_begin);
        }
        default: {
            fprintf(stderr, "Evaluation not implemented for node type %d\n", node->type);
            exit(1);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }
    const char* src = file_read(argv[1]);
    token_t* token = tokenize(src);
    node_t* node = parse(token);
    val_t result = eval(node);
    printf("eval result: %ld\n", result.value.i64);
    return 0;
}