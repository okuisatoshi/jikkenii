#include <stdio.h>

int main()
{
    int a = 1071, b = 1029;
    while (b > 0) {
        int t = a;
        a = b;
        b = t % b;
    }
    printf("%d\n", a); // 21
}
