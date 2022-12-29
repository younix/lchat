#ifndef SLACKLIINE_H
#define SLACKLIINE_H

enum esc_seq {ESC_NONE, ESC, ESC_BRACKET, ESC_BRACKET_NUM};
enum mode {SL_DEFAULT, SL_EMACS, SL_VI};

struct slackline {
	/* buffer */
	char *buf;
	char *ptr;	/* ptr of cursor */
	char *last;	/* ptr of last byte of string */
	size_t bufsize;

	/* byte positions */
	size_t bcur;	/* first byte of the rune of the cursor */
	size_t blen;	/* amount of bytes of current string */

	/* rune positions */
	size_t rcur;	/* cursor */
	size_t rlen;	/* amount of runes */

	enum esc_seq esc;
	char nummod;

	/* UTF-8 handling */
	char ubuf[6];	/* UTF-8 buffer */
	size_t ubuf_len;

	enum mode mode;
};

struct slackline *sl_init(void);
void sl_free(struct slackline *sl);
void sl_reset(struct slackline *sl);
int sl_keystroke(struct slackline *sl, int key);
void sl_mode(struct slackline *sl, enum mode mode);

#endif
