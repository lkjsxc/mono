#include "lkjagent.h"
#include "macro.h"
#include "types.h"

int main() {
    lkjagent_t lkjagent;

    if (lkjagent_init(&lkjagent) != RESULT_OK) {
        RETURN_ERR("Failed to initialize lkjagent");
    }

    if (lkjagent_loadconfig(&lkjagent) != RESULT_OK) {
        RETURN_ERR("Failed to load configuration");
    }

    if (lkjagent_run(&lkjagent) != RESULT_OK) {
        RETURN_ERR("Failed to run lkjagent");
    }

    return 0;
}