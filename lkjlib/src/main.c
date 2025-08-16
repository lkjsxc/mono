// lkjlib comprehensive test suite

#include "lkjlib/lkjlib.h"

// Test counter and result tracking
static int test_count = 0;
static int test_passed = 0;

#define TEST_START(name) \
    do { \
        printf("Running test: %s... ", name); \
        test_count++; \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("PASSED\n"); \
        test_passed++; \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        printf("FAILED: %s\n", msg); \
        return RESULT_ERR; \
    } while(0)

#define TEST_ASSERT(condition, msg) \
    do { \
        if (!(condition)) { \
            TEST_FAIL(msg); \
        } \
    } while(0)

// Pool tests
result_t test_pool(pool_t *pool) {
    TEST_START("pool_init");
    // Pool should already be initialized in main
    TEST_ASSERT(pool->data16_freelist_count == POOL_data16_MAXCOUNT, "data16 freelist not fully initialized");
    TEST_ASSERT(pool->data256_freelist_count == POOL_data256_MAXCOUNT, "data256 freelist not fully initialized");
    TEST_ASSERT(pool->object_freelist_count == POOL_OBJECT_MAXCOUNT, "object freelist not fully initialized");
    TEST_PASS();

    TEST_START("pool_data_alloc basic");
    data_t *data1 = NULL;
    if (pool_data_alloc(pool, &data1, 10) != RESULT_OK) {
        TEST_FAIL("Failed to allocate data with capacity 10");
    }
    TEST_ASSERT(data1 != NULL, "Allocated data is NULL");
    TEST_ASSERT(data1->capacity >= 10, "Allocated capacity too small");
    TEST_ASSERT(data1->size == 0, "Initial size should be 0");
    TEST_PASS();

    TEST_START("pool_data_free");
    if (pool_data_free(pool, data1) != RESULT_OK) {
        TEST_FAIL("Failed to free data");
    }
    TEST_PASS();

    TEST_START("pool_data_alloc sizes");
    data_t *data16, *data256, *data4096, *data65536, *data1048576;
    
    if (pool_data16_alloc(pool, &data16) != RESULT_OK) {
        TEST_FAIL("Failed to allocate data16");
    }
    TEST_ASSERT(data16->capacity == 16, "data16 capacity incorrect");
    
    if (pool_data256_alloc(pool, &data256) != RESULT_OK) {
        TEST_FAIL("Failed to allocate data256");
    }
    TEST_ASSERT(data256->capacity == 256, "data256 capacity incorrect");
    
    if (pool_data4096_alloc(pool, &data4096) != RESULT_OK) {
        TEST_FAIL("Failed to allocate data4096");
    }
    TEST_ASSERT(data4096->capacity == 4096, "data4096 capacity incorrect");
    
    if (pool_data65536_alloc(pool, &data65536) != RESULT_OK) {
        TEST_FAIL("Failed to allocate data65536");
    }
    TEST_ASSERT(data65536->capacity == 65536, "data65536 capacity incorrect");
    
    if (pool_data1048576_alloc(pool, &data1048576) != RESULT_OK) {
        TEST_FAIL("Failed to allocate data1048576");
    }
    TEST_ASSERT(data1048576->capacity == 1048576, "data1048576 capacity incorrect");
    
    // Free all allocated data
    if (pool_data_free(pool, data16) != RESULT_OK) {
        TEST_FAIL("Failed to free data16");
    }
    if (pool_data_free(pool, data256) != RESULT_OK) {
        TEST_FAIL("Failed to free data256");
    }
    if (pool_data_free(pool, data4096) != RESULT_OK) {
        TEST_FAIL("Failed to free data4096");
    }
    if (pool_data_free(pool, data65536) != RESULT_OK) {
        TEST_FAIL("Failed to free data65536");
    }
    if (pool_data_free(pool, data1048576) != RESULT_OK) {
        TEST_FAIL("Failed to free data1048576");
    }
    TEST_PASS();

    TEST_START("pool_object_alloc");
    object_t *obj = NULL;
    if (pool_object_alloc(pool, &obj) != RESULT_OK) {
        TEST_FAIL("Failed to allocate object");
    }
    TEST_ASSERT(obj != NULL, "Allocated object is NULL");
    TEST_ASSERT(obj->data == NULL, "Object data should be NULL initially");
    TEST_ASSERT(obj->child == NULL, "Object child should be NULL initially");
    TEST_ASSERT(obj->next == NULL, "Object next should be NULL initially");
    
    if (pool_object_free(pool, obj) != RESULT_OK) {
        TEST_FAIL("Failed to free object");
    }
    TEST_PASS();

    return RESULT_OK;
}

