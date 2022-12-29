#ifndef SLACKLINE_INTERNALS_H
#define SLACKLINE_INTERNALS_H

#include "slackline.h"

enum direction {LEFT, RIGHT, HOME, END};

size_t sl_postobyte(struct slackline *sl, size_t pos);
char *sl_postoptr(struct slackline *sl, size_t pos);
void sl_backspace(struct slackline *sl);
void sl_move(struct slackline *sl, enum direction dir);
void sl_emacs(struct slackline *sl, int key);

#endif
