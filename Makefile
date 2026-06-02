CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = -lraylib -lm -lpthread -ldl -lrt -lX11

astroids: main.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

SOURCES := $(wildcard *.c) $(wildcard *.h)

fmt:
	clang-format -i $(SOURCES)

run: astroids
	./astroids

clean:
	rm -f astroids

.PHONY: fmt run clean
