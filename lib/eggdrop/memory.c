/* memory.c: memory tracking functions 
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
 */

#ifndef lint
static const char rcsid[] = "$Id: memory.c,v 1.7 2004/06/25 17:44:03 darko Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#define _EGG_MEMORY_H /* don't include memory.h */
#include <eggdrop/eggdrop.h>

#define MEM_DEBUG_NONE               (1 << 0)
#define MEM_DEBUG_PRINT_EACH_CALL    (1 << 1)
#define MEM_DEBUG_PRINT_ERRORS       (1 << 2)

typedef struct {
	void *ptr;
	size_t size;
	const char *file;
	const char *func;
	int line;
} mem_block_t;

struct {
	int length;
	size_t size;
	mem_block_t *blocks;
} mem = { 0, 0, NULL };

static int options = MEM_DEBUG_NONE;

static int mem_dbg_trace_alloc(void *ptr, size_t size, const char *file, int line, const char *func);
static int mem_dbg_trace_realloc(void *newptr, void *oldptr, size_t size, const char *file, int line, const char *func);
static int mem_dbg_trace_free(void *ptr, const char *file, int line, const char *func);

void mem_dbg_set_options(int opts)
{
	options = opts;
}

void *mem_dbg_calloc(size_t nmemb, size_t size, const char *file, int line, const char *func)
{
	void *ptr;

	/* invalid size */
	if (size <= 0 || nmemb <= 0) {
		if (options & MEM_DEBUG_PRINT_ERRORS)
			fprintf(stderr, "*** Failed calloc call at %s in line %i (%s): negative (%i) size or no (%i) requested.\n",
					file, line, func, size, nmemb);
		return NULL;
	}

	ptr = calloc(nmemb, size);

	if (!mem_dbg_trace_alloc(ptr, (size * nmemb), file, line, func))
		/*return NULL*/;

	/* failed to allocate bytes */
	if (ptr == NULL) {
		if (options & MEM_DEBUG_PRINT_ERRORS)
			fprintf(stderr, "*** Failed calloc call at %s in line %i (%s): NULL returned.\n",
					file, line, func);
		return NULL;
	} 

	return ptr;
}

void *mem_dbg_alloc(size_t size, const char *file, int line, const char *func)
{
	void *ptr;

	/* invalid size */
	if (size <= 0) {
		if (options & MEM_DEBUG_PRINT_ERRORS)
			fprintf(stderr, "*** Failed malloc call at %s in line %i (%s): negative (%i) size requested.\n",
					file, line, func, size);
		return NULL;
	}

	ptr = malloc(size);

	if (!mem_dbg_trace_alloc(ptr, size, file, line, func))
		/*return NULL*/;
		
	/* failed to allocate bytes */
	if (ptr == NULL) {
		if (options & MEM_DEBUG_PRINT_ERRORS)
			fprintf(stderr, "*** Failed malloc call at %s in line %i (%s): NULL returned.\n",
					file, line, func);
		return NULL;
	}	

	return ptr;
}

char *mem_dbg_strdup(const char *str, const char *file, int line, const char *func)
{
	size_t size;
	void *ptr;

	if (file == NULL || !*str)
		return NULL;

	size = strlen(str) + 1;

	ptr = mem_dbg_alloc(size, file, line, func);
	if (ptr == NULL)
		return NULL;
	memcpy(ptr, str, size);
	return ptr;
}

void mem_dbg_free(void *ptr, const char *file, int line, const char *func)
{
	/* There are rumors that free(NULL) is invalid on some OS, but we
	 * allow it */
	if (ptr == NULL)
		return;

	if (!mem_dbg_trace_free(ptr, file, line, func))
		/*return*/;

	free(ptr);
}

void *mem_dbg_realloc(void *ptr, size_t size, const char *file, int line, const char *func)
{
	void *newptr;
	
	/* if ptr is NULL realloc is equivalent to malloc */
	if (ptr == NULL)
		return mem_dbg_alloc(size, file, line, func);
									
	/* if size is 0 realloc is equivalent to free */
	if (size == 0) {
		mem_dbg_free(ptr, file, line, func);
		return NULL;
	}

	/* invalid size */
	if (size < 0) {
		if (options & MEM_DEBUG_PRINT_ERRORS)
			fprintf(stderr, "*** Failed realloc call at %s in line %i (%s): negative (%i) size requested.\n",
					file, line, func, size);
		return NULL;
	}

	newptr = realloc(ptr, size);

	if (!mem_dbg_trace_realloc(newptr, ptr, size, file, line, func))
		/*return NULL*/;

	/* failed to allocate bytes */
	if (newptr == NULL) {
		if (options & MEM_DEBUG_PRINT_ERRORS)
			fprintf(stderr, "*** Failed realloc call at %s in line %i (%s): NULL returned.\n",
					file, line, func);
		return NULL;
	}

	return newptr;
}

