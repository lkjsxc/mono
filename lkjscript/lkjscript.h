#ifndef LKJSCRIPT_H
#define LKJSCRIPT_H

#include <fcntl.h>
#include <unistd.h>

#define MEM_SIZE (1024 * 1024)
#define SRC_PATH "script/main.lkjscript"

typedef long long int int64_t;

typedef enum {
    OK,
    ERR
} result_t;

typedef enum {
    TOKENTYPE_NUM,
    TOKENTYPE_STR,
    TOKENTYPE_IDENT,
} tokentype_t;

typedef struct {
    const char* data;
    int64_t size;
} token_t;

typedef enum {
    NODETYPE_NULL,

    NODETYPE_NOP,
    NODETYPE_END,

    NODETYPE_PUSH_CONST,
    NODETYPE_PUSH_LOCAL_VAL,
    NODETYPE_PUSH_LOCAL_ADDR,
    NODETYPE_JMP,
    NODETYPE_JZ,
    NODETYPE_CALL,
    NODETYPE_RETURN,

    NODETYPE_ASSIGN1,
    NODETYPE_ASSIGN2,
    NODETYPE_ASSIGN3,
    NODETYPE_ASSIGN4,

    NODETYPE_OR,
    NODETYPE_AND,
    NODETYPE_EQ,
    NODETYPE_NE,
    NODETYPE_LT,
    NODETYPE_LE,
    NODETYPE_GT,
    NODETYPE_GE,
    NODETYPE_NOT,
    NODETYPE_ADD,
    NODETYPE_SUB,
    NODETYPE_MUL,
    NODETYPE_DIV,
    NODETYPE_MOD,
    NODETYPE_SHL,
    NODETYPE_SHR,
    NODETYPE_BITOR,
    NODETYPE_BITXOR,
    NODETYPE_BITAND,

    NODETYPE_DEREF,
    NODETYPE_NEG,
    NODETYPE_BITNOT,

    NODETYPE_READ,
    NODETYPE_WRITE,
    NODETYPE_USLEEP,

    NODETYPE_FN,
    NODETYPE_STRUCT,

    NODETYPE_LABEL,
    NODETYPE_LABEL_SCOPE_OPEN,
    NODETYPE_LABEL_SCOPE_CLOSE,
} nodetype_t;

typedef struct node_t {
    nodetype_t nodetype;
    struct node_t* next;
    token_t* token;

    // PUSH_CONST: val, PUSH_LOCAL: offset, LABEL: bin addr
    int64_t val;

    // function, struct, goto
    struct node_t* child;

    // type
    struct node_t* type_ptr;

    //function
    int64_t arg_size;
    int64_t stack_size;

    // struct
    int64_t struct_size;
} node_t;

typedef struct {
    int64_t bin[MEM_SIZE / sizeof(int64_t) / 6];
    char src[MEM_SIZE / 6];
    token_t token[MEM_SIZE / sizeof(token_t) / 6];
    node_t node[MEM_SIZE / sizeof(node_t) / 6];
} compile_t;

typedef union {
    int64_t i64[MEM_SIZE];
    compile_t compile;
} mem_t;

int64_t token_eq(token_t* token1, token_t* token2);
int64_t token_eqstr(token_t* token, const char* str);

#endif