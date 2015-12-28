CC=cc
CFLAGS=-std=c99 -pedantic -Wall -Wextra

.PHONY: all clean

all: lchat
clean:
	rm -f lchat *.o

lchat: lchat.o slackline.o
	$(CC) -o $@ lchat.o slackline.o

lchat.o: lchat.c
	$(CC) -c $(CFLAGS) -D_BSD_SOURCE -D_XOPEN_SOURCE -D_GNU_SOURCE \
	    -o $@ lchat.c

slackline.o: slackline.c slackline.h
	$(CC) -c $(CFLAGS) -o $@ slackline.c
