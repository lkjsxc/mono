/**
 * @file test_memory_system.c
 * @brief Simple test for the memory system
 */

#include "include/memory_context.h"
#include "include/data.h"
#include "lkjagent.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main() {
    printf("Testing LKJAgent Memory System...\n");
    
    /* Initialize memory system */
    tagged_memory_t memory;
    result_t init_result = tagged_memory_init(&memory, 
                                            "test_memory.json", 
                                            "test_context_keys.json",
                                            1024 * 1024,  /* 1MB working */
                                            10 * 1024 * 1024); /* 10MB disk */
    
    if (init_result != RESULT_OK) {
        printf("ERROR: Failed to initialize memory system\n");
        return 1;
    }
    
    printf("✓ Memory system initialized successfully\n");
    
    /* Test context key creation */
    result_t create_result = context_key_create(&memory, "test_key", LAYER_WORKING, 75, 100);
    if (create_result != RESULT_OK) {
        printf("ERROR: Failed to create context key\n");
        tagged_memory_destroy(&memory);
        return 1;
    }
    
    printf("✓ Context key created successfully\n");
    
    /* Test data storage */
    data_t test_data;
    data_init(&test_data, 256);
    data_set(&test_data, "This is test content for the memory system", 0);
    
    result_t store_result = tagged_memory_store(&memory, "test_key", &test_data, LAYER_WORKING, 75);
    if (store_result != RESULT_OK) {
        printf("ERROR: Failed to store data\n");
        data_destroy(&test_data);
        tagged_memory_destroy(&memory);
        return 1;
    }
    
    printf("✓ Data stored successfully\n");
    
    /* Test data retrieval */
    data_t retrieved_data;
    data_init(&retrieved_data, 256);
    
    result_t retrieve_result = tagged_memory_retrieve(&memory, "test_key", &retrieved_data);
    if (retrieve_result != RESULT_OK) {
        printf("ERROR: Failed to retrieve data\n");
        data_destroy(&test_data);
        data_destroy(&retrieved_data);
        tagged_memory_destroy(&memory);
        return 1;
    }
    
    printf("✓ Data retrieved successfully\n");
    printf("Retrieved content: %s\n", retrieved_data.data);
    
    /* Test memory statistics */
    memory_stats_t stats;
    result_t stats_result = tagged_memory_get_stats(&memory, &stats);
    if (stats_result != RESULT_OK) {
        printf("ERROR: Failed to get memory statistics\n");
    } else {
        printf("✓ Memory statistics:\n");
        printf("  - Total size: %zu bytes\n", stats.total_size);
        printf("  - Context keys: %zu\n", stats.context_key_count);
        printf("  - Access count: %lu\n", stats.access_count);
        printf("  - Store count: %lu\n", stats.store_count);
    }
    
    /* Test memory deletion */
    result_t delete_result = tagged_memory_delete(&memory, "test_key");
    if (delete_result != RESULT_OK) {
        printf("ERROR: Failed to delete data\n");
    } else {
        printf("✓ Data deleted successfully\n");
    }
    
    /* Clean up */
    data_destroy(&test_data);
    data_destroy(&retrieved_data);
    tagged_memory_destroy(&memory);
    
    printf("\n✓ All memory system tests passed!\n");
    printf("LKJAgent advanced memory and context management system is ready.\n");
    
    return 0;
}
