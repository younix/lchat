CC=cc
CFLAGS=-std=c99 -pedantic -Wall -Wextra

lchat: lchat.c
	$(CC) $(CFLAGS) -o $@ lchat.c
