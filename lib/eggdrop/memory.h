/* memory.h: header for memory.c
 *
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
 *
 * $Id: memory.h,v 1.3 2004/09/26 09:42:09 stdarg Exp $
 */
#ifndef _EGG_MEMORY_H
#define _EGG_MEMORY_H

#include <stdlib.h>			/* size_t	*/

#define MEM_DEBUG_NONE			(1 << 0)
#define MEM_DEBUG_PRINT_EACH_CALL	(1 << 1)
#define MEM_DEBUG_PRINT_ERRORS		(1 << 2)

#if 0

/* calloc */
#	ifdef calloc
#		undef calloc
#	endif
#	define calloc(nmemb, size) mem_dbg_calloc(nmemb, size, __FILE__, __LINE__, __FUNCTION__)

/* malloc */
#	ifdef malloc
#		undef malloc
#	endif
#	define malloc(size) mem_dbg_alloc(size, __FILE__, __LINE__, __FUNCTION__)

/* realloc */
#	ifdef realloc
#		undef realloc
#	endif
#	define realloc(ptr, size) mem_dbg_realloc(ptr, size, __FILE__, __LINE__, __FUNCTION__)

/* free */
#	ifdef free
#		undef free
#	endif
#	define free(ptr) mem_dbg_free(ptr, __FILE__, __LINE__, __FUNCTION__)

/* strdup */
#	ifdef strdup
#		undef strdup
#	endif
#	define strdup(ptr) mem_dbg_strdup(ptr, __FILE__, __LINE__, __FUNCTION__)

#endif

char *mem_dbg_strdup(const char *str, const char *file, int line, const char *func);
void *mem_dbg_calloc(size_t nmemb, size_t size,  const char *file, int line, const char *func);
void *mem_dbg_alloc(size_t size, const char *file, int line, const char *func);
void *mem_dbg_realloc(void *ptr, size_t size, const char *file, int line, const char *func);
void mem_dbg_free(void *ptr, const char *file, int line, const char *func);

void mem_dbg_set_options(int options);
void mem_dbg_stats();

#endif /* _EGG_MEMORY_H */

