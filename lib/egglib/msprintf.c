/*
 * msprintf.c --
 */
/*
 * Copyright (C) 2002, 2003, 2004 Eggheads Development Team
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
static const char rcsid[] = "$Id: msprintf.c,v 1.5 2003/12/11 00:49:10 wcc Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

char *msprintf(char *format, ...)
{
	char *output;
	int n, len;
	va_list args;

	output = (char *)malloc(128);
	len = 128;
	while (1) {
		va_start(args, format);
		n = vsnprintf(output, len, format, args);
		va_end(args);
		if (n > -1 && n < len) return(output);
		if (n > len) len = n+1;
		else len *= 2;
		output = (char *)realloc(output, len);
	}
	return(output);
}
