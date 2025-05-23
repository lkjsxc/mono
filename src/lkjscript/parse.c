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

static result_t parse_stat_pre(stat_t stat) {
    token_t* token_itr = *stat.token_itr;
    int64_t nest = 0;
    while (nest >= 0) {
        if (token_itr->data == NULL) {
            return OK;
        } else if (token_eqstr(token_itr, "(")) {
            nest += 1;
            token_itr += 1;
        } else if (token_eqstr(token_itr, ")")) {
            nest -= 1;
            token_itr += 1;
        } else if (nest != 0) {
            token_itr += 1;
        } else if (token_eqstr(token_itr, "fn")) {
            node_t* node_fn = node_new(stat.node_itr);
            if (tokenitr_next(&token_itr) == ERR) {
                ERROUT;
                return ERR;
            }
            *node_fn = (node_t){.nodetype = NODETYPE_FN, .token = token_itr};
            node_pushfront(stat.identlist_begin, node_fn);
            token_itr += 1;
        } else if (token_eqstr(token_itr, "struct")) {
            node_t* node_struct = node_new(stat.node_itr);
            if (tokenitr_next(&token_itr) == ERR) {
                ERROUT;
                return ERR;
            }
            *node_struct = (node_t){.nodetype = NODETYPE_STRUCT, .token = token_itr};
            node_pushfront(stat.identlist_begin, node_struct);
            token_itr += 1;
        } else {
            token_itr += 1;
        }
    }
}

static result_t parse_primary(stat_t stat) {
    node_t* findvar_result = node_find_var(stat.identlist_begin, *stat.token_itr);
    node_t* findfn_result = node_find_fn(stat.identlist_begin, *stat.token_itr);
    if (findvar_result != NULL) {
        node_t* node_var = node_new(stat.node_itr);
        *node_var = (node_t){.nodetype = NODETYPE_VAR, .token = *stat.token_itr};
        if (tokenitr_next(stat.token_itr) == ERR) {
            ERROUT;
            return ERR;
        }
        node_pushback(stat.execlist_rbegin, node_var);
    } else if (findfn_result != NULL) {
    } else if (token_isdigit(*stat.token_itr)) {
    } else if (token_isstr(*stat.token_itr)) {
    } else {
        ERROUT;
        return ERR;
    }
    return OK;
}

static result_t parse_unary(stat_t stat) {
    if (token_eqstr(*stat.token_itr, "&")) {
    } else if (token_eqstr(*stat.token_itr, "*")) {
    } else if (token_eqstr(*stat.token_itr, "-")) {
    } else if (token_eqstr(*stat.token_itr, "!")) {
    } else {
        if (parse_primary(stat) == ERR) {
            ERROUT;
            return ERR;
        }
    }
}

static result_t parse_binary(stat_t stat) {
    struct {
        const char* operator;
        nodetype_t nodetype;
    } binary_operators[] = {
        {"||", NODETYPE_OR},
        {"&&", NODETYPE_AND},
        {"==", NODETYPE_EQ},
        {"!=", NODETYPE_NE},
        {"<", NODETYPE_LT},
        {"<=", NODETYPE_LE},
        {">", NODETYPE_GT},
        {">=", NODETYPE_GE},
        {"+", NODETYPE_ADD},
        {"-", NODETYPE_SUB},
        {"*", NODETYPE_MUL},
        {"/", NODETYPE_DIV},
        {"%", NODETYPE_MOD},
        {"<<", NODETYPE_SHL},
        {">>", NODETYPE_SHR},
        {"|", NODETYPE_BITOR},
        {"^", NODETYPE_BITXOR},
        {"&", NODETYPE_BITAND}};
    if (parse_unary(stat) == ERR) {
        ERROUT;
        return ERR;
    }
    while (1) {
        int64_t operator_found = 0;
        for (int64_t i = 0; i < sizeof(binary_operators) / sizeof(binary_operators[0]); i++) {
            if (token_eqstr(*stat.token_itr, binary_operators[i].operator)) {
                operator_found = 1;
                node_t* node_binary = node_new(stat.node_itr);
                *node_binary = (node_t){.nodetype = binary_operators[i].nodetype, .token = *stat.token_itr};
                if (tokenitr_next(stat.token_itr) == ERR) {
                    ERROUT;
                    return ERR;
                }
                if (parse_unary(stat) == ERR) {
                    ERROUT;
                    return ERR;
                }
                node_pushback(stat.execlist_rbegin, node_binary);
            }
        }
        if (operator_found == 0) {
            return OK;
        }
    }
}

static result_t parse_assign(stat_t stat) {
    if (parse_binary(stat) == ERR) {
        ERROUT;
        return ERR;
    }
    if (token_eqstr(*stat.token_itr, "=")) {
        node_t* node_assign = node_new(stat.node_itr);
        *node_assign = (node_t){.nodetype = NODETYPE_ASSIGN, .token = *stat.token_itr};
        if (tokenitr_next(stat.token_itr) == ERR) {
            ERROUT;
            return ERR;
        }
        if (parse_binary(stat) == ERR) {
            ERROUT;
            return ERR;
        }
        node_pushback(stat.execlist_rbegin, node_assign);
    }
    return OK;
}

static result_t parse_expr(stat_t stat) {
    if (token_eqstr(*stat.token_itr, "(")) {
        if (tokenitr_next(stat.token_itr) == ERR) {
            ERROUT;
            return ERR;
        }
        while (!token_eqstr(*stat.token_itr, ")")) {
            if (parse_expr(stat) == ERR) {
                ERROUT;
                return ERR;
            }
        }
        if (tokenitr_next(stat.token_itr) == ERR) {
            ERROUT;
            return ERR;
        }
    } else {
        parse_assign(stat);
    }
    return OK;
}

static result_t parse_stat(stat_t stat) {
    while (1) {
        while (1) {
            if ((*stat.token_itr)->data == NULL) {
                return OK;
            } else if (token_eqstr(*stat.token_itr, ",")) {
                *stat.token_itr += 1;
            } else if (token_eqstr(*stat.token_itr, "\n")) {
                *stat.token_itr += 1;
            } else {
                break;
            }
        }
        if (token_eqstr(*stat.token_itr, ")")) {
            return OK;
        } else if (token_eqstr(*stat.token_itr, "(")) {
            node_t* scope_identlist = *stat.identlist_rbegin;
            if (tokenitr_next(stat.token_itr) == ERR) {
                ERROUT;
                return ERR;
            }
            if (parse_stat_pre(stat) == ERR) {
                ERROUT;
                return ERR;
            }
            if (parse_stat(stat) == ERR) {
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
            *stat.identlist_rbegin = scope_identlist;
        } else if (token_eqstr(*stat.token_itr, "return")) {
        } else if (token_eqstr(*stat.token_itr, "break")) {
        } else if (token_eqstr(*stat.token_itr, "continue")) {
        } else if (token_eqstr(*stat.token_itr, "var")) {
        } else if (token_eqstr(*stat.token_itr, "fn")) {
        } else if (token_eqstr(*stat.token_itr, "struct")) {
        } else {
            if (parse_expr(stat) == ERR) {
                ERROUT;
                return ERR;
            }
        }
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

    stat_t stat = {
        .token_itr = &token_itr,
        .node_itr = &node_itr,
        .identlist_begin = &identlist_begin,
        .identlist_rbegin = &identlist_rbegin,
        .execlist_rbegin = &execlist_rbegin,
    };

    if (parse_stat_pre(stat) == ERR) {
        ERROUT;
        return ERR;
    }
    if (parse_stat(stat) == ERR) {
        ERROUT;
        return ERR;
    }
}