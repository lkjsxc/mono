/**
 * @file memory_context.h
 * @brief Memory context management interface for LKJAgent
 * 
 * This header provides the unified memory and context management system
 * with LLM-directed paging, context key management, and intelligent
 * memory operations for the autonomous agent system.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#ifndef LKJAGENT_MEMORY_CONTEXT_H
#define LKJAGENT_MEMORY_CONTEXT_H

#include "types.h"
#include "data.h"

/* Forward declaration to avoid circular includes */
#ifndef MAX_TAG_SIZE
#define MAX_TAG_SIZE 64
#endif

/**
 * @defgroup Memory_Context Memory Context Management
 * @{
 */

/**
 * @brief Memory query criteria structure
 * 
 * Defines criteria for complex memory queries across layers.
 */
typedef struct {
    /** Context key pattern (can include wildcards) */
    char key_pattern[MAX_TAG_SIZE];
    /** Target memory layer (or -1 for all layers) */
    int layer;
    /** Minimum importance score (or 0 for any) */
    size_t min_importance;
    /** Maximum importance score (or 100 for any) */
    size_t max_importance;
    /** Start time for temporal queries (or 0 for any) */
    time_t start_time;
    /** End time for temporal queries (or 0 for any) */
    time_t end_time;
    /** Maximum number of results to return */
    size_t max_results;
} memory_query_criteria_t;

/**
 * @brief Memory query result structure
 * 
 * Contains results from memory queries with metadata.
 */
typedef struct {
    /** Context key for this result */
    context_key_t key;
    /** Retrieved data */
    data_t data;
    /** Relevance score (0-100) */
    size_t relevance_score;
} memory_query_result_t;

/**
 * @brief Memory statistics structure
 * 
 * Provides comprehensive statistics about memory usage and performance.
 */
typedef struct {
    /** Total memory size across all layers */
    size_t total_size;
    /** Working memory size */
    size_t working_size;
    /** Disk memory size */
    size_t disk_size;
    /** Archived memory size */
    size_t archived_size;
    /** Number of context keys */
    size_t context_key_count;
    /** Number of access operations */
    uint64_t access_count;
    /** Number of store operations */
    uint64_t store_count;
    /** Number of delete operations */
    uint64_t delete_count;
    /** Last modification time */
    time_t last_modified;
    /** Average access time in microseconds */
    uint64_t avg_access_time;
} memory_stats_t;

/**
 * @brief Context window information structure
 * 
 * Contains information about context window state and limits.
 */
typedef struct {
    /** Current context size in characters */
    size_t current_size;
    /** Maximum context size allowed */
    size_t max_size;
    /** Number of context keys in window */
    size_t key_count;
    /** Estimated LLM context tokens */
    size_t estimated_tokens;
    /** Context utilization percentage */
    double utilization;
} context_window_info_t;

/**
 * @defgroup Context_Key_Operations Context Key Operations
 * @{
 */

/**
 * @brief Create a new context key with metadata
 * 
 * Creates a new context key entry in the directory with initial metadata
 * including importance score, layer assignment, and timestamps.
 * 
 * @param memory Pointer to tagged memory system
 * @param key_name Name of the context key to create
 * @param layer Initial memory layer for the key
 * @param importance Initial importance score (0-100)
 * @param data_size Size of data associated with this key
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Key names must be unique within the memory system
 * @note Importance scores are validated to be within 0-100 range
 * @note Creation timestamp is automatically set to current time
 * 
 * @warning memory parameter must not be NULL
 * @warning key_name parameter must not be NULL and non-empty
 * @warning layer must be a valid memory_layer_t value
 * @warning importance must be between 0 and 100
 */
result_t context_key_create(tagged_memory_t* memory, const char* key_name, 
                           memory_layer_t layer, size_t importance, size_t data_size) 
                           __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Find a context key in the directory
 * 
 * Searches for a context key by name and returns its metadata.
 * Updates access timestamp if key is found.
 * 
 * @param memory Pointer to tagged memory system
 * @param key_name Name of the context key to find
 * @param key Pointer to store found context key data
 * @return RESULT_OK if found, RESULT_ERR if not found or error
 * 
 * @note Access timestamp is updated on successful lookup
 * @note Key lookup is case-sensitive
 * @note Function performs bounds checking on all parameters
 * 
 * @warning memory parameter must not be NULL
 * @warning key_name parameter must not be NULL and non-empty
 * @warning key parameter must not be NULL
 */
result_t context_key_find(tagged_memory_t* memory, const char* key_name, context_key_t* key) 
                         __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Update importance score for a context key
 * 
 * Updates the importance score for an existing context key based on
 * LLM analysis or usage patterns.
 * 
 * @param memory Pointer to tagged memory system
 * @param key_name Name of the context key to update
 * @param new_importance New importance score (0-100)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Importance changes trigger recalculation of paging priorities
 * @note Last modified timestamp is updated on successful operation
 * @note Importance scores are validated to be within valid range
 * 
 * @warning memory parameter must not be NULL
 * @warning key_name parameter must not be NULL and non-empty
 * @warning new_importance must be between 0 and 100
 */
