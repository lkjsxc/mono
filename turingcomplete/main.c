#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define OP_IMMEDIATE 0b00000000
#define OP_CALCULATE 0b01000000
#define OP_COPY 0b10000000
#define OP_SYSTEM 0b11000000

#define CALC_OR 0b00000000
#define CALC_AND 0b00000001
#define CALC_XOR 0b00000010
#define CALC_NOT 0b00000011
#define CALC_ADD 0b00000100
#define CALC_SUB 0b00000101
#define CALC_SHL 0b00000110
#define CALC_SHR 0b00000111
#define CALC_NOP 0b00001000
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
    TY_DECL,
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
    struct node_t* next;
    struct node_t* parent;
    struct node_t* child;
    struct node_t* node_break;
    struct node_t* node_continue;
    int copy_src;
    int copy_dst;
    int immidiate_value;
    struct node_t* optimize_last;
} node_t;

typedef struct typetable_t {
    type_t type;
    int inst_size;
    int opcode;
} typetable_t;

typetable_t type_table[] = {
    {TY_NOP, 0, 0},
    {TY_BLOCK, 0, 0},
    {TY_DECL, 0, 0},
    {TY_IMMEDIATE, 7, OP_IMMEDIATE},
    {TY_IMMEDIATE_LABEL, 7, OP_IMMEDIATE},
    {TY_COPY, 1, OP_COPY},
    {TY_OR, 1, OP_CALCULATE | CALC_OR},
    {TY_NAND, 1, OP_CALCULATE | CALC_AND},
    {TY_NOR, 1, OP_CALCULATE | CALC_XOR},
    {TY_AND, 1, OP_CALCULATE | CALC_NOT},
    {TY_ADD, 1, OP_CALCULATE | CALC_ADD},
    {TY_SUB, 1, OP_CALCULATE | CALC_SUB},
    {TY_SHL, 1, OP_CALCULATE | CALC_SHL},
    {TY_SHR, 1, OP_CALCULATE | CALC_SHR},
    {TY_XOR, 1, OP_CALCULATE | CALC_XOR},
    {TY_EQ, 1, OP_CALCULATE | CALC_EQ},
    {TY_NEQ, 1, OP_CALCULATE | CALC_NEQ},
    {TY_LT, 1, OP_CALCULATE | CALC_LT},
    {TY_INPUT, 1, OP_SYSTEM | SYS_INPUT},
    {TY_OUTPUT, 1, OP_SYSTEM | SYS_OUTPUT},
    {TY_MEM_LOAD, 1, OP_SYSTEM | SYS_MEM_LOAD},
    {TY_MEM_SAVE, 1, OP_SYSTEM | SYS_MEM_SAVE},
    {TY_JMP, 1, OP_SYSTEM | SYS_JMP},
    {TY_JZE, 1, OP_SYSTEM | SYS_JZE},
    {TY_PUSH, 1, OP_SYSTEM | SYS_PUSH},
    {TY_POP, 1, OP_SYSTEM | SYS_POP},
    {TY_NULL, 0, 0}};

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
        .next = NULL,
        .parent = NULL,
        .child = NULL,
        .node_break = NULL,
        .node_continue = NULL,
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

void parse_loop(token_t** token_itr, node_t** node_itr, node_t* node_parent) {
    node_t* node_block = node_create(node_itr, TY_BLOCK);
    node_t* node_push_label = node_create(node_itr, TY_IMMEDIATE_LABEL);
    node_t* node_copy = node_create(node_itr, TY_COPY);
    node_t* node_jmp = node_create(node_itr, TY_JMP);
    node_t* node_label_continue = node_create(node_itr, TY_NOP);
    node_t* node_label_break = node_create(node_itr, TY_NOP);
    *token_itr += 1;
    node_push_label->child = node_label_continue;
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
}

void parse_continue(token_t** token_itr, node_t** node_itr, node_t* node_parent) {
    node_t* node_push_label = node_create(node_itr, TY_IMMEDIATE_LABEL);
    node_t* node_copy = node_create(node_itr, TY_COPY);
    node_t* node_jmp = node_create(node_itr, TY_JMP);
    node_copy->copy_src = REG0;
    node_copy->copy_dst = REG6;
    node_t* node_loop = node_parent;
    while (node_loop->node_continue == NULL) {
        node_loop = node_loop->parent;
    }
    node_push_label->child = node_loop->node_continue;
    *token_itr += 1;
    node_addchild(node_parent, node_push_label);
    node_addchild(node_parent, node_copy);
    node_addchild(node_parent, node_jmp);
}

