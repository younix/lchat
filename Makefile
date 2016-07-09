CC ?= cc
CFLAGS = -std=c99 -pedantic -Wall -Wextra -g

# utf.h
CFLAGS += -I/usr/local/include
LIBS = -L/usr/local/lib -lutf

.PHONY: all clean test debug

all: lchat
clean:
	rm -f lchat *.o *.core

test: sl_test
	./sl_test

debug:
	gdb lchat lchat.core

lchat: lchat.o slackline.o
	$(CC) -o $@ lchat.o slackline.o $(LIBS)

lchat.o: lchat.c
	$(CC) -c $(CFLAGS) -D_BSD_SOURCE -D_XOPEN_SOURCE -D_GNU_SOURCE \
	    -o $@ lchat.c

sl_test.o: sl_test.c slackline.h
	$(CC) $(CFLAGS) -c -o $@ sl_test.c

sl_test: sl_test.o slackline.o slackline.h
	$(CC) $(CFLAGS) -o $@ sl_test.o slackline.o $(LIBS)

slackline.o: slackline.c slackline.h
	$(CC) -c $(CFLAGS) -o $@ slackline.c
