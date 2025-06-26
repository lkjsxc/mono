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
    TY_IMMEDIATE,
    TY_COPY,
    TY_OR,
    TY_NAND,
    TY_NOR,
    TY_AND,
    TY_ADD,
    TY_SUB,
    TY_SHL,
    TY_SHR,
    TY_INPUT,
    TY_OUTPUT,
    TY_MEM_LOAD,
    TY_MEM_SAVE,
    TY_JMP,
    TY_JE,
    TY_JNE,
    TY_JL,
    TY_PUSH,
    TY_POP,
    TY_BLOCK,
    TY_PUSH_LABEL,
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
    int reg_index;
    int copy_src;
    int copy_dst;
} node_t;

char src_data[2048];
char code_data[2048];
token_t token_data[2048];
node_t node_data[2048];
node_t* nodelist_data[2048];

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
        .reg_index = -1};
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
        node_t* node_push_label = node_create(node_itr, TY_PUSH_LABEL);
        node_t* node_copy = node_create(node_itr, TY_COPY);
        node_t* node_jmp = node_create(node_itr, TY_JMP);
        node_t* node_continue = node_create(node_itr, TY_NOP);
        node_t* node_break = node_create(node_itr, TY_NOP);
        *token_itr += 1;
        node_push_label->node_child = node_continue;
        node_copy->copy_src = 0;
        node_copy->copy_dst = 6;
        node_block->node_continue = node_continue;
        node_block->node_break = node_break;
        node_addchild(node_parent, node_block);
        node_addchild(node_block, node_continue);
        parse_exprlist(token_itr, node_itr, node_block);
        node_addchild(node_block, node_push_label);
        node_addchild(node_block, node_copy);
        node_addchild(node_block, node_jmp);
        node_addchild(node_block, node_break);
        return;
    }
    if (token_eqstr(*token_itr, "continue")) {
        node_t* node_push_label = node_create(node_itr, TY_PUSH_LABEL);
        node_t* node_copy = node_create(node_itr, TY_COPY);
        node_t* node_jmp = node_create(node_itr, TY_JMP);
        node_copy->copy_src = 0;
        node_copy->copy_dst = 6;
        node_t* node_loop = node_parent;
        while(node_loop->node_continue == NULL) {
            node_loop = node_loop->node_parent;
        }
        node_push_label->node_child = node_loop->node_continue;
        *token_itr += 1;
        node_addchild(node_parent, node_push_label);
        node_addchild(node_parent, node_copy);
        node_addchild(node_parent, node_jmp);
    }
    if (token_eqstr(*token_itr, "break")) {
        node_t* node_push_label = node_create(node_itr, TY_PUSH_LABEL);
        node_t* node_copy = node_create(node_itr, TY_COPY);
        node_t* node_jmp = node_create(node_itr, TY_JMP);
        node_copy->copy_src = 0;
        node_copy->copy_dst = 6;
        node_t* node_loop = node_parent;
        while(node_loop->node_break == NULL) {
            node_loop = node_loop->node_parent;
        }
        node_push_label->node_child = node_loop->node_break;
        *token_itr += 1;
        node_addchild(node_parent, node_push_label);
        node_addchild(node_parent, node_copy);
        node_addchild(node_parent, node_jmp);
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
        node_copy->copy_src = 0;
        node_copy->copy_dst = 7;
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
        node_copy1->copy_src = 0;
        node_copy1->copy_dst = 1;
        node_copy2->copy_src = 0;
        node_copy2->copy_dst = 7;
        node_copy3->copy_src = 1;
        node_copy3->copy_dst = 0;
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
        node_copy1->copy_src = 0;
        node_copy1->copy_dst = 1;
        *token_itr += 1;
        parse_exprlist(token_itr, node_itr, node_parent);
        node_addchild(node_parent, node_pop1);
        node_addchild(node_parent, node_copy1);
        node_addchild(node_parent, node_pop2);
        node_addchild(node_parent, node_add);
        node_addchild(node_parent, node_push);
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

void optimize_block(node_t* node, node_t** nodelist_data, int* nodelist_size, node_t** reglist_data) {
    if (node->type == TY_BLOCK) {
        for (node_t* child = node->node_child; child != NULL; child = child->node_next) {
            optimize_block(child, nodelist_data, nodelist_size, reglist_data);
        }
    }
}

void optimize(node_t* node, node_t** nodelist_data) {
    node_t* reglist_data[8] = {NULL};
    int nodelist_size = 0;
    optimize_block(node, nodelist_data, &nodelist_size, reglist_data);
}

void codegen_pre(char* code_data, int* code_size) {
    code_data[(*code_size)++] = OP_IMMEDIATE | 63;
    code_data[(*code_size)++] = OP_COPY | REG5 << 3 | REG0;
}

void codegen_base(node_t* node, char* code_data, int* code_size, node_t** nodelist_data, int* nodelist_size) {
    node->code_index = *code_size;
    switch (node->type) {
        case TY_NOP: {
        } break;
        case TY_IMMEDIATE: {
            if (token_isdigit(node->token)) {
                int value = token_toint(node->token);
                code_data[(*code_size)++] = OP_IMMEDIATE | value;
            } else {
                int value = node_provide_var(nodelist_data, nodelist_size, node);
                code_data[(*code_size)++] = OP_IMMEDIATE | value;
            }
        } break;
        case TY_COPY: {
            int src_reg = node->copy_src;
            int dst_reg = node->copy_dst;
            code_data[(*code_size)++] = OP_COPY | dst_reg << 3 | src_reg;
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
        case TY_PUSH: {
            code_data[(*code_size)++] = OP_SYSTEM | SYS_PUSH;
        } break;
        case TY_POP: {
            code_data[(*code_size)++] = OP_SYSTEM | SYS_POP;
        } break;
        case TY_BLOCK: {
            for (node_t* child = node->node_child; child != NULL; child = child->node_next) {
                codegen_base(child, code_data, code_size, nodelist_data, nodelist_size);
            }
            break;
        }
        case TY_PUSH_LABEL: {
            code_data[(*code_size)++] = OP_IMMEDIATE | 0;
        } break;
        default:
            fprintf(stderr, "Unknown node type: %d\n", node->type);
            exit(EXIT_FAILURE);
    }
}

void codegen_link(node_t* node, char* code_data) {
    if (node->type == TY_BLOCK) {
        for (node_t* child = node->node_child; child != NULL; child = child->node_next) {
            codegen_link(child, code_data);
        }
    } else if (node->type == TY_PUSH_LABEL) {
        int target_index = node->node_child->code_index;
        code_data[node->code_index] = OP_IMMEDIATE | target_index;
    }
}

void codegen(node_t* node_data, char* code_data, node_t** nodelist_data) {
    int code_size = 0;
    int nodelist_size = 0;
    codegen_pre(code_data, &code_size);
    codegen_base(node_data, code_data, &code_size, nodelist_data, &nodelist_size);
    codegen_link(node_data, code_data);
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

    optimize(node_data, nodelist_data);

    codegen(node_data, code_data, nodelist_data);

    return 0;
}