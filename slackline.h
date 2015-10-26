#ifndef SLACKLIINE_H
#define SLACKLIINE_H

#include <stdbool.h>

enum esc_seq {ESC_NONE, ESC, ESC_BRACKET};

struct slackline {
	char *buf;
	size_t bufsize;
	size_t len;
	size_t cur;
	enum esc_seq esc;
};

struct slackline *sl_init(void);
void sl_free(struct slackline *sl);
void sl_reset(struct slackline *sl);
int sl_keystroke(struct slackline *sl, int key);

#endif
