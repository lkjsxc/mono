#include "lkjagent.h"
#include "token.c"
#include "http.c"

int main() {
    static char method_data[16];
    static char url_data[256];
    static char body_data[1024];
    static char response_data[4096];

    token_t method = {method_data, 0, sizeof(method_data)};
    token_t url = {url_data, 0, sizeof(url_data)};
    token_t body = {body_data, 0, sizeof(body_data)};
    token_t response = {response_data, 0, sizeof(response_data)};

    method.size = strlen("GET");
    strcpy(method.data, "GET");

    url.size = strlen("http://httpbin.org/get");
    strcpy(url.data, "http://httpbin.org/get");

    if (http_request(&method, &url, &body, &response) == RESULT_ERR) {
        printf("Test failed: expected RESULT_ERR for NULL parameters\n");
        return 1;
    }

    for (int i = 0; i < response.size; i++) {
        printf("%c", response.data[i]);
    }

    return 0;
}