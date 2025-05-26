#include <stdio.h>
#include <string.h>

char s[100002];

int main() {
    scanf("%s", s);
    int s_len = strlen(s);
    int ans = 0;
    int sum = 0;
    for (int i = s_len - 1; i >= 0; i--) {
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