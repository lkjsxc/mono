#include "lkjagent.h"
#include <stdio.h>

int main() {
    printf("LKJAgent starting...\n");
    
    lkjagent_t lkjagent;

    if(lkjagent_init(&lkjagent) != RESULT_OK) {
        RETURN_ERR("Failed to initialize LKJAgent");
        return 1;
    }

    printf("Agent initialized successfully.\n");

    if(lkjagent_run(&lkjagent) != RESULT_OK) {
        RETURN_ERR("Failed to run LKJAgent");  
        return 1;
    }
    
    printf("Agent completed successfully.\n");
    return 0;
}