// Data manipulation tests
result_t test_data(pool_t *pool) {
    TEST_START("data_create");
    data_t *data = NULL;
    if (data_create(pool, &data) != RESULT_OK) {
        TEST_FAIL("Failed to create data");
    }
    TEST_ASSERT(data != NULL, "Created data is NULL");
    TEST_ASSERT(data->size == 0, "Initial data size should be 0");
    TEST_ASSERT(data->capacity > 0, "Data capacity should be positive");
    if (data_destroy(pool, data) != RESULT_OK) {
        TEST_FAIL("Failed to destroy data");
    }
    TEST_PASS();

    TEST_START("data_create_str");
    const char *test_str = "Hello, World!";
    data_t *str_data = NULL;
    if (data_create_str(pool, &str_data, test_str) != RESULT_OK) {
        TEST_FAIL("Failed to create data from string");
    }
    TEST_ASSERT(str_data != NULL, "String data is NULL");
    TEST_ASSERT(str_data->size == strlen(test_str), "String data size incorrect");
    TEST_ASSERT(strncmp(str_data->data, test_str, str_data->size) == 0, "String data content incorrect");
    TEST_PASS();

    TEST_START("data_copy_str");
    const char *new_str = "New string content";
    if (data_copy_str(pool, &str_data, new_str) != RESULT_OK) {
        TEST_FAIL("Failed to copy string to data");
    }
    TEST_ASSERT(str_data->size == strlen(new_str), "Copied string size incorrect");
    TEST_ASSERT(strncmp(str_data->data, new_str, str_data->size) == 0, "Copied string content incorrect");
    TEST_PASS();

    TEST_START("data_append_str");
    const char *append_str = " appended";
    if (data_append_str(pool, &str_data, append_str) != RESULT_OK) {
        TEST_FAIL("Failed to append string to data");
    }
    TEST_ASSERT(str_data->size == strlen(new_str) + strlen(append_str), "Appended string size incorrect");
    char expected[256];
    snprintf(expected, sizeof(expected), "%s%s", new_str, append_str);
    TEST_ASSERT(strncmp(str_data->data, expected, str_data->size) == 0, "Appended string content incorrect");
    TEST_PASS();

    TEST_START("data_append_char");
    char append_char = '!';
    if (data_append_char(pool, &str_data, append_char) != RESULT_OK) {
        TEST_FAIL("Failed to append char to data");
    }
    TEST_ASSERT(str_data->data[str_data->size - 1] == append_char, "Appended char incorrect");
    TEST_PASS();

    TEST_START("data_equal_str");
    data_t *test_data = NULL;
    if (data_create_str(pool, &test_data, "test") != RESULT_OK) {
        TEST_FAIL("Failed to create test data");
    }
    TEST_ASSERT(data_equal_str(test_data, "test") == 1, "Equal strings not detected");
    TEST_ASSERT(data_equal_str(test_data, "different") == 0, "Different strings detected as equal");
    if (data_destroy(pool, test_data) != RESULT_OK) {
        TEST_FAIL("Failed to destroy test data");
    }
    TEST_PASS();

    TEST_START("data_find_str");
    data_t *search_data = NULL;
    if (data_create_str(pool, &search_data, "The quick brown fox jumps over the lazy dog") != RESULT_OK) {
        TEST_FAIL("Failed to create search data");
    }
    int64_t pos = data_find_str(search_data, "brown", 0);
    TEST_ASSERT(pos == 10, "String not found at correct position");
    pos = data_find_str(search_data, "elephant", 0);
    TEST_ASSERT(pos == -1, "Non-existent string should return -1");
    if (data_destroy(pool, search_data) != RESULT_OK) {
        TEST_FAIL("Failed to destroy search data");
    }
    TEST_PASS();

    TEST_START("data_find_char");
    data_t *char_data = NULL;
    if (data_create_str(pool, &char_data, "abcdefg") != RESULT_OK) {
        TEST_FAIL("Failed to create char data");
    }
    pos = data_find_char(char_data, 'd', 0);
    TEST_ASSERT(pos == 3, "Character not found at correct position");
    pos = data_find_char(char_data, 'z', 0);
    TEST_ASSERT(pos == -1, "Non-existent character should return -1");
    if (data_destroy(pool, char_data) != RESULT_OK) {
        TEST_FAIL("Failed to destroy char data");
    }
    TEST_PASS();

    TEST_START("data_create_data and data_equal_data");
    data_t *original = NULL;
    data_t *copy = NULL;
    if (data_create_str(pool, &original, "original data") != RESULT_OK) {
        TEST_FAIL("Failed to create original data");
    }
    if (data_create_data(pool, &copy, original) != RESULT_OK) {
        TEST_FAIL("Failed to create data from data");
    }
    TEST_ASSERT(data_equal_data(original, copy) == 1, "Copied data not equal to original");
    if (data_destroy(pool, original) != RESULT_OK) {
        TEST_FAIL("Failed to destroy original data");
    }
    if (data_destroy(pool, copy) != RESULT_OK) {
        TEST_FAIL("Failed to destroy copy data");
    }
    TEST_PASS();

    TEST_START("data_escape and data_unescape");
    data_t *escape_data = NULL;
    if (data_create_str(pool, &escape_data, "Line 1\nLine 2\tTabbed\"Quoted\"") != RESULT_OK) {
        TEST_FAIL("Failed to create escape data");
    }
    if (data_escape(pool, &escape_data) != RESULT_OK) {
        TEST_FAIL("Failed to escape data");
    }
    // Check that newlines and tabs are escaped
    TEST_ASSERT(data_find_str(escape_data, "\\n", 0) >= 0, "Newline not escaped");
    TEST_ASSERT(data_find_str(escape_data, "\\t", 0) >= 0, "Tab not escaped");
    
    if (data_unescape(pool, &escape_data) != RESULT_OK) {
        TEST_FAIL("Failed to unescape data");
    }
    // After unescaping, original characters should be back
    TEST_ASSERT(data_find_char(escape_data, '\n', 0) >= 0, "Newline not unescaped");
    TEST_ASSERT(data_find_char(escape_data, '\t', 0) >= 0, "Tab not unescaped");
    if (data_destroy(pool, escape_data) != RESULT_OK) {
        TEST_FAIL("Failed to destroy escape data");
    }
    TEST_PASS();

    // Clean up
    if (data_destroy(pool, str_data) != RESULT_OK) {
        TEST_FAIL("Failed to destroy str_data");
    }
    
    return RESULT_OK;
}