result_t context_key_update_importance(tagged_memory_t* memory, const char* key_name, size_t new_importance) 
                                      __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Move context key to different memory layer
 * 
 * Moves a context key and its associated data from one memory layer
 * to another based on LLM paging directives or automatic policies.
 * 
 * @param memory Pointer to tagged memory system
 * @param key_name Name of the context key to move
 * @param target_layer Target memory layer for the key
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Data is moved atomically - either completely succeeds or fails
 * @note Source data is only removed after successful target write
 * @note Layer transitions update all relevant metadata
 * @note Operation is logged for audit purposes
 * 
 * @warning memory parameter must not be NULL
 * @warning key_name parameter must not be NULL and non-empty
 * @warning target_layer must be a valid memory_layer_t value
 */
result_t context_key_move_layer(tagged_memory_t* memory, const char* key_name, memory_layer_t target_layer) 
                               __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Archive old context key to compressed storage
 * 
 * Archives a context key and its data to compressed archived storage,
 * removing it from active memory layers while preserving it for
 * potential future retrieval.
 * 
 * @param memory Pointer to tagged memory system
 * @param key_name Name of the context key to archive
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Archived data is compressed to save storage space
 * @note Archive operations are atomic and reversible
 * @note Archived keys remain searchable but with slower access
 * @note Original data is securely removed after successful archive
 * 
 * @warning memory parameter must not be NULL
 * @warning key_name parameter must not be NULL and non-empty
 */
result_t context_key_archive(tagged_memory_t* memory, const char* key_name) 
                            __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Validate context key integrity
 * 
 * Performs comprehensive validation of a context key's metadata
 * and associated data integrity.
 * 
 * @param key Pointer to context key to validate
 * @return RESULT_OK if valid, RESULT_ERR if invalid
 * 
 * @note Validates all metadata fields for consistency
 * @note Checks key name format and uniqueness requirements
 * @note Verifies timestamp validity and ordering
 * @note Validates importance score and layer assignment
 * 
 * @warning key parameter must not be NULL
 */
result_t context_key_validate(const context_key_t* key) 
                             __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief List context keys by memory layer
 * 
 * Enumerates all context keys in a specific memory layer,
 * optionally sorted by importance or access time.
 * 
 * @param memory Pointer to tagged memory system
 * @param layer Target memory layer to enumerate
 * @param keys Array to store found context keys
 * @param max_keys Maximum number of keys to return
 * @param key_count Pointer to store actual number of keys found
 * @param sort_by_importance If true, sort by importance; if false, sort by access time
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Results are sorted in descending order (highest first)
 * @note Function handles layer validation and bounds checking
 * @note Empty layers return success with key_count set to 0
 * 
 * @warning memory parameter must not be NULL
 * @warning layer must be a valid memory_layer_t value
 * @warning keys parameter must not be NULL if max_keys > 0
 * @warning key_count parameter must not be NULL
 */
result_t context_key_list_by_layer(tagged_memory_t* memory, memory_layer_t layer,
                                  context_key_t* keys, size_t max_keys, size_t* key_count,
                                  bool sort_by_importance) 
                                  __attribute__((warn_unused_result)) __attribute__((nonnull(1, 5)));

/**
 * @brief Cleanup expired context keys
 * 
 * Automatically removes or archives context keys that have not been
 * accessed within the specified time threshold.
 * 
 * @param memory Pointer to tagged memory system
 * @param expiry_threshold Age in seconds after which keys are considered expired
 * @param archive_instead_of_delete If true, archive expired keys; if false, delete them
 * @param cleaned_count Pointer to store number of keys cleaned up
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Cleanup operations are performed atomically
 * @note High-importance keys may be exempt from automatic cleanup
 * @note Cleanup statistics are logged for monitoring purposes
 * 
 * @warning memory parameter must not be NULL
 * @warning expiry_threshold must be greater than 0
 * @warning cleaned_count parameter must not be NULL
 */
result_t context_key_cleanup_expired(tagged_memory_t* memory, time_t expiry_threshold,
                                    bool archive_instead_of_delete, size_t* cleaned_count) 
                                    __attribute__((warn_unused_result)) __attribute__((nonnull(1, 4)));

/** @} */

/**
 * @defgroup Tagged_Memory_Operations Tagged Memory Core Operations
 * @{
 */

/**
 * @brief Initialize tagged memory system
 * 
 * Initializes the complete tagged memory system with unified storage,
 * context key directory, and performance monitoring.
 * 
 * @param memory Pointer to tagged memory structure to initialize
 * @param memory_file Path to memory.json file
 * @param context_keys_file Path to context_keys.json file
 * @param max_working_size Maximum size for working memory layer
 * @param max_disk_size Maximum size for disk memory layer
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Creates empty files if they don't exist
 * @note Loads existing data if files are present and valid
 * @note Validates file integrity during initialization
 * @note Sets up performance monitoring and statistics tracking
 * 
 * @warning memory parameter must not be NULL
 * @warning memory_file parameter must not be NULL
 * @warning context_keys_file parameter must not be NULL
 * @warning max_working_size and max_disk_size must be greater than 0
 */
