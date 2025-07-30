#include <stdio.h>

typedef struct {
    int x;
    int y;
} stat_t;

stat_t stat[100];

int main() {
    int n, l, r;

    scanf("%d %d %d", &n, &l, &r);
    for (int i = 0; i < n; i++) {
        scanf("%d %d", &stat[i].x, &stat[i].y);
    }

    int ans = 0;
    for (int i = 0; i < n; i++) {
        if (stat[i].x <= l && r <= stat[i].y) {
            ans++;
        }
    }

    printf("%d\n", ans);
    return 0;
}