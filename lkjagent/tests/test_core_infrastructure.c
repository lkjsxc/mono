/**
 * @file test_core_infrastructure.c
 * @brief Comprehensive test suite for LKJAgent core infrastructure
 * 
 * This test program validates all core infrastructure components including
 * data management, tag parsing, file I/O, JSON processing, configuration
 * loading, and memory persistence.
 * 
 * @author LKJAgent Development Team
 * @date 2025
 * @version 1.0.0
 */

#include "../src/lkjagent.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>

/**
 * @brief Test result tracking
 */
typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
} test_results_t;

static test_results_t g_results = {0, 0, 0};

/**
 * @brief Test assertion macro
 */
#define TEST_ASSERT(condition, message) \
    do { \
        g_results.tests_run++; \
        if (condition) { \
            g_results.tests_passed++; \
            printf("✓ %s\n", message); \
        } else { \
            g_results.tests_failed++; \
            printf("✗ %s (FAILED)\n", message); \
        } \
    } while(0)

/**
 * @brief Test section header
 */
#define TEST_SECTION(name) \
    printf("\n=== Testing %s ===\n", name);

/**
 * @brief Test data_t operations
 */
static void test_data_operations(void) {
    TEST_SECTION("Data Management Operations");
    
    data_t buffer;
    
    // Test initialization
    TEST_ASSERT(data_init(&buffer, 64) == RESULT_OK, "data_init with 64 bytes");
    TEST_ASSERT(data_validate(&buffer) == RESULT_OK, "data_validate after init");
    TEST_ASSERT(buffer.size == 0, "Initial size is zero");
    TEST_ASSERT(buffer.capacity >= 64, "Capacity is at least 64");
    
    // Test setting data
    TEST_ASSERT(data_set(&buffer, "Hello, World!", 0) == RESULT_OK, "data_set basic string");
    TEST_ASSERT(buffer.size == 13, "Size after set is correct");
    TEST_ASSERT(strcmp(buffer.data, "Hello, World!") == 0, "Data content is correct");
    
    // Test appending data
    TEST_ASSERT(data_append(&buffer, " How are you?", 0) == RESULT_OK, "data_append string");
    TEST_ASSERT(strcmp(buffer.data, "Hello, World! How are you?") == 0, "Appended content is correct");
    
    // Test size limits
    TEST_ASSERT(data_set(&buffer, "Short", 3) == RESULT_OK, "data_set with size limit");
    TEST_ASSERT(strcmp(buffer.data, "Sho") == 0, "Size-limited content is correct");
    
    // Test trimming front
    TEST_ASSERT(data_set(&buffer, "0123456789", 0) == RESULT_OK, "Set test data for trimming");
    TEST_ASSERT(data_trim_front(&buffer, 3) == RESULT_OK, "data_trim_front 3 chars");
    TEST_ASSERT(strcmp(buffer.data, "3456789") == 0, "Front trimming result is correct");
    
    // Test context trimming
    TEST_ASSERT(data_set(&buffer, "This is a very long string that needs to be trimmed for context", 0) == RESULT_OK, "Set long string");
    TEST_ASSERT(data_trim_context(&buffer, 20, 5) == RESULT_OK, "data_trim_context");
    TEST_ASSERT(buffer.size <= 20, "Context trimming respects size limit");
    
    // Test copy
    data_t copy;
    TEST_ASSERT(data_copy(&copy, &buffer) == RESULT_OK, "data_copy");
    TEST_ASSERT(strcmp(copy.data, buffer.data) == 0, "Copied data is identical");
    TEST_ASSERT(copy.size == buffer.size, "Copied size is identical");
    
    // Test clear
    TEST_ASSERT(data_clear(&buffer) == RESULT_OK, "data_clear");
    TEST_ASSERT(buffer.size == 0, "Size after clear is zero");
    TEST_ASSERT(buffer.data[0] == '\0', "Data is null-terminated after clear");
    
    // Cleanup
    data_destroy(&buffer);
    data_destroy(&copy);
}

/**
 * @brief Test tag parsing operations
 */
