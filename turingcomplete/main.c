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
#define CALC_XOR 0b00001000
#define CALC_EQ 0b00001001
#define CALC_NEQ 0b00001010
#define CALC_LT 0b00001011

#define SYS_INPUT 0b00000000
#define SYS_OUTPUT 0b00000001
#define SYS_MEM_LOAD 0b00000010
#define SYS_MEM_SAVE 0b00000011
#define SYS_JMP 0b00000100
#define SYS_JZE 0b00000101
#define SYS_PUSH 0b00000110
#define SYS_POP 0b00000111

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
    TY_IMMEDIATE,
    TY_IMMEDIATE_LABEL,
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
    TY_MEM_LOAD,
    TY_MEM_SAVE,
    TY_JMP,
    TY_JZE,
    TY_PUSH,
    TY_POP,
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
    int copy_src;
    int copy_dst;
    int immidiate_value;
    int optimize_isreduce;
} node_t;

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

node_t* node_create(node_t** node_itr, type_t type) {
    node_t* node = *node_itr;
    *node_itr += 1;
    *node = (node_t){
        .type = type,
        .token = NULL,
        .code_index = 0,
        .node_next = NULL,
        .node_parent = NULL,
        .node_child = NULL,
        .node_break = NULL,
        .node_continue = NULL,
    };
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

int node_provide_var(node_t** nodelist_data, int* nodelist_size, node_t* node) {
    for (int i = 0; i < *nodelist_size; i++) {
        if (token_eq(nodelist_data[i]->token, node->token)) {
            return i;
        }
    }
    nodelist_data[(*nodelist_size)++] = node;
    return (*nodelist_size) - 1;
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
        node_t* node_block = node_create(node_itr, TY_BLOCK);
        node_addchild(node_parent, node_block);
        *token_itr += 1;
        while (!token_eqstr(*token_itr, ")")) {
            parse_exprlist(token_itr, node_itr, node_block);
        }
        *token_itr += 1;
        return;
    }

    if (token_eqstr(*token_itr, "loop")) {
        node_t* node_block = node_create(node_itr, TY_BLOCK);
        node_t* node_push_label = node_create(node_itr, TY_IMMEDIATE_LABEL);
        node_t* node_copy = node_create(node_itr, TY_COPY);
        node_t* node_jmp = node_create(node_itr, TY_JMP);
        node_t* node_label_continue = node_create(node_itr, TY_NOP);
        node_t* node_label_break = node_create(node_itr, TY_NOP);
        *token_itr += 1;
        node_push_label->node_child = node_label_continue;
        node_copy->copy_src = REG0;
        node_copy->copy_dst = REG6;
        node_block->node_continue = node_label_continue;
        node_block->node_break = node_label_break;
        node_addchild(node_parent, node_block);
        node_addchild(node_block, node_label_continue);
        parse_exprlist(token_itr, node_itr, node_block);
        node_addchild(node_block, node_push_label);
        node_addchild(node_block, node_copy);
        node_addchild(node_block, node_jmp);
        node_addchild(node_block, node_label_break);
        return;
    }
    if (token_eqstr(*token_itr, "continue")) {
        node_t* node_push_label = node_create(node_itr, TY_IMMEDIATE_LABEL);
        node_t* node_copy = node_create(node_itr, TY_COPY);
        node_t* node_jmp = node_create(node_itr, TY_JMP);
        node_copy->copy_src = REG0;
        node_copy->copy_dst = REG6;
        node_t* node_loop = node_parent;
        while (node_loop->node_continue == NULL) {
            node_loop = node_loop->node_parent;
        }
        node_push_label->node_child = node_loop->node_continue;
        *token_itr += 1;
        node_addchild(node_parent, node_push_label);
        node_addchild(node_parent, node_copy);
        node_addchild(node_parent, node_jmp);
        return;
    }
    if (token_eqstr(*token_itr, "break")) {
        node_t* node_push_label = node_create(node_itr, TY_IMMEDIATE_LABEL);
        node_t* node_copy = node_create(node_itr, TY_COPY);
        node_t* node_jmp = node_create(node_itr, TY_JMP);
        node_copy->copy_src = REG0;
        node_copy->copy_dst = REG6;
        node_t* node_loop = node_parent;
        while (node_loop->node_break == NULL) {
            node_loop = node_loop->node_parent;
        }
        node_push_label->node_child = node_loop->node_break;
        *token_itr += 1;
        node_addchild(node_parent, node_push_label);
        node_addchild(node_parent, node_copy);
        node_addchild(node_parent, node_jmp);
        return;
    }
    if (token_eqstr(*token_itr, "if")) {
        node_t* node_block = node_create(node_itr, TY_BLOCK);
        node_t* node_ifend_immediate = node_create(node_itr, TY_IMMEDIATE_LABEL);
        node_t* node_ifend_copy = node_create(node_itr, TY_COPY);
        node_t* node_cmp_pop = node_create(node_itr, TY_POP);
        node_t* node_jze = node_create(node_itr, TY_JZE);
        node_t* node_label_ifend = node_create(node_itr, TY_NOP);
        node_ifend_immediate->node_child = node_label_ifend;
        node_ifend_copy->copy_src = REG0;
        node_ifend_copy->copy_dst = REG6;
        *token_itr += 1;
        node_addchild(node_parent, node_block);
        parse_exprlist(token_itr, node_itr, node_block);
        node_addchild(node_block, node_ifend_immediate);
        node_addchild(node_block, node_ifend_copy);
        node_addchild(node_block, node_cmp_pop);
        node_addchild(node_block, node_jze);
        parse_exprlist(token_itr, node_itr, node_block);
        node_addchild(node_block, node_label_ifend);
        return;
    }

    if (token_eqstr(*token_itr + 1, "(")) {
        if (token_eqstr(*token_itr, "input")) {
            node_t* node_input = node_create(node_itr, TY_INPUT);
            node_t* node_push = node_create(node_itr, TY_PUSH);
            *token_itr += 1;
            parse_exprlist(token_itr, node_itr, node_parent);
            node_addchild(node_parent, node_input);
            node_addchild(node_parent, node_push);
        } else if (token_eqstr(*token_itr, "output")) {
            node_t* node_output = node_create(node_itr, TY_OUTPUT);
            node_t* node_pop = node_create(node_itr, TY_POP);
            *token_itr += 1;
            parse_exprlist(token_itr, node_itr, node_parent);
            node_addchild(node_parent, node_pop);
            node_addchild(node_parent, node_output);
        } else {
            exit(EXIT_FAILURE);
        }
        return;
    }

    if (token_eqstr(*token_itr, "&")) {
        node_t* node_primary = node_create(node_itr, TY_IMMEDIATE);
        node_t* node_push = node_create(node_itr, TY_PUSH);
        *token_itr += 1;
        node_primary->token = *token_itr;
        *token_itr += 1;
        node_addchild(node_parent, node_primary);
        node_addchild(node_parent, node_push);
    } else if (token_isdigit(*token_itr)) {
        node_t* node_primary = node_create(node_itr, TY_IMMEDIATE);
        node_t* node_push = node_create(node_itr, TY_PUSH);
        node_primary->token = *token_itr;
        *token_itr += 1;
        node_addchild(node_parent, node_primary);
        node_addchild(node_parent, node_push);
    } else {
        node_t* node_primary = node_create(node_itr, TY_IMMEDIATE);
        node_t* node_copy = node_create(node_itr, TY_COPY);
        node_t* node_mem_load = node_create(node_itr, TY_MEM_LOAD);
        node_t* node_push = node_create(node_itr, TY_PUSH);
        node_primary->token = *token_itr;
        node_copy->copy_src = REG0;
        node_copy->copy_dst = REG7;
        *token_itr += 1;
        node_addchild(node_parent, node_primary);
        node_addchild(node_parent, node_copy);
        node_addchild(node_parent, node_mem_load);
        node_addchild(node_parent, node_push);
    }

    if (token_eqstr(*token_itr, "=")) {
        node_t* node_mem_save = node_create(node_itr, TY_MEM_SAVE);
        node_t* node_pop1 = node_create(node_itr, TY_POP);
        node_t* node_pop2 = node_create(node_itr, TY_POP);
        node_t* node_copy1 = node_create(node_itr, TY_COPY);
        node_t* node_copy2 = node_create(node_itr, TY_COPY);
        node_t* node_copy3 = node_create(node_itr, TY_COPY);
        node_copy1->copy_src = REG0;
        node_copy1->copy_dst = REG1;
        node_copy2->copy_src = REG0;
        node_copy2->copy_dst = REG7;
        node_copy3->copy_src = REG1;
        node_copy3->copy_dst = REG0;
        *token_itr += 1;
        parse_exprlist(token_itr, node_itr, node_parent);
        node_addchild(node_parent, node_pop1);
        node_addchild(node_parent, node_copy1);
        node_addchild(node_parent, node_pop2);
        node_addchild(node_parent, node_copy2);
        node_addchild(node_parent, node_copy3);
        node_addchild(node_parent, node_mem_save);
        return;
    }
    if (token_eqstr(*token_itr, "+")) {
        node_t* node_add = node_create(node_itr, TY_ADD);
        node_t* node_pop1 = node_create(node_itr, TY_POP);
        node_t* node_pop2 = node_create(node_itr, TY_POP);
        node_t* node_copy1 = node_create(node_itr, TY_COPY);
        node_t* node_push = node_create(node_itr, TY_PUSH);
        node_copy1->copy_src = REG0;
        node_copy1->copy_dst = REG1;
        *token_itr += 1;
        parse_exprlist(token_itr, node_itr, node_parent);
        node_addchild(node_parent, node_pop1);
        node_addchild(node_parent, node_copy1);
        node_addchild(node_parent, node_pop2);
        node_addchild(node_parent, node_add);
        node_addchild(node_parent, node_push);
        return;
    }
    if (token_eqstr(*token_itr, "-")) {
        node_t* node_sub = node_create(node_itr, TY_SUB);
        node_t* node_pop1 = node_create(node_itr, TY_POP);
        node_t* node_pop2 = node_create(node_itr, TY_POP);
        node_t* node_copy1 = node_create(node_itr, TY_COPY);
        node_t* node_push = node_create(node_itr, TY_PUSH);
        node_copy1->copy_src = REG0;
        node_copy1->copy_dst = REG1;
        *token_itr += 1;
        parse_exprlist(token_itr, node_itr, node_parent);
        node_addchild(node_parent, node_pop1);
        node_addchild(node_parent, node_copy1);
        node_addchild(node_parent, node_pop2);
        node_addchild(node_parent, node_sub);
        node_addchild(node_parent, node_push);
        return;
    }
    if (token_eqstr(*token_itr, "==")) {
        node_t* node_eq = node_create(node_itr, TY_EQ);
        node_t* node_pop1 = node_create(node_itr, TY_POP);
        node_t* node_pop2 = node_create(node_itr, TY_POP);
        node_t* node_copy1 = node_create(node_itr, TY_COPY);
        node_t* node_push = node_create(node_itr, TY_PUSH);
        node_copy1->copy_src = REG0;
        node_copy1->copy_dst = REG1;
        *token_itr += 1;
        parse_exprlist(token_itr, node_itr, node_parent);
        node_addchild(node_parent, node_pop1);
        node_addchild(node_parent, node_copy1);
        node_addchild(node_parent, node_pop2);
        node_addchild(node_parent, node_eq);
        node_addchild(node_parent, node_push);
        return;
    }
    if (token_eqstr(*token_itr, "!=")) {
        node_t* node_neq = node_create(node_itr, TY_NEQ);
        node_t* node_pop1 = node_create(node_itr, TY_POP);
        node_t* node_pop2 = node_create(node_itr, TY_POP);
        node_t* node_copy1 = node_create(node_itr, TY_COPY);
        node_t* node_push = node_create(node_itr, TY_PUSH);
        node_copy1->copy_src = REG0;
        node_copy1->copy_dst = REG1;
        *token_itr += 1;
        parse_exprlist(token_itr, node_itr, node_parent);
        node_addchild(node_parent, node_pop1);
        node_addchild(node_parent, node_copy1);
        node_addchild(node_parent, node_pop2);
        node_addchild(node_parent, node_neq);
        node_addchild(node_parent, node_push);
        return;
    }
    if (token_eqstr(*token_itr, "<")) {
        node_t* node_lt = node_create(node_itr, TY_LT);
        node_t* node_pop1 = node_create(node_itr, TY_POP);
        node_t* node_pop2 = node_create(node_itr, TY_POP);
        node_t* node_copy1 = node_create(node_itr, TY_COPY);
        node_t* node_push = node_create(node_itr, TY_PUSH);
        node_copy1->copy_src = REG0;
        node_copy1->copy_dst = REG1;
        *token_itr += 1;
        parse_exprlist(token_itr, node_itr, node_parent);
        node_addchild(node_parent, node_pop1);
        node_addchild(node_parent, node_copy1);
        node_addchild(node_parent, node_pop2);
        node_addchild(node_parent, node_lt);
        node_addchild(node_parent, node_push);
        return;
    }
}

