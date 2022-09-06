/*
 * Copyright (c) 2015-2017 Jan Klemkow <j.klemkow@wemelug.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/ioctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <term.h>
#include <termios.h>
#include <unistd.h>

#include "slackline.h"
#include "util.h"

#ifndef INFTIM
#define INFTIM -1
#endif

struct termios origin_term;
struct winsize winsize;

static void
sigwinch(int sig)
{
	if (sig == SIGWINCH)
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
}

static void
exit_handler(void)
{
	if (tcsetattr(STDIN_FILENO, TCSANOW, &origin_term) == -1)
		err(EXIT_FAILURE, "tcsetattr");
}

static char *
read_file_line(const char *file)
{
	FILE *fh;
	char buf[BUFSIZ];
	char *line = NULL;
	char *nl = NULL;

	if (access(file, R_OK) == -1)
		return NULL;

	if ((fh = fopen(file, "r")) == NULL)
		err(EXIT_FAILURE, "fopen");

	if (fgets(buf, sizeof buf, fh) == NULL)
		err(EXIT_FAILURE, "fgets");

	if (fclose(fh) == EOF)
		err(EXIT_FAILURE, "fclose");

	if ((nl = strchr(buf, '\n')) != NULL)	/* delete new line */
		*nl = '\0';

	if ((line = strdup(buf)) == NULL)
		err(EXIT_FAILURE ,"strdup");

	return line;
}

static void
line_output(struct slackline *sl, char *file)
{
	int fd;

	if ((fd = open(file, O_WRONLY|O_APPEND)) == -1)
		err(EXIT_FAILURE, "open: %s", file);

	if (write(fd, sl->buf, sl->blen) == -1)
		err(EXIT_FAILURE, "write");

	if (close(fd) == -1)
		err(EXIT_FAILURE, "close");
}

static void
fork_filter(int *read, int *write)
{
	int fds_read[2];	/* .filter -> lchat */
	int fds_write[2];	/* lchat -> .filter */

	if (pipe(fds_read) == -1)
		err(EXIT_FAILURE, "pipe");
	if (pipe(fds_write) == -1)
		err(EXIT_FAILURE, "pipe");

	switch (fork()) {
	case -1:
		err(EXIT_FAILURE, "fork of .filter");
	case 0:	/* child */
		if (dup2(fds_read[1], STDOUT_FILENO) == -1)
			err(EXIT_FAILURE, "dup2");
		if (dup2(fds_write[0], STDIN_FILENO) == -1)
			err(EXIT_FAILURE, "dup2");

		if (close(fds_read[0]) == -1)
			err(EXIT_FAILURE, "close");
		if (close(fds_write[1]) == -1)
			err(EXIT_FAILURE, "close");

		execl("./.filter", "./.filter", NULL);
		err(EXIT_FAILURE, "exec of .filter");
	}

	/* parent */
	if (close(fds_read[1]) == -1)
		err(EXIT_FAILURE, "close");
	if (close(fds_write[0]) == -1)
		err(EXIT_FAILURE, "close");

	*read = fds_read[0];
	*write = fds_write[1];
}

