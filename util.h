#ifndef _UTIL_H_
#define _UTIL_H_

void die(const char *fmt, ...);
bool bell_match(const char *str, const char *regex_file);
void set_title(const char *term, const char *title);

#endif