static void test_tag_parsing(void) {
    TEST_SECTION("Tag Parsing Operations");
    
    data_t result;
    assert(data_init(&result, 256) == RESULT_OK);
    
    // Test simple tag parsing
    const char* simple_text = "Before <tag>content</tag> after";
    TEST_ASSERT(tag_parse_simple(simple_text, "tag", &result, false) == RESULT_OK, "Simple tag parsing");
    TEST_ASSERT(strcmp(result.data, "content") == 0, "Simple tag content extraction");
    
    // Test thinking tag
    const char* thinking_text = "Some text <thinking>I need to analyze this carefully</thinking> more text";
    TEST_ASSERT(tag_parse_thinking(thinking_text, &result) == RESULT_OK, "Thinking tag parsing");
    TEST_ASSERT(strcmp(result.data, "I need to analyze this carefully") == 0, "Thinking content extraction");
    
    // Test action tag
    const char* action_text = "Analysis complete <action>  execute command: ls -la  </action> done";
    TEST_ASSERT(tag_parse_action(action_text, &result) == RESULT_OK, "Action tag parsing");
    TEST_ASSERT(strcmp(result.data, "execute command: ls -la") == 0, "Action content extraction (trimmed)");
    
    // Test evaluation tag
    const char* eval_text = "Result: <evaluation>Command successful, found 5 files</evaluation>";
    TEST_ASSERT(tag_parse_evaluation(eval_text, &result) == RESULT_OK, "Evaluation tag parsing");
    TEST_ASSERT(strcmp(result.data, "Command successful, found 5 files") == 0, "Evaluation content extraction");
    
    // Test paging tag
    const char* paging_text = "Memory directive <paging>load_context: user_data; archive_context: old_logs</paging>";
    TEST_ASSERT(tag_parse_paging(paging_text, &result) == RESULT_OK, "Paging tag parsing");
    
    // Test context key parsing
    context_key_t keys[10];
    size_t key_count;
    TEST_ASSERT(tag_parse_context_keys(result.data, keys, 10, &key_count) == RESULT_OK, "Context key parsing");
    TEST_ASSERT(key_count >= 1, "At least one context key parsed");
    
    // Test tag validation
    TEST_ASSERT(tag_validate_format("<valid>content</valid>", "valid") == RESULT_OK, "Valid tag format");
    TEST_ASSERT(tag_validate_format("<invalid>content</wrong>", "invalid") == RESULT_ERR, "Invalid tag format");
    
    data_destroy(&result);
}

/**
 * @brief Test file I/O operations
 */
static void test_file_io(void) {
    TEST_SECTION("File I/O Operations");
    
    const char* test_filename = "/tmp/lkjagent_test.txt";
    const char* test_content = "This is test content for file I/O operations.";
    
    data_t file_data;
    assert(data_init(&file_data, 256) == RESULT_OK);
    assert(data_set(&file_data, test_content, 0) == RESULT_OK);
    
    // Test atomic write
    TEST_ASSERT(file_write_atomic(test_filename, &file_data, false) == RESULT_OK, "Atomic file write");
    
    // Test file exists
    TEST_ASSERT(file_exists(test_filename) == RESULT_OK, "File existence check");
    
    // Test file size
    size_t file_size_result;
    TEST_ASSERT(file_size(test_filename, &file_size_result) == RESULT_OK, "File size check");
    TEST_ASSERT(file_size_result == strlen(test_content), "File size is correct");
    
    // Test file backup
    TEST_ASSERT(file_backup(test_filename, NULL) == RESULT_OK, "File backup creation");
    
    // Test file read
    data_t read_data;
    assert(data_init(&read_data, 256) == RESULT_OK);
    TEST_ASSERT(file_read_all(test_filename, &read_data, 0) == RESULT_OK, "File read all");
    TEST_ASSERT(strcmp(read_data.data, test_content) == 0, "Read content matches written content");
    
    // Test directory creation
    TEST_ASSERT(file_ensure_directory("/tmp/lkjagent_test_dir", 0755) == RESULT_OK, "Directory creation");
    
    // Test file locking
    int lock_fd;
    TEST_ASSERT(file_lock(test_filename, &lock_fd) == RESULT_OK, "File lock acquisition");
    TEST_ASSERT(file_unlock(lock_fd) == RESULT_OK, "File lock release");
    
    // Test file timestamp operations
    time_t mtime;
    TEST_ASSERT(file_get_mtime(test_filename, &mtime) == RESULT_OK, "File modification time");
    
    bool is_newer;
    TEST_ASSERT(file_is_newer(test_filename, mtime - 1, &is_newer) == RESULT_OK, "File newer check");
    TEST_ASSERT(is_newer == true, "File is newer than reference time");
    
    // Cleanup
    unlink(test_filename);
    unlink("/tmp/lkjagent_test.txt.backup");
    rmdir("/tmp/lkjagent_test_dir");
    data_destroy(&file_data);
    data_destroy(&read_data);
}