void parse(token_t* token_data, node_t* node_data) {
    token_t* token_itr = token_data;
    node_t* node_itr = node_data;
    *(node_itr++) = (node_t){
        .type = TY_BLOCK,
        .token = NULL,
        .code_index = 0,
        .node_next = NULL,
        .node_parent = NULL,
        .node_child = NULL,
        .node_break = NULL,
        .node_continue = NULL};
    node_t* node_sp_immediate = node_create(&node_itr, TY_IMMEDIATE);
    node_t* node_sp_copy = node_create(&node_itr, TY_COPY);
    node_sp_immediate->immidiate_value = 128;
    node_sp_copy->copy_dst = REG5;
    node_sp_copy->copy_src = REG0;
    node_addchild(&node_data[0], node_sp_immediate);
    node_addchild(&node_data[0], node_sp_copy);
    parse_exprlist(&token_itr, &node_itr, &node_data[0]);
}

int codegen_copy(char* code_data, int code_index, int dst_reg, int src_reg) {
    code_data[code_index++] = OP_COPY | dst_reg << 3 | src_reg;
    return code_index;
}

int codegen_immediate(char* code_data, int code_index, unsigned char value) {
    code_data[code_index++] = OP_IMMEDIATE | 4;
    code_index = codegen_copy(code_data, code_index, REG1, REG0);
    code_data[code_index++] = OP_IMMEDIATE | (value >> 4);
    code_data[code_index++] = OP_CALCULATE | CALC_SHL;
    code_index = codegen_copy(code_data, code_index, REG1, REG0);
    code_data[code_index++] = OP_IMMEDIATE | (value & 0b00001111);
    code_data[code_index++] = OP_CALCULATE | CALC_OR;
    return code_index;
}

