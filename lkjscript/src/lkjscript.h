#ifndef LKJSCRIPT_H
#define LKJSCRIPT_H

#define SRC_PATH "./src/lkjscriptsrc"
#define MEM_SIZE (1024 * 512 * 1)
#define MEM_GLOBAL_SIZE 32
#define MEM_STACK_SIZE 1024

#define INT64_MAX 9223372036854775807

typedef long long int64_t;

typedef enum {
    FALSE = 0,
    TRUE = 1,
} bool_t;

typedef enum {
    OK = 0,
    ERR = 1,
} result_t;

typedef enum {
    GLOBALADDR_ZERO,
    GLOBALADDR_IP,
    GLOBALADDR_SP,
    GLOBALADDR_BP,
} globaladdr_t;

typedef enum {

    TY_NULL,

    TY_INST_NOP,
    TY_INST_END,

    TY_INST_PUSH_CONST,
    TY_INST_PUSH_LOCAL_VAL,
    TY_INST_PUSH_LOCAL_ADDR,
    TY_INST_JMP,
    TY_INST_JZ,
    TY_INST_CALL,
    TY_INST_RETURN,

    TY_INST_ASSIGN1,
    TY_INST_ASSIGN2,
    TY_INST_ASSIGN3,
    TY_INST_ASSIGN4,

    TY_INST_OR,
    TY_INST_AND,
    TY_INST_EQ,
    TY_INST_NE,
    TY_INST_LT,
    TY_INST_LE,
    TY_INST_GT,
    TY_INST_GE,
    TY_INST_NOT,
    TY_INST_ADD,
    TY_INST_SUB,
    TY_INST_MUL,
    TY_INST_DIV,
    TY_INST_MOD,
    TY_INST_SHL,
    TY_INST_SHR,
    TY_INST_BITOR,
    TY_INST_BITXOR,
    TY_INST_BITAND,

    TY_INST_DEREF,
    TY_INST_NEG,
    TY_INST_BITNOT,

    TY_INST_READ,
    TY_INST_WRITE,
    TY_INST_USLEEP,

    TY_LABEL,
    TY_LABEL_FN_OPEN,
    TY_LABEL_FN_CLOSE,

} type_t;

typedef struct {
    const char* data;
    int64_t size;
} token_t;

typedef struct {
    type_t type;
    token_t* token;
    int64_t val;
} node_t;

typedef struct {
    token_t* key;
    int64_t val;
    int64_t argcnt;
    int64_t stacksize;
} map_t;

typedef struct {
    int64_t bin[MEM_SIZE / sizeof(int64_t) / 6];
    char src[MEM_SIZE / sizeof(char) / 6];
    token_t token[MEM_SIZE / sizeof(token_t) / 6];
    node_t node[MEM_SIZE / sizeof(node_t) / 6];
    map_t map[MEM_SIZE / sizeof(map_t) / 6];
} compile_t;

typedef union {
    int64_t bin[MEM_SIZE / sizeof(int64_t)];
    compile_t compile;
} mem_t;

#endif