void parse_break(token_t** token_itr, node_t** node_itr, node_t* node_parent) {
    node_t* node_push_label = node_create(node_itr, TY_IMMEDIATE_LABEL);
    node_t* node_copy = node_create(node_itr, TY_COPY);
    node_t* node_jmp = node_create(node_itr, TY_JMP);
    node_copy->copy_src = REG0;
    node_copy->copy_dst = REG6;
    node_t* node_loop = node_parent;
    while (node_loop->node_break == NULL) {
        node_loop = node_loop->parent;
    }
    node_push_label->child = node_loop->node_break;
    *token_itr += 1;
    node_addchild(node_parent, node_push_label);
    node_addchild(node_parent, node_copy);
    node_addchild(node_parent, node_jmp);
}

void parse_if(token_t** token_itr, node_t** node_itr, node_t* node_parent) {
    node_t* node_block = node_create(node_itr, TY_BLOCK);
    node_t* node_ifend_immediate = node_create(node_itr, TY_IMMEDIATE_LABEL);
    node_t* node_ifend_copy = node_create(node_itr, TY_COPY);
    node_t* node_cmp_pop = node_create(node_itr, TY_POP);
    node_t* node_jze = node_create(node_itr, TY_JZE);
    node_t* node_label_ifend = node_create(node_itr, TY_NOP);
    node_ifend_immediate->child = node_label_ifend;
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
}

void parse_decl(token_t** token_itr, node_t** node_itr, node_t* node_parent) {
    node_t* node_decl = node_create(node_itr, TY_DECL);
    *token_itr += 1;
    node_decl->token = *token_itr;
    *token_itr += 1;
    node_addchild(node_parent, node_decl);
}

void parse_function_call(token_t** token_itr, node_t** node_itr, node_t* node_parent) {
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
}

void parse_primary(token_t** token_itr, node_t** node_itr, node_t* node_parent) {
    if (token_eqstr(*token_itr, "&") && (*token_itr)->data[1] != ' ') {
        node_t* node_primary = node_create(node_itr, TY_IMMEDIATE);
        node_t* node_push = node_create(node_itr, TY_PUSH);
        *token_itr += 1;
        node_primary->token = *token_itr;
        *token_itr += 1;
        node_addchild(node_parent, node_primary);
        node_addchild(node_parent, node_push);
        node_t* node_decl = node_find_decl(node_primary);
        if (node_decl == NULL) {
            fprintf(stderr, "Error: Variable '%.*s' not declared.\n", (*token_itr)->size, (*token_itr)->data);
            exit(EXIT_FAILURE);
        }
        node_primary->child = node_decl;
        node_decl->optimize_last = node_primary;
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
        node_t* node_optimize = node_create(node_itr, TY_NOP);
        node_t* node_push = node_create(node_itr, TY_PUSH);
        node_primary->token = *token_itr;
        node_copy->copy_src = REG0;
        node_copy->copy_dst = REG7;
        *token_itr += 1;
        node_addchild(node_parent, node_primary);
        node_addchild(node_parent, node_copy);
        node_addchild(node_parent, node_mem_load);
        node_addchild(node_parent, node_optimize);
        node_addchild(node_parent, node_push);
        node_t* node_decl = node_find_decl(node_primary);
        if (node_decl == NULL) {
            fprintf(stderr, "Error: Variable '%.*s' not declared.\n", (*token_itr)->size, (*token_itr)->data);
            exit(EXIT_FAILURE);
        }
        node_primary->child = node_decl;
        node_decl->optimize_last = node_primary;
    }
}

void parse_binary_op(token_t** token_itr, node_t** node_itr, node_t* node_parent, type_t op_type) {
    node_t* node_op = node_create(node_itr, op_type);
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
    node_addchild(node_parent, node_op);
    node_addchild(node_parent, node_push);
}

