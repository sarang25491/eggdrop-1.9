/*
 * eggstring.c - some functions for string manipulation
 */

#if HAVE_CONFIG_H
	#include <config.h>
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

int egg_get_word(const char *text, const char **next, char **word)
{
	int len;
	const char *ptr;

	if (!word) return(-1);
	if (!text) {
		if (next) *next = NULL;
		*word = NULL;
		return(-1);
	}
	while (isspace(*text)) text++;
	ptr = text;
	while (*ptr && !isspace(*ptr)) ptr++;
	if (next) *next = ptr;
	len = ptr - text;
	if (!len) {
		*word = NULL;
		return(-1);
	}
	*word = malloc(len + 1);
	memcpy(*word, text, len);
	(*word)[len] = 0;
	return(0);
}

/* Syntax is:
 * nwords = egg_get_words(text, &next, &word1, &word2, &word3, NULL);
 * Returns the number of words successfully retrieved.
 * Words must be free()'d when done. */
int egg_get_words(const char *text, const char **next, char **word, ...)
{
	va_list args;
	int nwords = 0;

	va_start(args, word);
	while (word) {
		if (egg_get_word(text, &text, word)) break;
		nwords++;
		word = va_arg(args, char **);
	}
	while (word) {
		*word = NULL;
		word = va_arg(args, char **);
	}
	va_end(args);
	if (next) *next = text;
	return(nwords);
}

/* Append string to a static buffer until full. */
void egg_append_static_str(char **dest, int *remaining, const char *src)
{
	int len;

	if (!src || *remaining <= 0) return;

	len = strlen(src);
	if (len > *remaining) len = *remaining;

	memmove(*dest, src, len);
	*remaining -= len;
	*dest += len;
}

/* Append string to a dynamic buffer with reallocation. */
void egg_append_str(char **dest, int *cur, int *max, const char *src)
{
	int len;

	if (!src) return;

	len = strlen(src);
	if (*cur + len + 10 > *max) {
		*max += len + 128;
		*dest = realloc(*dest, *max+1);
	}
	memmove(*dest + *cur, src, len);
	*cur += len;
}