result_t tagged_memory_init(tagged_memory_t* memory, const char* memory_file,
                           const char* context_keys_file, size_t max_working_size,
                           size_t max_disk_size) 
                           __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Store data with context key
 * 
 * Stores data in the appropriate memory layer with the specified
 * context key, creating or updating the key as needed.
 * 
 * @param memory Pointer to tagged memory system
 * @param key_name Context key name for the data
 * @param data Data to store
 * @param layer Target memory layer for storage
 * @param importance Importance score for the data (0-100)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Automatically creates context key if it doesn't exist
 * @note Updates existing context key metadata if key exists
 * @note Handles layer capacity limits with intelligent overflow
 * @note Operations are atomic and thread-safe
 * 
 * @warning memory parameter must not be NULL
 * @warning key_name parameter must not be NULL and non-empty
 * @warning data parameter must not be NULL
 * @warning layer must be a valid memory_layer_t value
 * @warning importance must be between 0 and 100
 */
result_t tagged_memory_store(tagged_memory_t* memory, const char* key_name,
                            const data_t* data, memory_layer_t layer, size_t importance) 
                            __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Retrieve data by context key
 * 
 * Retrieves data associated with the specified context key from
 * any memory layer, updating access statistics.
 * 
 * @param memory Pointer to tagged memory system
 * @param key_name Context key name for the data
 * @param data Data buffer to store retrieved data
 * @return RESULT_OK on success, RESULT_ERR if key not found or error
 * 
 * @note Searches all memory layers starting with working memory
 * @note Updates access timestamp and statistics
 * @note Handles automatic layer promotion for frequently accessed data
 * @note Returns empty data if key exists but has no associated data
 * 
 * @warning memory parameter must not be NULL
 * @warning key_name parameter must not be NULL and non-empty
 * @warning data parameter must not be NULL and initialized
 */
result_t tagged_memory_retrieve(tagged_memory_t* memory, const char* key_name, data_t* data) 
                               __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Query memory with multiple criteria
 * 
 * Performs complex queries across memory layers based on multiple
 * criteria including patterns, importance, and time ranges.
 * 
 * @param memory Pointer to tagged memory system
 * @param criteria Query criteria specification
 * @param results Array to store query results
 * @param max_results Maximum number of results to return
 * @param result_count Pointer to store actual number of results found
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Results are ranked by relevance score
 * @note Supports wildcard patterns in key names
 * @note Handles complex temporal and importance-based filtering
 * @note Optimized for performance with indexed lookups
 * 
 * @warning memory parameter must not be NULL
 * @warning criteria parameter must not be NULL
 * @warning results parameter must not be NULL if max_results > 0
 * @warning result_count parameter must not be NULL
 */
result_t tagged_memory_query(tagged_memory_t* memory, const memory_query_criteria_t* criteria,
                            memory_query_result_t* results, size_t max_results, size_t* result_count) 
                            __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 5)));

/**
 * @brief Delete data and context key
 * 
 * Removes data and its associated context key from the memory system,
 * performing cleanup operations as needed.
 * 
 * @param memory Pointer to tagged memory system
 * @param key_name Context key name for the data to delete
 * @return RESULT_OK on success, RESULT_ERR if key not found or error
 * 
 * @note Deletion is atomic and cannot be undone
 * @note Removes data from all memory layers
 * @note Updates memory statistics and compaction triggers
 * @note Securely clears sensitive data before deallocation
 * 
 * @warning memory parameter must not be NULL
 * @warning key_name parameter must not be NULL and non-empty
 */
result_t tagged_memory_delete(tagged_memory_t* memory, const char* key_name) 
                             __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Get comprehensive memory statistics
 * 
 * Collects and returns comprehensive statistics about memory usage,
 * performance, and system health.
 * 
 * @param memory Pointer to tagged memory system
 * @param stats Pointer to store collected statistics
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Statistics collection is lightweight and real-time
 * @note Includes performance metrics and capacity utilization
 * @note Provides data for memory optimization decisions
 * 
 * @warning memory parameter must not be NULL
 * @warning stats parameter must not be NULL
 */
result_t tagged_memory_get_stats(tagged_memory_t* memory, memory_stats_t* stats) 
                                __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Compact and optimize memory storage
 * 
 * Performs memory compaction, defragmentation, and optimization
 * to maintain peak performance over extended operation.
 * 
 * @param memory Pointer to tagged memory system
 * @param aggressive If true, perform thorough compaction; if false, quick optimization
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Compaction is performed online without service interruption
 * @note Aggressive compaction may take longer but provides better optimization
 * @note Updates internal structures for optimal access patterns
 * @note Triggers backup creation before major structural changes
 * 
 * @warning memory parameter must not be NULL
 */
result_t tagged_memory_compact(tagged_memory_t* memory, bool aggressive) 
                              __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Destroy tagged memory system
 * 
 * Safely shuts down the memory system, saving all data and
 * cleaning up allocated resources.
 * 
 * @param memory Pointer to tagged memory system to destroy
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note All data is saved to persistent storage before cleanup
 * @note Memory structures are securely cleared
 * @note File handles and locks are properly released
 * 
 * @warning memory parameter must not be NULL
 */
result_t tagged_memory_destroy(tagged_memory_t* memory) 
                              __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Calculate total memory size across all layers
 * 
 * Helper function to calculate the total memory usage across
 * working memory, disk memory, and archived storage.
 * 
 * @param memory Pointer to tagged memory system
 * @return Total memory size in bytes
 * 
 * @note Function is safe to call frequently for monitoring
 * @note Includes metadata overhead in calculations
 * 
 * @warning memory parameter must not be NULL
 */
