#ifndef _REGEXP_H_
#define _REGEXP_H_

#define REGEXP_SENSITIVE	1
#define REGEXP_FIRST		2

char *regsub(char *expr, char *text, char *sub, int case_sensitive);

#endif
