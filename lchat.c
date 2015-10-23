#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

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
	int fd = STDIN_FILENO;
	int c;

	if (isatty(fd) == 0)
		err(EXIT_FAILURE, "isatty");

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
		//printf("c: %d\r\n", c);
		if (c >= 32 && c < 127)
			putchar(c);
	}

	return EXIT_SUCCESS;
}
