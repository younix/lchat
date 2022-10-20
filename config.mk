VERSION = 1.0

# paths
PREFIX	= /usr/local
BINDIR	= $(PREFIX)/bin
MANDIR	= $(PREFIX)/man
MAN1DIR	= $(MANDIR)/man1

CFLAGS = -std=c99 -pedantic -Wall -Wextra -I/usr/local/include

# grapheme.h
LIBS = -L/usr/local/lib -lgrapheme
