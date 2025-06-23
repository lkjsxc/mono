#include <stdio.h>
#include <stdlib.h>

#define OK 0
#define ERR 1

#define op_immediate 0b00000000
#define op_calulate 0b01000000
#define op_copy 0b10000000
#define op_sys 0b11000000
#define calc_add 0b00000100
#define calc_sub 0b00000101
#define sys_input 0b00000000
#define sys_output 0b00001000
#define sys_mem_load 0b00010000
#define sys_mem_save 0b00011000
#define sys_jmp 0b00100000
#define sys_je 0b00101000
#define sys_jne 0b00110000
#define sys_jl 0b00111000

typedef enum {
    TY_NULL,
    TY_PUSH,
    TY_POP1,
    TY_POP2,
    TY_POP3,
    TY_POP4,
    TY_POP5,
    TY_POP6,
    TY_ADD,
    TY_SUB,
    TY_MUL,
    TY_OR,
    TY_AND,
    TY_ASSIGN,
    TY_INPUT,
    TY_OUTPUT,
} type_t;

typedef struct node_t {
    type_t type;
    const char* token;
} node_t;

typedef struct {
    const char* token_itr;
    node_t* node_itr;
    int code_size;
    int stack_size;
    int localvar_size;
} stat_t;

char src_data[2048];
char code_data[2048];
node_t node_data[2048];
node_t* stack_data[2048];
node_t* localvar_data[2048];

int file_read(const char* filename, char* buffer, size_t buffer_size) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        exit(ERR);
    }
    size_t n = fread(buffer, 1, buffer_size - 1, file);
    buffer[n] = '\0';
    fclose(file);
    return OK;
}

int is_space(char c) {
    return c == ' ' || c == '\n';
}

const char* token_next(const char* token) {
    while (!is_space(*token)) {
        token++;
    }
    while (is_space(*token)) {
        token++;
    }
    return token;
}

int token_eq(const char* token1, const char* token2) {
    while (*token1 == *token2) {
        token1++;
        token2++;
    }
    return is_space(*token1) && is_space(*token2);
}

int token_eqstr(const char* token, const char* str) {
    while (*token == *str) {
        token++;
        str++;
    }
    return is_space(*token) && *str == '\0';
}

int token_is_num(const char* token) {
    return '0' <= *token && *token <= '9';
}

int token_to_num(const char* token) {
    int value = 0;
    while ('0' <= *token && *token <= '9') {
        value = value * 10 + (*token - '0');
        token++;
    }
    return value;
}

node_t* node_push(node_t* node_itr, type_t type, const char* token) {
    node_itr->type = type;
    node_itr->token = token;
    return node_itr + 1;
}

int localvar_provide(int* localvar_size, node_t* node) {
    for (int i = 0; i < *localvar_size; i++) {
        if (token_eq(localvar_data[i]->token, node->token)) {
            return i;
        }
    }
    localvar_data[*localvar_size] = node;
    (*localvar_size)++;
    return *localvar_size - 1;
}

stat_t parse_primary(stat_t stat) {
    stat.node_itr = node_push(stat.node_itr, TY_PUSH, stat.token_itr);
    stat.token_itr = token_next(stat.token_itr);
    return stat;
}

stat_t parse_exprlist(stat_t stat) {
    if (token_eqstr(stat.token_itr, "(")) {
        stat.token_itr = token_next(stat.token_itr);
        while (!token_eqstr(stat.token_itr, ")")) {
            stat = parse_exprlist(stat);
        }
        stat.token_itr = token_next(stat.token_itr);
        return stat;
    }
    stat = parse_primary(stat);
    if (token_eqstr(stat.token_itr, "+")) {
        const char* token = stat.token_itr;
        stat.token_itr = token_next(stat.token_itr);
        stat = parse_exprlist(stat);
        stat.node_itr = node_push(stat.node_itr, TY_ADD, token);
    } else if (token_eqstr(stat.token_itr, "=")) {
        const char* token = stat.token_itr;
        stat.token_itr = token_next(stat.token_itr);
        stat = parse_exprlist(stat);
        stat.node_itr = node_push(stat.node_itr, TY_ASSIGN, token);
    }
    return stat;
}

void codegen(stat_t stat) {
    while (stat.node_itr->type != TY_NULL) {
        switch (stat.node_itr->type) {
            case TY_PUSH:
                stack_data[stat.stack_size++] = stat.node_itr;
                break;
            case TY_ADD: {
            } break;
        }
        stat.node_itr++;
    }
    FILE* code_file = fopen("code.txt", "wb");
    for (int i = 0; i < stat.code_size; i++) {
        fprintf(code_file, "%hhu\n", code_data[i]);
    }
    fclose(code_file);
}

int main() {
    if (file_read("src.txt", src_data, sizeof(src_data)) == ERR) {
        return ERR;
    }

    stat_t stat1 = {.token_itr = src_data, .node_itr = node_data, .code_size = 0, .stack_size = 0, .localvar_size = 0};

    stat_t stat2 = parse_exprlist(stat1);

    stat2.node_itr = node_push(stat2.node_itr, TY_NULL, NULL);

    codegen(stat1);

    return OK;
}