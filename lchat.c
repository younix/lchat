#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <term.h>
#include <termios.h>
#include <unistd.h>

#include "slackline.h"

struct termios origin_term;

void
exit_handler(void)
{
	if (tcsetattr(STDIN_FILENO, TCSANOW, &origin_term) == -1)
		err(EXIT_FAILURE, "tcsetattr");
}

int
main(void)
{
	struct termios term;
	struct slackline *sl = sl_init();
	char *term_name = getenv("TERM");
	int fd = STDIN_FILENO;
	int c;

	if (term_name == NULL)
		errx(EXIT_FAILURE, "environment TERM is not set");

	switch (tgetent(NULL, term_name)) {
	case -1: err(EXIT_FAILURE, "tgetent");
	case 0: errx(EXIT_FAILURE, "no termcap entry found for %s", term_name);
	}

	if (isatty(fd) == 0)
		err(EXIT_FAILURE, "isatty");

	/* preprate terminal reset on exit */
	if (tcgetattr(fd, &origin_term) == -1)
		err(EXIT_FAILURE, "tcgetattr");

	if (atexit(exit_handler) == -1)
		err(EXIT_FAILURE, "atexit");

	/* prepare terminal */
	if (tcgetattr(fd, &term) == -1)
		err(EXIT_FAILURE, "tcgetattr");

	cfmakeraw(&term);

	if (tcsetattr(fd, TCSANOW, &term) == -1)
		err(EXIT_FAILURE, "tcsetattr");

	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	while ((c = getchar()) != 13) {
		if (sl_keystroke(sl, c) == -1)
			errx(EXIT_FAILURE, "sl_keystroke");
		printf("\r\033[2K%s", sl->buf);
	}

	puts("\r");

	return EXIT_SUCCESS;
}
