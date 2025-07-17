#include "lkjagent.h"
#include <stdio.h>

int main() {
    printf("LKJAgent starting...\n");
    
    lkjagent_t lkjagent;

    if(lkjagent_init(&lkjagent) != RESULT_OK) {
        fprintf(stderr, "Failed to initialize agent: %s\n", lkj_get_last_error());
        return 1;
    }

    printf("Agent initialized successfully.\n");

    if(lkjagent_run(&lkjagent) != RESULT_OK) {
        fprintf(stderr, "Agent execution failed: %s\n", lkj_get_last_error());
        return 1;
    }
    
    printf("Agent completed successfully.\n");
    return 0;
}