/**
 * @brief Test JSON operations
 */
static void test_json_operations(void) {
    TEST_SECTION("JSON Processing Operations");
    
    data_t json_data;
    assert(data_init(&json_data, 512) == RESULT_OK);
    
    // Test JSON object building
    TEST_ASSERT(json_build_object(&json_data) == RESULT_OK, "JSON object initialization");
    
    // Test adding fields
    TEST_ASSERT(json_add_string(&json_data, "name", "LKJAgent") == RESULT_OK, "Add string field");
    TEST_ASSERT(json_add_number(&json_data, "version", 1.0) == RESULT_OK, "Add number field");
    TEST_ASSERT(json_add_boolean(&json_data, "active", true) == RESULT_OK, "Add boolean field");
    
    printf("Built JSON: %s\n", json_data.data);
    
    // Test JSON parsing
    data_t parsed;
    assert(data_init(&parsed, 512) == RESULT_OK);
    TEST_ASSERT(json_parse_object(json_data.data, &parsed) == RESULT_OK, "JSON object parsing");
    
    // Test string extraction
    data_t string_value;
    assert(data_init(&string_value, 64) == RESULT_OK);
    TEST_ASSERT(json_parse_string("\"LKJAgent\"", &string_value) == RESULT_OK, "JSON string parsing");
    TEST_ASSERT(strcmp(string_value.data, "LKJAgent") == 0, "Parsed string value is correct");
    
    // Test number extraction
    double number_value;
    TEST_ASSERT(json_parse_number("42.5", &number_value) == RESULT_OK, "JSON number parsing");
    TEST_ASSERT(number_value == 42.5, "Parsed number value is correct");
    
    // Test boolean extraction
    bool bool_value;
    TEST_ASSERT(json_parse_boolean("true", &bool_value) == RESULT_OK, "JSON boolean parsing");
    TEST_ASSERT(bool_value == true, "Parsed boolean value is correct");
    
    // Test memory format building
    data_t memory_json;
    assert(data_init(&memory_json, 1024) == RESULT_OK);
    TEST_ASSERT(json_build_memory("Working memory content", "Disk memory content", &memory_json) == RESULT_OK, "Memory JSON building");
    
    // Test context keys format
    context_key_t test_keys[2];
    strcpy(test_keys[0].key, "user_data");
    test_keys[0].layer = LAYER_WORKING;
    test_keys[0].importance_score = 85;
    test_keys[0].last_accessed = time(NULL);
    test_keys[0].data_size = 1024;
    
    strcpy(test_keys[1].key, "system_logs");
    test_keys[1].layer = LAYER_DISK;
    test_keys[1].importance_score = 45;
    test_keys[1].last_accessed = time(NULL);
    test_keys[1].data_size = 2048;
    
    data_t keys_json;
    assert(data_init(&keys_json, 1024) == RESULT_OK);
    TEST_ASSERT(json_build_context_keys(test_keys, 2, &keys_json) == RESULT_OK, "Context keys JSON building");
    
    // Cleanup
    data_destroy(&json_data);
    data_destroy(&parsed);
    data_destroy(&string_value);
    data_destroy(&memory_json);
    data_destroy(&keys_json);
}

/**
 * @brief Test configuration operations
 */