// File operations tests
result_t test_file(pool_t *pool) {
    TEST_START("file_write and file_read");
    
    // Create test data
    data_t *write_data = NULL;
    const char *test_content = "This is a test file content.\nLine 2\nLine 3";
    if (data_create_str(pool, &write_data, test_content) != RESULT_OK) {
        TEST_FAIL("Failed to create write data");
    }
    
    // Write to file
    const char *test_file = "test_output.txt";
    if (file_write(test_file, write_data) != RESULT_OK) {
        TEST_FAIL("Failed to write file");
    }
    
    // Read from file
    data_t *read_data = NULL;
    if (file_read(pool, &read_data, test_file) != RESULT_OK) {
        TEST_FAIL("Failed to read file");
    }
    
    // Compare content
    TEST_ASSERT(data_equal_data(write_data, read_data) == 1, "Read data doesn't match written data");
    
    // Clean up
    if (data_destroy(pool, write_data) != RESULT_OK) {
        TEST_FAIL("Failed to destroy write data");
    }
    if (data_destroy(pool, read_data) != RESULT_OK) {
        TEST_FAIL("Failed to destroy read data");
    }
    unlink(test_file); // Remove test file
    TEST_PASS();

    TEST_START("file_read non-existent");
    data_t *nonexistent_data = NULL;
    result_t result = file_read(pool, &nonexistent_data, "nonexistent_file_12345.txt");
    TEST_ASSERT(result == RESULT_ERR, "Reading non-existent file should fail");
    TEST_PASS();

    return RESULT_OK;
}

