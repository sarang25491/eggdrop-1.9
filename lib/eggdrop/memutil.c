/*
 * memutil.c --
 *
 *	some macros and functions for common operations with strings and
 *	memory in general.
 */
/*
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Eggheads Development Team
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
static const char rcsid[] = "$Id: memutil.c,v 1.14 2003/03/06 07:56:53 tothwolf Exp $";
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include "memutil.h"

int my_strcpy(register char *a, register char *b)
{
  register char *c = b;

  while (*b)
    *a++ = *b++;
  *a = *b;
  return b - c;
}

/* Split first word off of rest and put it in first */
void splitc(char *first, char *rest, char divider)
{
  char *p = strchr(rest, divider);

  if (p == NULL) {
    if (first != rest && first)
      first[0] = 0;
    return;
  }
  *p = 0;
  if (first != NULL)
    strcpy(first, rest);
  if (first != rest)
    /*    In most circumstances, strcpy with src and dst being the same buffer
     *  can produce undefined results. We're safe here, as the src is
     *  guaranteed to be at least 2 bytes higher in memory than dest. <Cybah>
     */
    strcpy(rest, p + 1);
}

/*    As above, but lets you specify the 'max' number of bytes (EXCLUDING the
 * terminating null).
 *
 * Example of use:
 *
 * char buf[HANDLEN + 1];
 *
 * splitcn(buf, input, "@", HANDLEN);
 *
 * <Cybah>
 */
void splitcn(char *first, char *rest, char divider, size_t max)
{
  char *p = strchr(rest, divider);

  if (p == NULL) {
    if (first != rest && first)
      first[0] = 0;
    return;
  }
  *p = 0;
  if (first != NULL)
    strlcpy(first, rest, max);
  if (first != rest)
    /*    In most circumstances, strcpy with src and dst being the same buffer
     *  can produce undefined results. We're safe here, as the src is
     *  guaranteed to be at least 2 bytes higher in memory than dest. <Cybah>
     */
    strcpy(rest, p + 1);
}

char *newsplit(char **rest)
{
  register char *o, *r;

  if (!rest)
    return *rest = "";
  o = *rest;
  while (*o == ' ')
    o++;
  r = o;
  while (*o && (*o != ' '))
    o++;
  if (*o)
    *o++ = 0;
  *rest = o;
  return r;
}

/* Return an allocated buffer which contains a copy of the string
 * 'str', with all 'div' characters escaped by 'mask'. 'mask'
 * characters are escaped too.
 *
 * Remember to free the returned memory block.
 */
char *str_escape(const char *str, const char div, const char mask)
{
  const int      len = strlen(str);
  int            buflen = (2 * len), blen = 0;
  char          *buf = malloc(buflen + 1), *b = buf;
  const char    *s;

  if (!buf)
    return NULL;
  for (s = str; *s; s++) {
    /* Resize buffer. */
    if ((buflen - blen) <= 3) {
      buflen = (buflen * 2);
      buf = realloc(buf, buflen + 1);
      if (!buf)
        return NULL;
      b = buf + blen;
    }

    if (*s == div || *s == mask) {
      sprintf(b, "%c%02x", mask, *s);
      b += 3;
      blen += 3;
    } else {
      *(b++) = *s;
      blen++;
    }
  }
  *b = 0;
  return buf;
}

/* Search for a certain character 'div' in the string 'str', while
 * ignoring escaped characters prefixed with 'mask'.
 *
 * The string
 *
 *   "\\3a\\5c i am funny \\3a):further text\\5c):oink"
 *
 * as str, '\\' as mask and ':' as div would change the str buffer
 * to
 *
 *   ":\\ i am funny :)"
 *
 * and return a pointer to "further text\\5c):oink".
 *
 * NOTE: If you look carefully, you'll notice that strchr_unescape()
 *       behaves differently than strchr().
 */
char *strchr_unescape(char *str, const char div, register const char esc_char)
{
  char           buf[3];
  register char *s, *p;

  buf[2] = 0;
  for (s = p = str; *s; s++, p++) {
    if (*s == esc_char) {       /* Found escape character.              */
      /* Convert code to character. */
      buf[0] = s[1], buf[1] = s[2];
      *p = (unsigned char) strtol(buf, NULL, 16);
      s += 2;
    } else if (*s == div) {
      *p = *s = 0;
      return (s + 1);           /* Found searched for character.        */
    } else
      *p = *s;
  }
  *p = 0;
  return NULL;
}

/* Remove space characters from beginning and end of string 
 * (more efficent by Fred1)
 * (even more by stdarg)
 */
void rmspace(char *s)
{
  char *p;
  int len;

  /* Wipe end of string */
  for (p = s + strlen(s) - 1; (p >= s) && isspace(*p); p--) ; /* empty */

  len = (p+1) - s;
  *(p + 1) = 0;

  for (p = s; isspace(*p); p++) ; /* empty */

  if (p != s) {
    /* strcpy() shouldn't be used to copy overlapping strings */
    len -= (p - s);
    memmove(s, p, len);
    s[len] = 0;
  }
}

void str_redup(char **str, const char *newstr)
{
	int len;

	if (!newstr) {
		if (*str) free(*str);
		*str = NULL;
		return;
	}
	len = strlen(newstr)+1;
	*str = (char *)realloc(*str, len);
	memcpy(*str, newstr, len);
}

char *egg_mprintf(const char *format, ...)
{
	va_list args;
	char *ptr;

	va_start(args, format);
	ptr = egg_mvsprintf(NULL, 0, NULL, format, args);
	va_end(args);
	return(ptr);
}

char *egg_msprintf(char *buf, int len, int *final_len, const char *format, ...)
{
	va_list args;
	char *ptr;

	va_start(args, format);
	ptr = egg_mvsprintf(buf, len, final_len, format, args);
	va_end(args);
	return(ptr);
}

char *egg_mvsprintf(char *buf, int len, int *final_len, const char *format, va_list args)
{
	char *output;
	int n;

	if (buf && len > 10) {
		output = buf;
	}
	else {
		output = (char *)malloc(512);
		len = 512;
	}
	while (1) {
		len -= 10;
		n = vsnprintf(output, len, format, args);
		if (n > -1 && n < len) break;
		if (n > len) len = n+1;
		else len *= 2;
		len += 10;
		if (output == buf) output = (char *)malloc(len);
		else output = (char *)realloc(output, len);
		if (!output) return(NULL);
	}
	if (final_len) {
		if (!(n % 1024)) n = strlen(output);
		*final_len = n;
	}
	return(output);
}