size_t calculate_total_memory_size(const tagged_memory_t* memory) 
                                  __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/** @} */

/**
 * @defgroup Context_Window_Management Context Window Management
 * @{
 */

/**
 * @brief Calculate current context window size
 * 
 * Calculates the total size of the current context window
 * including all active memory layers and metadata.
 * 
 * @param memory Pointer to tagged memory system
 * @param info Pointer to store context window information
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Includes estimation of LLM token usage
 * @note Accounts for metadata overhead in calculations
 * @note Provides utilization percentage for monitoring
 * 
 * @warning memory parameter must not be NULL
 * @warning info parameter must not be NULL
 */
result_t context_window_calculate(tagged_memory_t* memory, context_window_info_t* info) 
                                 __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Trim context to fit size limits
 * 
 * Intelligently trims context content to fit within specified
 * size limits while preserving the most important information.
 * 
 * @param memory Pointer to tagged memory system
 * @param max_size Maximum allowed context size
 * @param preserve_recent If true, prioritize recent content
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Trimming preserves high-importance content first
 * @note Recent content can be prioritized over older content
 * @note Operation maintains context coherence where possible
 * @note Trimmed content is moved to disk layer, not deleted
 * 
 * @warning memory parameter must not be NULL
 * @warning max_size must be greater than 0
 */
result_t context_window_trim(tagged_memory_t* memory, size_t max_size, bool preserve_recent) 
                            __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Prioritize context by importance
 * 
 * Reorders context content based on importance scores and
 * access patterns for optimal LLM context utilization.
 * 
 * @param memory Pointer to tagged memory system
 * @param max_context_keys Maximum number of context keys to keep active
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note High-importance content is moved to working memory
 * @note Low-importance content is moved to disk memory
 * @note Recent access patterns influence prioritization
 * @note Operation is atomic and reversible
 * 
 * @warning memory parameter must not be NULL
 * @warning max_context_keys must be greater than 0
 */
result_t context_window_prioritize(tagged_memory_t* memory, size_t max_context_keys) 
                                  __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Prepare context for LLM processing
 * 
 * Prepares the optimal context for LLM processing based on
 * current state, importance scores, and context limits.
 * 
 * @param memory Pointer to tagged memory system
 * @param state Current agent state
 * @param context_buffer Buffer to store prepared context
 * @param max_tokens Maximum LLM context tokens allowed
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Context is optimized for the specific agent state
 * @note Token limits are respected with intelligent truncation
 * @note Context includes relevant metadata for LLM processing
 * @note Prepared context maintains logical coherence
 * 
 * @warning memory parameter must not be NULL
 * @warning context_buffer parameter must not be NULL and initialized
 * @warning max_tokens must be greater than 0
 */
result_t context_window_prepare_llm(tagged_memory_t* memory, agent_state_t state,
                                   data_t* context_buffer, size_t max_tokens) 
                                   __attribute__((warn_unused_result)) __attribute__((nonnull(1, 3)));

/** @} */

/**
 * @defgroup Memory_Query_Operations Memory Query Operations
 * @{
 */

/**
 * @brief Query memory by tag pattern
 * 
 * Searches for context keys matching the specified tag pattern using
 * wildcard matching and returns ranked results.
 * 
 * @param memory Pointer to tagged memory system
 * @param tag_pattern Tag pattern to search for (supports wildcards)
 * @param results Array to store query results
 * @param max_results Maximum number of results to return
 * @param result_count Pointer to store actual number of results found
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Supports standard wildcard patterns (* and ?)
 * @note Results are ranked by relevance score
 * @note Each result includes context key, data, and relevance score
 * 
 * @warning memory parameter must not be NULL
 * @warning tag_pattern parameter must not be NULL
 * @warning result_count parameter must not be NULL
 */
result_t memory_query_by_tag(tagged_memory_t* memory, const char* tag_pattern,
                           memory_query_result_t* results, size_t max_results, size_t* result_count) 
                           __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 5)));

/**
 * @brief Query memory by specific context key
 * 
 * Retrieves data for a specific context key with metadata and
 * relevance scoring.
 * 
 * @param memory Pointer to tagged memory system
 * @param key_name Name of the context key to query
 * @param result Pointer to store query result
 * @return RESULT_OK on success, RESULT_ERR if key not found
 * 
 * @note Updates access statistics for the key
 * @note Calculates relevance score based on current context
 * @note Result data buffer is initialized and populated
 * 
 * @warning memory parameter must not be NULL
 * @warning key_name parameter must not be NULL
 * @warning result parameter must not be NULL
 */
result_t memory_query_by_context_key(tagged_memory_t* memory, const char* key_name,
                                   memory_query_result_t* result) 
                                   __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Query memory by importance score range
 * 
 * Finds all context keys with importance scores within the
 * specified range, ranked by relevance.
 * 
 * @param memory Pointer to tagged memory system
 * @param min_importance Minimum importance score (0-100)
 * @param max_importance Maximum importance score (0-100)
 * @param results Array to store query results
 * @param max_results Maximum number of results to return
 * @param result_count Pointer to store actual number of results found
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Results are sorted by importance score descending
 * @note Includes relevance scoring based on access patterns
 * @note Empty results if no keys match the importance range
 * 
 * @warning memory parameter must not be NULL
 * @warning min_importance and max_importance must be 0-100
 * @warning min_importance must be <= max_importance
 * @warning result_count parameter must not be NULL
 */
