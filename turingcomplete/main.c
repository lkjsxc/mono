#include <stdio.h>
#include <stdlib.h>

#define OK 0
#define ERR 1

#define OP_IMMEDIATE 0b00000000
#define OP_CALCULATE 0b01000000
#define OP_COPY 0b10000000
#define OP_SYSTEM 0b11000000

#define CALC_OR 0b00000000
#define CALC_NAND 0b00001000
#define CALC_NOR 0b00010000
#define CALC_AND 0b00011000
#define CALC_ADD 0b00100000
#define CALC_SUB 0b00101000
#define CALC_SHL 0b00110000
#define CALC_SHR 0b00111000

#define SYS_INPUT 0b00000000
#define SYS_OUTPUT 0b00001000
#define SYS_MEM_LOAD 0b00010000
#define SYS_MEM_SAVE 0b00011000
#define SYS_JMP 0b00100000
#define SYS_JE 0b00101000
#define SYS_JNE 0b00110000
#define SYS_JL 0b00111000
#define SYS_PUSH 0b01000000
#define SYS_POP 0b01001000

#define REG0 0
#define REG1 1
#define REG2 2
#define REG3 3
#define REG4 4
#define REG5 5
#define REG6 6
#define REG7 7

typedef enum {
    TY_NULL,
    TY_PUSH_VAL,
    TY_PUSH_ADDR,
    TY_POP,
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
    if (stat.token_itr[0] == '&') {
        stat.node_itr = node_push(stat.node_itr, TY_PUSH_ADDR, stat.token_itr + 1);
        stat.token_itr = token_next(stat.token_itr);
    } else {
        stat.node_itr = node_push(stat.node_itr, TY_PUSH_VAL, stat.token_itr);
        stat.token_itr = token_next(stat.token_itr);
    }
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

stat_t emit_byte(stat_t stat, unsigned char byte) {
    code_data[stat.code_size++] = byte;
    return stat;
}

stat_t emit_immediate(stat_t stat, int value) {
    if (value < 0 || value > 63) {
        fprintf(stderr, "Immediate value out of range: %d\n", value);
        exit(ERR);
    }
    unsigned char instruction = OP_IMMEDIATE | (value & 0x3F);
    stat = emit_byte(stat, instruction);
    return stat;
}

stat_t emit_copy(stat_t stat, int src_reg, int dst_reg) {
    unsigned char instruction = OP_COPY | (dst_reg << 3) | src_reg;
    stat = emit_byte(stat, instruction);
    return stat;
}

stat_t emit_calculation(stat_t stat, int calc_op) {
    unsigned char instruction = OP_CALCULATE | calc_op;
    stat = emit_byte(stat, instruction);
    return stat;
}

stat_t emit_system(stat_t stat, int sys_op) {
    unsigned char instruction = OP_SYSTEM | sys_op;
    stat = emit_byte(stat, instruction);
    return stat;
}

void codegen(stat_t stat) {
    node_t* node_ptr = node_data;

    while (node_ptr->type != TY_NULL) {
        switch (node_ptr->type) {
            case TY_PUSH_VAL: {
                if (token_is_num(node_ptr->token)) {
                    int value = token_to_num(node_ptr->token);
                    stat = emit_immediate(stat, value);
                } else {
                    int localvar_index = localvar_provide(&stat.localvar_size, node_ptr);
                    stat = emit_immediate(stat, localvar_index);
                    stat = emit_copy(stat, REG0, REG7);
                    stat = emit_system(stat, SYS_MEM_LOAD);
                }
                stat = emit_system(stat, SYS_PUSH);
            } break;
            case TY_PUSH_ADDR: {
                int localvar_index = localvar_provide(&stat.localvar_size, node_ptr);
                stat = emit_immediate(stat, localvar_index);
                stat = emit_system(stat, SYS_PUSH);
            } break;
            case TY_ADD: {
                stat = emit_system(stat, SYS_POP);
                stat = emit_copy(stat, REG0, REG1);
                stat = emit_system(stat, SYS_POP);
                stat = emit_calculation(stat, CALC_ADD);
                stat = emit_system(stat, SYS_PUSH);
            } break;
            case TY_ASSIGN: {
                stat = emit_system(stat, SYS_POP);
                stat = emit_copy(stat, REG0, REG1);
                int localvar_index = localvar_provide(&stat.localvar_size, node_ptr);
                stat = emit_immediate(stat, localvar_index);
                stat = emit_copy(stat, REG0, REG7);
                stat = emit_system(stat, SYS_MEM_SAVE);
            } break;
        }
        node_ptr++;
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