#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slackline.h"

struct slackline *
sl_init(void)
{
	struct slackline *sl = malloc(sizeof *sl);

	if (sl == NULL)
		return NULL;

	sl->bufsize = BUFSIZ;
	sl->buf = malloc(sl->bufsize);
	sl->buf[0] = '\0';
	sl->len = 0;
	sl->cur = 0;

	return sl;
}

void
sl_free(struct slackline *sl)
{
	free(sl->buf);
	free(sl);
}

int
sl_keystroke(struct slackline *sl, int key)
{
	if (sl == NULL || sl->len < sl->cur)
		return -1;

	/* add character to buffer */
	if (key >= 32 && key <= 127) {
		if (sl->cur < sl->len) {
			memmove(sl->buf + sl->cur + 1, sl->buf + sl->cur,
			    sl->len - sl->cur);
			sl->buf[sl->cur++] = key;
		} else {
			sl->buf[sl->cur++] = key;
			sl->buf[sl->cur] = '\0';
		}

		return 0;
	}

	/* handle ctl keys */
	switch (key) {
	case 8:	/* backspace */
		sl->cur--;
		sl->buf[sl->cur] = '\0';
		break;
	}

	return 0;
}