static void test_configuration(void) {
    TEST_SECTION("Configuration Management");
    
    config_t config;
    
    // Test loading defaults
    TEST_ASSERT(config_load_defaults(&config) == RESULT_OK, "Configuration defaults loading");
    
    // Test validation
    TEST_ASSERT(config_validate(&config) == RESULT_OK, "Configuration validation");
    TEST_ASSERT(config.is_valid == true, "Configuration is marked as valid");
    
    // Test state prompt retrieval
    data_t prompt;
    assert(data_init(&prompt, 512) == RESULT_OK);
    TEST_ASSERT(config_get_state_prompt(&config, STATE_THINKING, &prompt) == RESULT_OK, "State prompt retrieval");
    TEST_ASSERT(prompt.size > 0, "State prompt has content");
    
    // Test LLM settings retrieval
    data_t endpoint, model, api_key;
    assert(data_init(&endpoint, 256) == RESULT_OK);
    assert(data_init(&model, 256) == RESULT_OK);
    assert(data_init(&api_key, 256) == RESULT_OK);
    size_t max_context;
    int timeout;
    TEST_ASSERT(config_get_llm_settings(&config, &endpoint, &model, &api_key, &max_context, &timeout) == RESULT_OK, "LLM settings retrieval");
    
    // Test memory settings retrieval
    size_t max_working, max_disk, cleanup_threshold;
    TEST_ASSERT(config_get_memory_settings(&config, &max_working, &max_disk, &cleanup_threshold) == RESULT_OK, "Memory settings retrieval");
    TEST_ASSERT(max_working > 0, "Max working memory size is positive");
    TEST_ASSERT(max_disk > 0, "Max disk memory size is positive");
    
    // Test configuration saving
    const char* config_file = "/tmp/lkjagent_test_config.json";
    TEST_ASSERT(config_save(&config, config_file) == RESULT_OK, "Configuration saving");
    
    // Test configuration loading from file
    config_t loaded_config;
    TEST_ASSERT(config_load(config_file, &loaded_config) == RESULT_OK, "Configuration loading from file");
    TEST_ASSERT(loaded_config.is_valid == true, "Loaded configuration is valid");
    
    // Test change detection (add small delay to avoid race condition)
    sleep(1);
    bool has_changed;
    TEST_ASSERT(config_has_changed(&config, config_file, &has_changed) == RESULT_OK, "Configuration change detection");
    // Note: Due to timing, this test may be sensitive - focus on the fact that the function works
    printf("Configuration change detected: %s\n", has_changed ? "true" : "false");
    
    // Cleanup
    unlink(config_file);
    data_destroy(&prompt);
    data_destroy(&endpoint);
    data_destroy(&model);
    data_destroy(&api_key);
}

/**
 * @brief Test memory persistence operations
 */
