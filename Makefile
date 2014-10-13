CFLAGS=-I./include/ -O3 -g -DUSE_PHS_INTERFACE -std=c99 -Wall -Wextra -Werror -msse2 -Wundef

ifdef AVX
override CFLAGS += -mavx2 -DUSE_AVX
ifdef OSX_GCC
override CFLAGS += -Wa,-q
endif
endif

REFDIR=ref/Lyra2-v2.5_PHC/src/

all: lyra2

.PHONY: test

lyra2: build/main.o build/lyra2.o
	$(CC) $(CFLAGS) $^ -o $@

ref:
	make -C $(REFDIR) linux-x86-64-sse2 nThreads=1

bench-ref:
	EXTRA_CFLAGS="-I$(PWD)/include -DUSE_PHS_INTERFACE" MAINC=$(PWD)/src/main.c make -C $(REFDIR) linux-x86-64-sse2 nThreads=1

build/lyra2.o: src/lyra2.c include/sponge.h
	mkdir -p build
	$(CC) $< $(CFLAGS) -c -o $@

build/%.o: src/%.c
	mkdir -p build
	$(CC) $^ $(CFLAGS) -c -o $@

test: test/sponge_test
	test/sponge_test

test/sponge_test: test/sponge_test.o
	$(CC) $^ -o $@ -lcheck

test/%.o: test/%.c
	$(CC) $(CFLAGS) -g -I./include/ $^ -c -o $@

test/%.c: test/%.check
	checkmk $^ > $@

clean:
	rm -rf build/ lyra2 test/*.{c,o} test/*_test
	make -C $(REFDIR) clean
