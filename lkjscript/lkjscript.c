#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LKJSCRIPT_PATH "/data/main.lkjscript"

typedef enum type_t type_t;
typedef struct token_t token_t;
typedef struct node_t node_t;
typedef struct val_t val_t;

void parse_expr(node_t* parent, token_t** token_itr);
static val_t eval(node_t* node);
static val_t value_null(void);
static val_t value_i64(int64_t value);
static void ensure_i64(val_t value, const char* context);
static size_t eval_collect_args(node_t* node, val_t* args, size_t max_args);

enum type_t {
    TYPE_NULL,
    TYPE_BLOCK,
    TYPE_IDENT,
    TYPE_I64,
    TYPE_STR,
    TYPE_FN,
    TYPE_CALL,
    TYPE_RETURN,
    TYPE_ADD,
    TYPE_SUB,
    TYPE_MUL,
    TYPE_DIV,
    TYPE_EQ,
    TYPE_NEQ,
    TYPE_LT,
    TYPE_GT,
    TYPE_LTE,
    TYPE_GTE,
    TYPE_AND,
    TYPE_OR,
    TYPE_NOT,
    TYPE_ASSIGN,
    TYPE_SYSCALL_DEBUG01,
    TYPE_SYSCALL_WRITE,
    TYPE_SYSCALL_READ,
};

struct token_t {
    const char* data;
    uint64_t size;
    token_t* next;
};

struct node_t {
    type_t type;
    node_t* next;
    node_t* parent;
    node_t* child_begin;
    node_t* child_end;
    int64_t value_i64;
    token_t* value_token;
};

struct val_t {
    type_t type;
    union {
        int64_t i64;
        char* str;
    } value;
};

static val_t value_null(void) {
    return (val_t){.type = TYPE_NULL, .value.i64 = 0};
}

static val_t value_i64(int64_t value) {
    return (val_t){.type = TYPE_I64, .value.i64 = value};
}

static void ensure_i64(val_t value, const char* context) {
    if (value.type != TYPE_I64) {
        fprintf(stderr, "%s: expected i64 but got type %d\n", context, value.type);
        exit(1);
    }
}

static size_t eval_collect_args(node_t* node, val_t* args, size_t max_args) {
    size_t count = 0;
    for (node_t* child = node->child_begin; child != NULL; child = child->next) {
        if (count >= max_args) {
            fprintf(stderr, "too many arguments (limit %zu)\n", max_args);
            exit(1);
        }
        args[count++] = eval(child);
    }
    return count;
}

bool token_equal(token_t* a, token_t* b) {
    if (a->size != b->size) {
        return false;
    }
    return strncmp(a->data, b->data, a->size) == 0;
}

bool token_equal_str(token_t* a, const char* b) {
    size_t b_size = strlen(b);
    if (a->size != b_size) {
        return false;
    }
    return strncmp(a->data, b, a->size) == 0;
}

int64_t token_to_i64(token_t* a) {
    int64_t value = 0;
    for (uint64_t i = 0; i < a->size; i++) {
        value = value * 10 + (a->data[i] - '0');
    }
    return value;
}

char* file_read(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", path);
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buffer = (char*)malloc(length + 2);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    fread(buffer, 1, length, file);
    buffer[length] = '\n';
    buffer[length + 1] = '\0';
    fclose(file);
    return buffer;
}

void tokenize_new(token_t** itr, const char* data, uint64_t size) {
    token_t* new_tok = (token_t*)malloc(sizeof(token_t));
    new_tok->data = data;
    new_tok->size = size;
    new_tok->next = NULL;
    (*itr)->next = new_tok;
    *itr = new_tok;
}

token_t* tokenize(const char* src) {
    token_t head;
    token_t* token_itr = &head;
    const char* src_itr = src;
    const char* sign_table[] = {
        "==",
        "!=",
        "&&",
        "||",
        "->",
        "=",
        "+",
        "-",
        "*",
        "/",
        "%",
        "<",
        ">",
        "<=",
        ">=",
        "!",
        "(",
        ")",
        "{",
        "}",
        ",",
        ";",
        NULL};
    while (*src_itr != '\0') {
        if (*src_itr == '\n') {
            tokenize_new(&token_itr, src_itr, 1);
            src_itr++;
            continue;
        }
        if (*src_itr == ' ' || *src_itr == '\t') {
            src_itr++;
            continue;
        }
        if (*src_itr == '/' && *(src_itr + 1) == '/') {
            while (*src_itr != '\n') {
                src_itr++;
            }
            continue;
        }
        if (strchr("abcdefghijklmnopqrstuvwxyz0123456789_", *src_itr)) {
            const char* start = src_itr;
            while (strchr("abcdefghijklmnopqrstuvwxyz0123456789_", *src_itr)) {
                src_itr++;
            }
            tokenize_new(&token_itr, start, src_itr - start);
            continue;
        }
        bool sign_found = false;
        for (const char** sign_itr = sign_table; *sign_itr != NULL; sign_itr++) {
            size_t sign_len = strlen(*sign_itr);
            if (strncmp(src_itr, *sign_itr, sign_len) == 0) {
                tokenize_new(&token_itr, src_itr, sign_len);
                src_itr += sign_len;
                sign_found = true;
                break;
            }
        }
        if (!sign_found) {
            fprintf(stderr, "Unknown token starting with: %c\n", *src_itr);
            exit(1);
        }
    }
    return head.next;
}

