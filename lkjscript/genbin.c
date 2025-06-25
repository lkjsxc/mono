#include "lkjscript.h"

static int64_t provide_offset(node_t* parent, node_t* node) {
    // TODO: when parent is root or function, offset be 0. when else offset is recursively decided
    int64_t offset = 0;
    node_t* itr = parent->node_child;
    while (itr != node->node_child) {
        if (itr->nodetype == NODETYPE_VAR) {
            if (token_eqstr(itr->node_child->token, "*")) {
                offset += sizeof(void*);
            } else if (token_eqstr(itr->node_child->token, "i64")) {
                offset += sizeof(int64_t*);
            } else {
                // TODO: implement offset later (inc struct size)
                ERROUT;
            }
        }
        itr = itr->node_next;
    }
    return offset;
}

__attribute__((warn_unused_result))
static result_t bin_link(uint8_t* bin, node_t* node) {
    if (node->node_child != NULL && node->nodetype != NODETYPE_PUSH_LOCAL_ADDR && node->nodetype != NODETYPE_PUSH_LOCAL_VAL) {
        if (bin_link(bin, node->node_child) == ERR) {
            ERROUT;
            return ERR;
        }
    }
    if (node->nodetype == NODETYPE_JMP || node->nodetype == NODETYPE_JZE || node->nodetype == NODETYPE_CALL) {
        *(int64_t*)((uint8_t*)(bin + node->bin + 1)) = node->node_child->bin;
    }
    if (node->node_next != NULL) {
        if (bin_link(bin, node->node_next) == ERR) {
            ERROUT;
            return ERR;
        }
    }
    return OK;
}

__attribute__((warn_unused_result))
static result_t bin_gen(uint8_t* bin, node_t* node, int64_t* bin_itr) {
    if (node->node_child != NULL && node->nodetype != NODETYPE_PUSH_LOCAL_ADDR && node->nodetype != NODETYPE_PUSH_LOCAL_VAL) {
        if (bin_gen(bin, node->node_child, bin_itr) == ERR) {
            ERROUT;
            return ERR;
        }
    }
    node->bin = *bin_itr;
    switch (node->nodetype) {
        case NODETYPE_NOP:
            break;
        case NODETYPE_END:
            *(nodetype_t*)((uint8_t*)(bin + *bin_itr)) = node->nodetype;
            *bin_itr += sizeof(nodetype_t);
            break;
        case NODETYPE_PUSH_CONST: {
            if (node->token == NULL) {
                *(nodetype_t*)((uint8_t*)(bin + *bin_itr)) = NODETYPE_PUSH_CONST;
                *bin_itr += sizeof(nodetype_t);
                *(int64_t*)((uint8_t*)(bin + *bin_itr)) = node->val;
                *bin_itr += sizeof(int64_t);
            } else if (token_isdigit(node->token)) {
                *(nodetype_t*)((uint8_t*)(bin + *bin_itr)) = NODETYPE_PUSH_CONST;
                *bin_itr += sizeof(nodetype_t);
                *(int64_t*)((uint8_t*)(bin + *bin_itr)) = token_toint(node->token);
                *bin_itr += sizeof(int64_t);
            } else if (token_isstr(node->token)) {  // TODO: implement this later
                ERROUT;
                return ERR;
            } else {
                ERROUT;
                return ERR;
            }
        } break;
        case NODETYPE_PUSH_LOCAL_VAL:
        case NODETYPE_PUSH_LOCAL_ADDR: {
            *(nodetype_t*)((uint8_t*)(bin + *bin_itr)) = node->nodetype;
            *bin_itr += sizeof(nodetype_t);
            *(int64_t*)((uint8_t*)(bin + *bin_itr)) = provide_offset(node->node_parent, node);  // TODO: implement offset later (parse_stmt_post in parser.c)
            *bin_itr += sizeof(int64_t);
        } break;
        case NODETYPE_JMP:
        case NODETYPE_JZE:
        case NODETYPE_CALL:
            // implement later
            break;
        case NODETYPE_RETURN:
        case NODETYPE_ASSIGN:
            *(nodetype_t*)((uint8_t*)(bin + *bin_itr)) = node->nodetype;
            *bin_itr += sizeof(nodetype_t);
            break;
        case NODETYPE_ASSIGN1:
        case NODETYPE_ASSIGN2:
        case NODETYPE_ASSIGN3:
        case NODETYPE_ASSIGN4:
            // implement later
            break;
        case NODETYPE_OR:
        case NODETYPE_AND:
        case NODETYPE_EQ:
        case NODETYPE_NE:
        case NODETYPE_LT:
        case NODETYPE_LE:
        case NODETYPE_GT:
        case NODETYPE_GE:
        case NODETYPE_NOT:
        case NODETYPE_ADD:
        case NODETYPE_SUB:
        case NODETYPE_MUL:
        case NODETYPE_DIV:
        case NODETYPE_MOD:
        case NODETYPE_SHL:
        case NODETYPE_SHR:
        case NODETYPE_BITOR:
        case NODETYPE_BITXOR:
        case NODETYPE_BITAND:
        case NODETYPE_DEREF:
        case NODETYPE_NEG:
        case NODETYPE_BITNOT:
        case NODETYPE_READ:
        case NODETYPE_WRITE:
        case NODETYPE_USLEEP:
            *(nodetype_t*)((uint8_t*)(bin + *bin_itr)) = node->nodetype;
            *bin_itr += sizeof(nodetype_t);
            break;
        case NODETYPE_GETSTRUCTMEMBER:
            // implement later
            break;
        default:
            break;
    }
    if (node->node_next != NULL) {
        if (bin_gen(bin, node->node_next, bin_itr) == ERR) {
            ERROUT;
            return ERR;
        }
    }
    return OK;
}

__attribute__((warn_unused_result))
result_t genbin(node_t* root, uint8_t* bin) {
    int64_t bin_itr = GLOBALOFFSET_INST;
    if (bin_gen(bin, root, &bin_itr) == ERR) {
        ERROUT;
        return ERR;
    }
    if (bin_link(bin, root) == ERR) {
        ERROUT;
        return ERR;
    }
    *(int64_t*)((uint8_t*)(bin + GLOBALOFFSET_IP)) = GLOBALOFFSET_INST;
    *(int64_t*)((uint8_t*)(bin + GLOBALOFFSET_SP)) = bin_itr;
    *(int64_t*)((uint8_t*)(bin + GLOBALOFFSET_BP)) = bin_itr + 256;
    return OK;
}