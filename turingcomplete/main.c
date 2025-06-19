#include <stdio.h>

#define OK 0
#define ERR 1

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
} stat_t;

char src_data[2048];
char code_data[2048];
node_t* stack_data[2048];
node_t node_data[2048];

int file_read(const char* filename, char* buffer, size_t buffer_size) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return ERR;
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

int token_reg_to_num(const char* token) {
    if (token_eqstr(token, "r0")) {
        return 0;
    }
    if (token_eqstr(token, "r1")) {
        return 1;
    }
    if (token_eqstr(token, "r2")) {
        return 2;
    }
    if (token_eqstr(token, "r3")) {
        return 3;
    }
    if (token_eqstr(token, "r4")) {
        return 4;
    }
    if (token_eqstr(token, "r5")) {
        return 5;
    }
}

node_t* node_push(node_t* node_itr, type_t type, const char* token) {
    node_itr->type = type;
    node_itr->token = token;
    return node_itr + 1;
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
    int code_size = 0;
    int stack_size = 0;
    for (node_t* node = stat.node_itr; node->type != TY_NULL; node++) {
        switch (node->type) {
            case TY_PUSH: {
                if (token_is_num(node->token)) {
                    int val = token_to_num(node->token);
                    code_data[code_size++] = val;
                } else {
                    int val = token_reg_to_num(node->token);
                    code_data[code_size++] = 0b1000000 | val;
                }
            } break;
            case TY_ASSIGN: {
                int dst = token_reg_to_num(stack_data[--stack_size]->token);
                int src = token_reg_to_num(stack_data[--stack_size]->token);
                code_data[code_size++] = 0b10000000 | (dst << 3) | src;
            } break;
            case TY_ADD:
                code_data[code_size++] = 0b01000100;
                break;
            case TY_SUB:
                code_data[code_size++] = 0b01000101;
                break;
            case TY_MUL:
                code_data[code_size++] = 0b01000110;
                break;
            case TY_OR:
                code_data[code_size++] = 0b01000000;
                break;
            case TY_AND:
                code_data[code_size++] = 0b01000011;
                break;
            default:
                fprintf(stderr, "Error: Unknown node type %d\n", node->type);
        }
    }
    FILE* code_file = fopen("code.txt", "wb");
    for (int i = 0; i < code_size; i++) {
        fprintf(code_file, "%d\n", code_data[i]);
    }
    fclose(code_file);
}

int main() {
    if (file_read("src.txt", src_data, sizeof(src_data)) == ERR) {
        return ERR;
    }

    stat_t stat1 = {src_data, node_data};

    stat_t stat2 = parse_exprlist(stat1);

    stat2.node_itr = node_push(stat2.node_itr, TY_NULL, NULL);

    codegen(stat1);

    return OK;
}