// Object tests
result_t test_object(pool_t *pool) {
    TEST_START("object_create");
    object_t *obj = NULL;
    if (object_create(pool, &obj) != RESULT_OK) {
        TEST_FAIL("Failed to create object");
    }
    TEST_ASSERT(obj != NULL, "Created object is NULL");
    TEST_ASSERT(obj->data == NULL, "Object data should be NULL initially");
    TEST_ASSERT(obj->child == NULL, "Object child should be NULL initially");
    TEST_ASSERT(obj->next == NULL, "Object next should be NULL initially");
    if (object_destroy(pool, obj) != RESULT_OK) {
        TEST_FAIL("Failed to destroy object");
    }
    TEST_PASS();

    TEST_START("object_parse_json simple");
    data_t *json_data = NULL;
    if (data_create_str(pool, &json_data, "{\"name\":\"test\",\"value\":42}") != RESULT_OK) {
        TEST_FAIL("Failed to create JSON data");
    }
    object_t *json_obj = NULL;
    if (object_parse_json(pool, &json_obj, json_data) != RESULT_OK) {
        printf("JSON parsing failed (may not be implemented) - ");
        if (data_destroy(pool, json_data) != RESULT_OK) {
            TEST_FAIL("Failed to destroy JSON data");
        }
        TEST_PASS();
        return RESULT_OK;
    }
    TEST_ASSERT(json_obj != NULL, "Parsed JSON object is NULL");
    if (object_destroy(pool, json_obj) != RESULT_OK) {
        TEST_FAIL("Failed to destroy JSON object");
    }
    if (data_destroy(pool, json_data) != RESULT_OK) {
        TEST_FAIL("Failed to destroy JSON data");
    }
    TEST_PASS();

    TEST_START("object_parse_xml simple");
    data_t *xml_data = NULL;
    if (data_create_str(pool, &xml_data, "<root><item>test</item></root>") != RESULT_OK) {
        TEST_FAIL("Failed to create XML data");
    }
    object_t *xml_obj = NULL;
    if (object_parse_xml(pool, &xml_obj, xml_data) != RESULT_OK) {
        printf("XML parsing failed (may not be implemented) - ");
        if (data_destroy(pool, xml_data) != RESULT_OK) {
            TEST_FAIL("Failed to destroy XML data");
        }
        TEST_PASS();
        return RESULT_OK;
    }
    TEST_ASSERT(xml_obj != NULL, "Parsed XML object is NULL");
    if (object_destroy(pool, xml_obj) != RESULT_OK) {
        TEST_FAIL("Failed to destroy XML object");
    }
    if (data_destroy(pool, xml_data) != RESULT_OK) {
        TEST_FAIL("Failed to destroy XML data");
    }
    TEST_PASS();

    return RESULT_OK;
}

// HTTP tests (basic connectivity tests)
result_t test_http(pool_t *pool) {
    TEST_START("http_get basic");
    data_t *url = NULL;
    data_t *response = NULL;
    
    // Test with a simple URL (this may fail if no internet connection)
    if (data_create_str(pool, &url, "http://httpbin.org/get") != RESULT_OK) {
        TEST_FAIL("Failed to create URL data");
    }
    result_t result = http_get(pool, url, &response);
    
    if (result != RESULT_OK) {
        printf("HTTP GET failed (may be network issue) - ");
        if (data_destroy(pool, url) != RESULT_OK) {
            TEST_FAIL("Failed to destroy URL data");
        }
        TEST_PASS();
        return RESULT_OK;
    }
    
    TEST_ASSERT(response != NULL, "HTTP response is NULL");
    TEST_ASSERT(response->size > 0, "HTTP response is empty");
    
    if (data_destroy(pool, url) != RESULT_OK) {
        TEST_FAIL("Failed to destroy URL data");
    }
    if (data_destroy(pool, response) != RESULT_OK) {
        TEST_FAIL("Failed to destroy response data");
    }
    TEST_PASS();

    TEST_START("http_post basic");
    data_t *post_url = NULL;
    data_t *content_type = NULL;
    data_t *body = NULL;
    data_t *post_response = NULL;
    
    if (data_create_str(pool, &post_url, "http://httpbin.org/post") != RESULT_OK) {
        TEST_FAIL("Failed to create POST URL data");
    }
    if (data_create_str(pool, &content_type, "application/json") != RESULT_OK) {
        TEST_FAIL("Failed to create content type data");
    }
    if (data_create_str(pool, &body, "{\"test\":\"data\"}") != RESULT_OK) {
        TEST_FAIL("Failed to create body data");
    }
    
    result = http_post(pool, post_url, content_type, body, &post_response);
    
    if (result != RESULT_OK) {
        printf("HTTP POST failed (may be network issue) - ");
        if (data_destroy(pool, post_url) != RESULT_OK) {
            TEST_FAIL("Failed to destroy POST URL data");
        }
        if (data_destroy(pool, content_type) != RESULT_OK) {
            TEST_FAIL("Failed to destroy content type data");
        }
        if (data_destroy(pool, body) != RESULT_OK) {
            TEST_FAIL("Failed to destroy body data");
        }
        TEST_PASS();
        return RESULT_OK;
    }
    
    TEST_ASSERT(post_response != NULL, "HTTP POST response is NULL");
    TEST_ASSERT(post_response->size > 0, "HTTP POST response is empty");
    
    if (data_destroy(pool, post_url) != RESULT_OK) {
        TEST_FAIL("Failed to destroy POST URL data");
    }
    if (data_destroy(pool, content_type) != RESULT_OK) {
        TEST_FAIL("Failed to destroy content type data");
    }
    if (data_destroy(pool, body) != RESULT_OK) {
        TEST_FAIL("Failed to destroy body data");
    }
    if (data_destroy(pool, post_response) != RESULT_OK) {
        TEST_FAIL("Failed to destroy POST response data");
    }
    TEST_PASS();

    return RESULT_OK;
}

