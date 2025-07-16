#include "lkjagent.h"

// Global error state - thread-local storage would be better in multi-threaded environments
static char last_error_message[512] = {0};
static int error_logging_enabled = 1;

/**
 * @brief Log an error message with function context
 * @param function Name of the function where the error occurred
 * @param message Descriptive error message
 */
void lkj_log_error(const char* function, const char* message) {
    if (!function || !message) {
        return;
    }
    
    // Store the last error message
    int written = snprintf(last_error_message, sizeof(last_error_message), 
                          "[%s] %s", function, message);
    
    // Ensure null termination even if truncated
    if (written >= (int)sizeof(last_error_message)) {
        last_error_message[sizeof(last_error_message) - 1] = '\0';
    }
    
    // Print to stderr if logging is enabled
    if (error_logging_enabled) {
        fprintf(stderr, "ERROR: %s\n", last_error_message);
    }
}

/**
 * @brief Log an error with errno context
 * @param function Name of the function where the error occurred
 * @param operation Description of the operation that failed
 */
void lkj_log_errno(const char* function, const char* operation) {
    if (!function || !operation) {
        return;
    }
    
    // Get the errno message
    const char* errno_msg = strerror(errno);
    
    // Store the last error message with errno
    int written = snprintf(last_error_message, sizeof(last_error_message), 
                          "[%s] %s: %s (errno=%d)", function, operation, errno_msg, errno);
    
    // Ensure null termination even if truncated
    if (written >= (int)sizeof(last_error_message)) {
        last_error_message[sizeof(last_error_message) - 1] = '\0';
    }
    
    // Print to stderr if logging is enabled
    if (error_logging_enabled) {
        fprintf(stderr, "ERROR: %s\n", last_error_message);
    }
}

/**
 * @brief Get the last recorded error message
 * @return Pointer to the last error message string
 */
const char* lkj_get_last_error(void) {
    return last_error_message;
}

/**
 * @brief Clear the last error message
 */
void lkj_clear_last_error(void) {
    last_error_message[0] = '\0';
}

/**
 * @brief Enable or disable error logging to stderr
 * @param enable 1 to enable, 0 to disable
 */
void lkj_set_error_logging(int enable) {
    error_logging_enabled = enable ? 1 : 0;
}

/**
 * @brief Check if error logging is enabled
 * @return 1 if enabled, 0 if disabled
 */
int lkj_is_error_logging_enabled(void) {
    return error_logging_enabled;
}
