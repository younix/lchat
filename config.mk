# paths
PREFIX	= /usr/local
BINDIR	= $(PREFIX)/bin
MANDIR	= $(PREFIX)/share/man
MAN1DIR	= $(MANDIR)/man1

CC = c99
CFLAGS = -pedantic -Wall -Wextra -I/usr/local/include

# grapheme.h
LIBS = -L/usr/local/lib -lgrapheme
