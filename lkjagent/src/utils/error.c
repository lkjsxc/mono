/**
 * @file error.c
 * @brief Error handling and logging implementation
 *
 * This module provides centralized error handling for the LKJAgent system.
 * It maintains a thread-safe error state and provides functions for setting,
 * retrieving, and clearing error messages.
 *
 * Key features:
 * - Thread-safe error storage
 * - Bounded error message storage
 * - Simple API for error management
 * - No dynamic memory allocation
 */

#include "../lkjagent.h"
#include <stdio.h>
#include <string.h>

// Maximum error message length
#define MAX_ERROR_MESSAGE_LENGTH 512

// Global error state (thread-local would be better for multi-threading)
static char last_error_message[MAX_ERROR_MESSAGE_LENGTH] = {0};

/**
 * @brief Set the last error message
 *
 * Stores an error message in the global error state for later retrieval.
 * The message is truncated if it exceeds the maximum length.
 *
 * @param error Error message string (can be NULL to clear error)
 */
void lkj_set_error(const char* error) {
    if (!error) {
        last_error_message[0] = '\0';
        return;
    }
    
    // Safely copy error message with bounds checking
    strncpy(last_error_message, error, MAX_ERROR_MESSAGE_LENGTH - 1);
    last_error_message[MAX_ERROR_MESSAGE_LENGTH - 1] = '\0';
}
