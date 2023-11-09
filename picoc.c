/*
 * Pico言語コンパイラ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scan.h"
#include "hashmap.h"

int token;             // 最後に読んだトークン
char name[MAX_TK_LEN]; // ...の名前
int vsuffix = 1;       // 値の一時名の接尾番号
int lsuffix = 1;       // ラベルの接尾番号
#define MAX_TMP_LEN 16 // 値の一時名の最大長
hashmap *hmap;         // 変数名と行番号の対を保存するハッシュ表

// デバッグモードを有効にするには以下の行
//#define DEBUG
//をコメントアウトしてmake clean; makeする。あるいは
// make clean; CFLAGS=-DDEBUG makeのようにしてコンパイルする
// デバッグモードでは標準エラー出力に構文解析木が出力される
int findent = 0;
#ifdef DEBUG
#define ENTER(f) { findent += 3; fprintf(stderr, "%*c|- %s\n", findent, ' ', f); }
#define LEAVE() findent -= 3
#define FEED() fprintf(stderr, "%*c   \'\e[7m%s\e[0m\' \e[2m (行%d:%d)\e[0m\n", findent, ' ', name, row, col)
#else
#define ENTER(f)
#define LEAVE()
#define FEED()
#endif

void statement(void);


void expect(int expected)
{
    if (token == expected) {
        FEED();
        token = get_next_token(name);
        return;
    }
    fprintf(stderr, "期待されない字句(行%d:%d): %s\n", row, col, name);
    exit(1);
}


void id(char *ret)
{
    ENTER("id()");
    char x[MAX_TK_LEN];
    strcpy(x, name);
    int r = row, c = col;
    expect(TK_ID);
    if (hget(hmap, x) < 0) {
        fprintf(stderr, "不明な変数(行%d:%d): %s\n", r, c, x);
        exit(1);
    }
    int v = vsuffix++;
    printf("  %%t.%d = load i32, ptr %%%s\n", v, x);
    sprintf(ret, "%%t.%d", v);
    LEAVE();
}


void num(char *ret)
{
    ENTER("num()");
    strcpy(ret, name);
    expect(TK_NUM);
    LEAVE();
}


void factor(char *ret)
{
    ENTER("factor()");
    switch (token) {
        case TK_ID:
            id(ret);
            break;
        case TK_NUM:
            num(ret);
            break;
        default:
            fprintf(stderr, "期待されない字句(行%d:%d): %s\n", row, col, name);
            exit(1);
    }
    LEAVE();
}


void expr(char *left)
{
    ENTER("expr()");
    factor(left);
    while (token == '+' || token == '-' || token == '<') {
        int op = token;
        expect(op);
        char right[MAX_TMP_LEN];
        factor(right);
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
    LEAVE();
}


void print_st(void)
{
    ENTER("print_st()");
    expect(TK_PRINT);
    char e[MAX_TMP_LEN];
    expr(e);
    int v = vsuffix++;
    printf("  %%t.%d = getelementptr [4 x i8], ptr @fmt_pd, i32 0, i32 0\n", v);
    printf("  %%t.%d = call i32 (ptr, ...) @printf(ptr %%t.%d, i32 %s)\n", vsuffix++, v, e);
    expect(';');
    LEAVE();
}


void assign_st(void)
{
    ENTER("assign_st()");
    char x[MAX_TK_LEN];
    strcpy(x, name);
    int r = row, c = col;
    expect(TK_ID);
    if (hget(hmap, x) < 0) {
        fprintf(stderr, "不明な変数(行%d:%d): %s\n", r, c, x);
        exit(1);
    }
    expect('=');
    char e[MAX_TMP_LEN];
    expr(e);
    printf("  store i32 %s, ptr %%%s\n", e, x);
    expect(';');
    LEAVE();
}


void while_st(void)
{
    ENTER("while_st()");
    int while_entry = lsuffix++;
    int while_body = lsuffix++;
    int while_end = lsuffix++;
    printf("  br label %%L.%d\n", while_entry);

    // whileループ冒頭のブロック
    printf("L.%d:\n", while_entry);
    expect(TK_WHILE);
    expect('(');
    char e[MAX_TMP_LEN];
    expr(e);
    expect(')');
    int c = vsuffix++;
    printf("  %%t.%d = icmp eq i32 %s, 0\n", c, e);
    printf("  br i1 %%t.%d, label %%L.%d, label %%L.%d\n", c, while_end, while_body);

    // whileループ本体のブロック
    printf("L.%d:\n", while_body);
    statement();
    printf("  br label %%L.%d\n", while_entry);

    // whileループの直後のブロック
    printf("L.%d:\n", while_end);
    LEAVE();
}


void var_decl(void)
{
    ENTER("var_decl()");
    expect(TK_INT);
    int r = hget(hmap, name);
    if (r >= 0) {
        fprintf(stderr, "宣言の重複(行%d:%d): %s (行%dで既出)\n", row, col, name, r);
        exit(1);
    }
    printf("  %%%s = alloca i32\n", name);
    char *x = malloc(MAX_TK_LEN); // hdestroy()内でfree()される
    strcpy(x, name);
    hinsert(hmap, x, row); // xの宣言の行番号を記録
    expect(TK_ID);
    expect(';');
    LEAVE();
}


void block(void)
{
    ENTER("block()");
    expect('{');
    while (token != '}') {
        if (token == TK_INT) var_decl(); else statement();
    }
    expect('}');
    LEAVE();
}


void statement(void)
{
    ENTER("statement()");
    switch (token) {
        case TK_PRINT:
            print_st();
            break;
        case TK_ID:
            assign_st();
            break;
        case TK_WHILE:
            while_st();
            break;
        case '{':
            block();
            break;
        default:
            fprintf(stderr, "期待されない字句(行%d:%d): %s\n", row, col, name);
            exit(1);
    }
    LEAVE();
}


void program(void)
{
    ENTER("program()");
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
    block();
    printf("  ret i32 0\n");
    printf("}\n");
    LEAVE();
}


int main()
{
    hmap = hnew();
    token = get_next_token(name);
    program();
    hdestroy(hmap); // キーもここでfreeしている
}