result_t memory_query_by_importance(tagged_memory_t* memory, size_t min_importance, size_t max_importance,
                                  memory_query_result_t* results, size_t max_results, size_t* result_count) 
                                  __attribute__((warn_unused_result)) __attribute__((nonnull(1, 6)));

/**
 * @brief Query memory by time range
 * 
 * Finds all context keys with last access times within the
 * specified time range.
 * 
 * @param memory Pointer to tagged memory system
 * @param start_time Start of time range (0 for no lower bound)
 * @param end_time End of time range (0 for no upper bound)
 * @param results Array to store query results
 * @param max_results Maximum number of results to return
 * @param result_count Pointer to store actual number of results found
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Results are sorted by access time descending (most recent first)
 * @note Time bounds are inclusive
 * @note Both bounds can be 0 to disable time filtering
 * 
 * @warning memory parameter must not be NULL
 * @warning start_time must be <= end_time if both are non-zero
 * @warning result_count parameter must not be NULL
 */
result_t memory_query_by_timerange(tagged_memory_t* memory, time_t start_time, time_t end_time,
                                 memory_query_result_t* results, size_t max_results, size_t* result_count) 
                                 __attribute__((warn_unused_result)) __attribute__((nonnull(1, 6)));

/**
 * @brief Query for keys related to a reference key
 * 
 * Finds context keys that are semantically or structurally related
 * to the specified reference key using pattern analysis.
 * 
 * @param memory Pointer to tagged memory system
 * @param reference_key Reference key to find related items for
 * @param results Array to store query results
 * @param max_results Maximum number of results to return
 * @param result_count Pointer to store actual number of results found
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Uses pattern matching and semantic analysis
 * @note Excludes the reference key itself from results
 * @note Results are ranked by relationship strength
 * 
 * @warning memory parameter must not be NULL
 * @warning reference_key parameter must not be NULL
 * @warning result_count parameter must not be NULL
 */
result_t memory_query_related(tagged_memory_t* memory, const char* reference_key,
                            memory_query_result_t* results, size_t max_results, size_t* result_count) 
                            __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 5)));

/**
 * @brief Generate query summary report
 * 
 * Executes a query and generates a comprehensive summary report
 * of the results with metadata and statistics.
 * 
 * @param memory Pointer to tagged memory system
 * @param criteria Query criteria specification
 * @param summary_buffer Buffer to store generated summary
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Summary includes result statistics and previews
 * @note Formatted for human readability
 * @note Includes performance metrics and relevance scores
 * 
 * @warning memory parameter must not be NULL
 * @warning criteria parameter must not be NULL
 * @warning summary_buffer parameter must not be NULL and initialized
 */
result_t memory_query_summary(tagged_memory_t* memory, const memory_query_criteria_t* criteria,
                            data_t* summary_buffer) 
                            __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Optimize query criteria for performance
 * 
 * Analyzes and optimizes query criteria based on current memory
 * statistics to improve query performance and relevance.
 * 
 * @param memory Pointer to tagged memory system
 * @param criteria Query criteria to optimize (modified in place)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Modifies criteria based on memory statistics
 * @note Adjusts limits and filters for optimal performance
 * @note Considers current memory load and distribution
 * 
 * @warning memory parameter must not be NULL
 * @warning criteria parameter must not be NULL
 */
result_t memory_query_optimize(tagged_memory_t* memory, memory_query_criteria_t* criteria) 
                              __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/** @} */

/**
 * @defgroup LLM_Memory_Integration LLM Memory Integration
 * @{
 */

/**
 * @brief Analyze LLM response for context keys
 * 
 * Analyzes an LLM response to identify context keys and patterns
 * that should be stored or retrieved from memory.
 * 
 * @param memory Pointer to tagged memory system
 * @param llm_response LLM response text to analyze
 * @param context_keys Array to store identified context keys
 * @param max_keys Maximum number of keys to extract
 * @param key_count Pointer to store number of keys found
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Uses simple tag parsing and pattern recognition
 * @note Generates default keys if none found explicitly
 * @note Context keys are suitable for memory operations
 * 
 * @warning memory parameter must not be NULL
 * @warning llm_response parameter must not be NULL
 * @warning key_count parameter must not be NULL
 */
result_t memory_llm_analyze_context(tagged_memory_t* memory, const char* llm_response,
                                  char context_keys[][MAX_TAG_SIZE], size_t max_keys, size_t* key_count) 
                                  __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 5)));

/**
 * @brief Identify memory keys from LLM response
 * 
 * Extracts explicit memory key references from LLM response text
 * using keyword detection and pattern matching.
 * 
 * @param llm_response LLM response text to analyze
 * @param identified_keys Array to store found keys
 * @param max_keys Maximum number of keys to extract
 * @param key_count Pointer to store number of keys found
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Looks for explicit memory references and directives
 * @note Handles quoted and unquoted key names
 * @note Validates extracted keys for proper format
 * 
 * @warning llm_response parameter must not be NULL
 * @warning key_count parameter must not be NULL
 */
