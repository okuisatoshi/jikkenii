CC := clang-15
# CC := clang-15 --sysroot=./wasi-sysroot --target=wasm32-wasi
# CC := zig cc --target=wasm32-wasi
CFLAGS := -Wall -std=c11 -O0 -g3 $(CFLAGS)
SHELL := /bin/bash

picoc: scan.o hashmap.o tidwall_hashmap.o picoc.o
	$(CC) $(CFLAGS) $^ -o $@

picoc_pre1: scan.o picoc_pre1.o
	$(CC) $(CFLAGS) $^ -o $@

picoc_pre2: scan.o picoc_pre2.o
	$(CC) $(CFLAGS) $^ -o $@

testscan: scan.o testscan.o
	$(CC) $(CFLAGS) $^ -o $@

scan.o testscan.o picoc.o picoc_pre1.o picoc_pre2.o: scan.h

hashmap.o picoc.o: hashmap.h

tidwall_hashmap.o: tidwall/tidwall_hashmap.c tidwall/tidwall_hashmap.h
	$(CC) $(CFLAGS) -c $< -o $@

all: picoc picoc_pre1 picoc_pre2 testscan

test: picoc
	diff <(echo "966") <(./picoc < example/bpmod.pc | lli-15) && echo "OK" || ehco "NG"

download:
	curl -L https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-19/wasi-sysroot-19.0.tar.gz | tar zxf -

clean:
	rm -rf *.o *~ picoc picoc_pre1 picoc_pre2 testscan a.out

.PHONY: all test clean


