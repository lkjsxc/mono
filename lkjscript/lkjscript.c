#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum nodetype_t nodetype_t;
typedef enum typekind_t typekind_t;
typedef struct token_t token_t;
typedef struct node_t node_t;
typedef struct var_t var_t;
typedef struct fn_t fn_t;
typedef struct structinfo_t structinfo_t;
typedef struct struct_instance_t struct_instance_t;
typedef struct primitive_t primitive_t;
typedef struct typeinfo_t typeinfo_t;
typedef struct object_t object_t;
typedef struct frame_t frame_t;

enum nodetype_t {
    NODETYPE_NULL,
    NODETYPE_BLOCK,
    NODETYPE_DECL_VAR,
    NODETYPE_DECL_FN,
    NODETYPE_DECL_STRUCT,
    NODETYPE_CALL,
    NODETYPE_RETURN,
    NODETYPE_INT,
    NODETYPE_IDENT,
    NODETYPE_IDENT_FN,
    NODETYPE_IDENT_STRUCT,
    NODETYPE_ASSIGN,
    NODETYPE_ADDR,
    NODETYPE_DEREF,
    NODETYPE_MEMBER,
    NODETYPE_STRUCT_INIT,
    NODETYPE_STRUCT_FIELD_INIT,
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
};

enum typekind_t {
    TYPEKIND_UNIT,
    TYPEKIND_I64,
    TYPEKIND_PTR,
    TYPEKIND_STRUCT,
};

struct token_t {
    const char* data;
    uint32_t size;
    token_t* next;
};

struct typeinfo_t {
    typekind_t kind;
    typeinfo_t* inner;
    structinfo_t* structinfo;
    token_t* token;
};

struct var_t {
    token_t* token;
    typeinfo_t* typeinfo;
    int64_t offset;
};

struct fn_t {
    token_t* token;
    node_t* args;
    node_t* body;
    typeinfo_t* result_type;
    int64_t stacksize;
};

struct structinfo_t {
    token_t* token;
    node_t* fields;
    var_t** field_vars;
    int field_count;
};

struct primitive_t {
    const char* str;
    nodetype_t type;
};

union node_value_t {
    int64_t i64;
    token_t* token;
    node_t* node;
    var_t* var;
    fn_t* fn;
    structinfo_t* structinfo;
    typeinfo_t* typeinfo;
};

typedef union node_value_t node_value_t;

struct node_t {
    nodetype_t type;
    node_value_t value;
    node_t* next;
    node_t* parent;
    node_t* child_begin;
    node_t* child_rbegin;
};

struct struct_instance_t {
    structinfo_t* info;
    object_t* fields;
};

struct object_t {
    typeinfo_t* typeinfo;
    union {
        int64_t i64;
        void* ptr;
        struct_instance_t* instance;
    } value;
};

struct frame_t {
    frame_t* parent;
    object_t* slots;
    int64_t slot_count;
};

static primitive_t primitive_table[] = {
    {"return", NODETYPE_RETURN},
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

static bool token_equal(const token_t* a, const token_t* b);
static bool token_equal_str(const token_t* token, const char* str);
static bool token_is_number(const token_t* token);
static int64_t token_to_i64(const token_t* token);
static const char* readsrc(const char* path);
static token_t* tokenize(const char* src);
static node_t* parse(token_t* token);
static void resolve_program(node_t* root);
static object_t eval(node_t* node, frame_t* frame, bool* did_return);
static void object_drop(object_t* object);
static void frame_free(frame_t* frame);

static void* checked_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return ptr;
}

static bool token_equal(const token_t* a, const token_t* b) {
    if (a == NULL || b == NULL) {
        return false;
    }
    if (a->size != b->size) {
        return false;
    }
    return strncmp(a->data, b->data, a->size) == 0;
}

static bool token_equal_str(const token_t* token, const char* str) {
    if (token == NULL || str == NULL) {
        return false;
    }
    size_t len = strlen(str);
    if (token->size != len) {
        return false;
    }
    return strncmp(token->data, str, len) == 0;
}

static bool token_is_number(const token_t* token) {
    if (token == NULL || token->size == 0) {
        return false;
    }
    for (uint32_t i = 0; i < token->size; i++) {
        if (!isdigit((unsigned char)token->data[i])) {
            return false;
        }
    }
    return true;
}

