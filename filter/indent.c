#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define color1 34
#define color2 36

int
main(void)
{
	char buf[BUFSIZ];
	char timestr[BUFSIZ];
	char old_nick[BUFSIZ] = "";
	char *fmt = "%H:%M";
	char *next, *nick, *word;
	int cols = 80;		/* terminal width */
	int color = color1;

	while (fgets(buf, sizeof buf, stdin) != NULL) {
		time_t time = strtol(buf, &next, 10);
		struct tm *tm = localtime(&time);

		next++;				/* skip space */

		if (next == NULL || next[0] == '-' || time == 0) {
			fputs(buf, stdout);
			fflush(stdout);
			continue;
		}

		nick = strsep(&next, ">");
		nick++;				/* skip '>'   */
		next++;				/* skip space */

		strftime(timestr, sizeof timestr, fmt, tm);

		/* swap color */
		if (strcmp(nick, old_nick) != 0)
			color = color == color1 ? color2 : color1;

		/* print prompt */
		/* HH:MM nnnnnnnnnnnn ttttttttttttt */
		printf("\033[%dm%s %*s", color, timestr, 12,
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
		fputs("\033[0m", stdout);	/* turn color off */
		fflush(stdout);
	}

	return EXIT_SUCCESS;
}
