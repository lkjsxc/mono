#include "lkjscript.h"

static inline int64_t* provide_ip(uint8_t* mem) {
    return (int64_t*)((uint8_t*)(mem + GLOBALOFFSET_IP));
}

static inline int64_t* provide_sp(uint8_t* mem) {
    return (int64_t*)((uint8_t*)(mem + GLOBALOFFSET_SP));
}

static inline void* provide_inst(uint8_t* mem, int64_t offset) {
    return (void*)(mem + *provide_ip(mem) + offset);
}

static inline void* provide_stack(uint8_t* mem, int64_t offset) {
    return (void*)(mem + *provide_sp(mem) + offset);
}

result_t exec(uint8_t* mem) {
    while (1) {
        nodetype_t opcode = *(nodetype_t*)provide_inst(mem, 0);
        *provide_ip(mem) += sizeof(opcode);
        switch (opcode) {
            case NODETYPE_NULL:
                return ERR;
            case NODETYPE_NOP:
                break;
            case NODETYPE_END:
                return OK;
            case NODETYPE_PUSH_CONST: {
                int64_t val = *(int64_t*)provide_inst(mem, 0);
                *(int64_t*)provide_stack(mem, 0) = val;
                *provide_ip(mem) += sizeof(int64_t);
                *provide_sp(mem) += sizeof(int64_t);
            } break;
            case NODETYPE_PUSH_LOCAL_VAL:
                break;
            case NODETYPE_PUSH_LOCAL_ADDR:
                break;
            case NODETYPE_JMP:
                break;
            case NODETYPE_JZE:
                break;
            case NODETYPE_CALL:
                break;
            case NODETYPE_RETURN:
                write(STDOUT_FILENO, "test\n", 5);
                return OK;
            case NODETYPE_ASSIGN:
                break;
            case NODETYPE_ASSIGN1:
                break;
            case NODETYPE_ASSIGN2:
                break;
            case NODETYPE_ASSIGN3:
                break;
            case NODETYPE_ASSIGN4:
                break;
            case NODETYPE_OR:
                break;
            case NODETYPE_AND:
                break;
            case NODETYPE_EQ:
                break;
            case NODETYPE_NE:
                break;
            case NODETYPE_LT:
                break;
            case NODETYPE_LE:
                break;
            case NODETYPE_GT:
                break;
            case NODETYPE_GE:
                break;
            case NODETYPE_NOT:
                break;
            case NODETYPE_ADD: {
                int64_t val1 = *(int64_t*)provide_stack(mem, -16);
                int64_t val2 = *(int64_t*)provide_stack(mem, -8);
                *(int64_t*)provide_stack(mem, -16) = val1 + val2;
                *provide_sp(mem) -= sizeof(int64_t);
            } break;
            case NODETYPE_SUB:
                break;
            case NODETYPE_MUL:
                break;
            case NODETYPE_DIV:
                break;
            case NODETYPE_MOD:
                break;
            case NODETYPE_SHL:
                break;
            case NODETYPE_SHR:
                break;
            case NODETYPE_BITOR:
                break;
            case NODETYPE_BITXOR:
                break;
            case NODETYPE_BITAND:
                break;
            case NODETYPE_DEREF:
                break;
            case NODETYPE_NEG:
                break;
            case NODETYPE_BITNOT:
                break;
            case NODETYPE_READ:
                break;
            case NODETYPE_WRITE:
                break;
            case NODETYPE_USLEEP:
                break;
            default:
                ERROUT;
                return ERR;
        }
    }
}