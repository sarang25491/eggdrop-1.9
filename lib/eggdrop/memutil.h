/*
 * memutil.h --
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
/*
 * $Id: memutil.h,v 1.12 2003/01/02 21:33:13 wcc Exp $
 */

#ifndef _EGG_MEMUTIL_H
#define _EGG_MEMUTIL_H

#include <stdlib.h>
#include <string.h>
#include <eggdrop/common.h>

BEGIN_C_DECLS

#define free_null(ptr)                                                  \
  if (ptr) {                                                            \
    free(ptr);                                                          \
    ptr = NULL;                                                         \
  }

/* Copy entry to target -- Uses dynamic memory allocation, which
 * means you'll eventually have to free the memory again. 'target'
 * will be overwritten.
 */
#define realloc_strcpy(target, entry)                                   \
do {                                                                    \
  if (entry) {                                                          \
    (target) = realloc((target), strlen(entry) + 1);                    \
    strcpy((target), (entry));                                          \
  } else                                                                \
    free_null(target);                                                  \
} while (0)

extern int egg_strcatn(char *, const char *, size_t);
extern int my_strcpy(register char *, register char *);

extern void splitc(char *, char *, char);
extern void splitcn(char *, char *, char, size_t);
extern char *newsplit(char **);

extern char *str_escape(const char *, const char, const char);
extern char *strchr_unescape(char *, const char, register const char);
#define str_unescape(str, esc_char) strchr_unescape(str, 0, esc_char)

extern void rmspace(char *);

extern void str_redup(char **str, const char *newstr);

extern char *egg_msprintf(char *buf, int len, int *final_len, const char *format, ...);
extern char *egg_mvsprintf(char *buf, int len, int *final_len, const char *format, va_list args);

END_C_DECLS

#endif				/* !_EGG_MEMUTIL_H */