void parse_tokenitr_next(token_t** token_itr) {
    *token_itr = (*token_itr)->next;
}

void parse_tokenitr_skiplf(token_t** token_itr) {
    while (*token_itr != NULL && token_equal_str(*token_itr, "\n")) {
        parse_tokenitr_next(token_itr);
    }
}

node_t* parse_new(type_t type) {
    node_t* new_node = (node_t*)malloc(sizeof(node_t));
    new_node->type = type;
    new_node->next = NULL;
    new_node->parent = NULL;
    new_node->child_begin = NULL;
    new_node->child_end = NULL;
    return new_node;
}

void parse_addchild(node_t* parent, node_t* child) {
    child->parent = parent;
    if (parent->child_begin == NULL) {
        parent->child_begin = child;
        parent->child_end = child;
    } else {
        parent->child_end->next = child;
        parent->child_end = child;
    }
}

node_t* parse_fn_find(node_t* parent, token_t* token) {
    for (node_t* base = parent->child_begin; base != NULL; base = base->parent) {
        for (node_t* itr = base->child_begin; itr != NULL; itr = itr->next) {
            if (itr->type == TYPE_FN && token_equal(itr->value_token, token)) {
                return itr;
            }
        }
    }
    return NULL;
}

void parse_primary(node_t* parent, token_t** token_itr) {
    node_t* fn = parse_fn_find(parent, *token_itr);
    if (fn != NULL) {
        fprintf(stderr, "Placeholder for function call parsing\n");
        exit(1);
    } else if (token_equal_str(*token_itr, "debug01")) {
        parse_tokenitr_next(token_itr);
        node_t* debug_node = parse_new(TYPE_SYSCALL_DEBUG01);
        parse_addchild(parent, debug_node);
        parse_expr(debug_node, token_itr);
    } else if (token_equal_str(*token_itr, "write")) {
        parse_tokenitr_next(token_itr);
        node_t* write_node = parse_new(TYPE_SYSCALL_WRITE);
        parse_addchild(parent, write_node);
        parse_expr(write_node, token_itr);
    } else if (token_equal_str(*token_itr, "read")) {
        parse_tokenitr_next(token_itr);
        node_t* read_node = parse_new(TYPE_SYSCALL_READ);
        parse_addchild(parent, read_node);
        parse_expr(read_node, token_itr);
    } else if (strchr("0123456789", (*token_itr)->data[0])) {
        node_t* number_node = parse_new(TYPE_I64);
        number_node->value_i64 = token_to_i64(*token_itr);
        parse_addchild(parent, number_node);
        parse_tokenitr_next(token_itr);
    } else if (strchr("abcdefghijklmnopqrstuvwxyz_", (*token_itr)->data[0])) {
        node_t* ident_node = parse_new(TYPE_IDENT);
        ident_node->value_token = *token_itr;
        parse_addchild(parent, ident_node);
        parse_tokenitr_next(token_itr);
    } else {
        fprintf(stderr, "Unexpected token: %.*s\n", (int)(*token_itr)->size, (*token_itr)->data);
        exit(1);
    }
}

void parse_binary(node_t* parent, token_t** token_itr) {
    parse_primary(parent, token_itr);
}

void parse_expr(node_t* parent, token_t** token_itr) {
    if (token_equal_str(*token_itr, "(")) {
        parse_tokenitr_next(token_itr);
        node_t* block_node = parse_new(TYPE_BLOCK);
        parse_addchild(parent, block_node);
        parse_tokenitr_skiplf(token_itr);
        while (!token_equal_str(*token_itr, ")")) {
            parse_expr(block_node, token_itr);
            parse_tokenitr_skiplf(token_itr);
        }
        parse_tokenitr_next(token_itr);
    } else {
        parse_binary(parent, token_itr);
        while (token_equal_str(*token_itr, ",")) {
            parse_tokenitr_next(token_itr);
            parse_expr(parent, token_itr);
        }
    }
}

