#include "lkjagent.h"

result_t lkjagent_init(lkjagent_t* lkjagent) {
    for(uint64_t i = 0; i < COUNTOF(lkjagent->pool_string256); i++) {
        string_init(&lkjagent->pool_string256[i], lkjagent->pool_string256_data[i], sizeof(lkjagent->pool_string256_data[i]));
        lkjagent->pool_string256_freelist_data[i] = &lkjagent->pool_string256[i];
    }
    for(uint64_t i = 0; i < COUNTOF(lkjagent->pool_string4096); i++) {
        string_init(&lkjagent->pool_string4096[i], lkjagent->pool_string4096_data[i], sizeof(lkjagent->pool_string4096_data[i]));
        lkjagent->pool_string4096_freelist_data[i] = &lkjagent->pool_string4096[i];
    }
    lkjagent->pool_string256_freelist_count = COUNTOF(lkjagent->pool_string256);
    lkjagent->pool_string4096_freelist_count = COUNTOF(lkjagent->pool_string4096);
    return RESULT_OK;
}

result_t lkjagent_run(lkjagent_t* lkjagent) {
    return RESULT_OK;
}

int main() {
    lkjagent_t lkjagent;

    if (lkjagent_init(&lkjagent) != RESULT_OK) {
        RETURN_ERR("Failed to initialize lkjagent");
    }

    if (lkjagent_run(&lkjagent) != RESULT_OK) {
        RETURN_ERR("Failed to run lkjagent");
    }

    return 0;
}