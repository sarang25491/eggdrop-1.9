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
