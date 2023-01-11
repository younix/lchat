#ifndef SLACKLINE_INTERNALS_H
#define SLACKLINE_INTERNALS_H

#include "slackline.h"

enum direction {LEFT, RIGHT, HOME, END};

enum {
	CTRL_A = 1,
	CTRL_B = 2,
	CTRL_D = 4,
	CTRL_E = 5,
	CTRL_F = 6,
	CTRL_K = 11,
	CTRL_U = 21,
	CTRL_T = 20,
	CTRL_W = 23,

	BACKSPACE = 127,
	VT_BACKSPACE = 8,

	ESC_KEY = 27
};

size_t sl_postobyte(struct slackline *sl, size_t pos);
char *sl_postoptr(struct slackline *sl, size_t pos);
void sl_backspace(struct slackline *sl);
void sl_move(struct slackline *sl, enum direction dir);
void sl_emacs(struct slackline *sl, int key);

#endif
