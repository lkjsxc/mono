#include "lkjscript.h"

typedef struct {
    token_t** token_itr;
    node_t** node_itr;
    node_t** node_begin;
    node_t** node_rbegin;
    node_t* label_continue;
    node_t* label_break;
} stat_t;

static node_t* node_new(node_t** node_itr) {
    node_t* node = (*node_itr)++;
    return node;
}

static void node_push(node_t** node_rbegin, node_t* node) {
    (*node_rbegin)->next = node;
    *node_rbegin = node;
}

result_t parse(token_t* token, node_t* node) {
    token_t* token_itr = token;
    node_t* node_itr = node;
    node_t* node_begin = node;
    node_t* node_rbegin = node;
    *node = (node_t){.nodetype = NODETYPE_NOP, .token = NULL};
    stat_t stat = (stat_t){.token_itr = &token_itr, .node_itr = &node_itr, .node_begin = &node_begin, .node_rbegin = &node_rbegin, .label_continue = NULL, .label_break = NULL};

    // preparse
    while (token_itr->data != NULL) {
    }

    // parse
    token_itr = token;
    while (token_itr->data != NULL) {
    }
}