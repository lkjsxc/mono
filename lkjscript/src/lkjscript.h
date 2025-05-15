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
} bool;

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

    INST_NULL,

    INST_NOP,

    INST_START,
    INST_END,

    INST_PUSH_CONST,
    INST_PUSH_LOCAL_VAL,
    INST_PUSH_LOCAL_ADDR,
    INST_JMP,
    INST_JZ,
    INST_CALL,
    INST_RETURN,

    INST_ASSIGN1,
    INST_ASSIGN2,
    INST_ASSIGN3,
    INST_ASSIGN4,

    INST_OR,
    INST_AND,
    INST_EQ,
    INST_NE,
    INST_LT,
    INST_LE,
    INST_GT,
    INST_GE,
    INST_NOT,
    INST_ADD,
    INST_SUB,
    INST_MUL,
    INST_DIV,
    INST_MOD,
    INST_SHL,
    INST_SHR,
    INST_BITOR,
    INST_BITXOR,
    INST_BITAND,

    INST_DEREF,
    INST_NEG,
    INST_BITNOT,

    INST_READ,
    INST_WRITE,
    INST_USLEEP,
} inst_t;

typedef enum {
    TYPE_VOID,
    TYPE_I64,
    TYPE_PTR,
    TYPE_FN,
    TYPE_STRUCT,
} typekind_t;

typedef enum {
    TK_NULL,
    TK_IDENT,
    TK_STR,
    TK_NUM,
} tokenkind_t;

typedef struct {
    tokenkind_t kind;
    const char* data;
    int64_t size;
} token_t;

typedef struct node_t node_t;
struct node_t{
    // node type
    inst_t inst;

    // token
    token_t* token;

    // type
    typekind_t typekind;
    node_t* typeptr;
    
    // flags
    bool is_fn;
    bool is_struct;

    // local variable
    int64_t local_offset;

    // function
    int64_t argcnt;
    int64_t stacksize;

    // struct
    node_t* struct_next;
    int64_t struct_offset;
};

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
    int64_t i64[MEM_SIZE / sizeof(int64_t)];
    compile_t compile;
} mem_t;

#endif