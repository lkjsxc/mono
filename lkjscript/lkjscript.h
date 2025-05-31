#ifndef LKJSCRIPT_H
#define LKJSCRIPT_H

#include <fcntl.h>
#include <unistd.h>

#define MEM_SIZE (1024 * 1024)
#define SRC_PATH "script/main.lkjscript"

#define ERROUT3(n) #n
#define ERROUT2(n) ERROUT3(n)
#define ERROUT                                                              \
    {                                                                       \
        write(STDERR_FILENO, "{Error: { file: \"", 18);                     \
        write(STDERR_FILENO, __FILE__, sizeof(__FILE__));                   \
        write(STDERR_FILENO, "\", func: \"", 11);                           \
        write(STDERR_FILENO, __func__, sizeof(__func__));                   \
        write(STDERR_FILENO, "\", line: ", 10);                             \
        write(STDERR_FILENO, ERROUT2(__LINE__), sizeof(ERROUT2(__LINE__))); \
        write(STDERR_FILENO, "}}\n", 4);                                    \
    }

typedef unsigned char uint8_t;
typedef long long int int64_t;

typedef enum {
    OK,
    ERR
} result_t;

typedef enum {
    GLOBALOFFSET_NULL = 0,
    GLOBALOFFSET_IP = 8,
    GLOBALOFFSET_SP = 16,
    GLOBALOFFSET_BP = 24,
    GLOBALOFFSET_INST = 256,
} globalmem_t;

typedef enum {
    TOKENTYPE_NUM,
    TOKENTYPE_STR,
    TOKENTYPE_IDENT,
} tokentype_t;

typedef enum {
    NODETYPE_NULL,

    NODETYPE_NOP,
    NODETYPE_END,

    NODETYPE_PUSH_CONST,
    NODETYPE_PUSH_LOCAL_VAL,
    NODETYPE_PUSH_LOCAL_ADDR,
    NODETYPE_JMP,
    NODETYPE_JZE,
    NODETYPE_CALL,
    NODETYPE_RETURN,

    NODETYPE_ASSIGN,
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
    NODETYPE_GETSTRUCTMEMBER,

    NODETYPE_DEREF,
    NODETYPE_NEG,
    NODETYPE_BITNOT,

    NODETYPE_READ,
    NODETYPE_WRITE,
    NODETYPE_USLEEP,

    // parse
    NODETYPE_VAR,
    NODETYPE_FN,
    NODETYPE_STRUCT,

    NODETYPE_LABEL,
    NODETYPE_LABEL_SCOPE_OPEN,
    NODETYPE_LABEL_SCOPE_CLOSE,
    NODETYPE_LABEL_GLOBAL_END,
} nodetype_t;

typedef struct {
    const char* data;
    int64_t size;
} token_t;

typedef struct node_t {
    nodetype_t nodetype;
    token_t* token;
    struct node_t* next;

    // PUSH_CONST: val, STRUCT: size, PUSH_LOCAL: offset, LABEL: bin_addr
    int64_t val;

    // STRUCT: member, FN: type and stmt, (JMP,JZE,CALL): target, decl: type
    struct node_t* child;
    struct node_t* parent;

    // bin index
    int64_t bin;
} node_t;

typedef struct {
    uint8_t bin[MEM_SIZE / sizeof(uint8_t) / 6];
    char src[MEM_SIZE / 6];
    token_t token[MEM_SIZE / sizeof(token_t) / 6];
    node_t node[MEM_SIZE / sizeof(node_t) / 6];
} compile_t;

typedef union {
    uint8_t u8[MEM_SIZE];
    compile_t compile;
} mem_t;

int64_t token_eq(token_t* token1, token_t* token2);
int64_t token_eqstr(token_t* token, const char* str);
int64_t token_isdigit(token_t* token);
int64_t token_isstr(token_t* token);

#endif