static int64_t token_to_i64(const token_t* token) {
    if (!token_is_number(token)) {
        fprintf(stderr, "Token is not a number: '%.*s'\n", token != NULL ? (int)token->size : 0, token != NULL ? token->data : "");
        exit(1);
    }
    int64_t value = 0;
    for (uint32_t i = 0; i < token->size; i++) {
        value = value * 10 + (token->data[i] - '0');
    }
    return value;
}

static const char* readsrc(const char* path) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        fprintf(stderr, "Failed to open %s\n", path);
        exit(1);
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "Failed to seek %s\n", path);
        exit(1);
    }
    long n = ftell(file);
    if (n < 0) {
        fprintf(stderr, "Failed to determine length of %s\n", path);
        exit(1);
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "Failed to rewind %s\n", path);
        exit(1);
    }
    char* buf = (char*)checked_malloc((size_t)n + 4);
    buf[0] = '(';
    size_t read_bytes = fread(buf + 1, 1, (size_t)n, file);
    if (read_bytes != (size_t)n) {
        fprintf(stderr, "Failed to read %s\n", path);
        exit(1);
    }
    memcpy(buf + 1 + n, ")\n\0", 3);
    fclose(file);
    return buf;
}

static void tokenize_pushback(token_t** token_rbegin, const char* data, uint32_t size) {
    token_t* token_new = (token_t*)checked_malloc(sizeof(token_t));
    *token_new = (token_t){.data = data, .size = size, .next = NULL};
    (*token_rbegin)->next = token_new;
    *token_rbegin = token_new;
}

static token_t* tokenize(const char* src) {
    token_t token_tmp = {.data = NULL, .size = 0, .next = NULL};
    token_t* token_rbegin = &token_tmp;
    const char* itr = src;
    while (*itr != '\0') {
        if (*itr == ' ' || *itr == '\t' || *itr == '\n' || *itr == '\r') {
            itr += 1;
        } else if (*itr == '/' && *(itr + 1) == '/') {
            while (*itr != '\n' && *itr != '\0') {
                itr += 1;
            }
        } else if (strchr("().,{}", *itr) != NULL) {
            tokenize_pushback(&token_rbegin, itr, 1);
            itr += 1;
        } else if (strchr("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_", *itr) != NULL) {
            const char* begin = itr;
            while (strchr("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_", *itr) != NULL) {
                itr += 1;
            }
            tokenize_pushback(&token_rbegin, begin, (uint32_t)(itr - begin));
        } else {
            fprintf(stderr, "Unknown token: '%c'\n", *itr);
            exit(1);
        }
    }
    return token_tmp.next;
}

static node_t* node_new(nodetype_t type) {
    node_t* node = (node_t*)checked_malloc(sizeof(node_t));
    *node = (node_t){.type = type, .value = {.i64 = 0}, .next = NULL, .parent = NULL, .child_begin = NULL, .child_rbegin = NULL};
    return node;
}

static node_t* node_new_var(token_t* token) {
    node_t* node = node_new(NODETYPE_DECL_VAR);
    var_t* var = (var_t*)checked_malloc(sizeof(var_t));
    *var = (var_t){.token = token, .typeinfo = NULL, .offset = -1};
    node->value.var = var;
    return node;
}

static node_t* node_new_fn(token_t* token) {
    node_t* node = node_new(NODETYPE_DECL_FN);
    fn_t* fn = (fn_t*)checked_malloc(sizeof(fn_t));
    *fn = (fn_t){.token = token, .args = NULL, .body = NULL, .result_type = NULL, .stacksize = 0};
    node->value.fn = fn;
    return node;
}

static node_t* node_new_struct(token_t* token) {
    node_t* node = node_new(NODETYPE_DECL_STRUCT);
    structinfo_t* info = (structinfo_t*)checked_malloc(sizeof(structinfo_t));
    *info = (structinfo_t){.token = token, .fields = NULL, .field_vars = NULL, .field_count = 0};
    node->value.structinfo = info;
    return node;
}

static void node_add_child(node_t* parent, node_t* child) {
    child->parent = parent;
    if (parent->child_begin == NULL) {
        parent->child_begin = child;
    } else {
        parent->child_rbegin->next = child;
    }
    parent->child_rbegin = child;
}

static typeinfo_t* typeinfo_new(typekind_t kind) {
    typeinfo_t* info = (typeinfo_t*)checked_malloc(sizeof(typeinfo_t));
    *info = (typeinfo_t){.kind = kind, .inner = NULL, .structinfo = NULL, .token = NULL};
    return info;
}