void codegen_base(node_t* node, char* code_data, int* code_size, node_t** nodelist_data, int* nodelist_size) {
    node->code_index = *code_size;
    switch (node->type) {
        case TY_BLOCK: {
            for (node_t* child = node->node_child; child != NULL; child = child->node_next) {
                codegen_base(child, code_data, code_size, nodelist_data, nodelist_size);
            }
        } break;
        case TY_NOP: {
        } break;
        case TY_IMMEDIATE: {
            int value;
            if (node->token == NULL) {
                value = node->immidiate_value;
            } else if (token_isdigit(node->token)) {
                value = token_toint(node->token);
            } else {
                value = node_provide_var(nodelist_data, nodelist_size, node);
            }
            node->immidiate_value = value;
            *code_size = codegen_immediate(code_data, *code_size, value);
        } break;
        case TY_COPY: {
            *code_size = codegen_copy(code_data, *code_size, node->copy_dst, node->copy_src);
        } break;
        case TY_OR: {
            code_data[(*code_size)++] = OP_CALCULATE | CALC_OR;
        } break;
        case TY_NAND: {
            code_data[(*code_size)++] = OP_CALCULATE | CALC_NAND;
        } break;
        case TY_NOR: {
            code_data[(*code_size)++] = OP_CALCULATE | CALC_NOR;
        } break;
        case TY_AND: {
            code_data[(*code_size)++] = OP_CALCULATE | CALC_AND;
        } break;
        case TY_ADD: {
            code_data[(*code_size)++] = OP_CALCULATE | CALC_ADD;
        } break;
        case TY_SUB: {
            code_data[(*code_size)++] = OP_CALCULATE | CALC_SUB;
        } break;
        case TY_SHL: {
            code_data[(*code_size)++] = OP_CALCULATE | CALC_SHL;
        } break;
        case TY_SHR: {
            code_data[(*code_size)++] = OP_CALCULATE | CALC_SHR;
        } break;
        case TY_XOR: {
            code_data[(*code_size)++] = OP_CALCULATE | CALC_XOR;
        } break;
        case TY_EQ: {
            code_data[(*code_size)++] = OP_CALCULATE | CALC_EQ;
        } break;
        case TY_NEQ: {
            code_data[(*code_size)++] = OP_CALCULATE | CALC_NEQ;
        } break;
        case TY_LT: {
            code_data[(*code_size)++] = OP_CALCULATE | CALC_LT;
        } break;
        case TY_INPUT: {
            code_data[(*code_size)++] = OP_SYSTEM | SYS_INPUT;
        } break;
        case TY_OUTPUT: {
            code_data[(*code_size)++] = OP_SYSTEM | SYS_OUTPUT;
        } break;
        case TY_MEM_LOAD: {
            code_data[(*code_size)++] = OP_SYSTEM | SYS_MEM_LOAD;
        } break;
        case TY_MEM_SAVE: {
            code_data[(*code_size)++] = OP_SYSTEM | SYS_MEM_SAVE;
        } break;
        case TY_JMP: {
            code_data[(*code_size)++] = OP_SYSTEM | SYS_JMP;
        } break;
        case TY_JZE: {
            code_data[(*code_size)++] = OP_SYSTEM | SYS_JZE;
        } break;
        case TY_PUSH: {
            code_data[(*code_size)++] = OP_SYSTEM | SYS_PUSH;
        } break;
        case TY_POP: {
            code_data[(*code_size)++] = OP_SYSTEM | SYS_POP;
        } break;
        case TY_IMMEDIATE_LABEL: {
            *code_size = codegen_immediate(code_data, *code_size, 0);
        } break;
        default:
            fprintf(stderr, "Unknown node type: %d\n", node->type);
            exit(EXIT_FAILURE);
    }
}

