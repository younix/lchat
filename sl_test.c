#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slackline.h"

static void
strokes(struct slackline *sl, const char *str)
{
	for (const char *c = str; *c != '\0'; c++)
		sl_keystroke(sl, *c);
}

static void
check_init(struct slackline *sl)
{
	assert(sl != NULL);
	assert(sl->buf != NULL);
	assert(sl->ptr == sl->buf);
	assert(sl->rlen == 0);
	assert(sl->blen == 0);
	assert(sl->rcur == 0);
	assert(sl->bcur == 0);
	assert(sl->last == sl->buf);
}

static void
check_ascii(struct slackline *sl)
{
	strokes(sl, "abc");
	assert(sl->blen == 3);
	assert(sl->rlen == 3);
	assert(sl->bcur == 3);
	assert(sl->rcur == 3);
	assert(sl->last - sl->buf == sl->blen);

	strokes(sl, "\x08");	/* backspace */
	assert(sl->blen == 2);
	assert(sl->rlen == 2);
	assert(sl->bcur == 2);
	assert(sl->rcur == 2);
	assert(sl->last - sl->buf == sl->blen);

	strokes(sl, "\x1b[D");	/* left arrow key */
	assert(sl->blen == 2);
	assert(sl->rlen == 2);
	assert(sl->bcur == 1);
	assert(sl->rcur == 1);
	assert(sl->last - sl->buf == sl->blen);
}

static void
check_utf8(struct slackline *sl)
{
	            /*   ae  |   oe  |   ue  |   ss  */
	strokes(sl, "\xC3\xA4\xC3\xBC\xC3\xB6\xC3\x9F");
	assert(sl->blen == 8);
	assert(sl->rlen == 4);
	assert(sl->bcur == 8);
	assert(sl->rcur == 4);
	assert(sl->last - sl->buf == sl->blen);

	strokes(sl, "\x08");	/* backspace */
	assert(sl->blen == 6);
	assert(sl->rlen == 3);
	assert(sl->bcur == 6);
	assert(sl->rcur == 3);
	assert(sl->last - sl->buf == sl->blen);

	strokes(sl, "\x1b[D");	/* left arrow key */
	assert(sl->blen == 6);
	assert(sl->rlen == 3);
	assert(sl->bcur == 4);
	assert(sl->rcur == 2);
	assert(sl->last - sl->buf == sl->blen);

	strokes(sl, "\x08");	/* backspace */
	assert(sl->blen == 4);
	assert(sl->rlen == 2);
	assert(sl->bcur == 2);
	assert(sl->rcur == 1);
	assert(sl->last - sl->buf == sl->blen);

	strokes(sl, "\x1b[C");	/* right arrow key */
	assert(sl->blen == 4);
	assert(sl->rlen == 2);
	assert(sl->bcur == 4);
	assert(sl->rcur == 2);
	assert(sl->last - sl->buf == sl->blen);
}

int
main(void)
{
	struct slackline *sl;

	sl = sl_init();
	check_init(sl);
	check_ascii(sl);

	sl_reset(sl);
	check_init(sl);
	check_utf8(sl);

	sl_free(sl);

	return EXIT_SUCCESS;
}
