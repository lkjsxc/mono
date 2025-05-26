#include <stdio.h>
#include <string.h>

char s_data[500002];

int main() {
    char* s = s_data + 1;
    s_data[0] = '\0';
    int scan_result = scanf("%s", s);
    int s_len = strlen(s);
    int ans = 0;
    int sum = 0;
    for (int i = s_len - 1; s[i] != '\0'; i--) {
        while (1) {
            int digit = s[i] - '0';
            if ((10 + digit - (sum % 10)) % 10 == 0) {
                break;
            }
            sum++;
            ans++;
        }
        ans++;
    }
    printf("%d\n", ans);
}