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
	case ESC_KEY:
		sl->esc = ESC;
		break;
	case CTRL_A:
		sl_move(sl, HOME);
		break;
	case CTRL_B:
		sl_move(sl, LEFT);
		break;
	case CTRL_D:
		if (sl->rcur < sl->rlen) {
			sl_move(sl, RIGHT);
			sl_backspace(sl);
		} else {
			exit(EXIT_SUCCESS);
		}
		break;
	case CTRL_E:
		sl_move(sl, END);
		break;
	case CTRL_F:
		sl_move(sl, RIGHT);
		break;
	case CTRL_K:
		for (int i = sl->rlen - sl->rcur; i > 0; --i) {
			sl_move(sl, RIGHT);
			sl_backspace(sl);
		}
		break;
	case CTRL_T:
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
