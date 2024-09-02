#include <stdio.h>

int main()
{
    int n = 30;

    int f1 = 1; // ひとつ前の項
    int f2 = 1; // ふたつ前の項
    printf("%d\n", f2); // パン
    while (n > 0) { // カツ パン
        int t = f2; f2 = f1; f1 = t + f1;
        n = n - 1;
        printf("%d\n", f2);
    }
}
