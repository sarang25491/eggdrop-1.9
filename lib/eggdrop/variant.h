/* variant.h: header for variant.c
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
 * $Id: variant.h,v 1.2 2004/06/30 17:07:20 wingman Exp $
 */
#ifndef _EGG_VARIANT_H
#define _EGG_VARIANT_H

#include <sys/time.h>				/* time_t		*/

/* Is there a standard .h file with this? */
#ifndef MAX_INT
#	define MAX_INT ((int)(((unsigned) ~0)>>1))
#endif

#ifndef MIN_INT
#	define MIN_INT (~MAX_INT)
#endif

#ifndef MAX_LONG
#	define MAX_LONG ((long)(((unsigned) ~0L)>>1))
#endif

#ifndef MIN_LONG
#	define MIN_LONG (~MAX_LONG)
#endif

/* Any better solution for this? */
#ifndef MIN_FLOAT
#	define MIN_FLOAT   ((float)-1.0e36)
#endif

#ifndef MAX_FLOAT
#	define MAX_FLOAT   ((float)1.0e36)
#endif

#ifndef MAX_DOUBLE
#	define MAX_DOUBLE (double)MAX_FLOAT
#endif

#ifndef MIN_DOUBLE
#	define MIN_DOUBLE (double)MIN_FLOAT
#endif

typedef enum {
	VARIANT_STRING = 0,
	VARIANT_INT,
	VARIANT_BOOL,
	VARIANT_TIMESTAMP
} variant_type_t;

typedef struct {
	union {
		char *s_val;	/* string       */
		int i_val;	/* int          */
		int b_val;	/* bool		*/
		time_t ts_val;	/* timestamp	*/	
	} value;
	variant_type_t type;
} variant_t;

void variant_set_int(variant_t *data, int value);
int variant_get_int(variant_t *data, int nil);

void variant_set_str(variant_t *data, const char *str);
const char *variant_get_str(variant_t *data, const char *nil);

int variant_get_bool(variant_t *data, int nil);
void variant_set_bool(variant_t *data, int value);

time_t variant_get_ts(variant_t *data, time_t nil);
void variant_set_ts(variant_t *data, time_t value);
	

#endif /* _EGG_VARIANT_H */
