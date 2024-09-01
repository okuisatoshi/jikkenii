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
int lsuffix = 1;       // ラベルの接尾番号
hashmap *hmap;         // 変数名と接尾番号の対を保存するハッシュ表
#define MAX_OFFSET 320 // 320で変数40個分, 16の倍数に要整列

// デバッグモードを有効にするには
// make clean; CFLAGS=-DDEBUG make
// のようにしてコンパイルする
// デバッグモードでは標準エラー出力に構文解析木が出力される
int findent = 0;
#ifdef DEBUG
#define ENTER(f) { findent += 3; fprintf(stderr, "%*c|- %s\n", findent, ' ', f); }
#define LEAVE() findent -= 3
#define FEED() fprintf(stderr, "%*c   \'%s\' in line %d (at %d)\n", findent, ' ', name, row, col)
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
    fprintf(stderr, "Unexpected token line %d (at %d): %s\n", row, col, name);
    exit(1);
}


void id(const char *reg)
{
    ENTER("id()");
    char x[MAX_TK_LEN];
    strcpy(x, name);
    int r = row, c = col;
    expect(TK_ID);
    int off = hget(hmap, x);
    if (off < 0) {
        fprintf(stderr, "Undefined variable in line %d (at %d): %s\n", r, c, x);
        exit(1);
    }
    printf("  mov  %s, [rbp - %d]\t# %s\n", reg, off, x);
    LEAVE();
}


void factor(const char *reg)
{
    ENTER("factor()");
    switch (token) {
        case TK_ID:
            id(reg);
            break;
        case TK_NUM:
            printf("  mov  %s, %s\n", reg, name);
            expect(TK_NUM);
            break;
        default:
            fprintf(stderr,
                    "Syntax error around line %d (at %d): identifier or number is expected, but found: %s\n",
                    row, col, name);
            exit(1);
    }
    LEAVE();
}


void expr(void)
{
    // expr()の結果は常にアキュムレータ(rax)に置かれる
    ENTER("expr()");
    factor("rax");
    while (token == '+' || token == '-' || token == '<') {
        int op = token;
        expect(op);
        factor("rdx");
        const char *f = op == '+' ? "add" : op == '-' ? "sub" : "cmp";
        printf("  %s  rax, rdx\n", f);
        if (op == '<') {
            // 比較(<)の結果(0,1)をalレジスタ(raxレジスタの下位8bit)にセット
            printf("  setl al\t# <?\n");
            // raxの上位ビットはゼロで埋める(自動的には埋めてくれないので)
            printf("  movzx rax, al\n");
        }
    }
    LEAVE();
}


void print_st(void)
{
    ENTER("print_st()");
    expect(TK_PRINT);
    expr();
    printf("  lea  rdi, [rip + fmt]\n"); // rdi=第1引数
    printf("  mov  rsi, rax\n"); // rsi=第2引数
    printf("  xor  eax, eax\n"); // al <- 0 (可変引数の場合のベクタ(浮動小数点)レジスタの数）
    printf("  call printf\n");
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
    int off = hget(hmap, x);
    if (off < 0) {
        fprintf(stderr, "Undefined variable in line %d (at %d): %s\n", r, c, x);
        exit(1);
    }
    expect('=');
    expr();
    printf("  mov  [rbp - %d], rax\t# %s\n", off, x);
    expect(';');
    LEAVE();
}


void while_st(void)
{
    ENTER("while_st()");
    int while_entry = lsuffix++;
    int while_end   = lsuffix++;

    // whileループ冒頭のブロック
    printf(".L.%d:\n", while_entry);
    expect(TK_WHILE);
    expect('(');
    expr();
    expect(')');
    printf("  test rax, rax\t# rax & rax\n");
    printf("  je   .L.%d\t# if 0 goto .L.%d\n", while_end, while_end);

    // whileループ本体のブロック
    statement();
    printf("  jmp  .L.%d\n", while_entry);

    // whileループの直後のブロック
    printf(".L.%d:\n", while_end);
    LEAVE();
}


void var_decl(void)
{
    ENTER("var_decl()");
    expect(TK_INT);
    // 宣言重複のチェック
    if (hget(hmap, name) >= 0) {
        fprintf(stderr, "Duplicated variable declaration in line %d (at %d): %s\n",
                row, col, name);
        exit(1);
    }
    char *x = malloc(MAX_TK_LEN); // hdestroy()でfreeされる
    strcpy(x, name);
    expect(TK_ID);
    // キー(識別子名)とオフセットをハッシュ表に登録
    static int off = 8; // rbpからのオフセットの絶対値,8の倍数
    if (off > MAX_OFFSET) {
        fprintf(stderr, "Error: Too many variables\n");
        exit(1);
    }
    printf("  # %s is allocated to rbp - %d\n", x, off);
    hinsert(hmap, x, off);
    off += 8;
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
            fprintf(stderr,
                    "Syntax error around line %d (at %d): statement is expected, but found: %s\n",
                    row, col, name);
            exit(1);
    }
    LEAVE();
}

void program(void)
{
    ENTER("program()");
    printf("  .intel_syntax noprefix\n");
    printf("  .section .note.GNU-stack, \"\", @progbits\n");
    printf("  .text\n");
    printf("fmt: .asciz \"%%d\\n\"\n");
    printf("  .global main\n");
    expect(TK_INT);
    if (strcmp(name, "main") != 0) {
        fprintf(stderr, "Syntax error: function main needed");
        exit(1);
    }
    expect(TK_ID);
    expect('(');
    expect(')');
    printf("main:\n");
    printf("  push rbp\n");
    // 復帰アドレス(8バイト) + rbpのバックアップ(8バイト)で
    // この時点でrspは16バイトで整列されているはず
    printf("  mov  rbp, rsp\n");
    // rspの16バイト整列が保たれるようにMAX_OFFSETは16の倍数
    printf("  sub  rsp, %d\n", MAX_OFFSET);
    block();
    printf("  xor  rax, rax\n"); // rax <- 0
    printf("  mov  rsp, rbp\n");
    printf("  pop  rbp\n");
    printf("  ret\n");
    LEAVE();
}


int main()
{
    // 識別子(変数名)とその接尾番号の対を保存するハッシュ表
    hmap = hnew();
    token = get_next_token(name);
    program();
    hdestroy(hmap); // キーもここでfreeされる
}