void parse_assignment(token_t** token_itr, node_t** node_itr, node_t* node_parent) {
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
        parse_loop(token_itr, node_itr, node_parent);
        return;
    }
    if (token_eqstr(*token_itr, "continue")) {
        parse_continue(token_itr, node_itr, node_parent);
        return;
    }
    if (token_eqstr(*token_itr, "break")) {
        parse_break(token_itr, node_itr, node_parent);
        return;
    }
    if (token_eqstr(*token_itr, "if")) {
        parse_if(token_itr, node_itr, node_parent);
        return;
    }
    if (token_eqstr(*token_itr, "var")) {
        parse_decl(token_itr, node_itr, node_parent);
        return;
    }

    if (token_eqstr(*token_itr + 1, "(")) {
        parse_function_call(token_itr, node_itr, node_parent);
        return;
    }

    parse_primary(token_itr, node_itr, node_parent);

    if (token_eqstr(*token_itr, "=")) {
        parse_assignment(token_itr, node_itr, node_parent);
        return;
    }
    if (token_eqstr(*token_itr, "+")) {
        parse_binary_op(token_itr, node_itr, node_parent, TY_ADD);
        return;
    }
    if (token_eqstr(*token_itr, "-")) {
        parse_binary_op(token_itr, node_itr, node_parent, TY_SUB);
        return;
    }
    if (token_eqstr(*token_itr, "==")) {
        parse_binary_op(token_itr, node_itr, node_parent, TY_EQ);
        return;
    }
    if (token_eqstr(*token_itr, "!=")) {
        parse_binary_op(token_itr, node_itr, node_parent, TY_NEQ);
        return;
    }
    if (token_eqstr(*token_itr, "<")) {
        parse_binary_op(token_itr, node_itr, node_parent, TY_LT);
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
        .next = NULL,
        .parent = NULL,
        .child = NULL,
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
            for (node_t* child = node->child; child != NULL; child = child->next) {
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
        case TY_IMMEDIATE_LABEL: {
            *code_size = codegen_immediate(code_data, *code_size, 0);
        } break;
        case TY_COPY: {
            *code_size = codegen_copy(code_data, *code_size, node->copy_dst, node->copy_src);
        } break;
        default: {
            int typetable_index = -1;
            for (int i = 0; type_table[i].type != TY_NULL; i++) {
                if (type_table[i].type == node->type) {
                    typetable_index = i;
                    break;
                }
            }
            if (typetable_index == -1) {
                fprintf(stderr, "Unknown node type: %d\n", node->type);
                exit(EXIT_FAILURE);
            }
            if (type_table[typetable_index].inst_size > 0) {
                code_data[*code_size] = type_table[typetable_index].opcode;
                *code_size += type_table[typetable_index].inst_size;
            }
        } break;
    }
}

void codegen(node_t* node_data, char* code_data, node_t** nodelist_data, int* code_size) {
    int nodelist_size = 0;
    codegen_base(node_data, code_data, code_size, nodelist_data, &nodelist_size);
}

void optimize_node1(node_t* node, node_t** nodelist_data, int* nodelist_size) {
    switch (node->type) {
        case TY_BLOCK: {
            for (node_t* child = node->child; child != NULL; child = child->next) {
                optimize_node1(child, nodelist_data, nodelist_size);
            }
        } break;
        case TY_PUSH: {
            if (node->next && node->next->type == TY_POP) {
                node->type = TY_NOP;
                node->next->type = TY_NOP;
            } else {
                nodelist_data[(*nodelist_size)++] = node;
            }
        } break;
        case TY_POP: {
            node_t* last_push = nodelist_data[--(*nodelist_size)];
        } break;
        default: {
        } break;
    }
}

void optimize_node2(node_t* node, node_t** nodelist_data, int* nodelist_size, node_t** reglist_data) {
    switch (node->type) {
        case TY_BLOCK: {
            for (node_t* child = node->child; child != NULL; child = child->next) {
                optimize_node2(child, nodelist_data, nodelist_size, reglist_data);
            }
        } break;
        case TY_MEM_LOAD: {
        } break;
        default: {
        } break;
    }
}

void optimize_code(node_t* node, char* code1_data, char* code2_data, int* code2_size) {
    if (node->type == TY_BLOCK) {
        node->code_index = *code2_size;
        for (node_t* child = node->child; child != NULL; child = child->next) {
            optimize_code(child, code1_data, code2_data, code2_size);
        }
        return;
    }
    int typetable_index = -1;
    for (int i = 0; type_table[i].type != TY_NULL; i++) {
        if (type_table[i].type == node->type) {
            typetable_index = i;
            break;
        }
    }
    if(type_table[typetable_index].inst_size > 0) {
        memcpy(&code2_data[*code2_size], &code1_data[node->code_index], type_table[typetable_index].inst_size);
    }
    node->code_index = *code2_size;
    *code2_size += type_table[typetable_index].inst_size;
}

void optimize(node_t* node_data, node_t** nodelist_data, char* code1_data, char* code2_data, int* code2_size) {
    node_t* reglist_data[8] = {NULL};
    int nodelist_size1 = 0;
    int nodelist_size2 = 0;
    optimize_node1(node_data, nodelist_data, &nodelist_size1);
    optimize_node2(node_data, nodelist_data, &nodelist_size2, reglist_data);
    optimize_code(node_data, code1_data, code2_data, code2_size);
}

void codelink(node_t* node, char* code_data) {
    if (node->type == TY_BLOCK) {
        for (node_t* child = node->child; child != NULL; child = child->next) {
            codelink(child, code_data);
        }
    } else if (node->type == TY_IMMEDIATE_LABEL) {
        int target_index = node->child->code_index;
        codegen_immediate(code_data, node->code_index, target_index);
    }
}

int main() {
    static char src_data[65536];
    static char code1_data[65536];
    static char code2_data[65536];
    static token_t token_data[65536];
    static node_t node_data[65536];
    static node_t* nodelist_data[65536];
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