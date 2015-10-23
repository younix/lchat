#include <err.h>
#include <stdio.h>
#include <stdlib.h>
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
	int fd = STDIN_FILENO;
	int c;

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
		sl_keystroke(sl, c);
		printf("c: %d: buf: %s\r\n", c, sl->buf);
	}

	puts("\r");

	return EXIT_SUCCESS;
}