result_t memory_llm_identify_keys(const char* llm_response, char identified_keys[][MAX_TAG_SIZE],
                                size_t max_keys, size_t* key_count) 
                                __attribute__((warn_unused_result)) __attribute__((nonnull(1, 4)));

/**
 * @brief Request LLM paging directives
 * 
 * Generates a comprehensive paging request for the LLM based on
 * current memory state and context requirements.
 * 
 * @param memory Pointer to tagged memory system
 * @param current_state Current agent state
 * @param context_limit Maximum context size allowed
 * @param paging_request Buffer to store generated request
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Request includes memory statistics and current state
 * @note Formatted for LLM understanding with clear directives
 * @note Includes examples of expected paging commands
 * 
 * @warning memory parameter must not be NULL
 * @warning context_limit must be greater than 0
 * @warning paging_request parameter must not be NULL and initialized
 */
result_t memory_llm_request_paging(tagged_memory_t* memory, agent_state_t current_state,
                                 size_t context_limit, data_t* paging_request) 
                                 __attribute__((warn_unused_result)) __attribute__((nonnull(1, 4)));

/**
 * @brief Process LLM paging directives
 * 
 * Parses and executes paging directives from LLM response using
 * simple tag format for memory management operations.
 * 
 * @param memory Pointer to tagged memory system
 * @param llm_response LLM response containing paging directives
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Supports move, archive, delete, and importance directives
 * @note Operations are executed atomically where possible
 * @note Invalid directives are skipped with error logging
 * 
 * @warning memory parameter must not be NULL
 * @warning llm_response parameter must not be NULL
 */
result_t memory_llm_process_directives(tagged_memory_t* memory, const char* llm_response) 
                                      __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Evaluate content importance using LLM-style analysis
 * 
 * Analyzes content to determine its importance score using
 * heuristics and pattern matching similar to LLM evaluation.
 * 
 * @param content Content text to evaluate
 * @param context Optional context for evaluation
 * @param importance_score Pointer to store calculated importance (0-100)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Uses keyword analysis and content structure
 * @note Context information influences scoring
 * @note Explicit importance markers are respected
 * 
 * @warning content parameter must not be NULL
 * @warning importance_score parameter must not be NULL
 */
result_t memory_llm_evaluate_importance(const char* content, const char* context, size_t* importance_score) 
                                       __attribute__((warn_unused_result)) __attribute__((nonnull(1, 3)));

/**
 * @brief Suggest related context keys
 * 
 * Uses LLM-style analysis to suggest context keys that are
 * related to a base key for improved context management.
 * 
 * @param memory Pointer to tagged memory system
 * @param base_key Base context key to find relations for
 * @param related_keys Array to store suggested related keys
 * @param max_keys Maximum number of keys to suggest
 * @param key_count Pointer to store number of keys suggested
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Uses existing memory query for relationship detection
 * @note Suggestions are based on key patterns and content similarity
 * @note Results are ranked by relationship strength
 * 
 * @warning memory parameter must not be NULL
 * @warning base_key parameter must not be NULL
 * @warning key_count parameter must not be NULL
 */
result_t memory_llm_suggest_relationships(tagged_memory_t* memory, const char* base_key,
                                        char related_keys[][MAX_TAG_SIZE], size_t max_keys, size_t* key_count) 
                                        __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 5)));

/**
 * @brief Optimize storage based on LLM analysis
 * 
 * Performs state-aware storage optimization using strategies
 * similar to LLM-directed memory management.
 * 
 * @param memory Pointer to tagged memory system
 * @param current_state Current agent state for optimization context
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Optimization strategy varies by agent state
 * @note Considers importance, recency, and access patterns
 * @note Automatically redistributes content across layers
 * 
 * @warning memory parameter must not be NULL
 */
result_t memory_llm_optimize_storage(tagged_memory_t* memory, agent_state_t current_state) 
                                    __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/** @} */

/**
 * @defgroup Memory_Cleanup_Operations Memory Cleanup Operations
 * @{
 */

/**
 * @brief Clean up expired content from memory
 * 
 * Removes or archives content that has not been accessed within
 * the specified time threshold.
 * 
 * @param memory Pointer to tagged memory system
 * @param expiry_threshold Age in seconds after which content is considered expired
 * @param cleaned_count Pointer to store number of items cleaned
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note High-importance content is exempt from cleanup
 * @note Cleanup operations are performed atomically
 * @note Statistics are updated after cleanup
 * 
 * @warning memory parameter must not be NULL
 * @warning expiry_threshold must be greater than 0
 * @warning cleaned_count parameter must not be NULL
 */
result_t memory_cleanup_expired(tagged_memory_t* memory, time_t expiry_threshold, size_t* cleaned_count) 
                               __attribute__((warn_unused_result)) __attribute__((nonnull(1, 3)));

/**
 * @brief Remove duplicate content from memory
 * 
 * Identifies and removes duplicate or near-duplicate content
 * based on similarity analysis.
 * 
 * @param memory Pointer to tagged memory system
 * @param similarity_threshold Similarity threshold for duplicate detection (0.0-1.0)
 * @param removed_count Pointer to store number of duplicates removed
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Higher-importance duplicates are preserved
 * @note Similarity includes both key names and content
 * @note Removal operations are atomic
 * 
 * @warning memory parameter must not be NULL
 * @warning similarity_threshold must be between 0.0 and 1.0
 * @warning removed_count parameter must not be NULL
 */
