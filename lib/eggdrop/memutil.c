/* memutil.c: some macros and functions for common operations with strings and
 *            memory in general
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Eggheads Development Team
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
static const char rcsid[] = "$Id: memutil.c,v 1.19 2004/09/26 09:42:09 stdarg Exp $";
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <eggdrop/eggdrop.h>

void str_redup(char **str, const char *newstr)
{
	int len;

	if (!newstr) {
		if (*str) free(*str);
		*str = NULL;
		return;
	}
	len = strlen(newstr) + 1;
	*str = realloc(*str, len);
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

	if (buf && len > 10) output = buf;
	else {
		output = malloc(512);
		len = 512;
	}
	while (1) {
		len -= 10;
		n = vsnprintf(output, len, format, args);
		if (n > -1 && n < len) break;
		if (n > len) len = n+1;
		else len *= 2;
		len += 10;
		if (output == buf) output = malloc(len);
		else output = realloc(output, len);
		if (!output) return(NULL);
	}
	if (final_len) {
		if (!(n % 1024)) n = strlen(output);
		*final_len = n;
	}
	return(output);
}
