/*
 * memutil.h
 *   some macros for common operations with strings and memory in general
 *
 * $Id: memutil.h,v 1.1 2002/01/04 02:56:22 ite Exp $
 */
/*
 * Copyright (C) 1999, 2000, 2001 Eggheads Development Team
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

#ifndef _EGG_MEMUTIL_H
#define _EGG_MEMUTIL_H

#include <stdlib.h>
#include <string.h>

#define free_null(ptr)                                                  \
  if (ptr) {                                                            \
    free(ptr);                                                          \
    ptr = NULL;                                                         \
  }

#define malloc_strcpy(target, entry)                                    \
do {                                                                    \
  (target) = malloc(strlen(entry) + 1);                                 \
  strcpy((target), (entry));                                            \
} while (0)

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

/* This macro copies (_len - 1) bytes from _source to _target. The
 * target string is NULL-terminated.
 */
#define strncpyz(_target, _source, _len)        do {                    \
	strncpy((_target), (_source), (_len) - 1);                      \
	(_target)[(_len) - 1] = 0;                                      \
} while (0)

#endif /* _EGG_MEMUTIL_H */
