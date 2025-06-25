#include <stdio.h>
#include <stdlib.h>

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

typedef enum result_t {
    OK = 0,
    ERR = 1,
} result_t;

typedef enum type_t {
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
    TY_CALL,
    TY_RETURN,
    TY_JMP,
    TY_JE,
    TY_JNE,
    TY_JL,
    TY_INPUT,
    TY_OUTPUT,
} type_t;

typedef struct token_t {
    const char* data;
    int size;
} token_t;

typedef struct node_t {
    type_t type;
    token_t* token;
} node_t;

typedef struct stat_t {
    token_t* token_itr;
    node_t* node_itr;
    node_t* parent;
} stat_t;

char src_data[2048];
char code_data[2048];
token_t token_data[2048];
node_t node_data[2048];
node_t* stack_data[2048];
node_t* localvar_data[2048];

__attribute__((warn_unused_result)) result_t file_read(const char* filename, char* dst, size_t dst_size) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return ERR;
    }
    size_t n = fread(dst, 1, dst_size - 1, file);
    dst[n] = '\0';
    fclose(file);
    return OK;
}

__attribute__((warn_unused_result)) result_t file_write(const char* filename, int n) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open file");
        return ERR;
    }
    for (int i = 0; i < n; i++) {
        fprintf(file, "%hhu\n", code_data[i]);
    }
    fclose(file);
    return OK;
}

int main() {
    if (file_read("src.txt", src_data, sizeof(src_data)) == ERR) {
        return ERR;
    }

    return OK;
}