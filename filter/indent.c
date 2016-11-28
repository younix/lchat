#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int
main(int argc, char *argv[])
{
	char buf[BUFSIZ];
	char timestr[BUFSIZ];
	char *fmt = "%H:%m";
	char *next, *nick, *word;
	struct tm tm;

	while (fgets(buf, sizeof buf, stdin) != NULL) {
		next = strptime(buf, "%Y-%m-%d %H:%M ", &tm);

		if (next == NULL || next[0] == '-') {
			fputs(buf, stdout);
			fflush(stdout);
			continue;
		}

		nick = strsep(&next, ">");
		nick++;
		next++;

		strftime(timestr, sizeof timestr, fmt, &tm);

		/* print prompt */
		printf("%s %*s", timestr, 12, nick);

		ssize_t pw = 18;
		ssize_t tw = 104 - pw;

		/* print indented text */
		while ((word = strsep(&next, " ")) != NULL) {
			tw -= strlen(word) + 1;
			if (tw < 0) {
				fputs("\n                  ", stdout);
				tw = 80 - pw;
			}

			fputc(' ', stdout);
			fputs(word, stdout);
			fflush(stdout);
		}
	}

	return EXIT_SUCCESS;
}