static void
usage(void)
{
	fputs("lchat [-aeh] [-n lines] [-p prompt] [-t title] [-i in] [-o out]"
	    " [directory]\n", stderr);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct pollfd pfd[3];
	struct termios term;
	struct slackline *sl = sl_init();
	int fd = STDIN_FILENO;
	int read_fd = 6;
	int read_filter = -1;
	int backend_sink = STDOUT_FILENO;
	char c;
	int ch;
	bool empty_line = false;
	bool bell_flag = true;
	bool ucspi = false;
	char *bell_file = ".bellmatch";
	size_t history_len = 5;
	char *prompt = read_file_line(".prompt");
	char *title = read_file_line(".title");

	if (prompt == NULL)	/* set default prompt */
		prompt = "> ";

	size_t prompt_len = strlen(prompt);
	size_t loverhang = 0;
	char *dir = ".";
	char *in_file = NULL;
	char *out_file = NULL;

	while ((ch = getopt(argc, argv, "an:i:eo:p:t:uh")) != -1) {
		switch (ch) {
		case 'a':
			bell_flag = false;
			break;
		case 'n':
			errno = 0;
			history_len = strtoull(optarg, NULL, 0);
			if (errno != 0)
				err(EXIT_FAILURE, "strtoull");
			break;
		case 'i':
			if ((in_file = strdup(optarg)) == NULL)
				err(EXIT_FAILURE, "strdup");
			break;
		case 'e':
			empty_line = true;
			break;
		case 'o':
			if ((out_file = strdup(optarg)) == NULL)
				err(EXIT_FAILURE, "strdup");
			break;
		case 'p':
			if ((prompt = strdup(optarg)) == NULL)
				err(EXIT_FAILURE, "strdup");
			prompt_len = strlen(prompt);
			break;
		case 't':
			if ((title = strdup(optarg)) == NULL)
				err(EXIT_FAILURE, "strdup");
			break;
		case 'u':
			ucspi = true;
			break;
		case 'h':
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 1)
		usage();

	if (argc == 1)
		if ((dir = strdup(argv[0])) == NULL)
			err(EXIT_FAILURE, "strdup");

	if (in_file == NULL)
		if (asprintf(&in_file, "%s/in", dir) == -1)
			err(EXIT_FAILURE, "asprintf");

	if (out_file == NULL)
		if (asprintf(&out_file, "%s/out", dir) == -1)
			err(EXIT_FAILURE, "asprintf");

	if (isatty(fd) == 0)
		err(EXIT_FAILURE, "isatty");

	/* set terminal's window title */
	if (title == NULL) {
		char path[PATH_MAX];
		if (getcwd(path, sizeof path) == NULL)
			err(EXIT_FAILURE, "getcwd");
		if ((title = basename(path)) == NULL)
			err(EXIT_FAILURE, "basename");
	}
	if (strcmp(getenv("TERM"), "screen") == 0)
		printf("\033k%s\033\\", title);
	else
		printf("\033]0;%s\a", title);

	/* preprate terminal reset on exit */
	if (tcgetattr(fd, &origin_term) == -1)
		err(EXIT_FAILURE, "tcgetattr");

	if (atexit(exit_handler) == -1)
		err(EXIT_FAILURE, "atexit");

	/* prepare terminal */
	if (tcgetattr(fd, &term) == -1)
		err(EXIT_FAILURE, "tcgetattr");

	/* TODO: clean up this block.  copied from cfmakeraw(3) */
	term.c_iflag &= ~(IMAXBEL|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
//        term.c_oflag &= ~OPOST;
	term.c_lflag &= ~(ECHO|ECHONL|ICANON|IEXTEN);
	term.c_cflag &= ~(CSIZE|PARENB);
	term.c_cflag |= CS8;
	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;

	if (tcsetattr(fd, TCSANOW, &term) == -1)
		err(EXIT_FAILURE, "tcsetattr");

	/* get the terminal size */
	sigwinch(SIGWINCH);
	signal(SIGWINCH, sigwinch);

	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	if (!ucspi) {
		char tail_cmd[BUFSIZ];
		FILE *fh;

		/* open external source */
		snprintf(tail_cmd, sizeof tail_cmd, "exec tail -n %zd -f %s",
		    history_len, out_file);

		if ((fh = popen(tail_cmd, "r")) == NULL)
			err(EXIT_FAILURE, "unable to open pipe to tail");

		read_fd = fileno(fh);
	}

	int nfds = 2;

	pfd[0].fd = fd;
	pfd[0].events = POLLIN;

	pfd[1].fd = read_fd;
	pfd[1].events = POLLIN;

	if (access(".filter", X_OK) == 0) {
		fork_filter(&read_filter, &backend_sink);

		pfd[2].fd = read_filter;
		pfd[2].events = POLLIN;

		nfds = 3;
	}

	/* print initial prompt */
	fputs(prompt, stdout);

	for (;;) {
		if (fflush(stdout) == EOF)
			err(EXIT_FAILURE, "fflush");

		errno = 0;
		if (poll(pfd, nfds, INFTIM) == -1 && errno != EINTR)
			err(EXIT_FAILURE, "poll");

		/* moves cursor back after linewrap */
		if (loverhang > 0) {
			fputs("\r\033[2K", stdout);	/* cr + ... */
			printf("\033[%zuA", loverhang);	/* x times UP */
		}

		/* carriage return and erase the whole line */
		fputs("\r\033[2K", stdout);

		/* handle keyboard intput */
		if (pfd[0].revents & POLLIN) {
			ssize_t ret = read(fd, &c, sizeof c);

			if (ret == -1)
				err(EXIT_FAILURE, "read");

			if (ret == 0)
				return EXIT_SUCCESS;

			switch (c) {
			case 4:		/* eot */
				return EXIT_SUCCESS;
			case 13:	/* return */
				if (sl->rlen == 0 && empty_line == false)
					goto out;
				/* replace NUL-terminator with newline */
				sl->buf[sl->blen++] = '\n';
				if (ucspi) {
					if (write(7, sl->buf, sl->blen) == -1)
						err(EXIT_FAILURE, "write");
				} else {
					line_output(sl, in_file);
				}
				sl_reset(sl);
				break;
			default:
				if (sl_keystroke(sl, c) == -1)
					errx(EXIT_FAILURE, "sl_keystroke");
			}
		}

		/* handle backend error and its broken pipe */
		if (pfd[1].revents & POLLHUP)
			break;
		if (pfd[1].revents & POLLERR || pfd[1].revents & POLLNVAL)
			errx(EXIT_FAILURE, "backend error");

		/* handle backend input */
		if (pfd[1].revents & POLLIN) {
			char buf[BUFSIZ];
			ssize_t n = read(pfd[1].fd, buf, sizeof buf);
			if (n == 0)
				errx(EXIT_FAILURE, "backend exited");
			if (n == -1)
				err(EXIT_FAILURE, "read");
			if (write(backend_sink, buf, n) == -1)
				err(EXIT_FAILURE, "write");

			/* terminate the input buffer with NUL */
			buf[n == BUFSIZ ? n - 1 : n] = '\0';

			/* ring the bell on external input */
			if (bell_flag && bell_match(buf, bell_file))
				putchar('\a');
		}

		/* handel optional .filter i/o */
		if (nfds > 2) {
			/* handle .filter error and its broken pipe */
			if (pfd[2].revents & POLLHUP)
				break;
			if (pfd[2].revents & POLLERR ||
			    pfd[2].revents & POLLNVAL)
				errx(EXIT_FAILURE, ".filter error");

			/* handle .filter output */
			if (pfd[2].revents & POLLIN) {
				char buf[BUFSIZ];
				ssize_t n = read(pfd[2].fd, buf, sizeof buf);
				if (n == 0)
					errx(EXIT_FAILURE, ".filter exited");
				if (n == -1)
					err(EXIT_FAILURE, "read");
				if (write(STDOUT_FILENO, buf, n) == -1)
					err(EXIT_FAILURE, "write");
			}
		}
 out:
		/* show current input line */
		fputs(prompt, stdout);
		fputs(sl->buf, stdout);

		/* save amount of overhanging lines */
		loverhang = (prompt_len + sl->rlen) / winsize.ws_col;

		/* correct line wrap handling */
		if ((prompt_len + sl->rlen) > 0 &&
		    (prompt_len + sl->rlen) % winsize.ws_col == 0)
			fputs("\n", stdout);

		if (sl->rcur < sl->rlen) {	/* move the cursor */
			putchar('\r');
			/* HACK: because \033[0C does the same as \033[1C */
			if (sl->rcur + prompt_len > 0)
				printf("\033[%zuC", sl->rcur + prompt_len);
		}
	}
	return EXIT_SUCCESS;
}
