#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define OP_IMMEDIATE 0b00000000
#define OP_CALCULATE 0b01000000
#define OP_COPY 0b10000000
#define OP_SYSTEM 0b11000000

#define CALC_OR 0b00000000
#define CALC_NAND 0b00000001
#define CALC_NOR 0b00000010
#define CALC_AND 0b00000011
#define CALC_ADD 0b00000100
#define CALC_SUB 0b00000101
#define CALC_SHL 0b00000110
#define CALC_SHR 0b00000111

#define SYS_INPUT 0b00000000
#define SYS_OUTPUT 0b00000001
#define SYS_MEM_LOAD 0b00000010
#define SYS_MEM_SAVE 0b00000011
#define SYS_JMP 0b00000100
#define SYS_JE 0b00000101
#define SYS_JNE 0b00000110
#define SYS_JL 0b00000111
#define SYS_PUSH 0b00001000
#define SYS_POP 0b00001001

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
    TY_PUSH_VAL,
    TY_PUSH_ADDR,
    TY_POP,
    TY_ADD,
    TY_SUB,
    TY_MUL,
    TY_OR,
    TY_AND,
    TY_ASSIGN,
    TY_JMP,
    TY_JE,
    TY_JNE,
    TY_JL,
    TY_INPUT,
    TY_OUTPUT,
    TY_BLOCK,
} type_t;

typedef struct token_t {
    const char* data;
    int size;
} token_t;

typedef struct node_t {
    type_t type;
    token_t* token;
    int bin_index;
    struct node_t* node_next;
    struct node_t* node_parent;
    struct node_t* node_child;
    struct node_t* node_break;
    struct node_t* node_continue;
} node_t;

char src_data[2048];
char code_data[2048];
token_t token_data[2048];
node_t node_data[2048];
node_t* stack_data[2048];
node_t* localvar_data[2048];

void parse_exprlist(token_t** token_itr, node_t** node_itr, node_t* node_parent);

int token_eq(token_t* token1, token_t* token2) {
    if (token1->size != token2->size)
        return 0;
    return strncmp(token1->data, token2->data, token1->size) == 0;
}

int token_eqstr(token_t* token, const char* str) {
    int size = strlen(str);
    if (token->size != size)
        return 0;
    return strncmp(token->data, str, size) == 0;
}

node_t* node_create(node_t** node_itr, type_t type, token_t* token) {
    node_t* node = *node_itr;
    *node_itr += 1;
    *node = (node_t){
        .type = type,
        .token = token,
        .bin_index = 0,
        .node_next = NULL,
        .node_parent = NULL,
        .node_child = NULL,
        .node_break = NULL,
        .node_continue = NULL};
    return node;
}

void node_addchild(node_t* parent, node_t* node) {
    if (parent->node_child == NULL) {
        parent->node_child = node;
    } else {
        node_t* current = parent->node_child;
        while (current->node_next != NULL) {
            current = current->node_next;
        }
        current->node_next = node;
    }
    node->node_parent = parent;
}

void file_read(const char* filename, char* dst, size_t dst_size) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    size_t n = fread(dst, 1, dst_size - 1, file);
    dst[n] = '\0';
    fclose(file);
}

void file_write(const char* filename, int n) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < n; i += 1) {
        fprintf(file, "%hhu\n", code_data[i]);
    }
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
        } else if (*src_itr == '(' || *src_itr == ')' || *src_itr == ',') {
            if (size > 0) {
                token_itr->data = base;
                token_itr->size = size;
                token_itr += 1;
                size = 0;
            }
            token_itr->data = src_itr;
            token_itr->size = 1;
            token_itr += 1;
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

void parse_exprlist(token_t** token_itr, node_t** node_itr, node_t* node_parent) {
    if (token_eqstr(*token_itr, "(")) {
        node_t* node_block = node_create(node_itr, TY_BLOCK, *token_itr);
        node_addchild(node_itr, node_block);
        *token_itr += 1;
        while (!token_eqstr(*token_itr, ")")) {
            parse_exprlist(token_itr, node_itr, node_block);
        }
        *token_itr += 1;
        return;
    }
    if(token_eqstr(*token_itr, "loop")) {
        node_t* node_block = node_create(node_itr, TY_BLOCK, *token_itr);
        node_t* node_continue = node_create(node_itr, TY_NOP, *token_itr);
        node_t* node_break = node_create(node_itr, TY_NOP, *token_itr);
        *token_itr += 1;
        node_block->node_continue = node_continue;
        node_block->node_break = node_break;
        node_addchild(node_parent, node_block);
        node_addchild(node_block, node_continue);
        parse_exprlist(token_itr, node_itr, node_block);
        node_addchild(node_block, node_break);
        return;
    }
    if(token_eqstr(*token_itr, "continue")) {
        node_t* node_continue = node_create(node_itr, TY_JMP, *token_itr);
        node_t* node_loop = node_parent;
        while(node_loop->node_continue == NULL) {
            node_loop = node_loop->node_parent;
        }
        node_continue->node_child = node_loop->node_continue;
        *token_itr += 1;
        node_addchild(node_parent, node_continue);
        return;
    }
    if(token_eqstr(*token_itr, "break")) {
        node_t* node_break = node_create(node_itr, TY_JMP, *token_itr);
        node_t* node_loop = node_parent;
        while(node_loop->node_break == NULL) {
            node_loop = node_loop->node_parent;
        }
        node_break->node_child = node_loop->node_break;
        *token_itr += 1;
        node_addchild(node_parent, node_break);
        return;
    }
}

void parse(token_t* token_data, node_t* node_data) {
    token_t* token_itr = token_data;
    node_t* node_itr = node_data + 1;
    node_data[0] = (node_t){
        .type = TY_NOP,
        .token = NULL,
        .bin_index = 0,
        .node_next = NULL,
        .node_parent = NULL,
        .node_child = NULL,
        .node_break = NULL,
        .node_continue = NULL};
    parse_exprlist(&token_itr, &node_itr, &node_data[0]);
}

int main() {
    file_read("src.txt", src_data, sizeof(src_data));

    tokenize(src_data, token_data);

    parse(token_data, node_data);

    return 0;
}