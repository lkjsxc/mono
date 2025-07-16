#include "../src/lkjagent.h"
#include "../src/token.c"
#include "../src/http.c"

int main() {
    static char method_data[16];
    static char url_data[256];
    static char body_data[1024];
    static char response_data[4096];

    token_t method, url, body, response;

    // Initialize tokens with their respective buffers
    if (token_init(&method, method_data, sizeof(method_data)) != RESULT_OK ||
        token_init(&url, url_data, sizeof(url_data)) != RESULT_OK ||
        token_init(&body, body_data, sizeof(body_data)) != RESULT_OK ||
        token_init(&response, response_data, sizeof(response_data)) != RESULT_OK) {
        printf("Failed to initialize tokens\n");
        return 1;
    }

    // Test POST request with JSON body
    if (token_set(&method, "POST") != RESULT_OK) {
        printf("Failed to set method\n");
        return 1;
    }

    if (token_set(&url, "http://httpbin.org/post") != RESULT_OK) {
        printf("Failed to set URL\n");
        return 1;
    }

    // Set JSON body
    if (token_set(&body, "{\"name\":\"lkjagent\",\"version\":\"1.0\"}") != RESULT_OK) {
        printf("Failed to set body\n");
        return 1;
    }

    // Make HTTP request
    result_t result = http_request(&method, &url, &body, &response);
    if (result != RESULT_OK) {
        printf("HTTP request failed\n");
        return 1;
    }

    // Print response
    printf("POST Response received (%zu bytes):\n", response.size);
    for (size_t i = 0; i < response.size; i++) {
        printf("%c", response.data[i]);
    }
    printf("\n");

    return 0;
}
