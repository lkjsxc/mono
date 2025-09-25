#include "lkjagent.h"

// **UNIFIED ACTION DISPATCHER** 
// High-performance, clean implementation with consistent error handling
// and unified data formats across all operations

typedef struct {
    data_t* sorted_tags_array[MAX_TAGS];
    data_t* sorted_tags_string; 
    result_t status;
} action_context_t;

// Efficient cleanup function - handles all cases safely
static void action_context_cleanup(pool_t* pool, action_context_t* ctx) {
    if (!ctx) return;
    
    // Clean sorted tags array
    for (uint64_t i = 0; i < MAX_TAGS && ctx->sorted_tags_array[i] != NULL; i++) {
        if (data_destroy(pool, ctx->sorted_tags_array[i]) != RESULT_OK) {
            PRINT_ERR("Warning: Failed to cleanup sorted tag array element");
        }
        ctx->sorted_tags_array[i] = NULL;
    }
    
    // Clean sorted tags string
    if (ctx->sorted_tags_string) {
        if (data_destroy(pool, ctx->sorted_tags_string) != RESULT_OK) {
            PRINT_ERR("Warning: Failed to cleanup sorted tags string");
        }
        ctx->sorted_tags_string = NULL;
    }
}

// Initialize action context with proper error handling
static result_t action_context_init(pool_t* pool, action_context_t* ctx, const data_t* tags) {
    // Initialize context
    for (uint64_t i = 0; i < MAX_TAGS; i++) {
        ctx->sorted_tags_array[i] = NULL;
    }
    ctx->sorted_tags_string = NULL;
    ctx->status = RESULT_OK;

    // Sort and normalize tags
    if (tags_sort(pool, ctx->sorted_tags_array, tags) != RESULT_OK) {
        RETURN_ERR("Failed to sort action tags");
    }

    // Convert to string format for compatibility
    if (tags_array_to_string(pool, ctx->sorted_tags_array, &ctx->sorted_tags_string) != RESULT_OK) {
        action_context_cleanup(pool, ctx);
        RETURN_ERR("Failed to convert sorted tags to string");
    }

    return RESULT_OK;
}

// **OPTIMIZED ACTION DISPATCHER**
result_t lkjagent_action(pool_t* pool, lkjagent_t* lkjagent, object_t* action, uint64_t iteration) {
    // Input validation
    if (!pool || !lkjagent || !action) {
        RETURN_ERR("Invalid parameters: pool, lkjagent, or action is NULL");
    }

    // Extract action components
    object_t* action_type = NULL;
    object_t* action_tags = NULL; 
    object_t* action_value = NULL;

    if (object_provide_str(&action_type, action, "type") != RESULT_OK) {
        RETURN_ERR("Failed to get action type");
    }
    if (object_provide_str(&action_tags, action, "tags") != RESULT_OK) {
        RETURN_ERR("Failed to get action tags");
    }
    if (object_provide_str(&action_value, action, "value") != RESULT_OK) {
        RETURN_ERR("Failed to get action value");
    }

    // Initialize action context
    action_context_t ctx;
    if (action_context_init(pool, &ctx, action_tags->data) != RESULT_OK) {
        RETURN_ERR("Failed to initialize action context");
    }

    // **UNIFIED ACTION DISPATCH - No more code duplication!**
    result_t result = RESULT_ERR;

    if (data_equal_str(action_type->data, "working_memory_add")) {
        result = lkjagent_action_working_memory_add(pool, lkjagent, ctx.sorted_tags_string, action_value->data, iteration);
    } else if (data_equal_str(action_type->data, "working_memory_remove")) {
        result = lkjagent_action_working_memory_remove(pool, lkjagent, ctx.sorted_tags_string);
    } else if (data_equal_str(action_type->data, "storage_save")) {
        result = lkjagent_action_storage_save(pool, lkjagent, ctx.sorted_tags_string, action_value->data);
    } else if (data_equal_str(action_type->data, "storage_load")) {
        result = lkjagent_action_storage_load(pool, lkjagent, ctx.sorted_tags_string, iteration);
    } else if (data_equal_str(action_type->data, "storage_search")) {
        result = lkjagent_action_storage_search(pool, lkjagent, ctx.sorted_tags_string, action_value->data, iteration);
    } else {
        action_context_cleanup(pool, &ctx);
        RETURN_ERR("Unknown action type");
    }

    // Cleanup and return result
    action_context_cleanup(pool, &ctx);
    
    if (result != RESULT_OK) {
        RETURN_ERR("Action execution failed");
    }

    return RESULT_OK;
}
