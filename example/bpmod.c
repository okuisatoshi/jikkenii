#include <stdio.h>

int main()
{
    int x; int n; int m;
    x = 2; n = 1234567890; m = 1234;

    int a;
    a = 1;
    while (0 < n) {
        int q; int r; int c; int t;
        // --- q = n / 2, r = n % 2 の計算
        q = 0; r = n;
        while (r < 2 < 1) { // r >= 2
            q = q + 1;
            r = r - 2;
        }
        c = r;
        while (c) { // if (n % 2)のシミュレート
            // --- a = (a * x) % m の計算
            r = 0; t = x;
            while (t) { r = r + a; t = t - 1; }
            // r == a * x
            while (r < m < 1) r = r - m;
            // r == a * x % m
            a = r;
            c = 0;
        }
        // --- x = (x * x) % m の計算
        r = 0; t = x;
        while (t) { r = r + x; t = t - 1; }
        // r == x * x
        while (r < m < 1) r = r - m;
        // r == x * x % m
        x = r;

        // --- n = n / 2 の計算
        n = q;
    }
    printf("%d\n", a); // 966
}
