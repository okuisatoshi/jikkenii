CC := clang-15
CFLAGS := -Wall -std=c11 -O0 -g3 $(CFLAGS)
SHELL := /bin/bash

picoc: scan.o hashmap.o tidwall_hashmap.o picoc.o
	$(CC) $(CFLAGS) $^ -o $@

picoc_x64_mem: scan.o hashmap.o tidwall_hashmap.o picoc_x64_mem.o
	$(CC) $(CFLAGS) $^ -o $@

picoc_x64_reg: scan.o hashmap.o tidwall_hashmap.o picoc_x64_reg.o
	$(CC) $(CFLAGS) $^ -o $@

picoc_pre1: scan.o picoc_pre1.o
	$(CC) $(CFLAGS) $^ -o $@

picoc_pre2: scan.o picoc_pre2.o
	$(CC) $(CFLAGS) $^ -o $@

testscan: scan.o testscan.o
	$(CC) $(CFLAGS) $^ -o $@

scan.o testscan.o picoc.o picoc_pre1.o picoc_pre2.o picoc_x64_mem.o picoc_x64_reg.o: scan.h

hashmap.o picoc.o picoc_x64_mem.o picoc_x64_reg.o: hashmap.h

tidwall_hashmap.o: tidwall/tidwall_hashmap.c tidwall/tidwall_hashmap.h
	$(CC) $(CFLAGS) -c $< -o $@

all: picoc picoc_pre1 picoc_pre2 picoc_x64_mem picoc_x64_reg testscan

test: picoc
	diff <(echo "966") <(./picoc < example/bpmod.pc | lli-15) && echo "OK" || ehco "NG"

clean:
	rm -rf *.o *~ picoc picoc_pre1 picoc_pre2 picoc_x64_mem picoc_x64_reg testscan a.out

gtags:
	gtags

htags:
	htags -gasn

clean_all:
	make -s clean
	rm -rf GTAGS GRTAGS GPATH HTML

.PHONY: all test clean gtags htags clean_all
