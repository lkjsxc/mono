#include "lkjscript.h"

result_t genbin(node_t* execlist_rbegin, uint8_t* bin) {
    uint8_t* bin_itr = bin;
    node_t* node_itr = execlist_rbegin;
    while (node_itr != NULL) {
        switch (node_itr->nodetype) {
            case NODETYPE_NOP:
                break;
            case NODETYPE_PUSH_CONST:
                *(uint8_t*)bin_itr++ = NODETYPE_PUSH_CONST;
                *(int64_t*)bin_itr++ = node_itr->val;
                break;
            default:
                ERROUT;  // Unsupported nodetype
                return ERR;
        }
        node_itr = node_itr->next;
    }
    return OK;
}