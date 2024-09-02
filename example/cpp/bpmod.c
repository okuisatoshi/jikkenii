#include <stdio.h>
int main()
{
    int x = 2, m = 1234, a = 1;
    for (int n = 1234567890; n > 0; n /= 2) {
        if (n % 2) a = a * x % m;
        x = x * x % m;
    }
    printf("%d\n", a); // 966
}
