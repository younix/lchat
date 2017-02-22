#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int
main(void)
{
	char buf[BUFSIZ];
	char timestr[BUFSIZ];
	char old_nick[BUFSIZ] = "";
	char *fmt = "%H:%M";
	char *next, *nick, *word;
	struct tm tm;
	int cols = 80;		/* terminal width */

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
		/* HH:MM nnnnnnnnnnnn ttttttttttttt */
		printf("%s %*s", timestr, 12,
		    strcmp(nick, old_nick) == 0 ? "" : nick);

		strlcpy(old_nick, nick, sizeof old_nick);

		ssize_t pw = 18;	/* prompt width */
		ssize_t tw = cols - pw;	/* text width */
		bool first = true;

		/* print indented text */
		while ((word = strsep(&next, " ")) != NULL) {
			tw -= strlen(word) + 1;
			if (tw < 0 && !first) {
				fputs("\n                  ", stdout);
				tw = cols - pw;
				first = true;
			}

			fputc(' ', stdout);
			fputs(word, stdout);
			first = false;
		}

		fflush(stdout);
	}

	return EXIT_SUCCESS;
}
