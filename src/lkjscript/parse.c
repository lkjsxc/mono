#include "lkjscript.h"

typedef struct {
    token_t** token_itr;
    node_t** node_itr;
    node_t** identlist_begin;
    node_t** identlist_rbegin;
    node_t** execlist_rbegin;
    node_t* label_continue;
    node_t* label_break;
} stat_t;

static node_t* node_new(node_t** node_itr) {
    node_t* node = (*node_itr)++;
    return node;
}

static void node_pushback(node_t** rbegin, node_t* node) {
    (*rbegin)->next = node;
    *rbegin = node;
}

static void node_pushfront(node_t** begin, node_t* node) {
    node->next = *begin;
    *begin = node;
}

static node_t* node_find_var(node_t** begin, token_t* token) {
    node_t* itr = *begin;
    while (itr != NULL) {
        if (token_eq(itr->token, token) && itr->nodetype == NODETYPE_VAR) {
            return itr;
        }
        itr = itr->next;
    }
    return NULL;
}

// when not found, return NULL
static node_t* node_find_fn(node_t** begin, token_t* token) {
    node_t* itr = *begin;
    while (itr != NULL) {
        if (itr->nodetype == NODETYPE_LABEL_GLOBAL_END) {
            break;
        }
        if (token_eq(itr->token, token) && itr->nodetype == NODETYPE_FN) {
            return itr;
        }
        itr = itr->next;
    }
    return NULL;
}

// when not found, return NULL
static node_t* node_find_struct(node_t** begin, token_t* token) {
    node_t* itr = *begin;
    while (itr != NULL) {
        if (itr->nodetype == NODETYPE_LABEL_GLOBAL_END) {
            break;
        }
        if (token_eq(itr->token, token) && itr->nodetype == NODETYPE_STRUCT) {
            return itr;
        }
        itr = itr->next;
    }
    return NULL;
}

// when token_itr is at the end of the file, return ERR
static result_t tokenitr_next(token_t** token_itr) {
    *token_itr += 1;
    if ((*token_itr)->data == NULL) {
        ERROUT;
        return ERR;
    }
}

static result_t tokenitr_skiplinebreak(token_t** token_itr) {
    while (1) {
        if ((*token_itr)->data == NULL) {
            ERROUT;
            return ERR;
        } else if (token_eqstr(*token_itr, "\n")) {
            *token_itr += 1;
        } else {
            return OK;
        }
    }
}

static result_t parse_pre(token_t** token_itr, node_t** node_itr, node_t** identlist_rbegin) {
    node_t* globalend = node_new(node_itr);
    *globalend = (node_t){.nodetype = NODETYPE_LABEL_GLOBAL_END, .token = NULL};
    while ((*token_itr)->data != NULL) {
        if (token_eqstr(*token_itr, "fn")) {
            if (tokenitr_next(token_itr) == ERR) {
                ERROUT;
                return ERR;
            }
            node_t* fn_node = node_new(node_itr);
            *fn_node = (node_t){.nodetype = NODETYPE_FN, .token = *token_itr};
            node_pushback(identlist_rbegin, fn_node);
        } else if (token_eqstr(*token_itr, "struct")) {
            if (tokenitr_next(token_itr) == ERR) {
                ERROUT;
                ERROUT;
                return ERR;
            }
            node_t* struct_node = node_new(node_itr);
            *struct_node = (node_t){.nodetype = NODETYPE_STRUCT, .token = *token_itr};
            node_pushback(identlist_rbegin, struct_node);
        } else {
            *token_itr += 1;
        }
    }
    // push back global end
    node_pushback(identlist_rbegin, globalend);
    return OK;
}

static result_t parse_expr(stat_t stat) {
    node_t* findvar_result = node_find_var(stat.identlist_begin, *stat.token_itr);
    node_t* findfn_result = node_find_fn(stat.identlist_begin, *stat.token_itr);
    node_t* findstruct_result = node_find_struct(stat.identlist_begin, *stat.token_itr);
}

result_t parse(token_t* token, node_t* node) {
    token_t* token_itr = token;
    node_t* node_itr = node;

    node_t* execlist_root = node_new(&node_itr);
    node_t* execlist_begin = execlist_root;
    node_t* execlist_rbegin = execlist_root;
    node_t* identlist_root = node_new(&node_itr);
    node_t* identlist_begin = identlist_root;
    node_t* identlist_rbegin = identlist_root;

    *execlist_root = (node_t){.nodetype = NODETYPE_NOP, .token = NULL};
    *identlist_root = (node_t){.nodetype = NODETYPE_NOP, .token = NULL};

    // pre parse
    if (parse_pre(&token_itr, &node_itr, &identlist_root) == ERR) {
        ERROUT;
        return ERR;
    }

    // main parse
    token_itr = token;
    while (token_itr->data != NULL) {
        
    }
}