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

static void node_push(node_t** node_main_rbegin, node_t* node) {
    (*node_main_rbegin)->next = node;
    *node_main_rbegin = node;
}

static result_t parse_pre(stat_t* stat) {
    token_t** token_itr = stat->token_itr;
    node_t** node_itr = stat->node_itr;
    node_t** node_global_begin = stat->node_global_begin;
    node_t** node_global_rbegin = stat->node_global_rbegin;

    while ((*token_itr)->data != NULL) {
        if (token_eqstr(*token_itr, "struct")) {
            *token_itr += 1;
            node_t* node_struct = node_new(node_itr);
            node_push(node_global_rbegin, node_struct);
            *node_struct = (node_t){.nodetype = NODETYPE_STRUCT, .next = NULL, .token = *token_itr};
        } else if (token_eqstr(*token_itr, "fn")) {
            *token_itr += 1;
            node_t* node_struct = node_new(node_itr);
            node_push(node_global_rbegin, node_struct);
            *node_struct = (node_t){.nodetype = NODETYPE_FN, .next = NULL, .token = *token_itr};
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

    *node_root = (node_t){.nodetype = NODETYPE_NOP, .next = NULL, .token = NULL};
    *node_global = (node_t){.nodetype = NODETYPE_NOP, .next = NULL, .token = NULL};

    stat_t stat = (stat_t){.token_itr = &token_itr, .node_itr = &node_itr, .node_global_begin = &node_global, .node_global_rbegin = &node_global, .node_main_begin = &node_main_begin, .node_main_rbegin = &node_main_rbegin, .label_continue = NULL, .label_break = NULL};

    // preparse
    if (parse_pre(&stat) == ERR) {
        return ERR;
    }

    // parse
    // token_itr = token;
    // while (token_itr->data != NULL) {
    // }
}