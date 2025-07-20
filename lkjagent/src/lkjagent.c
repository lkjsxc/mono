#include "lkjagent.h"

result_t lkjagent_init(lkjagent_t* lkjagent) {
    if (string_init(&lkjagent->io_buf, lkjagent->io_buf_data, IO_BUF_CAPACITY) != RESULT_OK) {
        RETURN_ERR("Failed to initialize IO buffer");
    }
    return RESULT_OK;
}

result_t lkjagent_loadconfig(lkjagent_t* lkjagent) {
    if (file_read("data/config.json", &lkjagent->io_buf) != RESULT_OK) {
        RETURN_ERR("Failed to load config file");
    }
    return RESULT_OK;
}

result_t lkjagent_run(lkjagent_t* lkjagent) {
    printf("Loaded config: %s\n", lkjagent->io_buf.data);
    return RESULT_OK;
}

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