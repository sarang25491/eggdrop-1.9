/*
 * mempool.h --
 */
/*
 * Copyright (C) 2002, 2003 Eggheads Development Team
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
 * $Id: mempool.h,v 1.3 2003/01/02 21:33:14 wcc Exp $
 */

#ifndef _EGG_MEMPOOL_H
#define _EGG_MEMPOOL_H

typedef struct mempool_b {
	int chunk_size;
	int nchunks;
	char *free_chunk_ptr;
	char *pools;
} mempool_t;

mempool_t *mempool_create(mempool_t *pool, int nchunks, int chunk_size);
int mempool_destroy(mempool_t *pool);
int mempool_grow(mempool_t *pool, int nchunks);
void *mempool_get_chunk(mempool_t *pool);
int mempool_free_chunk(mempool_t *pool, void *chunk);

#endif				/* _EGG_MEMPOOL_H */