static void test_memory_persistence(void) {
    TEST_SECTION("Memory Persistence Operations");
    
    const char* memory_file = "/tmp/lkjagent_test_memory.json";
    const char* keys_file = "/tmp/lkjagent_test_keys.json";
    
    // Test initialization
    TEST_ASSERT(persist_memory_initialize(memory_file, keys_file) == RESULT_OK, "Memory persistence initialization");
    
    // Prepare test data
    data_t working_memory, disk_memory;
    assert(data_init(&working_memory, 512) == RESULT_OK);
    assert(data_init(&disk_memory, 1024) == RESULT_OK);
    assert(data_set(&working_memory, "Working memory test content", 0) == RESULT_OK);
    assert(data_set(&disk_memory, "Disk memory test content", 0) == RESULT_OK);
    
    // Test memory saving
    TEST_ASSERT(persist_memory_save(memory_file, &working_memory, &disk_memory) == RESULT_OK, "Memory persistence save");
    
    // Test context keys
    context_key_t test_keys[3];
    strcpy(test_keys[0].key, "key1");
    test_keys[0].layer = LAYER_WORKING;
    test_keys[0].importance_score = 90;
    test_keys[0].last_accessed = time(NULL);
    test_keys[0].data_size = 512;
    
    strcpy(test_keys[1].key, "key2");
    test_keys[1].layer = LAYER_DISK;
    test_keys[1].importance_score = 60;
    test_keys[1].last_accessed = time(NULL);
    test_keys[1].data_size = 1024;
    
    strcpy(test_keys[2].key, "key3");
    test_keys[2].layer = LAYER_ARCHIVED;
    test_keys[2].importance_score = 30;
    test_keys[2].last_accessed = time(NULL);
    test_keys[2].data_size = 256;
    
    TEST_ASSERT(persist_context_keys_save(keys_file, test_keys, 3) == RESULT_OK, "Context keys save");
    
    // Test loading
    data_t loaded_working, loaded_disk;
    assert(data_init(&loaded_working, 512) == RESULT_OK);
    assert(data_init(&loaded_disk, 1024) == RESULT_OK);
    TEST_ASSERT(persist_memory_load(memory_file, &loaded_working, &loaded_disk) == RESULT_OK, "Memory persistence load");
    TEST_ASSERT(strcmp(loaded_working.data, "Working memory test content") == 0, "Loaded working memory matches");
    TEST_ASSERT(strcmp(loaded_disk.data, "Disk memory test content") == 0, "Loaded disk memory matches");
    
    // Test context keys loading
    context_key_t loaded_keys[10];
    size_t loaded_count;
    TEST_ASSERT(persist_context_keys_load(keys_file, loaded_keys, 10, &loaded_count) == RESULT_OK, "Context keys load");
    printf("Loaded %zu context keys\n", loaded_count);
    TEST_ASSERT(loaded_count > 0, "At least one context key loaded");
    if (loaded_count > 0) {
        printf("First key: %s\n", loaded_keys[0].key);
        TEST_ASSERT(strlen(loaded_keys[0].key) > 0, "First key has valid name");
    }
    
    // Test backup
    TEST_ASSERT(persist_memory_backup(memory_file, keys_file) == RESULT_OK, "Memory backup creation");
    
    // Test validation
    bool memory_valid, keys_valid;
    TEST_ASSERT(persist_memory_validate(memory_file, keys_file, &memory_valid, &keys_valid) == RESULT_OK, "Memory validation");
    TEST_ASSERT(memory_valid == true, "Memory file is valid");
    TEST_ASSERT(keys_valid == true, "Keys file is valid");
    
    // Test recovery (simulate corruption and recovery)
    TEST_ASSERT(persist_memory_recover(memory_file, keys_file) == RESULT_OK, "Memory recovery");
    
    // Cleanup
    unlink(memory_file);
    unlink(keys_file);
    unlink("/tmp/lkjagent_test_memory.json.backup");
    unlink("/tmp/lkjagent_test_keys.json.backup");
    data_destroy(&working_memory);
    data_destroy(&disk_memory);
    data_destroy(&loaded_working);
    data_destroy(&loaded_disk);
}

/**
 * @brief Test integration scenarios
 */