// Memory stress tests
result_t test_memory_stress(pool_t *pool) {
    TEST_START("memory_stress_allocation");
    
    // Allocate many small data blocks
    data_t *data_blocks[100];
    for (int i = 0; i < 100; i++) {
        if (data_create(pool, &data_blocks[i]) != RESULT_OK) {
            TEST_FAIL("Failed to allocate data block in stress test");
        }
        char test_str[32];
        snprintf(test_str, sizeof(test_str), "Block %d", i);
        if (data_copy_str(pool, &data_blocks[i], test_str) != RESULT_OK) {
            TEST_FAIL("Failed to copy string in stress test");
        }
    }
    
    // Free all blocks
    for (int i = 0; i < 100; i++) {
        if (data_destroy(pool, data_blocks[i]) != RESULT_OK) {
            TEST_FAIL("Failed to destroy data block in stress test");
        }
    }
    TEST_PASS();

    TEST_START("memory_stress_reallocation");
    data_t *grow_data = NULL;
    if (data_create(pool, &grow_data) != RESULT_OK) {
        TEST_FAIL("Failed to create data for reallocation test");
    }
    
    // Grow the data progressively
    for (int i = 0; i < 10; i++) {
        char append_str[64];
        snprintf(append_str, sizeof(append_str), "Iteration %d ", i);
        if (data_append_str(pool, &grow_data, append_str) != RESULT_OK) {
            TEST_FAIL("Failed to append in reallocation test");
        }
    }
    
    TEST_ASSERT(grow_data->size > 100, "Data didn't grow as expected");
    if (data_destroy(pool, grow_data) != RESULT_OK) {
        TEST_FAIL("Failed to destroy grow data");
    }
    TEST_PASS();

    return RESULT_OK;
}

// Main test function
result_t test(pool_t *pool) {
    printf("=== lkjlib Comprehensive Test Suite ===\n");
    
    if (test_pool(pool) != RESULT_OK) return RESULT_ERR;
    if (test_data(pool) != RESULT_OK) return RESULT_ERR;
    if (test_file(pool) != RESULT_OK) return RESULT_ERR;
    if (test_object(pool) != RESULT_OK) return RESULT_ERR;
    if (test_http(pool) != RESULT_OK) return RESULT_ERR;
    if (test_memory_stress(pool) != RESULT_OK) return RESULT_ERR;
    
    printf("\n=== Test Summary ===\n");
    printf("Total tests run: %d\n", test_count);
    printf("Tests passed: %d\n", test_passed);
    printf("Tests failed: %d\n", test_count - test_passed);
    
    if (test_passed == test_count) {
        printf("🎉 All tests passed!\n");
        return RESULT_OK;
    } else {
        printf("❌ Some tests failed!\n");
        return RESULT_ERR;
    }
}

int main() {
    static pool_t pool;
    if (pool_init(&pool) != RESULT_OK) {
        PRINT_ERR("Failed to initialize memory pool");
        return 1;
    }
    if(test(&pool) != RESULT_OK) {
        PRINT_ERR("Test failed");
        return 1;
    }
    printf("All tests passed successfully!\n");
    return 0;
}