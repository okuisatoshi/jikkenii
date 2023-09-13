#!/bin/bash
# picoc言語のソースファイルをC言語のソースファイルに変換
# (1) 先頭に#include <stdio.h>を補う
# (2) print 式をprintf("%d\n", 式)に書き換える
sed -e '1i#include <stdio.h>' -e 's/print\s*\([^;]*\);/printf("%d\\n", \1);/g'
