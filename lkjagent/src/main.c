#include "file.c"
#include "http.c"
#include "lkjagent.h"
#include "token.c"

int main() {
    static char method_data[16];
    static char url_data[256];
    static char body_data[1024];
    static char response_data[4096];

    token_t method;
    token_t url;
    token_t body;
    token_t response;

    // Initialize tokens with their respective buffers
    if (token_init(&method, method_data, sizeof(method_data)) != RESULT_OK ||
        token_init(&url, url_data, sizeof(url_data)) != RESULT_OK ||
        token_init(&body, body_data, sizeof(body_data)) != RESULT_OK ||
        token_init(&response, response_data, sizeof(response_data)) != RESULT_OK) {
        printf("Failed to initialize tokens\n");
        return 1;
    }

    // Set method and URL for LMStudio
    if (token_set(&method, "POST") != RESULT_OK) {
        printf("Failed to set method\n");
        return 1;
    }

    if (token_set(&url, "http://host.docker.internal:1234/v1/chat/completions") != RESULT_OK) {
        printf("Failed to set URL\n");
        return 1;
    }

    if (file_read("./data/system.txt", &body) != RESULT_OK) {
        printf("Failed to read body from file\n");
        return 1;
    }

    // Make HTTP request to LMStudio
    if (http_request(&method, &url, &body, &response) != RESULT_OK) {
        printf("HTTP request failed\n");
        printf("\nTroubleshooting:\n");
        printf("1. Make sure LMStudio is running\n");
        printf("2. Load a model in LMStudio\n");
        printf("3. Start the local server (usually on port 1234)\n");
        printf("4. Check that the server is accessible at http://127.0.0.1:1234\n");
        return 1;
    }

    // Print response
    printf("LMStudio Response received (%zu bytes):\n", response.size);
    printf("%s\n", response.data);

    return 0;
}