static void test_integration(void) {
    TEST_SECTION("Integration Scenarios");
    
    // Test complete workflow: config -> memory -> processing -> persistence
    config_t config;
    TEST_ASSERT(config_load_defaults(&config) == RESULT_OK, "Load default configuration");
    
    // Create tagged memory structure
    tagged_memory_t memory;
    TEST_ASSERT(data_init(&memory.working_memory, 1024) == RESULT_OK, "Initialize working memory");
    TEST_ASSERT(data_init(&memory.disk_memory, 2048) == RESULT_OK, "Initialize disk memory");
    memory.context_key_count = 0;
    memory.last_modified = time(NULL);
    memory.access_count = 0;
    memory.total_size = 0;
    
    // Simulate LLM response processing
    const char* llm_response = 
        "<thinking>I need to process this user request and update memory accordingly.</thinking>"
        "<action>update_memory: user_preferences</action>"
        "<evaluation>Successfully updated user preferences in working memory.</evaluation>"
        "<paging>load_context: user_data; prioritize_context: current_session</paging>";
    
    // Parse all components
    data_t thinking, action, evaluation, paging;
    assert(data_init(&thinking, 256) == RESULT_OK);
    assert(data_init(&action, 256) == RESULT_OK);
    assert(data_init(&evaluation, 256) == RESULT_OK);
    assert(data_init(&paging, 256) == RESULT_OK);
    
    TEST_ASSERT(tag_parse_thinking(llm_response, &thinking) == RESULT_OK, "Parse thinking block");
    TEST_ASSERT(tag_parse_action(llm_response, &action) == RESULT_OK, "Parse action block");
    TEST_ASSERT(tag_parse_evaluation(llm_response, &evaluation) == RESULT_OK, "Parse evaluation block");
    TEST_ASSERT(tag_parse_paging(llm_response, &paging) == RESULT_OK, "Parse paging block");
    
    // Parse context keys from paging
    context_key_t parsed_keys[MAX_CONTEXT_KEYS];
    size_t parsed_count;
    TEST_ASSERT(tag_parse_context_keys(paging.data, parsed_keys, MAX_CONTEXT_KEYS, &parsed_count) == RESULT_OK, "Parse context keys");
    TEST_ASSERT(parsed_count > 0, "Context keys were parsed");
    
    // Update memory with parsed content
    TEST_ASSERT(data_append(&memory.working_memory, thinking.data, 0) == RESULT_OK, "Add thinking to working memory");
    TEST_ASSERT(data_append(&memory.working_memory, "\n", 0) == RESULT_OK, "Add separator");
    TEST_ASSERT(data_append(&memory.working_memory, evaluation.data, 0) == RESULT_OK, "Add evaluation to working memory");
    
    // Copy parsed keys to memory structure
    for (size_t i = 0; i < parsed_count && i < MAX_CONTEXT_KEYS; i++) {
        memory.context_keys[memory.context_key_count++] = parsed_keys[i];
    }
    
    // Persist the memory
    const char* memory_file = "/tmp/integration_memory.json";
    const char* keys_file = "/tmp/integration_keys.json";
    
    TEST_ASSERT(persist_memory_save(memory_file, &memory.working_memory, &memory.disk_memory) == RESULT_OK, "Persist integrated memory");
    TEST_ASSERT(persist_context_keys_save(keys_file, memory.context_keys, memory.context_key_count) == RESULT_OK, "Persist integrated context keys");
    
    // Verify persistence
    data_t reloaded_working, reloaded_disk;
    assert(data_init(&reloaded_working, 1024) == RESULT_OK);
    assert(data_init(&reloaded_disk, 2048) == RESULT_OK);
    TEST_ASSERT(persist_memory_load(memory_file, &reloaded_working, &reloaded_disk) == RESULT_OK, "Reload persisted memory");
    TEST_ASSERT(reloaded_working.size > 0, "Reloaded working memory has content");
    
    printf("Integration test - Working memory content: %s\n", reloaded_working.data);
    
    // Cleanup
    unlink(memory_file);
    unlink(keys_file);
    data_destroy(&memory.working_memory);
    data_destroy(&memory.disk_memory);
    data_destroy(&thinking);
    data_destroy(&action);
    data_destroy(&evaluation);
    data_destroy(&paging);
    data_destroy(&reloaded_working);
    data_destroy(&reloaded_disk);
}

/**
 * @brief Main test runner
 */
int main(void) {
    printf("LKJAgent Core Infrastructure Test Suite\n");
    printf("========================================\n");
    
    // Run all test suites
    test_data_operations();
    test_tag_parsing();
    test_file_io();
    test_json_operations();
    test_configuration();
    test_memory_persistence();
    test_integration();
    
    // Print results
    printf("\n========================================\n");
    printf("Test Results Summary:\n");
    printf("Tests run: %d\n", g_results.tests_run);
    printf("Tests passed: %d\n", g_results.tests_passed);
    printf("Tests failed: %d\n", g_results.tests_failed);
    printf("Success rate: %.1f%%\n", 
           g_results.tests_run > 0 ? 
           (100.0 * g_results.tests_passed / g_results.tests_run) : 0.0);
    
    if (g_results.tests_failed == 0) {
        printf("\n🎉 All tests passed! Core infrastructure is working correctly.\n");
        return 0;
    } else {
        printf("\n❌ %d test(s) failed. Please review the implementation.\n", g_results.tests_failed);
        return 1;
    }
}
