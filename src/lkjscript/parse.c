#include "lkjscript.h"

typedef struct {
    token_t** token_itr;
    node_t** node_itr;
    node_t** varlist_begin;
    node_t** varlist_rbegin;
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

static result_t parse_pre(token_t** token_itr, node_t** node_itr, node_t** varlist_rbegin) {
    node_t* globalend = node_new(node_itr);
    *globalend = (node_t){.nodetype = NODETYPE_LABEL_GLOBAL_END, .next = NULL, .token = NULL};
    while ((*token_itr)->data != NULL) {
        if (token_eqstr(*token_itr, "fn")) {
            if (tokenitr_next(token_itr) == ERR) {
                ERROUT;
                return ERR;
            }
            node_t* fn_node = node_new(node_itr);
            *fn_node = (node_t){.nodetype = NODETYPE_FN, .next = NULL, .token = *token_itr};
            node_pushback(varlist_rbegin, fn_node);
        } else if (token_eqstr(*token_itr, "struct")) {
            if (tokenitr_next(token_itr) == ERR) {
                ERROUT;
                ERROUT;
                return ERR;
            }
            node_t* struct_node = node_new(node_itr);
            *struct_node = (node_t){.nodetype = NODETYPE_STRUCT, .next = NULL, .token = *token_itr};
            node_pushback(varlist_rbegin, struct_node);
        } else {
            *token_itr += 1;
        }
    }
    // push back global end
    node_pushback(varlist_rbegin, globalend);
    return OK;
}

static result_t parse_type(stat_t stat, node_t* parent) {
    node_t* findstruct_result = node_find_struct(stat.varlist_begin, *stat.token_itr);
    node_t* node_type = node_new(stat.node_itr);
    node_t* node_head = node_type;
    if (token_eqstr(*stat.token_itr, "i64")) {
        *node_type = (node_t){.nodetype = NODETYPE_NOP, .next = NULL, .token = *stat.token_itr, .type_size = sizeof(int64_t)};
    } else if (findstruct_result != NULL) {
        *node_type = (node_t){.nodetype = NODETYPE_NOP, .next = NULL, .token = *stat.token_itr};
    } else {
        ERROUT;
        return ERR;
    }
    if (tokenitr_next(stat.token_itr) == ERR) {
        ERROUT;
        return ERR;
    }
    while(token_eqstr(*stat.token_itr, "*")) {
        ERROUT;
        return ERR;
    }
    parent->type_ptr = node_head;
    return OK;
}

static result_t parse_expr(stat_t stat) {
    node_t* findvar_result = node_find_var(stat.varlist_begin, *stat.token_itr);
    node_t* findfn_result = node_find_fn(stat.varlist_begin, *stat.token_itr);
    node_t* findstruct_result = node_find_struct(stat.varlist_begin, *stat.token_itr);
    if (token_eqstr(*stat.token_itr, "(")) {  // "(" <linebreak>* <expr> ")"
        node_t* scope_open = node_new(stat.node_itr);
        node_t* scope_close = node_new(stat.node_itr);
        *scope_open = (node_t){.nodetype = NODETYPE_LABEL_SCOPE_OPEN, .next = NULL, .token = *stat.token_itr};
        *scope_close = (node_t){.nodetype = NODETYPE_LABEL_SCOPE_CLOSE, .next = NULL, .token = *stat.token_itr};
        node_pushback(stat.execlist_rbegin, scope_open);
        if (tokenitr_next(stat.token_itr) == ERR) {
            ERROUT;
            return ERR;
        }
        if (tokenitr_skiplinebreak(stat.token_itr) == ERR) {
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
        node_pushback(stat.execlist_rbegin, scope_close);
    } else if (token_eqstr(*stat.token_itr, "fn")) {  // "fn" <fn_name> "(" <split>* <expr> <split>* ")" "->" <type> <expr>
        ERROUT;
        return ERR;
    } else if (token_eqstr(*stat.token_itr, "struct")) {  // "struct" <type> <expr>
        ERROUT;
        return ERR;
    } else if (token_eqstr(*stat.token_itr, "if")) {  // "if" <expr> <expr> | if <expr> <expr> "else" <expr>
        ERROUT;
        return ERR;
    } else if (token_eqstr(*stat.token_itr, "loop")) {  // "loop" <expr>
        ERROUT;
        return ERR;
    } else if (token_eqstr(*stat.token_itr, "return")) {  // "return" <expr>
        ERROUT;
        return ERR;
    } else if (token_eqstr(*stat.token_itr, "break")) {  // "break" <expr>
        ERROUT;
        return ERR;
    } else if (token_eqstr(*stat.token_itr, "continue")) {  // "continue"
        ERROUT;
        return ERR;
    } else if (token_eqstr(*stat.token_itr, "var")) {  // "var" <var_name> | "var" <var_name> "=" <expr>
        if (tokenitr_next(stat.token_itr) == ERR) {
            ERROUT;
            return ERR;
        }
        node_t* node_var = node_new(stat.node_itr);
        *node_var = (node_t){.nodetype = NODETYPE_VAR, .next = NULL, .token = *stat.token_itr};
        if (parse_type(stat, node_var) == ERR) {
            ERROUT;
            return ERR;
        }
    } else {
        ERROUT;
        return ERR;
    }
    return OK;
}

result_t parse(token_t* token, node_t* node) {
    token_t* token_itr = token;
    node_t* node_itr = node;

    node_t* execlist_root = node_new(&node_itr);
    node_t* execlist_begin = execlist_root;
    node_t* execlist_rbegin = execlist_root;
    node_t* varlist_root = node_new(&node_itr);
    node_t* varlist_begin = varlist_root;
    node_t* varlist_rbegin = varlist_root;

    *execlist_root = (node_t){.nodetype = NODETYPE_NOP, .next = NULL, .token = NULL};
    *varlist_root = (node_t){.nodetype = NODETYPE_NOP, .next = NULL, .token = NULL};

    // pre parse
    if (parse_pre(&token_itr, &node_itr, &varlist_root) == ERR) {
        ERROUT;
        return ERR;
    }

    // main parse
    token_itr = token;
    while (1) {
        while (1) {
            if ((token_itr)->data == NULL) {
                return OK;
            } else if (token_eqstr(token_itr, "\n")) {
                token_itr += 1;
            } else if (token_eqstr(token_itr, ",")) {
                token_itr += 1;
            } else {
                break;
            }
        }
        if (parse_expr((stat_t){.token_itr = &token_itr, .node_itr = &node_itr, .varlist_begin = &varlist_begin, .varlist_rbegin = &varlist_rbegin, .execlist_rbegin = &execlist_rbegin}) == ERR) {
            ERROUT;
            return ERR;
        }
    }
}