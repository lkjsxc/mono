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

static result_t parse_expr(stat_t stat);
static result_t parse_statement(stat_t stat);

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

static node_t* node_find(node_t** begin, token_t* token) {
    node_t* itr = *begin;
    while (itr != NULL) {
        if (token_eq(itr->token, token)) {
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

// when token_itr is at the end of the file, return ERR
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

static result_t parse_pre(token_t** token_itr, node_t** node_itr, node_t** varlist_rbegin) {
    while ((*token_itr)->data != NULL) {
        if (token_eqstr(*token_itr, "struct")) {
            if (tokenitr_next(token_itr) == ERR) {
                write(STDERR_FILENO, "Error: struct name expected\n", 28);
                return ERR;
            }
            node_t* struct_node = node_new(node_itr);
            *struct_node = (node_t){.nodetype = NODETYPE_STRUCT, .next = NULL, .token = *token_itr};
            node_pushback(varlist_rbegin, struct_node);
        } else if (token_eqstr(*token_itr, "fn")) {
            if (tokenitr_next(token_itr) == ERR) {
                write(STDERR_FILENO, "Error: function name expected\n", 30);
                return ERR;
            }
            node_t* fn_node = node_new(node_itr);
            *fn_node = (node_t){.nodetype = NODETYPE_FN, .next = NULL, .token = *token_itr};
            node_pushback(varlist_rbegin, fn_node);
        } else {
            *token_itr += 1;
        }
    }
    return OK;
}

static result_t parse_expr(stat_t stat) {
    node_t* findfn_result = node_find_fn(stat.varlist_begin, *stat.token_itr);
    node_t* findstruct_result = node_find_struct(stat.varlist_begin, *stat.token_itr);
    if (token_eqstr(*stat.token_itr, "(")) {  // ( <expr>, <expr>, ... )
        if (tokenitr_next(stat.token_itr) == ERR) {
            write(STDERR_FILENO, "Error: '(' expected\n", 20);
            return ERR;
        }
        while (!token_eqstr(*stat.token_itr, ")")) {
            if (parse_expr(stat) == ERR) {
                write(STDERR_FILENO, "Error: parse_expr failed\n", 25);
                return ERR;
            }
            if (token_eqstr(*stat.token_itr, ",")) {
                if (tokenitr_next(stat.token_itr) == ERR) {
                    write(STDERR_FILENO, "Error: ',' expected\n", 20);
                    return ERR;
                }
            }
        }
        if (tokenitr_next(stat.token_itr) == ERR) {
            write(STDERR_FILENO, "Error: ')' expected\n", 20);
            return ERR;
        }
    } else if (token_eqstr(*stat.token_itr, "loop")) {                            // loop <statement>
    } else if (token_eqstr(*stat.token_itr, "if")) {                              // if <expr> <statement else <statement>
    } else if (findfn_result != NULL && token_eqstr(*stat.token_itr + 1, "(")) {  // <ident> ( <expr>, <expr>, ... )
        node_t* fn_call = node_new(stat.node_itr);
        *fn_call = (node_t){.nodetype = NODETYPE_CALL, .next = NULL, .token = *stat.token_itr, .child = findfn_result};
        if (tokenitr_next(stat.token_itr) == ERR) {
            write(STDERR_FILENO, "Error: '(' expected\n", 20);
            return ERR;
        }
        if (parse_expr(stat) == ERR) {
            write(STDERR_FILENO, "Error: parse_expr failed\n", 25);
            return ERR;
        }
        node_pushback(stat.execlist_rbegin, fn_call);
    } else {
    }
}

static result_t parse_decl(stat_t stat, node_t* decl_node) {
}

static result_t parse_statement(stat_t stat) {
    node_t* findstruct_result = node_find_struct(stat.varlist_begin, *stat.token_itr);
    if (token_eqstr(*stat.token_itr, "(")) {  // ( <statement>* )
        node_t* scope_open = node_new(stat.node_itr);
        node_t* scope_close = node_new(stat.node_itr);
        *scope_open = (node_t){.nodetype = NODETYPE_LABEL_SCOPE_OPEN, .next = NULL, .token = *stat.token_itr};
        *scope_close = (node_t){.nodetype = NODETYPE_LABEL_SCOPE_CLOSE, .next = NULL, .token = *stat.token_itr};
        node_pushback(stat.execlist_rbegin, scope_open);
        if (tokenitr_next(stat.token_itr) == ERR) {
            write(STDERR_FILENO, "Error: '(' expected\n", 20);
            return ERR;
        }
        while (token_eqstr(*stat.token_itr, ")") == 0) {
            if (parse_statement(stat) == ERR) {
                write(STDERR_FILENO, "Error: parse_statement failed\n", 25);
                return ERR;
            }
        }
        if (tokenitr_next(stat.token_itr) == ERR) {
            write(STDERR_FILENO, "Error: ')' expected\n", 20);
            return ERR;
        }
        node_pushback(stat.execlist_rbegin, scope_close);
    } else if (token_eqstr(*stat.token_itr, "return")) {  // return <expr>
        node_t* node_return = node_new(stat.node_itr);
        *node_return = (node_t){.nodetype = NODETYPE_RETURN, .next = NULL, .token = *stat.token_itr};
        if (tokenitr_next(stat.token_itr) == ERR) {
            write(STDERR_FILENO, "Error: return value expected\n", 30);
            return ERR;
        }
        if (parse_expr(stat) == ERR) {
            write(STDERR_FILENO, "Error: parse_expr failed\n", 25);
            return ERR;
        }
        node_pushback(stat.execlist_rbegin, node_return);
    } else if (token_eqstr(*stat.token_itr, "continue")) {  // continue
        node_t* node_continue = node_new(stat.node_itr);
        *node_continue = (node_t){.nodetype = NODETYPE_JMP, .next = NULL, .token = *stat.token_itr, .child = stat.label_continue};
        node_pushback(stat.execlist_rbegin, node_continue);
        if (tokenitr_next(stat.token_itr) == ERR) {
            write(STDERR_FILENO, "Error: continue value expected\n", 30);
            return ERR;
        }
    } else if (token_eqstr(*stat.token_itr, "break")) {  // break <expr>
        node_t* node_break = node_new(stat.node_itr);
        *node_break = (node_t){.nodetype = NODETYPE_JMP, .next = NULL, .token = *stat.token_itr, .child = stat.label_break};
        if (tokenitr_next(stat.token_itr) == ERR) {
            write(STDERR_FILENO, "Error: break value expected\n", 28);
            return ERR;
        }
        if (parse_expr(stat) == ERR) {
            write(STDERR_FILENO, "Error: parse_expr failed\n", 25);
            return ERR;
        }
        node_pushback(stat.execlist_rbegin, node_break);
    } else if (token_eqstr(*stat.token_itr, "struct")) {  // struct <ident> (<decl>)
    } else if (token_eqstr(*stat.token_itr, "fn")) {      // fn <ident> (<decl>) <statement>
    } else if (findstruct_result != NULL) {               // <decl>
        node_t* decl_node = node_new(stat.node_itr);
        if (parse_decl(stat, decl_node) == ERR) {
            write(STDERR_FILENO, "Error: parse_decl failed\n", 25);
            return ERR;
        }
        node_pushfront(stat.varlist_begin, decl_node);
    } else {  // <expr>
        if (parse_expr(stat) == ERR) {
            write(STDERR_FILENO, "Error: parse_expr failed\n", 25);
            return ERR;
        }
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
    node_t* globalend = node_new(&node_itr);

    *execlist_root = (node_t){.nodetype = NODETYPE_NOP, .next = NULL, .token = NULL};
    *varlist_root = (node_t){.nodetype = NODETYPE_NOP, .next = NULL, .token = NULL};
    *globalend = (node_t){.nodetype = NODETYPE_LABEL_GLOBAL_END, .next = NULL, .token = NULL};

    // pre parse
    if (parse_pre(&token_itr, &node_itr, &varlist_root) == ERR) {
        write(STDERR_FILENO, "Error: parse_pre failed\n", 24);
        return ERR;
    }

    // push back global end
    node_pushback(&execlist_rbegin, globalend);

    // main parse
    token_itr = token;
    while (1) {
        if (tokenitr_next(&token_itr) == ERR) {
            break;
        }
        if (parse_statement((stat_t){.token_itr = &token_itr, .node_itr = &node_itr, .varlist_begin = &varlist_begin, .varlist_rbegin = &varlist_rbegin, .execlist_rbegin = &execlist_rbegin}) == ERR) {
            write(STDERR_FILENO, "Error: parse_statement failed\n", 25);
            return ERR;
        }
    }
}