static int mem_dbg_trace_alloc(void *ptr, size_t size, const char *file, int line, const char *func)
{
	mem_block_t *block;
	
	/* resize it if necessary */
	if ((mem.length + 1) >= mem.size) {		
		mem.blocks = realloc(mem.blocks, sizeof(mem_block_t) * (mem.length + 25));
		if (mem.blocks == NULL)		
			return 0;
		mem.size += 25;			
	}
	
	block = &mem.blocks[mem.length++];
	
	if (options & MEM_DEBUG_PRINT_EACH_CALL)
		fprintf(stderr, "*** N %s:%i@%s: %i %p\n", file, line, func, size, ptr);

	block->ptr = ptr;
	block->size = size;
	block->file = file;
	block->line = line;
	block->func = func;
	
	return 1;
}

static int mem_dbg_trace_realloc(void *newptr, void *oldptr, size_t size, const char *file, int line, const char *func)
{
	int i;
	mem_block_t *block = NULL;
	
	for (i = 0; i < mem.length; i++) {
		block = &mem.blocks[i];
		if (block->ptr == oldptr)
			break;
		block = NULL;		
	}
	
	if (block == NULL) {
		if (options & MEM_DEBUG_PRINT_ERRORS)
			fprintf(stderr, "*** Invalid realloc call at %s in line %i (%s): Unknown pointer passed (%p).\n",
				file, line, func, oldptr);
		return 0;
	}
	
	if (options & MEM_DEBUG_PRINT_EACH_CALL)
		fprintf(stderr, "*** U %s:%i@%s: %i %p %p\n", file, line, func, size, newptr, oldptr);

	/* update block */
	block->ptr = newptr;
	block->size = size;
	block->file = file;
	block->line = line;
	block->func = func;
	
	return 1;
}

static int mem_dbg_trace_free(void *ptr, const char *file, int line, const char *func)
{
	int i;
	mem_block_t *block = NULL;
	
	for (i = 0; i < mem.length; i++) {
		block = &mem.blocks[i];
		if (block->ptr == ptr)
			break;
		block = NULL;		
	}
	
	if (block == NULL) {
		if (options & MEM_DEBUG_PRINT_ERRORS)
			fprintf(stderr, "Invalid free call at %s in line %i (%s): Unknown pointer passed (%p).\n",
					file, line, func, ptr);
		return 0;
	}
	
	if (options & MEM_DEBUG_PRINT_EACH_CALL)
		fprintf(stderr, "*** F %s:%i@%s: %i %p\n", file, line, func, block->size, ptr);

	/* overwrite with last entry, we don't care about ordered array */
	mem.blocks[i] = mem.blocks[--mem.length];
	
	/* shrink if necessary */
	if ((mem.size - mem.length) > 25) {		
		mem.blocks = realloc(mem.blocks, sizeof(mem_block_t) * mem.length);
		if (mem.blocks == NULL)		
			return 0;
		mem.size = mem.length;
	}
		
	return 1;
}

void mem_dbg_stats()
{
	int i;
	size_t size;
	char *data;
	mem_block_t *block;
	FILE *fd;
	size = 0;

	fd = fopen("memory.log", "w");
	if (fd == NULL)
		fd = stderr;

	fprintf(fd, "%-16s %-4s %-25s %-5s %-9s %s\n", "FILE", "LINE", "FUNC", "SIZE", "ADDR", "DATA");
	for (i = 0; i < mem.length; i++) {
		block = &mem.blocks[i];
		
		data = NULL;
		if (0 == strcmp(block->func, "read_name")
			|| 0 == strcmp(block->func, "read_text")
			|| 0 == strcmp(block->func, "xml_node_set_str")
			|| 0 == strcmp(block->func, "xml_node_set_int")
			|| 0 == strcmp(block->func, "str_redup")) {
			data = block->ptr;
		} else if (0 == strcmp(block->func, "bind_table_add") && block->line == 82) {
			data = ((bind_table_t *)block->ptr)->name;
		} else if (0 == strcmp(block->func, "xml_node_new")) {
			data = ((xml_node_t *)block->ptr)->name;
		}

		fprintf(fd, "%-16s %4i %-25s %5i %p %s\n", block->file, block->line, block->func, block->size, block->ptr, data);

		size += block->size;
	}

	fprintf(fd, "---\n");
	fprintf(fd, "%i blocks allocated.\n", mem.length);
	fprintf(fd, "%i bytes allocated.\n", size);

	if (fd != stderr)
		fclose(fd);
}
