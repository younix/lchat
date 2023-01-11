/*
 * Copyright (c) 2015-2022 Jan Klemkow <j.klemkow@wemelug.de>
 * Copyright (c) 2022 Tom Schwindl <schwindl@posteo.de>
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

#include "slackline_internals.h"
#include "slackline.h"
#include "util.h"

struct slackline *
sl_init(void)
{
	char *mode = getenv("EDITOR");
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

	sl->mode = SL_DEFAULT;
	if (mode != NULL) {
		if (strcmp(mode, "emacs") == 0)
			sl->mode = SL_EMACS;
		else if (strcmp(mode, "vi") == 0)
			sl->mode = SL_VI;
	}

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

void
sl_mode(struct slackline *sl, enum mode mode)
{
	sl->mode = mode;
}

size_t
sl_postobyte(struct slackline *sl, size_t pos)
{
	char *ptr = &sl->buf[0];
	size_t byte = 0;

	for (;pos > 0; pos--)
		byte += grapheme_next_character_break_utf8(ptr+byte,
		    sl->blen-byte);

	return byte;
}

char *
sl_postoptr(struct slackline *sl, size_t pos)
{
	return &sl->buf[sl_postobyte(sl, pos)];
}

void
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

void
sl_move(struct slackline *sl, enum direction dir)
{
	switch (dir) {
	case HOME:
		sl->bcur = sl->rcur = 0;
		sl->ptr = sl->buf;
		return;
	case END:
		sl->rcur = sl->rlen;
		break;
	case RIGHT:
		if (sl->rcur < sl->rlen)
			sl->rcur++;
		break;
	case LEFT:
		if (sl->rcur > 0)
			sl->rcur--;
		break;
	}

	sl->bcur = sl_postobyte(sl, sl->rcur);
	sl->ptr = sl->buf + sl->bcur;
}

static void
sl_default(struct slackline *sl, int key)
{
	switch (key) {
	case ESC_KEY:
		sl->esc = ESC;
		break;
	case CTRL_U:
		sl_reset(sl);
		break;
	case CTRL_W:
		while (sl->rcur != 0 && isspace((unsigned char) *(sl->ptr-1)))
			sl_backspace(sl);
		while (sl->rcur != 0 && !isspace((unsigned char) *(sl->ptr-1)))
			sl_backspace(sl);
		break;
	case BACKSPACE:
	case VT_BACKSPACE:
		sl_backspace(sl);
		break;
	default:
		break;
	}
}

static int
sl_esc(struct slackline *sl, int key)
{
	/* handle escape sequences */
	switch (sl->esc) {
	case ESC_NONE:
		break;
	case ESC:
		sl->esc = key == '[' ? ESC_BRACKET : ESC_NONE;
		return 1;
	case ESC_BRACKET:
		switch (key) {
		case 'A':	/* up    */
		case 'B':	/* down  */
			break;
		case 'C':	/* right */
			sl_move(sl, RIGHT);
			break;
		case 'D':	/* left */
			sl_move(sl, LEFT);
			break;
		case 'H':	/* Home  */
			sl_move(sl, HOME);
			break;
		case 'F':	/* End   */
			sl_move(sl, END);
			break;
		case 'P':	/* delete */
			if (sl->rcur == sl->rlen)
				break;
			sl_move(sl, RIGHT);
			sl_backspace(sl);
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
			return 1;
		}
		sl->esc = ESC_NONE;
		return 1;
	case ESC_BRACKET_NUM:
		switch(key) {
		case '~':
			switch(sl->nummod) {
			case '1':	/* Home */
			case '7':
				sl_move(sl, HOME);
				break;
			case '4':	/* End */
			case '8':
				sl_move(sl, END);
				break;
			case '3':	/* Delete */
				if (sl->rcur == sl->rlen)
					break;
				sl_move(sl, RIGHT);
				sl_backspace(sl);
				break;
			}
			sl->esc = ESC_NONE;
			return 1;
		}
	}

	return 0;
}

int
sl_keystroke(struct slackline *sl, int key)
{
	uint_least32_t cp;

	if (sl == NULL || sl->rlen < sl->rcur)
		return -1;
	if (sl_esc(sl, key))
		return 0;
	if (!iscntrl((unsigned char) key))
		goto compose;

	switch (sl->mode) {
	case SL_DEFAULT:
		sl_default(sl, key);
		break;
	case SL_EMACS:
		sl_default(sl, key);
		sl_emacs(sl, key);
		break;
	case SL_VI:
		/* TODO: implement vi-mode */
		break;
	}
	return 0;

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
