#include "lkjscript.h"

result_t genbin(node_t* execlist_rbegin, uint8_t* bin) {
    uint8_t* bin_itr = bin;
    node_t* node_itr = execlist_rbegin;
    while (node_itr != NULL) {
        node_itr = node_itr->next;
    }
    return OK;
}