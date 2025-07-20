#include "lkjagent.h"

result_t lkjagent_init(lkjagent_t* lkjagent) {
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