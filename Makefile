CC=gcc
#CC=clang
CFLAGS=-Wall -Wextra -Werror -pedantic -std=c11 -O2 -pthread
OBJ=build/bench.o

ifeq ($(DEBUG),ON)
	CFLAGS+=-g -DDEBUG
else
	CFLAGS+=-DNDEBUG
endif

.PHONY: all clean

all: build/bench

build/bench: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

build/%.o: src/%.c
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm -f $(OBJ) build/bench
