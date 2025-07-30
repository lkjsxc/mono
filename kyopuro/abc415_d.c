#include <stdint.h>
#include <stdio.h>

typedef struct {
    int64_t a;
    int64_t b;
    int64_t c;
} data_t;

data_t data[200000];
data_t merge_tmp[200000];

void merge(data_t* arr, int64_t left, int64_t mid, int64_t right) {
    int64_t i = left;
    int64_t j = mid + 1;
    int64_t k = left;

    for (int64_t l = left; l <= right; l++) {
        merge_tmp[l] = arr[l];
    }

    while (i <= mid && j <= right) {
        if (merge_tmp[i].c <= merge_tmp[j].c) {
            arr[k++] = merge_tmp[i++];
        } else {
            arr[k++] = merge_tmp[j++];
        }
    }

    while (i <= mid) {
        arr[k++] = merge_tmp[i++];
    }
}

void merge_sort(data_t* arr, int64_t left, int64_t right) {
    if (left < right) {
        int64_t mid = left + (right - left) / 2;
        merge_sort(arr, left, mid);
        merge_sort(arr, mid + 1, right);
        merge(arr, left, mid, right);
    }
}

int main() {
    int64_t n;
    int64_t m;

    scanf("%lld %lld", &n, &m);

    for (int64_t i = 0; i < m; i++) {
        scanf("%lld %lld", &data[i].a, &data[i].b);
        data[i].c = data[i].a - data[i].b;
    }

    merge_sort(data, 0, m - 1);

    int64_t x = n;
    int64_t ans = 0;

    for (int64_t i = 0; i < m; i++) {
        int64_t a = data[i].a;
        int64_t b = data[i].b;
        int64_t c = data[i].c;
        if (x >= a) {
            int64_t num_exchanges = (x - a) / c + 1;
            ans += num_exchanges;
            x -= num_exchanges * c;
        }
    }

    printf("%lld\n", ans);

    return 0;
}