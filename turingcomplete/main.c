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
    int code_index;
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
node_t* var_data[2048];

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

node_t* node_create(node_t** node_itr, type_t type, token_t* token) {
    node_t* node = *node_itr;
    *node_itr += 1;
    *node = (node_t){
        .type = type,
        .token = token,
        .code_index = 0,
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

int node_provide_var(node_t** var_data, int* var_size, node_t* node) {
    for (int i = 0; i < *var_size; i++) {
        if (var_data[i] == NULL) {
            var_data[i] = node;
            return i;
        }
        if (node->token && var_data[i]->token && token_eq(var_data[i]->token, node->token)) {
            return i;
        }
    }
    var_data[*var_size] = node;
    (*var_size)++;
    return *var_size - 1;
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

void parse_exprlist(token_t** token_itr, node_t** node_itr, node_t* node_parent) {
    if (token_eqstr(*token_itr, "(")) {
        node_t* node_block = node_create(node_itr, TY_BLOCK, *token_itr);
        node_addchild(node_parent, node_block);
        *token_itr += 1;
        while (!token_eqstr(*token_itr, ")")) {
            parse_exprlist(token_itr, node_itr, node_block);
        }
        *token_itr += 1;
        return;
    }
    if (token_eqstr(*token_itr + 1, "(")) {
        if (token_eqstr(*token_itr, "input")) {
            node_t* node_input = node_create(node_itr, TY_INPUT, *token_itr);
            *token_itr += 1;
            parse_exprlist(token_itr, node_itr, node_input);
            node_addchild(node_parent, node_input);
        } else if (token_eqstr(*token_itr, "output")) {
            node_t* node_output = node_create(node_itr, TY_OUTPUT, *token_itr);
            *token_itr += 1;
            parse_exprlist(token_itr, node_itr, node_output);
            node_addchild(node_parent, node_output);
        } else {
            exit(EXIT_FAILURE);
        }
        return;
    }
    if (token_eqstr(*token_itr, "loop")) {
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
    if (token_eqstr(*token_itr, "continue")) {
        node_t* node_continue = node_create(node_itr, TY_JMP, *token_itr);
        node_t* node_loop = node_parent;
        while (node_loop->node_continue == NULL) {
            node_loop = node_loop->node_parent;
        }
        node_continue->node_child = node_loop->node_continue;
        *token_itr += 1;
        node_addchild(node_parent, node_continue);
        return;
    }
    if (token_eqstr(*token_itr, "break")) {
        node_t* node_break = node_create(node_itr, TY_JMP, *token_itr);
        node_t* node_loop = node_parent;
        while (node_loop->node_break == NULL) {
            node_loop = node_loop->node_parent;
        }
        node_break->node_child = node_loop->node_break;
        *token_itr += 1;
        node_addchild(node_parent, node_break);
        return;
    }

    if (token_eqstr(*token_itr, "&")) {
        *token_itr += 1;
        node_t* node = node_create(node_itr, TY_PUSH_ADDR, *token_itr);
        *token_itr += 1;
        node_addchild(node_parent, node);
    } else {
        node_t* node = node_create(node_itr, TY_PUSH_VAL, *token_itr);
        *token_itr += 1;
        node_addchild(node_parent, node);
    }

    if (token_eqstr(*token_itr, "=")) {
        node_t* node = node_create(node_itr, TY_ASSIGN, *token_itr);
        *token_itr += 1;
        parse_exprlist(token_itr, node_itr, node_parent);
        node_addchild(node_parent, node);
        return;
    }
    if (token_eqstr(*token_itr, "+")) {
        node_t* node = node_create(node_itr, TY_ADD, *token_itr);
        *token_itr += 1;
        parse_exprlist(token_itr, node_itr, node_parent);
        node_addchild(node_parent, node);
        return;
    }
}

void parse(token_t* token_data, node_t* node_data) {
    token_t* token_itr = token_data;
    node_t* node_itr = node_data + 1;
    node_data[0] = (node_t){
        .type = TY_BLOCK,
        .token = NULL,
        .code_index = 0,
        .node_next = NULL,
        .node_parent = NULL,
        .node_child = NULL,
        .node_break = NULL,
        .node_continue = NULL};
    parse_exprlist(&token_itr, &node_itr, &node_data[0]);
}

void codegen_node(node_t* node, char* code_data, int* code_size) {
    switch (node->type) {
        case TY_NOP:
            break;
        case TY_PUSH_VAL: {
            int val;
            if (token_isdigit(node->token)) {
                val = token_toint(node->token);
                code_data[(*code_size)++] = OP_IMMEDIATE | val;
                code_data[(*code_size)++] = OP_SYSTEM | SYS_PUSH;
            } else {
                val = node_provide_var(var_data, code_size, node);
                code_data[(*code_size)++] = OP_IMMEDIATE | val;
                code_data[(*code_size)++] = OP_COPY | REG7 << 3 | REG0;
                code_data[(*code_size)++] = OP_SYSTEM | SYS_MEM_LOAD;
                code_data[(*code_size)++] = OP_SYSTEM | SYS_PUSH;
            }
            if (val >= 64) {
                exit(EXIT_FAILURE);
            }
        } break;
        case TY_PUSH_ADDR: {
            int val = node_provide_var(var_data, code_size, node);
            code_data[(*code_size)++] = OP_IMMEDIATE | val;
            code_data[(*code_size)++] = OP_SYSTEM | SYS_PUSH;
        } break;
        case TY_ADD:
            code_data[(*code_size)++] = OP_SYSTEM | SYS_POP;
            code_data[(*code_size)++] = OP_COPY | REG1 << 3 | REG0;
            code_data[(*code_size)++] = OP_SYSTEM | SYS_POP;
            code_data[(*code_size)++] = OP_CALCULATE | CALC_ADD;
            code_data[(*code_size)++] = OP_SYSTEM | SYS_PUSH;
            break;
        case TY_ASSIGN:
            code_data[(*code_size)++] = OP_SYSTEM | SYS_POP;
            code_data[(*code_size)++] = OP_COPY | REG7 << 3 | REG0;
            code_data[(*code_size)++] = OP_SYSTEM | SYS_POP;
            code_data[(*code_size)++] = OP_SYSTEM | SYS_MEM_SAVE;
            break;
        case TY_INPUT:
            code_data[(*code_size)++] = OP_SYSTEM | SYS_INPUT;
            code_data[(*code_size)++] = OP_SYSTEM | SYS_PUSH;
            break;
        case TY_OUTPUT:
            code_data[(*code_size)++] = OP_SYSTEM | SYS_POP;
            code_data[(*code_size)++] = OP_SYSTEM | SYS_OUTPUT;
            break;
        case TY_BLOCK:
            for (node_t* child = node->node_child; child != NULL; child = child->node_next) {
                codegen_node(child, code_data, code_size);
            }
            break;
        default:
            exit(EXIT_FAILURE);
    }
}

void link_node(node_t* node, char* code_data) {
}

void codegen(node_t* node_data, char* code_data, node_t** var_data) {
    node_t* node_itr = node_data;
    int code_size = 0;
    code_data[code_size++] = OP_IMMEDIATE | 32;
    code_data[code_size++] = OP_COPY | REG5 << 3 | REG0;
    codegen_node(node_itr, code_data, &code_size);
    link_node(node_itr, code_data);
    FILE* file = fopen("code.txt", "wb");
    if (!file) {
        perror("Failed to open code file");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < code_size; i++) {
        fprintf(file, "%hhu\n", code_data[i]);
    }
    fclose(file);
}

int main() {
    file_read("src.txt", src_data, sizeof(src_data));

    tokenize(src_data, token_data);

    parse(token_data, node_data);

    codegen(node_data, code_data, var_data);

    return 0;
}