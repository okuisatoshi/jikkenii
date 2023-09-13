/*
 * Pico言語コンパイラ（ステップ1）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scan.h"

int token;             // 最後に読んだトークン
char name[MAX_TK_LEN]; // ...の名前
int vsuffix = 1;       // 値の一時名の接尾番号
#define MAX_TMP_LEN 16 // 値の一時名の最大長


void expect(int expected)
{
    if (token == expected) {
        token = get_next_token(name);
        return;
    }
    fprintf(stderr, "期待されない字句(行%d:%d): %s\n", row, col, name);
    exit(1);
}


void num(char *ret)
{
    strcpy(ret, name);
    expect(TK_NUM);
}


void expr(char *left)
{
    num(left);
    while (token == '+' || token == '-' || token == '<') {
        int op = token;
        expect(op);
        char right[MAX_TMP_LEN];
        num(right);
        int v = vsuffix++;
        const char *f = op == '+' ? "add" : op == '-' ? "sub" : "icmp slt";
        printf("  %%t.%d = %s i32 %s, %s\n", v, f, left, right);
        if (op == '<') {
            int u = vsuffix++;
            printf("  %%t.%d = zext i1 %%t.%d to i32\n", u, v);
            v = u;
        }
        sprintf(left, "%%t.%d", v);
    }
}


void print_st(void)
{
    expect(TK_PRINT);
    char e[MAX_TMP_LEN];
    expr(e);
    int v = vsuffix++;
    printf("  %%t.%d = getelementptr [4 x i8], ptr @fmt_pd, i32 0, i32 0\n", v);
    printf("  %%t.%d = call i32 (ptr, ...) @printf(ptr %%t.%d, i32 %s)\n", vsuffix++, v, e);
    expect(';');
}


void program(void)
{
    printf("@fmt_pd = private constant [4 x i8] c\"%%d\\0A\\00\"\n");
    printf("declare i32 @printf(ptr, ...)\n\n");

    expect(TK_INT);
    if (strcmp(name, "main") != 0) {
        fprintf(stderr, "main関数が必要(行%d:%d): %s\n", row, col, name);
        exit(1);
    }
    expect(TK_ID);
    expect('(');
    expect(')');
    printf("define i32 @main() {\n");
    expect('{');
    print_st();
    expect('}');
    printf("  ret i32 0\n");
    printf("}\n");
}


int main()
{
    token = get_next_token(name);
    program();
}
