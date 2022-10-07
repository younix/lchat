/*
 * Copyright (c) 2015-2016 Jan Klemkow <j.klemkow@wemelug.de>
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grapheme.h>

#include "slackline.h"

struct slackline *
sl_init(void)
{
	struct slackline *sl = malloc(sizeof *sl);

	if (sl == NULL)
		return NULL;

	sl->bufsize = BUFSIZ;
	if ((sl->buf = malloc(sl->bufsize)) == NULL) {
		free(sl);
		return NULL;
	}

	memset(sl->ubuf, 0, sizeof(sl->ubuf));
	sl->ubuf_len = 0;

	sl_reset(sl);

	return sl;
}

void
sl_free(struct slackline *sl)
{
	free(sl->buf);
	free(sl);
}

void
sl_reset(struct slackline *sl)
{
	sl->buf[0] = '\0';
	sl->ptr = sl->buf;
	sl->last = sl->buf;

	sl->bcur = 0;
	sl->blen = 0;
	sl->rcur = 0;
	sl->rlen = 0;

	sl->esc = ESC_NONE;
	sl->ubuf_len = 0;
}

static size_t
sl_postobyte(struct slackline *sl, size_t pos)
{
	char *ptr = &sl->buf[0];
	size_t byte = 0;

	for (;pos > 0; pos--)
		byte += grapheme_next_character_break_utf8(ptr+byte, sl->blen-byte);

	return byte;
}

static char *
sl_postoptr(struct slackline *sl, size_t pos)
{
	return &sl->buf[sl_postobyte(sl, pos)];
}

static void
sl_backspace(struct slackline *sl)
{
	char *ncur;

	if (sl->rcur == 0)
		return;

	ncur = sl_postoptr(sl, sl->rcur - 1);

	if (sl->rcur < sl->rlen)
		memmove(ncur, sl->ptr, sl->last - sl->ptr);

	sl->rcur--;
	sl->rlen--;
	sl->bcur = sl_postobyte(sl, sl->rcur);
	sl->blen = sl_postobyte(sl, sl->rlen);

	sl->last -= sl->ptr - ncur;
	*sl->last = '\0';

	sl->ptr = ncur;
}

int
sl_keystroke(struct slackline *sl, int key)
{
	uint_least32_t cp;

	if (sl == NULL || sl->rlen < sl->rcur)
		return -1;

	/* handle escape sequences */
	switch (sl->esc) {
	case ESC_NONE:
		break;
	case ESC:
		sl->esc = key == '[' ? ESC_BRACKET : ESC_NONE;
		return 0;
	case ESC_BRACKET:
		switch (key) {
		case 'A':	/* up    */
		case 'B':	/* down  */
			break;
		case 'C':	/* right */
			if (sl->rcur < sl->rlen)
				sl->rcur++;
			sl->bcur = sl_postobyte(sl, sl->rcur);
			sl->ptr = sl->buf + sl->bcur;
			break;
		case 'D':	/* left */
			if (sl->rcur > 0)
				sl->rcur--;
			sl->bcur = sl_postobyte(sl, sl->rcur);
			sl->ptr = sl->buf + sl->bcur;
			break;
		case 'H':	/* Home  */
			sl->bcur = sl->rcur = 0;
			sl->ptr = sl->buf;
			break;
		case 'F':	/* End   */
			sl->rcur = sl->rlen;
			sl->bcur = sl_postobyte(sl, sl->rcur);
			sl->ptr = sl->buf + sl->bcur;
			break;
		case 'P':	/* delete */
			if (sl->rcur == sl->rlen)
				break;

			char *ncur = sl_postoptr(sl, sl->rcur + 1);

			memmove(sl->ptr, ncur, sl->last - ncur);

			sl->rlen--;
			sl->blen = sl_postobyte(sl, sl->rlen);

			sl->last -= ncur - sl->ptr;
			*sl->last = '\0';
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			sl->nummod = key;
			sl->esc = ESC_BRACKET_NUM;
			return 0;
		}
		sl->esc = ESC_NONE;
		return 0;
	case ESC_BRACKET_NUM:
		switch(key) {
		case '~':
			switch(sl->nummod) {
			case '7':
				sl->bcur = sl->rcur = 0;
				sl->ptr = sl->buf;
				break;
			case '8':
				sl->rcur = sl->rlen;
				sl->bcur = sl_postobyte(sl, sl->rcur);
				sl->ptr = sl->buf + sl->bcur;
				break;
			}
			sl->esc = ESC_NONE;
			return 0;
		}
	}

	if (!iscntrl((unsigned char) key))
		goto compose;

	/* handle ctl keys */
	switch (key) {
	case 27:	/* Escape */
		sl->esc = ESC;
		return 0;
	case 127:	/* backspace */
	case 8:		/* backspace */
		sl_backspace(sl);
		return 0;
	case 21: /* ctrl+u -- clearline */
		sl_reset(sl);
		return 0;
	case 23: /* ctrl+w -- erase previous word */
		while (sl->rcur != 0 && isspace((unsigned char) *(sl->ptr-1)))
			sl_backspace(sl);

		while (sl->rcur != 0 && !isspace((unsigned char) *(sl->ptr-1)))
			sl_backspace(sl);
		return 0;
	default:
		return 0;
	}

compose:
	/* byte-wise composing of UTF-8 runes */
	sl->ubuf[sl->ubuf_len++] = key;
	if (grapheme_decode_utf8(sl->ubuf, sl->ubuf_len, &cp) > sl->ubuf_len ||
	    cp == GRAPHEME_INVALID_CODEPOINT)
		return 0;

	if (sl->blen + sl->ubuf_len >= sl->bufsize) {
		char *nbuf;

		if ((nbuf = realloc(sl->buf, sl->bufsize * 2)) == NULL)
			return -1;

		sl->ptr = nbuf + (sl->ptr - sl->buf);
		sl->last = nbuf + (sl->last - sl->buf);
		sl->buf = nbuf;
		sl->bufsize *= 2;
	}

	/* add character to buffer */
	if (sl->rcur < sl->rlen) {	/* insert into buffer */
		char *cur = sl_postoptr(sl, sl->rcur);
		char *end = sl_postoptr(sl, sl->rlen);
		char *ncur = cur + sl->ubuf_len;

		memmove(ncur, cur, end - cur);
	}

	memcpy(sl_postoptr(sl, sl->rcur), sl->ubuf, sl->ubuf_len);

	sl->ptr  += sl->ubuf_len;
	sl->last += sl->ubuf_len;
	sl->bcur += sl->ubuf_len;
	sl->blen += sl->ubuf_len;
	sl->ubuf_len = 0;

	sl->rcur++;
	sl->rlen++;

	*sl->last = '\0';

	return 0;
}
