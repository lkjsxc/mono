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

// when not found, return NULL
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

static result_t parse_expr_pre(stat_t stat) {
}

static result_t parse_primary(stat_t stat) {
}

static result_t parse_unary(stat_t stat) {
}

static result_t parse_binary(stat_t stat) {
}

static result_t parse_expr(stat_t stat) {
    node_t* findvar_result = node_find_var(stat.identlist_begin, *stat.token_itr);
    node_t* findfn_result = node_find_fn(stat.identlist_begin, *stat.token_itr);
    node_t* findstruct_result = node_find_struct(stat.identlist_begin, *stat.token_itr);
    if (findvar_result != NULL) {
    } else if (findfn_result != NULL) {
    } else if (findstruct_result != NULL) {
    } else if (token_eqstr(*stat.token_itr, "(")) {
        node_t* scope_execlist = stat.execlist_rbegin;
        node_t* scope_identlist = stat.identlist_rbegin;
        if (tokenitr_next(stat.token_itr) == ERR) {
            ERROUT;
            return ERR;
        }
        if (parse_expr_pre(stat) == ERR) {
            ERROUT;
            return ERR;
        }
        if (parse_expr(stat) == ERR) {
            ERROUT;
            return ERR;
        }
        if (!token_eqstr(*stat.token_itr, ")")) {
            ERROUT;
            return ERR;
        }
        if (tokenitr_next(stat.token_itr) == ERR) {
            ERROUT;
            return ERR;
        }
        stat.execlist_rbegin = scope_execlist;
        stat.identlist_rbegin = scope_identlist;
    } else if (token_eqstr(*stat.token_itr, "var")) {
    } else if (token_eqstr(*stat.token_itr, "fn")) {
    } else if (token_eqstr(*stat.token_itr, "struct")) {
    } else {
        ERROUT;
        return ERR;
    }
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

    if (parse_expr((stat_t){.token_itr = &token_itr, .node_itr = &node_itr, .execlist_rbegin = &execlist_rbegin, .identlist_rbegin = &identlist_rbegin, .label_continue = NULL, .label_break = NULL}) == ERR) {
        ERROUT;
        return ERR;
    }
}