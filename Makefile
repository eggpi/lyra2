CFLAGS=-I./include/ -O3 -g -DUSE_PHS_INTERFACE -std=c99 -Wall -Wextra -Werror -march=native -Wundef -Wshadow

ifdef OSX_GCC
override CFLAGS += -Wa,-q
endif

ifdef NO_AVX2
override CFLAGS += -DNO_AVX2
endif

REFDIR=ref/Lyra2-v2.5_PHC/

all: lyra2

.PHONY: test bench-ref

HAS_CHECK=$(shell command -v checkmk >/dev/null 2>&1 && echo true)
ifeq ($(HAS_CHECK),true)
	CHECK_LDFLAGS=-lcheck
endif

lyra2: build/main.o build/lyra2.o
	$(CC) $(CFLAGS) -lm $^ -o $@

bench-ref:
	EXTRA_CFLAGS="-I$(PWD)/include -DUSE_PHS_INTERFACE" MAINC=$(PWD)/src/main.c make -C $(REFDIR)/src linux-x86-64-sse2 nThreads=1
	ln $(REFDIR)/bin/Lyra2 lyra2

build/lyra2.o: src/lyra2.c include/sponge.h
	mkdir -p build
	$(CC) $< $(CFLAGS) -c -o $@

build/%.o: src/%.c
	mkdir -p build
	$(CC) $^ $(CFLAGS) -c -o $@

test: test/sponge_test
	test/sponge_test

test/sponge_test: test/sponge_test.o
	$(CC) $^ -o $@ $(CHECK_LDFLAGS)

test/%.o: test/%.c
	$(CC) $(CFLAGS) -g -I./include/ $^ -c -o $@

test/%.c: test/%.check
ifeq ($(HAS_CHECK),true)
	checkmk $^ > $@
else
	@echo need checkmk to update test files!
	exit 1
endif

clean:
	rm -rf build/ lyra2 test/*.o test/*_test
	make -C $(REFDIR)/src clean
