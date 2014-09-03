CFLAGS=-I./include/ -O3 -g -std=c99 -Wall -Wextra -Werror

all: lyra2

lyra2: main.o sponge.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: src/%.c
	$(CC) $^ $(CFLAGS) -c -o $@

clean:
	rm -f *.o lyra2