node_t* parse(token_t* token) {
    node_t* root = parse_new(TYPE_BLOCK);
    token_t* token_itr = token;
    parse_tokenitr_skiplf(&token_itr);
    while (token_itr != NULL) {
        parse_expr(root, &token_itr);
        parse_tokenitr_skiplf(&token_itr);
    }
    return root;
}

val_t eval(node_t* node) {
    switch (node->type) {
        case TYPE_NULL:
            return value_null();
        case TYPE_BLOCK: {
            val_t result = value_null();
            for (node_t* child = node->child_begin; child != NULL; child = child->next) {
                result = eval(child);
            }
            return result;
        }
        case TYPE_I64:
            return value_i64(node->value_i64);
        case TYPE_SYSCALL_DEBUG01: {
            val_t args[1];
            size_t argc = eval_collect_args(node, args, 1);
            if (argc != 1) {
                fprintf(stderr, "debug01 expects 1 argument, got %zu\n", argc);
                exit(1);
            }
            ensure_i64(args[0], "debug01");
            printf("%c", (char)args[0].value.i64);
            fflush(stdout);
            return value_null();
        }
        case TYPE_SYSCALL_WRITE: {
            val_t args[2];
            size_t argc = eval_collect_args(node, args, 2);
            if (argc != 2) {
                fprintf(stderr, "write expects 2 arguments, got %zu\n", argc);
                exit(1);
            }
            ensure_i64(args[0], "write fd");
            if (args[1].type == TYPE_I64) {
                char buffer[64];
                int len = snprintf(buffer, sizeof(buffer), "%ld", args[1].value.i64);
                write((int)args[0].value.i64, buffer, (size_t)len);
            } else if (args[1].type == TYPE_STR && args[1].value.str != NULL) {
                write((int)args[0].value.i64, args[1].value.str, strlen(args[1].value.str));
            } else {
                fprintf(stderr, "write expects string or integer data\n");
                exit(1);
            }
            return value_null();
        }
        case TYPE_SYSCALL_READ: {
            val_t args[2];
            size_t argc = eval_collect_args(node, args, 2);
            if (argc != 2) {
                fprintf(stderr, "read expects 2 arguments, got %zu\n", argc);
                exit(1);
            }
            ensure_i64(args[0], "read fd");
            ensure_i64(args[1], "read size");
            size_t size = (size_t)args[1].value.i64;
            char* buffer = (char*)malloc(size + 1);
            if (!buffer) {
                fprintf(stderr, "read: malloc failed\n");
                exit(1);
            }
            ssize_t read_len = read((int)args[0].value.i64, buffer, size);
            if (read_len < 0) {
                fprintf(stderr, "read failed\n");
                free(buffer);
                exit(1);
            }
            buffer[read_len] = '\0';
            val_t result = value_null();
            result.type = TYPE_STR;
            result.value.str = buffer;
            return result;
        }
        case TYPE_ADD: {
            val_t lhs = eval(node->child_begin);
            val_t rhs = eval(node->child_begin->next);
            ensure_i64(lhs, "+");
            ensure_i64(rhs, "+");
            return value_i64(lhs.value.i64 + rhs.value.i64);
        }
        case TYPE_SUB: {
            val_t lhs = eval(node->child_begin);
            val_t rhs = eval(node->child_begin->next);
            ensure_i64(lhs, "-");
            ensure_i64(rhs, "-");
            return value_i64(lhs.value.i64 - rhs.value.i64);
        }
        case TYPE_MUL: {
            val_t lhs = eval(node->child_begin);
            val_t rhs = eval(node->child_begin->next);
            ensure_i64(lhs, "*");
            ensure_i64(rhs, "*");
            return value_i64(lhs.value.i64 * rhs.value.i64);
        }
        case TYPE_DIV: {
            val_t lhs = eval(node->child_begin);
            val_t rhs = eval(node->child_begin->next);
            ensure_i64(lhs, "/");
            ensure_i64(rhs, "/");
            if (rhs.value.i64 == 0) {
                fprintf(stderr, "division by zero\n");
                exit(1);
            }
            return value_i64(lhs.value.i64 / rhs.value.i64);
        }
        default: {
            fprintf(stderr, "Eval not implemented for node type: %d\n", node->type);
            exit(1);
        }
    }
}

int main() {
    char* src = file_read(LKJSCRIPT_PATH);
    token_t* token = tokenize(src);
    node_t* node = parse(token);
    val_t result = eval(node);
    return result.value.i64;
}