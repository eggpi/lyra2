CFLAGS=-I./include/ -O3 -g -std=c99 -Wall -Wextra -Werror -msse2

ifdef AVX
override CFLAGS += -mavx2 -DUSE_AVX
ifdef OSX_GCC
override CFLAGS += -Wa,-q
endif
endif

all: lyra2

.PHONY: test

lyra2: build/main.o build/lyra2.o
	$(CC) $(CFLAGS) $^ -o $@

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