result_t memory_cleanup_duplicates(tagged_memory_t* memory, double similarity_threshold, size_t* removed_count) 
                                  __attribute__((warn_unused_result)) __attribute__((nonnull(1, 3)));

/**
 * @brief Clean up orphaned context keys
 * 
 * Removes context keys that have no associated data or point
 * to invalid/missing content.
 * 
 * @param memory Pointer to tagged memory system
 * @param cleaned_count Pointer to store number of orphaned keys cleaned
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Validates each key by attempting data retrieval
 * @note Orphaned keys are removed from the directory
 * @note Operation maintains directory consistency
 * 
 * @warning memory parameter must not be NULL
 * @warning cleaned_count parameter must not be NULL
 */
result_t memory_cleanup_orphaned(tagged_memory_t* memory, size_t* cleaned_count) 
                                __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Optimize memory storage layout
 * 
 * Performs comprehensive storage optimization including cleanup,
 * compression, and layer redistribution.
 * 
 * @param memory Pointer to tagged memory system
 * @param aggressive If true, perform thorough optimization; if false, quick optimization
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Combines multiple optimization strategies
 * @note Aggressive mode includes compression and defragmentation
 * @note Operations preserve data integrity
 * 
 * @warning memory parameter must not be NULL
 */
result_t memory_optimize_storage(tagged_memory_t* memory, bool aggressive) 
                                __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Compress archived memory content
 * 
 * Applies compression to archived content to reduce storage
 * requirements while maintaining accessibility.
 * 
 * @param memory Pointer to tagged memory system
 * @param compression_ratio Target compression ratio (0.0-1.0)
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Only operates on archived layer content
 * @note Compression is reversible for data retrieval
 * @note Updates metadata to reflect compressed sizes
 * 
 * @warning memory parameter must not be NULL
 * @warning compression_ratio must be between 0.0 and 1.0
 */
result_t memory_compress_archives(tagged_memory_t* memory, double compression_ratio) 
                                 __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Defragment memory storage
 * 
 * Reorganizes memory storage to eliminate fragmentation and
 * improve access performance.
 * 
 * @param memory Pointer to tagged memory system
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Rebuilds memory buffers without gaps
 * @note Maintains data ordering and accessibility
 * @note Updates internal structures for optimization
 * 
 * @warning memory parameter must not be NULL
 */
result_t memory_defragment(tagged_memory_t* memory) 
                          __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Analyze memory usage patterns
 * 
 * Generates comprehensive analysis report of memory usage
 * patterns, performance metrics, and optimization recommendations.
 * 
 * @param memory Pointer to tagged memory system
 * @param analysis_report Buffer to store generated analysis report
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Report includes statistics, trends, and recommendations
 * @note Analysis covers performance, distribution, and health metrics
 * @note Formatted for human readability and decision making
 * 
 * @warning memory parameter must not be NULL
 * @warning analysis_report parameter must not be NULL and initialized
 */
result_t memory_analyze_usage(tagged_memory_t* memory, data_t* analysis_report) 
                             __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/** @} */

/**
 * @defgroup Disk_Memory_Operations Disk Memory Operations
 * @{
 */

/**
 * @brief Store data to disk memory layer
 * 
 * Stores context data to the disk memory layer with optional
 * compression and storage path specification.
 * 
 * @param memory Pointer to tagged memory system
 * @param key_name Context key name for the data
 * @param data Data to store to disk
 * @param compress If true, compress data before storage
 * @param storage_path Path for disk storage operations
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Checks available disk space before storage
 * @note Updates context key metadata for disk layer
 * @note Compression reduces storage requirements
 * 
 * @warning memory parameter must not be NULL
 * @warning key_name parameter must not be NULL
 * @warning data parameter must not be NULL
 * @warning storage_path parameter must not be NULL
 */
result_t memory_disk_store(tagged_memory_t* memory, const char* key_name, const data_t* data,
                          bool compress, const char* storage_path) 
                          __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3, 5)));

/**
 * @brief Retrieve data from disk memory layer
 * 
 * Retrieves context data from the disk memory layer with optional
 * decompression and access tracking.
 * 
 * @param memory Pointer to tagged memory system
 * @param key_name Context key name for the data
 * @param data Buffer to store retrieved data
 * @param decompress If true, decompress data after retrieval
 * @return RESULT_OK on success, RESULT_ERR if key not found
 * 
 * @note Updates access timestamp on successful retrieval
 * @note Handles compressed data transparently
 * @note Maintains importance scores during access
 * 
 * @warning memory parameter must not be NULL
 * @warning key_name parameter must not be NULL
 * @warning data parameter must not be NULL and initialized
 */
result_t memory_disk_retrieve(tagged_memory_t* memory, const char* key_name, data_t* data,
                             bool decompress) 
                             __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Archive data to compressed storage
 * 
 * Archives context data to compressed long-term storage with
 * maximum compression and metadata preservation.
 * 
 * @param memory Pointer to tagged memory system
 * @param key_name Context key name for the data to archive
 * @param archive_path Path for archive storage
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Applies maximum compression for space efficiency
 * @note Creates separate archive files for each key
 * @note Updates context key to archived status
 * 
 * @warning memory parameter must not be NULL
 * @warning key_name parameter must not be NULL
 * @warning archive_path parameter must not be NULL
 */
