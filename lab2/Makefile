CC = gcc
CFLAGS = -Wall -Wextra -g

all: parent child

parent: parent.c process.c process.h
	$(CC) $(CFLAGS) -o parent parent.c process.c

child: child.c sum.c sum.h
	$(CC) $(CFLAGS) -o child child.c sum.c

test: all
	sh tests/run_tests.sh

clean:
	rm -f parent child

.PHONY: all test clean
