/* string.c: functions for string manipulation
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
static const char rcsid[] = "$Id: string.c,v 1.2 2004/10/04 15:48:29 stdarg Exp $";
#endif

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include <eggdrop/eggdrop.h>				/* egg_return_val_if_fail	*/
#include <eggdrop/string.h>				/* prototypes			*/

int egg_get_word(const unsigned char *text, const char **next, char **word)
{
	int len;
	const unsigned char *ptr;

	if (!word || !text) {
		if (next) *next = NULL;
		if (word) *word = NULL;
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

int egg_get_arg(const unsigned char *text, const char **next, char **arg)
{
	int len = 0, max = 64, inquote = 0, insingle = 0, n, done;

	if (!arg || !text) {
		if (next) *next = NULL;
		if (arg) *arg = NULL;
		return(-1);
	}
	while (isspace(*text)) text++;
	if (!*text) {
		if (next) *next = NULL;
		*arg = NULL;
		return(-1);
	}
	*arg = malloc(max+10);

	done = 0;
	while (!done) {
		n = strcspn(text, "\"\\ \t'");
		if (n) {
			/* Guarantee there is always some unused space left. */
			if (n + len + 10 > max) {
				max = 2*max + n + 10;
				*arg = realloc(*arg, max+1);
			}
			memcpy(*arg + len, text, n);
			len += n;
		}
		text += n;
		switch (*text) {
			case '"':
				if (insingle) (*arg)[len++] = '"';
				else if (inquote) inquote = 0;
				else inquote = 1;
				text++;
				break;
			case '\'':
				if (inquote) (*arg)[len++] = '\'';
				else if (insingle) insingle = 0;
				else insingle = 1;
				text++;
				break;
			case ' ':
			case '\t':
				/* If we're *not* in a quoted string, this is
				 * the end. Otherwise, just copy it. */
				if (!insingle && !inquote) done = 1;
				else (*arg)[len++] = *text;
				text++;
				break;
			case '\\':
				text++;
				if (insingle) (*arg)[len++] = '\\';
				else if (!inquote) (*arg)[len++] = *text++;
				else if (*text == '\\' || *text == '"') (*arg)[len++] = *text++;
				else (*arg)[len++] = '\\';
				break;
			default:
				done = 1;
				break;
		}
	}
	while (*text && isspace(*text)) text++;
	if (next) *next = text;
	(*arg)[len] = 0;
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

int egg_get_args(const char *text, const char **next, char **arg, ...)
{
	va_list args;
	int nargs = 0;

	va_start(args, arg);
	while (arg) {
		if (egg_get_arg(text, &text, arg)) break;
		nargs++;
		arg = va_arg(args, char **);
	}
	while (arg) {
		*arg = NULL;
		arg = va_arg(args, char **);
	}
	va_end(args);
	if (next) *next = text;
	return(nargs);
}

int egg_get_word_array(const char *text, const char **next, char **word, int nwords)
{
	int i;

	for (i = 0; i < nwords; i++) {
		if (egg_get_word(text, &text, word+i)) break;
	}
	while (i < nwords) word[i++] = NULL;
	if (next) *next = text;
	return(i);
}

int egg_get_arg_array(const char *text, const char **next, char **args, int nargs)
{
	int i;

	for (i = 0; i < nargs; i++) {
		if (egg_get_arg(text, &text, args+i)) break;
	}
	while (i < nargs) args[i++] = NULL;
	if (next) *next = text;
	return(i);
}

int egg_free_word_array(char **words, int nwords)
{
	int i;

	for (i = 0; i < nwords; i++) {
		if (words[i]) free(words[i]);
		words[i] = NULL;
	}
	return(0);
}

int egg_free_arg_array(char **args, int nargs)
{
	return egg_free_word_array(args, nargs);
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

int
str_ends_with(const char *text, const char *str)
{
	int len1, len2;

	egg_return_val_if_fail (text != NULL, 0);
	egg_return_val_if_fail (str != NULL, 0);

	len1 = strlen (text);
	len2 = strlen (str);

	if (len2 > len1)
		return 0;

	return (strcmp (text + (len1 - len2), str) == 0);
}

int
str_starts_with(const char *text, const char *str)
{
	egg_return_val_if_fail (text != NULL, 0);
	egg_return_val_if_fail (str != NULL, 0);

	while (*text && *str && *text++ == *str++)
		;

	return (*str);
}

void str_tolower(char *str)
{
	while (*str) {
		*str = tolower(*str);
		str++;
	}
}
