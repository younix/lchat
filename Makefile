include config.mk

.PHONY: all install uninstall filter clean test dist

all: lchat
clean:
	rm -f lchat *.o *.core sl_test filter/indent

install: lchat
	cp lchat $(DESTDIR)$(BINDIR)
	cp lchat.1 $(DESTDIR)$(MAN1DIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/lchat $(DESTDIR)$(MAN1DIR)/lchat.1

test: sl_test
	./sl_test

dist:
	mkdir -p lchat-$(VERSION)
	cp -r $$(git ls-tree --name-only HEAD) lchat-$(VERSION)
	tar -czf lchat-$(VERSION).tar.gz lchat-$(VERSION)
	rm -fr lchat-$(VERSION)

lchat: lchat.o slackline.o util.o slackline_emacs.o
	$(CC) -o $@ lchat.o slackline.o slackline_emacs.o util.o $(LIBS)

lchat.o: lchat.c
	$(CC) -c $(CFLAGS) -D_BSD_SOURCE -D_XOPEN_SOURCE -D_GNU_SOURCE \
	    -o $@ lchat.c

filter: filter/indent
filter/indent: filter/indent.c util.o util.h
	$(CC) $(CFLAGS) -o $@ filter/indent.c util.o

sl_test.o: sl_test.c slackline.h
	$(CC) $(CFLAGS) -Wno-sign-compare -c -o $@ sl_test.c

sl_test: sl_test.o slackline.o slackline.h
	$(CC) $(CFLAGS) -o $@ sl_test.o slackline.o $(LIBS)

slackline.o: slackline.c slackline.h
	$(CC) -c $(CFLAGS) -o $@ slackline.c

slackline_emacs.o: slackline_emacs.c slackline.h
	$(CC) -c $(CFLAGS) -o $@ slackline_emacs.c

util.o: util.c util.h
	$(CC) -c $(CFLAGS) -D_BSD_SOURCE -o $@ util.c
