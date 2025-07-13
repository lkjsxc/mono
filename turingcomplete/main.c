#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define OP_IMMEDIATE 0b00000000
#define OP_CALCULATE 0b01000000
#define OP_COPY 0b10000000
#define OP_SYSTEM 0b11000000

#define CALC_OR 0b01000000
#define CALC_AND 0b01000001
#define CALC_XOR 0b01000010
#define CALC_NOT 0b01000011
#define CALC_ADD 0b01000100
#define CALC_SUB 0b01000101
#define CALC_SHL 0b01000110
#define CALC_SHR 0b01000111
#define CALC_NOP 0b01001000
#define CALC_EQ 0b01001001
#define CALC_NEQ 0b01001010
#define CALC_LT 0b01001011

#define SYS_INPUT 0b11000000
#define SYS_OUTPUT 0b11000001
#define SYS_MEM_LOAD 0b11000010
#define SYS_MEM_SAVE 0b11000011
#define SYS_JMP 0b11000100
#define SYS_JZE 0b11000101
#define SYS_PUSH 0b11000110
#define SYS_POP 0b11000111

#define REG0 0
#define REG1 1
#define REG2 2
#define REG3 3
#define REG4 4
#define REG5 5
#define REG6 6
#define REG7 7

typedef enum type_t {
    TY_NULL,
    TY_NOP,
    TY_BLOCK,
    TY_DECL,
    TY_PUSH_CONST,
    TY_PUSH_VAR,
    TY_PUSH_ADDR,
    TY_PUSH_LABEL,
    TY_COPY,
    TY_OR,
    TY_NAND,
    TY_NOR,
    TY_AND,
    TY_ADD,
    TY_SUB,
    TY_SHL,
    TY_SHR,
    TY_XOR,
    TY_EQ,
    TY_NEQ,
    TY_LT,
    TY_INPUT,
    TY_OUTPUT,
    TY_JMP,
    TY_JZE,
} type_t;

typedef struct token_t {
    const char* data;
    int size;
} token_t;

typedef struct node_t {
    type_t type;
    token_t* token;
    int code_index;
    struct node_t* next;
    struct node_t* parent;
    struct node_t* child;
    struct node_t* lhs;
    struct node_t* rhs;
    struct node_t* optimize_last;
    int optimize_refcnt;
} node_t;

typedef struct stat_t {
    token_t* token;
    node_t* node;
    node_t* node_root;
    node_t* node_pre;
    node_t* node_post;
    node_t* node_parent;
    node_t* node_continue;
    node_t* node_break;
} stat_t;

int token_eq(token_t* token1, token_t* token2) {
    if (token1->size != token2->size){
        return 0;}
    return strncmp(token1->data, token2->data, token1->size) == 0;
}

int token_eqstr(token_t* token, const char* str) {
    int size = strlen(str);
    if (token->size != size){
        return 0;}
    return strncmp(token->data, str, size) == 0;
}

int token_isdigit(token_t* token) {
    for (int i = 0; i < token->size; i++) {
        if (token->data[i] < '0' || token->data[i] > '9') {
            return 0;
        }
    }
    return 1;
}

int token_toint(token_t* token) {
    int value = 0;
    for (int i = 0; i < token->size; i++) {
        value = value * 10 + (token->data[i] - '0');
    }
    return value;
}

node_t* node_create(node_t** node_itr, type_t type) {
    node_t* node = *node_itr;
    *node_itr += 1;
    *node = (node_t){
        .type = type
    };
    return node;
}

node_t* node_find_decl(node_t* node) {
    node_t* itr = node;
    node_t* parent = itr->parent;
    while (parent != NULL) {
        itr = parent->child;
        while (itr != NULL) {
            if (itr->type == TY_DECL && token_eq(itr->token, node->token)) {
                return itr;
            }
            itr = itr->next;
        }
        parent = parent->parent;
    }
    return NULL;
}

void node_addchild(node_t* parent, node_t* node) {
    if (parent->child == NULL) {
        parent->child = node;
    } else {
        node_t* current = parent->child;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = node;
    }
    node->parent = parent;
}

int node_provide_var(node_t** nodelist_data, int* nodelist_size, node_t* node) {
    for (int i = 0; i < *nodelist_size; i++) {
        if (token_eq(nodelist_data[i]->token, node->token)) {
            return i;
        }
    }
    nodelist_data[(*nodelist_size)++] = node;
    return (*nodelist_size) - 1;
}

void input(const char* filename, char* dst) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    size_t n = fread(dst, 1, dst_size - 1, file);
    dst[n] = '\0';
    fclose(file);
}

void tokenize(const char* src, token_t* token_data) {
    const char* src_itr = src;
    token_t* token_itr = token_data;
    const char* base = src;
    int size = 0;
    while (*src_itr) {
        if (*src_itr == ' ' || *src_itr == '\n' || *src_itr == '\t') {
            if (size > 0) {
                token_itr->data = base;
                token_itr->size = size;
                token_itr += 1;
                size = 0;
            }
            base = src_itr + 1;
        } else if (*src_itr == '(' || *src_itr == ')' || *src_itr == ',' || *src_itr == '&') {
            if (size > 0) {
                token_itr->data = base;
                token_itr->size = size;
                token_itr += 1;
                size = 0;
            }
            token_itr->data = src_itr;
            token_itr->size = 1;
            token_itr += 1;
            base = src_itr + 1;
        } else {
            size += 1;
        }
        src_itr += 1;
    }
    if (size > 0) {
        token_itr->data = base;
        token_itr->size = size;
        token_itr += 1;
        size = 0;
    }
    token_itr->data = NULL;
    token_itr->size = 0;
}

void parse(token_t* token_data, node_t* node_data) {
    token_t* token_itr = token_data;
    node_t* node_itr = node_data;
}

void optimize() {
}

void codegen() {
}

void codelink() {
}

void output() {
}

int main() {
    static char src_data[65536];
    static char code_data[65536];
    static token_t token_data[65536];
    static node_t node_data[65536];

    input("src.txt", src_data);

    tokenize(src_data, token_data);

    parse();

    optimize();

    codegen();

    codelink();

    output();

    return 0;
}