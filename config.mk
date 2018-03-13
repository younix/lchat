# paths
PREFIX	= /usr/local
BINDIR	= ${PREFIX}/bin
MANDIR	= ${PREFIX}/share/man
MAN1DIR	= ${MANDIR}/man1

CC ?= cc
CFLAGS = -std=c99 -pedantic -Wall -Wextra -g

# utf.h
CFLAGS += -I/usr/local/include
LIBS = -L/usr/local/lib -lutf