void codegen(node_t* node_data, char* code_data, node_t** nodelist_data, int* code_size) {
    int nodelist_size = 0;
    codegen_base(node_data, code_data, code_size, nodelist_data, &nodelist_size);
}

void optimize1(node_t* node, node_t** nodelist_data, int* nodelist_size, node_t** reglist_data) {
    if (node->optimize_isreduce) {
        return;
    }
    switch (node->type) {
        case TY_BLOCK: {
            for (node_t* child = node->node_child; child != NULL; child = child->node_next) {
                optimize1(child, nodelist_data, nodelist_size, reglist_data);
            }
        } break;
        case TY_PUSH: {
            if (node->node_next && node->node_next->type == TY_POP) {
                node->optimize_isreduce = 1;
                node->node_next->optimize_isreduce = 1;
            } else {
                nodelist_data[(*nodelist_size)++] = node;
            }
        } break;
        case TY_POP: {
            node_t* last_push = nodelist_data[--(*nodelist_size)];
            reglist_data[0] = last_push;
        } break;
        case TY_COPY: {
            reglist_data[node->copy_dst] = reglist_data[node->copy_src];
        } break;
        case TY_IMMEDIATE:
        case TY_IMMEDIATE_LABEL:
        case TY_OR:
        case TY_NAND:
        case TY_NOR:
        case TY_AND:
        case TY_ADD:
        case TY_SUB:
        case TY_SHL:
        case TY_SHR:
        case TY_XOR:
        case TY_EQ:
        case TY_NEQ:
        case TY_LT:
        case TY_INPUT:
        case TY_MEM_LOAD: {
            reglist_data[0] = node;
        } break;
        case TY_MEM_SAVE: {
        } break;
        default: {
        } break;
    }
}

