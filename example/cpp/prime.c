#include <stdio.h>
int main()
{
    int i, j, n = 100000;
    for (i = 2; i < n; i++) {
        for (j = 2; j * j <= i; j++) {
            if (i % j == 0) break;
        }
        if (j * j > i) printf("%d\n", i);
    }
}
