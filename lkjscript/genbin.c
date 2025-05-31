#include "lkjscript.h"

result_t bin_link(uint8_t* bin, node_t* node) {
    if (node->child != NULL) {
        if (bin_link(bin, node->child) == ERR) {
            ERROUT;
            return ERR;
        }
    }
    if (node->nodetype == NODETYPE_JMP || node->nodetype == NODETYPE_JZE || node->nodetype == NODETYPE_CALL) {
        *(int64_t*)((uint8_t*)(bin + node->bin + 1)) = node->child->bin;
    }
    if (node->next != NULL) {
        if (bin_link(bin, node->next) == ERR) {
            ERROUT;
            return ERR;
        }
    }
    return OK;
}

result_t bin_gen(uint8_t* bin, node_t* node, int64_t* bin_itr) {
    if (node->child != NULL) {
        if (bin_gen(bin, node->child, bin_itr) == ERR) {
            ERROUT;
            return ERR;
        }
    }
    node->bin = *bin_itr;
    switch (node->nodetype) {
        case NODETYPE_NOP:
            break;
        case NODETYPE_END:
            *(uint8_t*)((uint8_t*)(bin + *bin_itr)) = node->nodetype;
            *bin_itr += sizeof(uint8_t);
            break;
        case NODETYPE_PUSH_CONST:
            *(uint8_t*)((uint8_t*)(bin + *bin_itr)) = node->nodetype;
            *(int64_t*)((uint8_t*)(bin + *bin_itr + 1)) = node->nodetype;
            *bin_itr += sizeof(uint8_t) + sizeof(int64_t);
            break;
        case NODETYPE_PUSH_LOCAL_VAL:
            // implement later
            break;
        case NODETYPE_PUSH_LOCAL_ADDR:
            // implement later
            break;
        case NODETYPE_JMP:
        case NODETYPE_JZE:
        case NODETYPE_CALL:
            // implement later
            break;
        case NODETYPE_RETURN:
        case NODETYPE_ASSIGN:
            *(uint8_t*)((uint8_t*)(bin + *bin_itr)) = node->nodetype;
            *bin_itr += sizeof(uint8_t);
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
            *(uint8_t*)((uint8_t*)(bin + *bin_itr)) = node->nodetype;
            *bin_itr += sizeof(uint8_t);
            break;
        case NODETYPE_GETSTRUCTMEMBER:
            // implement later
            break;
        default:
            break;
    }
    if (node->next != NULL) {
        if (bin_gen(bin, node->next, bin_itr) == ERR) {
            ERROUT;
            return ERR;
        }
    }
}

result_t genbin(node_t* root, uint8_t* bin) {
    int64_t bin_itr = GLOBALOFFSET_INST;
    *(int64_t*)((uint8_t*)(bin + GLOBALOFFSET_IP)) = GLOBALOFFSET_INST;
    if (bin_gen(bin, root, &bin_itr) == ERR) {
        ERROUT;
        return ERR;
    }
    if (bin_link(bin, root) == ERR) {
        ERROUT;
        return ERR;
    }
}