result_t memory_disk_archive(tagged_memory_t* memory, const char* key_name, const char* archive_path) 
                            __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Clean up disk storage space
 * 
 * Performs disk cleanup to maintain storage usage within specified
 * limits by archiving or removing old content.
 * 
 * @param memory Pointer to tagged memory system
 * @param storage_path Path for disk storage operations
 * @param max_disk_usage Maximum allowed disk usage in bytes
 * @param freed_bytes Pointer to store number of bytes freed
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Prioritizes removal of old, low-importance content
 * @note Archives medium-importance content before deletion
 * @note Preserves high-importance content regardless of age
 * 
 * @warning memory parameter must not be NULL
 * @warning storage_path parameter must not be NULL
 * @warning freed_bytes parameter must not be NULL
 */
result_t memory_disk_cleanup(tagged_memory_t* memory, const char* storage_path, 
                           size_t max_disk_usage, size_t* freed_bytes) 
                           __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 4)));

/**
 * @brief Compact disk storage
 * 
 * Performs comprehensive disk storage compaction including
 * defragmentation, compression, and backup creation.
 * 
 * @param memory Pointer to tagged memory system
 * @param storage_path Path for disk storage operations
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Creates backup before compaction
 * @note Applies compression to all disk content
 * @note Rebuilds storage files for optimal layout
 * 
 * @warning memory parameter must not be NULL
 * @warning storage_path parameter must not be NULL
 */
result_t memory_disk_compact(tagged_memory_t* memory, const char* storage_path) 
                            __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2)));

/**
 * @brief Create backup of disk storage
 * 
 * Creates timestamped backup of all disk storage files with
 * automatic cleanup of old backups.
 * 
 * @param memory Pointer to tagged memory system
 * @param storage_path Path for current storage files
 * @param backup_path Path for backup storage
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Creates timestamped backup files
 * @note Automatically cleans up old backups
 * @note Backs up both memory and context key files
 * 
 * @warning memory parameter must not be NULL
 * @warning storage_path parameter must not be NULL
 * @warning backup_path parameter must not be NULL
 */
result_t memory_disk_backup(tagged_memory_t* memory, const char* storage_path, const char* backup_path) 
                           __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/**
 * @brief Verify disk storage integrity
 * 
 * Performs comprehensive integrity verification of disk storage
 * files and cross-validation with memory structures.
 * 
 * @param memory Pointer to tagged memory system
 * @param storage_path Path for storage files to verify
 * @param is_valid Pointer to store validation result
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Validates file structure and content integrity
 * @note Cross-checks memory data with context keys
 * @note Reports detailed validation status
 * 
 * @warning memory parameter must not be NULL
 * @warning storage_path parameter must not be NULL
 * @warning is_valid parameter must not be NULL
 */
result_t memory_disk_verify(tagged_memory_t* memory, const char* storage_path, bool* is_valid) 
                           __attribute__((warn_unused_result)) __attribute__((nonnull(1, 2, 3)));

/** @} */

/**
 * @defgroup Context_Window_Extended Extended Context Window Management
 * @{
 */

/**
 * @brief Manage context window overflow
 * 
 * Handles context window overflow situations by intelligently
 * selecting content to preserve and content to move or archive.
 * 
 * @param memory Pointer to tagged memory system
 * @param max_size Maximum allowed context window size
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Uses progressive overflow management strategies
 * @note Preserves high-importance and recent content
 * @note Moves content to appropriate layers based on importance
 * 
 * @warning memory parameter must not be NULL
 * @warning max_size must be greater than 0
 */
result_t context_window_manage_overflow(tagged_memory_t* memory, size_t max_size) 
                                       __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Preserve recent content in context window
 * 
 * Ensures that recently accessed content is preserved in the
 * working memory layer and receives priority treatment.
 * 
 * @param memory Pointer to tagged memory system
 * @param preserve_threshold Time threshold for considering content recent
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Moves recent content to working memory
 * @note Boosts importance scores for recent content
 * @note Applies timestamp-based preservation logic
 * 
 * @warning memory parameter must not be NULL
 */
result_t context_window_preserve_recent(tagged_memory_t* memory, time_t preserve_threshold) 
                                       __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/**
 * @brief Optimize context window for agent state
 * 
 * Performs state-specific context window optimization using
 * strategies tailored to the current agent state.
 * 
 * @param memory Pointer to tagged memory system
 * @param state Current agent state for optimization context
 * @return RESULT_OK on success, RESULT_ERR on failure
 * 
 * @note Different optimization strategies for each state
 * @note Adjusts memory layer priorities based on state needs
 * @note Maintains optimal context composition for agent operations
 * 
 * @warning memory parameter must not be NULL
 */
result_t context_window_optimize(tagged_memory_t* memory, agent_state_t state) 
                                __attribute__((warn_unused_result)) __attribute__((nonnull(1)));

/** @} */

/** @} */

#endif /* LKJAGENT_MEMORY_CONTEXT_H */
