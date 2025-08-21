#include "lkjscript.h"

result_t lkjscript_tokenize(pool_t* pool, object_t* dst, data_t* src) {
    if(object_create(pool, dst) != RESULT_OK) {
        RETURN_ERR("Failed to create destination object");
    }
    return RESULT_OK;
}
