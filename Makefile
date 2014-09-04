CFLAGS=-I./include/ -O3 -g -std=c99 -Wall -Wextra -Werror

all: lyra2

.PHONY: test

lyra2: main.o sponge.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: src/%.c
	$(CC) $^ $(CFLAGS) -c -o $@

test: test/sponge_test
	test/sponge_test

test/sponge_test: test/sponge_test.o sponge.o
	$(CC) $^ -o $@ -lcheck

test/sponge_test.o: test/sponge_test.c
	$(CC) -I./include/ $^ -c -o $@

test/sponge_test.c: test/sponge_test.check
	checkmk $^ > $@

clean:
	rm -f *.o lyra2 test/*.c test/sponge_test
