/* eggstring.c: functions for string manipulation
 *
 * Copyright (C) 2003, 2004 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef lint
static const char rcsid[] = "$Id: eggstring.c,v 1.4 2003/12/17 07:39:14 wcc Exp $";
#endif

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "eggstring.h"

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

int egg_get_word_array(const char *text, const char **next, char **word, int nwords)
{
	int i, j;

	for (i = 0; i < nwords; i++) {
		if (egg_get_word(text, &text, word+i)) break;
	}
	for (j = i; i < nwords; j++) {
		word[j] = NULL;
	}
	if (next) *next = text;
	return(i);
}

int egg_free_word_array(char **word, int nwords)
{
	int i;

	for (i = 0; i < nwords; i++) {
		if (word[i]) free(word[i]);
		word[i] = NULL;
	}
	return(0);
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
