#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>

#include "slackline_internals.h"

void
sl_emacs(struct slackline *sl, int key)
{
	char tmp;

	switch (key) {
	case 27:	/* Escape */
		sl->esc = ESC;
		break;
	case 1: /* ctrl+a -- start of line */
		sl_move(sl, HOME);
		break;
	case 2: /* ctrl+b -- previous char */
		sl_move(sl, LEFT);
		break;
	case 4: /* ctrl+d -- delete char in front of the cursor or exit */
		if (sl->rcur < sl->rlen) {
			sl_move(sl, RIGHT);
			sl_backspace(sl);
		} else {
			exit(EXIT_SUCCESS);
		}
		break;
	case 5: /* ctrl+e -- end of line */
		sl_move(sl, END);
		break;
	case 6: /* ctrl+f -- next char */
		sl_move(sl, RIGHT);
		break;
	case 11: /* ctrl+k -- delete line from cursor to end */
		for (int i = sl->rlen - sl->rcur; i > 0; --i) {
			sl_move(sl, RIGHT);
			sl_backspace(sl);
		}
		break;
	case 20: /* ctrl+t -- swap last two chars */
		if (sl->rcur >= 2) {
			tmp = *sl_postoptr(sl, sl->rcur-1);
			sl->buf[sl->rcur-1] = *sl_postoptr(sl, sl->rcur-2);
			sl->buf[sl->rcur-2] = tmp;
		}
		break;
	default:
		break;
	}
}
