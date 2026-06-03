CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = -lraylib -lm -lpthread -ldl -lrt -lX11

pong: main.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

SOURCES := $(wildcard *.c) $(wildcard *.h)

fmt:
	clang-format -i $(SOURCES)

run: pong
	./pong

clean:
	rm -f pong

.PHONY: fmt run clean
