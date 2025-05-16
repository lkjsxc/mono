#include "lkjscript.h"

typedef struct {
    token_t** token_itr;
    node_t** node_itr;
    node_t** node_global_begin;
    node_t** node_global_rbegin;
    node_t** node_main_begin;
    node_t** node_main_rbegin;
    node_t* label_continue;
    node_t* label_break;
} stat_t;

static node_t* node_new(node_t** node_itr) {
    node_t* node = (*node_itr)++;
    return node;
}

static void node_pushback(node_t** node_main_rbegin, node_t* node) {
    (*node_main_rbegin)->next = node;
    *node_main_rbegin = node;
}

static void node_pushfront(node_t** node_main_begin, node_t* node) {
    node->next = *node_main_begin;
    *node_main_begin = node;
}

static result_t tokenitr_next(token_t** token_itr) {
    *token_itr += 1;
    while (1) {
        if ((*token_itr)->data == NULL) {
            write(STDERR_FILENO, "Error: unexpected end of file\n", 30);
            return ERR;
        } else if (token_eqstr(*token_itr, "\n")) {
            *token_itr += 1;
        } else {
            return OK;
        }
    }
}

static result_t parse_pre(token_t** token_itr, node_t** node_itr, node_t** node_global_rbegin) {
    while ((*token_itr)->data != NULL) {
        if (token_eqstr(*token_itr, "struct")) {
            if(tokenitr_next(token_itr) == ERR) {
                write(STDERR_FILENO, "Error: struct name expected\n", 28);
                return ERR;
            }
            node_t* struct_node = node_new(node_itr);
            *struct_node = (node_t){.nodetype = NODETYPE_STRUCT, .next = NULL, .token = *token_itr};
            node_pushback(node_global_rbegin, struct_node);
        } else if (token_eqstr(*token_itr, "fn")) {
            if(tokenitr_next(token_itr) == ERR) {
                write(STDERR_FILENO, "Error: function name expected\n", 30);
                return ERR;
            }
            node_t* fn_node = node_new(node_itr);
            *fn_node = (node_t){.nodetype = NODETYPE_FN, .next = NULL, .token = *token_itr};
            node_pushback(node_global_rbegin, fn_node);
        } else {
            *token_itr += 1;
        }
    }
    return OK;
}

result_t parse(token_t* token, node_t* node) {
    token_t* token_itr = token;
    node_t* node_itr = node;

    node_t* node_root = node_new(&node_itr);
    node_t* node_global = node_new(&node_itr);
    node_t* node_main_begin = node_root;
    node_t* node_main_rbegin = node_root;
    node_t* node_global_begin = node_global;
    node_t* node_global_rbegin = node_global;

    *node_root = (node_t){.nodetype = NODETYPE_NOP, .next = NULL, .token = NULL};
    *node_global = (node_t){.nodetype = NODETYPE_NOP, .next = NULL, .token = NULL};

    stat_t stat = (stat_t){.token_itr = &token_itr, .node_itr = &node_itr, .node_global_begin = &node_global_begin, .node_global_rbegin = &node_global_rbegin, .node_main_begin = &node_main_begin, .node_main_rbegin = &node_main_rbegin, .label_continue = NULL, .label_break = NULL};

    // preparse
    if (parse_pre(&token_itr, &node_itr, &node_global) == ERR) {
        write(STDERR_FILENO, "Error: parse_pre failed\n", 24);
        return ERR;
    }

    // parse
    // token_itr = token;
    // while (token_itr->data != NULL) {
    // }
}