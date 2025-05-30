#include "lkjscript.h"

result_t exec(uint8_t* mem) {
    nodetype_t opcode = *(nodetype_t*)provide_ip(mem);
    while (1) {
        switch (opcode) {
            case NODETYPE_NULL:
                return ERR;
            case NODETYPE_NOP:
                break;
            case NODETYPE_END:
                return OK;
            case NODETYPE_PUSH_CONST:
                break;
            case NODETYPE_PUSH_LOCAL_VAL:
            case NODETYPE_PUSH_LOCAL_ADDR:
            case NODETYPE_JMP:
            case NODETYPE_JZE:
            case NODETYPE_CALL:
            case NODETYPE_RETURN:

            case NODETYPE_ASSIGN:
            case NODETYPE_ASSIGN1:
            case NODETYPE_ASSIGN2:
            case NODETYPE_ASSIGN3:
            case NODETYPE_ASSIGN4:

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
            case NODETYPE_GETSTRUCTMEMBER:

            case NODETYPE_DEREF:
            case NODETYPE_NEG:
            case NODETYPE_BITNOT:

            case NODETYPE_READ:
            case NODETYPE_WRITE:
            case NODETYPE_USLEEP:
        }
    }
}