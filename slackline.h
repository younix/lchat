#ifndef SLACKLIINE_H
#define SLACKLIINE_H

/*
 * +-+-+-+-+-+
 * |c|c|c|0|0|
 * +-+-+-+-+-+
 *      ^   ^
 *     len  bufsize
 */

struct slackline {
	char *buf;
	size_t bufsize;
	size_t len;
	size_t cur;
};

struct slackline * sl_init(void);
void sl_free(struct slackline *);
int sl_keystroke(struct slackline *sl, int key);

#endif
