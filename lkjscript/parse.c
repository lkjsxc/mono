#include "lkjscript.h"

typedef struct {
    token_t** token_itr;
    node_t** node_itr;
    node_t** execlist_rbegin;
    node_t* parent;
    node_t* label_continue;
    node_t* label_break;
} stat_t;

static result_t parse_expr(stat_t stat);
static result_t parse_stmt(stat_t stat);

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

static void node_addmember(node_t* parent, node_t* node) {
    node->parent = parent;
    if (parent->child == NULL) {
        parent->child = node;
        return;
    }
    node_t* member_itr = parent->child;
    while (member_itr->next != NULL) {
        member_itr = member_itr->next;
    }
    member_itr->next = node;
}

// when not found, return NULL
static node_t* node_find(node_t* parent, token_t* token, nodetype_t nodetype) {
    node_t* parent_itr = parent;
    node_t* member_itr;
    while (parent_itr != NULL) {
        member_itr = parent_itr->child;
        while (member_itr != NULL) {
            if (token_eq(member_itr->token, token) && member_itr->nodetype == nodetype) {
                return member_itr;
            }
            member_itr = member_itr->next;
        }
        parent_itr = parent_itr->parent;
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

static result_t tokenitr_skipsplit(token_t** token_itr) {
    while (1) {
        if ((*token_itr)->data == NULL) {
            return OK;
        } else if (token_eqstr(*token_itr, "\n")) {
            *token_itr += 1;
        } else if (token_eqstr(*token_itr, ",")) {
            *token_itr += 1;
        } else {
            break;
        }
    }
}

static result_t parse_stmt_pre(stat_t stat) {
    token_t* token_itr = *stat.token_itr;
    int64_t nest = 0;
    while (nest >= 0) {
        if ((token_itr + 1)->data == NULL) {
            return OK;
        } else if (token_eqstr(token_itr, "(")) {
            nest += 1;
            if (tokenitr_next(&token_itr) == ERR) {
                ERROUT;
                return ERR;
            }
        } else if (token_eqstr(token_itr, ")")) {
            nest -= 1;
            if (tokenitr_next(&token_itr) == ERR) {
                ERROUT;
                return ERR;
            }
        } else if (nest != 0) {
            if (tokenitr_next(&token_itr) == ERR) {
                ERROUT;
                return ERR;
            }
        } else if (token_eqstr(token_itr, "fn")) {
            node_t* node_fn = node_new(stat.node_itr);
            if (tokenitr_next(&token_itr) == ERR) {
                ERROUT;
                return ERR;
            }
            *node_fn = (node_t){.nodetype = NODETYPE_FN, .token = token_itr, .parent = stat.parent};
            node_addmember(stat.parent, node_fn);
        } else if (token_eqstr(token_itr, "struct")) {
            node_t* node_struct = node_new(stat.node_itr);
            if (tokenitr_next(&token_itr) == ERR) {
                ERROUT;
                return ERR;
            }
            *node_struct = (node_t){.nodetype = NODETYPE_STRUCT, .token = token_itr, .parent = stat.parent};
            node_addmember(stat.parent, node_struct);
        } else {
            if (tokenitr_next(&token_itr) == ERR) {
                ERROUT;
                return ERR;
            }
        }
    }
    return OK;
}

static result_t parse_type(stat_t stat) {
    node_t* node_type_head;
    node_t* findstruct_result = node_find(stat.parent, *stat.token_itr, NODETYPE_STRUCT);
    if (findstruct_result != NULL) {  // struct
        node_type_head = findstruct_result;
    } else if (token_eqstr(*stat.token_itr, "i64")) {  // i64
        node_t* node_type_body = node_new(stat.node_itr);
        *node_type_body = (node_t){.nodetype = NODETYPE_NOP, .token = *stat.token_itr};
        node_type_head = node_type_body;
    } else {
        ERROUT;
        return ERR;
    }
    if (tokenitr_next(stat.token_itr) == ERR) {
        ERROUT;
        return ERR;
    }
    // type ptr
    while (1) {
        if (token_eqstr(*stat.token_itr, "*")) {
            node_t* node_type_ptr = node_new(stat.node_itr);
            *node_type_ptr = (node_t){.nodetype = NODETYPE_NOP, .token = *stat.token_itr};
            node_type_ptr->child = node_type_head;
            node_type_head = node_type_ptr;
            if (tokenitr_next(stat.token_itr) == ERR) {
                ERROUT;
                return ERR;
            }
        } else {
            break;
        }
    }
    node_addmember(stat.parent, node_type_head);
    return OK;
}

static result_t parse_var(stat_t stat) {
    node_t* node_var = node_new(stat.node_itr);
    stat_t stat2 = (stat_t){
        .token_itr = stat.token_itr,
        .node_itr = stat.node_itr,
        .execlist_rbegin = stat.execlist_rbegin,
        .parent = node_var,
    };

    // var, const
    if (token_eqstr(*stat.token_itr, "var")) {
        if (tokenitr_next(stat.token_itr) == ERR) {
            ERROUT;
            return ERR;
        }
    } else {
        ERROUT;
        return ERR;
    }

    // ident
    *node_var = (node_t){.nodetype = NODETYPE_VAR, .token = *stat.token_itr};
    if (tokenitr_next(stat.token_itr) == ERR) {
        ERROUT;
        return ERR;
    }
    node_addmember(stat.parent, node_var);

    // auto type
    if (!token_eqstr(*stat.token_itr, ":")) {
        ERROUT;
        return ERR;
    }
    if (tokenitr_next(stat.token_itr) == ERR) {
        ERROUT;
        return ERR;
    }

    // type body
    parse_type(stat2);
    return OK;
}

static result_t parse_fn(stat_t stat) {
    // fn
    if (tokenitr_next(stat.token_itr) == ERR) {
        ERROUT;
        return ERR;
    }
    // fn_name
    node_t* node_fn = node_find(stat.parent, *stat.token_itr, NODETYPE_FN);
    stat_t stat2 = {
        .token_itr = stat.token_itr,
        .node_itr = stat.node_itr,
        .execlist_rbegin = stat.execlist_rbegin,
        .parent = node_fn,
    };
    if (node_fn == NULL) {
        ERROUT;
        return ERR;
    }
    if (tokenitr_next(stat.token_itr) == ERR) {
        ERROUT;
        return ERR;
    }
    // args
    if (!token_eqstr(*stat.token_itr, "(")) {
        ERROUT;
        return ERR;
    }
    if (tokenitr_next(stat.token_itr) == ERR) {
        ERROUT;
        return ERR;
    }
    if (parse_stmt(stat2) == ERR) {
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
    // explicit return_type
    if (!token_eqstr(*stat.token_itr, "->")) {
        ERROUT;
        return ERR;
    }
    if (tokenitr_next(stat.token_itr) == ERR) {
        ERROUT;
        return ERR;
    }
    // return_type
    if (parse_type(stat2) == ERR) {
        ERROUT;
        return ERR;
    }
    // stmt
    if (!token_eqstr(*stat.token_itr, "(")) {
        ERROUT;
        return ERR;
    }
    if (tokenitr_next(stat.token_itr) == ERR) {
        ERROUT;
        return ERR;
    }
    if (parse_stmt(stat2) == ERR) {
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
    return OK;
}

static result_t parse_struct(stat_t stat) {
    if (tokenitr_next(stat.token_itr) == ERR) {
        ERROUT;
        return ERR;
    }
    node_t* node_struct = node_find(stat.parent, *stat.token_itr, NODETYPE_STRUCT);
    stat_t stat2 = {
        .token_itr = stat.token_itr,
        .node_itr = stat.node_itr,
        .execlist_rbegin = stat.execlist_rbegin,
        .parent = node_struct,
    };
    if (node_struct == NULL) {
        ERROUT;
        return ERR;
    }
    if (tokenitr_next(stat.token_itr) == ERR) {
        ERROUT;
        return ERR;
    }
    if (!token_eqstr(*stat.token_itr, "(")) {
        ERROUT;
        return ERR;
    }
    if (tokenitr_next(stat.token_itr) == ERR) {
        ERROUT;
        return ERR;
    }
    if (parse_stmt(stat2) == ERR) {
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
    return OK;
}

static result_t parse_structmember(stat_t stat, node_t* node_struct) {
}

static result_t parse_primary(stat_t stat) {
    node_t* findvar_result = node_find(stat.parent->child, *stat.token_itr, NODETYPE_VAR);
    if (findvar_result != NULL) {
        node_t* node_var = node_new(stat.node_itr);
        *node_var = (node_t){.nodetype = NODETYPE_VAR, .token = *stat.token_itr, .child = findvar_result};
        if (tokenitr_next(stat.token_itr) == ERR) {
            ERROUT;
            return ERR;
        }
        node_pushback(stat.execlist_rbegin, node_var);
    } else if (token_isdigit(*stat.token_itr)) {
        node_t* node_digit = node_new(stat.node_itr);
        *node_digit = (node_t){.nodetype = NODETYPE_PUSH_CONST, .token = *stat.token_itr};
        // set val (implement later)
        node_pushback(stat.execlist_rbegin, node_digit);
        if (tokenitr_next(stat.token_itr) == ERR) {
            ERROUT;
            return ERR;
        }
    } else if (token_isstr(*stat.token_itr)) {
        node_t* node_str = node_new(stat.node_itr);
        *node_str = (node_t){.nodetype = NODETYPE_PUSH_CONST, .token = *stat.token_itr};
        // set val (implement later)
        node_pushback(stat.execlist_rbegin, node_str);
        if (tokenitr_next(stat.token_itr) == ERR) {
            ERROUT;
            return ERR;
        }
    } else {
        ERROUT;
        return ERR;
    }
    return OK;
}

static result_t parse_postfix(stat_t stat) {
    while (1) {
        node_t* findfn_result = node_find(stat.parent->child, *stat.token_itr, NODETYPE_FN);
        if (parse_primary(stat) == ERR) {
            ERROUT;
            return ERR;
        }
        if (findfn_result != NULL) {
            node_t* node_fn = node_new(stat.node_itr);
            *node_fn = (node_t){.nodetype = NODETYPE_FN, .token = *stat.token_itr, .child = findfn_result};
            if (tokenitr_next(stat.token_itr) == ERR) {
                ERROUT;
                return ERR;
            }
            if (!token_eqstr(*stat.token_itr, "(")) {
                ERROUT;
                return ERR;
            }
            if (tokenitr_next(stat.token_itr) == ERR) {
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
            node_pushback(stat.execlist_rbegin, node_fn);
            return OK;
        }
        if (token_eqstr(*stat.token_itr, ".")) {
        } else if (token_eqstr(*stat.token_itr, "->")) {
        } else {
            return OK;  // no more postfix operators
        }
    }
}

static result_t parse_unary(stat_t stat) {
    if (token_eqstr(*stat.token_itr, "&")) {
    } else if (token_eqstr(*stat.token_itr, "*")) {
    } else if (token_eqstr(*stat.token_itr, "-")) {
    } else if (token_eqstr(*stat.token_itr, "!")) {
    } else {
        if (parse_postfix(stat) == ERR) {
            ERROUT;
            return ERR;
        }
    }
}

static result_t parse_binary(stat_t stat) {
    struct {
        const char* str;
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
            if (token_eqstr(*stat.token_itr, binary_operators[i].str)) {
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
    while (1) {
        if (parse_assign(stat) == ERR) {
            ERROUT;
            return ERR;
        }
        if (token_eqstr(*stat.token_itr, ",")) {
            if (tokenitr_next(stat.token_itr) == ERR) {
                ERROUT;
                return ERR;
            }
        } else {
            return OK;
        }
    }
}

static result_t parse_stmt(stat_t stat) {
    if (parse_stmt_pre(stat) == ERR) {
        ERROUT;
        return ERR;
    }
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
            node_t* node_scope = node_new(stat.node_itr);
            *node_scope = (node_t){.nodetype = NODETYPE_NOP, .token = *stat.token_itr};
            node_addmember(stat.parent, node_scope);
            stat_t stat2 = {
                .token_itr = stat.token_itr,
                .node_itr = stat.node_itr,
                .execlist_rbegin = stat.execlist_rbegin,
                .parent = node_scope,
            };
            if (tokenitr_next(stat.token_itr) == ERR) {
                ERROUT;
                return ERR;
            }
            if (parse_stmt(stat2) == ERR) {
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
        } else if (token_eqstr(*stat.token_itr, "return")) {
            node_t* node_return = node_new(stat.node_itr);
            *node_return = (node_t){.nodetype = NODETYPE_RETURN, .token = *stat.token_itr};
            if (tokenitr_next(stat.token_itr) == ERR) {
                ERROUT;
                return ERR;
            }
            if (parse_expr(stat) == ERR) {
                ERROUT;
                return ERR;
            }
            node_pushback(stat.execlist_rbegin, node_return);
        } else if (token_eqstr(*stat.token_itr, "if")) {
        } else if (token_eqstr(*stat.token_itr, "else")) {
        } else if (token_eqstr(*stat.token_itr, "while")) {
        } else if (token_eqstr(*stat.token_itr, "break")) {
        } else if (token_eqstr(*stat.token_itr, "continue")) {
        } else if (token_eqstr(*stat.token_itr, "var")) {
            if (parse_var(stat) == ERR) {
                ERROUT;
                return ERR;
            }
        } else if (token_eqstr(*stat.token_itr, "fn")) {
            if (parse_fn(stat) == ERR) {
                ERROUT;
                return ERR;
            }
        } else if (token_eqstr(*stat.token_itr, "struct")) {
            if (parse_struct(stat) == ERR) {
                ERROUT;
                return ERR;
            }
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
    node_t* root = node_new(&node_itr);
    node_t* execlist_begin = execlist_root;
    node_t* execlist_rbegin = execlist_root;

    *execlist_root = (node_t){.nodetype = NODETYPE_NOP, .token = NULL};
    *root = (node_t){.nodetype = NODETYPE_NOP, .token = NULL};

    stat_t stat = {
        .token_itr = &token_itr,
        .node_itr = &node_itr,
        .execlist_rbegin = &execlist_rbegin,
        .parent = root,
    };

    if (parse_stmt(stat) == ERR) {
        ERROUT;
        return ERR;
    }
}