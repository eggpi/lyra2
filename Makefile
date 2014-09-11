CFLAGS=-I./include/ -O3 -g -std=c99 -Wall -Wextra -Werror

all: lyra2

.PHONY: test

lyra2: build/main.o build/lyra2.o build/sponge.o
	$(CC) $(CFLAGS) $^ -o $@

build/%.o: src/%.c
	mkdir -p build
	$(CC) $^ $(CFLAGS) -c -o $@

test: test/sponge_test
	test/sponge_test

test/sponge_test: test/sponge_test.o build/sponge.o
	$(CC) $^ -o $@ -lcheck

test/%.o: test/%.c
	$(CC) -I./include/ $^ -c -o $@

test/%.c: test/%.check
	checkmk $^ > $@

clean:
	rm -rf build/ lyra2 test/*.{c,o} test/*_test