void optimize2(node_t* node, char* code1_data, char* code2_data, int* code2_size) {
    if (node->type == TY_BLOCK) {
        node->code_index = *code2_size;
        for (node_t* child = node->node_child; child != NULL; child = child->node_next) {
            optimize2(child, code1_data, code2_data, code2_size);
        }
        return;
    }
    if (node->type == TY_NOP) {
        node->code_index = *code2_size;
        return;
    }
    if (node->optimize_isreduce) {
        node->code_index = *code2_size;
        return;
    }
    if ((node->type == TY_IMMEDIATE || node->type == TY_IMMEDIATE_LABEL)) {
        memcpy(code2_data + *code2_size, code1_data + node->code_index, 7);
        node->code_index = *code2_size;
        *code2_size += 7;
    } else {
        code2_data[*code2_size] = code1_data[node->code_index];
        node->code_index = *code2_size;
        *code2_size += 1;
    }
}

void optimize(node_t* node_data, node_t** nodelist_data, char* code1_data, char* code2_data, int* code2_size) {
    node_t* reglist_data[8] = {NULL};
    int nodelist_size = 0;
    optimize1(node_data, nodelist_data, &nodelist_size, reglist_data);
    optimize2(node_data, code1_data, code2_data, code2_size);
}

void codelink(node_t* node, char* code_data) {
    if (node->type == TY_BLOCK) {
        for (node_t* child = node->node_child; child != NULL; child = child->node_next) {
            codelink(child, code_data);
        }
    } else if (node->optimize_isreduce) {
        return;
    } else if (node->type == TY_IMMEDIATE_LABEL) {
        int target_index = node->node_child->code_index;
        codegen_immediate(code_data, node->code_index, target_index);
    }
}

int main() {
    char src_data[4096];
    char code1_data[4096];
    char code2_data[4096];
    token_t token_data[4096];
    node_t node_data[4096];
    node_t* nodelist_data[4096];
    int code1_size = 0;
    int code2_size = 0;

    file_read("src.txt", src_data, sizeof(src_data));

    tokenize(src_data, token_data);

    parse(token_data, node_data);

    codegen(node_data, code1_data, nodelist_data, &code1_size);

    optimize(node_data, nodelist_data, code1_data, code2_data, &code2_size);

    codelink(node_data, code2_data);

    FILE* file = fopen("code.txt", "wb");
    if (!file) {
        perror("Failed to open code file");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < code2_size; i++) {
        fprintf(file, "%hhu\n", code2_data[i]);
    }
    fclose(file);

    return 0;
}