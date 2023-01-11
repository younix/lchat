#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>

#include "slackline.h"
#include "slackline_internals.h"

void
sl_emacs(struct slackline *sl, int key)
{
	char tmp;

	switch (key) {
	case ESC_KEY:
		sl->esc = ESC;
		break;
	case CTRL_A:	/* start of line */
		sl_move(sl, HOME);
		break;
	case CTRL_B:	 /* previous char */
		sl_move(sl, LEFT);
		break;
	case CTRL_D:	/* delete char in front of the cursor or exit */
		if (sl->rcur < sl->rlen) {
			sl_move(sl, RIGHT);
			sl_backspace(sl);
		} else {
			exit(EXIT_SUCCESS);
		}
		break;
	case CTRL_E:	/* end of line */
		sl_move(sl, END);
		break;
	case CTRL_F:	/* next char */
		sl_move(sl, RIGHT);
		break;
	case CTRL_K:	/* delete line from cursor to end */
		for (int i = sl->rlen - sl->rcur; i > 0; --i) {
			sl_move(sl, RIGHT);
			sl_backspace(sl);
		}
		break;
	case CTRL_T:	/* swap last two chars */
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
