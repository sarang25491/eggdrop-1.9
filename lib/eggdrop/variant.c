/* variant.c: variant, mutable datatype
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
static const char rcsid[] = "$Id: variant.c,v 1.1 2004/06/28 17:36:34 wingman Exp $";
#endif

#include <stdio.h>				/* snprintf		*/
#include <string.h>				/* strcmp		*/

#include <eggdrop/memory.h>			/* malloc		*/
#include <eggdrop/memutil.h>			/* str_redup		*/
#include <eggdrop/variant.h>			/* prototypes		*/

void variant_set_str(variant_t *target, const char *value)
{
	if (target->type == VARIANT_STRING) {
		str_redup(&target->value.s_val, value);
		return;
	}

	target->value.s_val = (value) ? strdup(value) : NULL;
	target->type = VARIANT_STRING;
}

const char *variant_get_str(variant_t *target, const char *nil)
{
	switch (target->type) {

		case (VARIANT_STRING):
		{
			return target->value.s_val;
		}

		case (VARIANT_INT):
		{
			char buf[32];
			snprintf(buf, sizeof(buf), "%d", target->value.i_val);
			variant_set_str(target, buf);
			return variant_get_str(target, buf);
		}
		
		case (VARIANT_BOOL):
		{
			variant_set_str(target, target->value.b_val ? "yes" : "no");
			return variant_get_str(target, NULL);
		}

		case (VARIANT_TIMESTAMP):
		{
			char buf[32];
			snprintf(buf, sizeof(buf), "%li", target->value.ts_val);
			variant_set_str(target, buf);
			return variant_get_str(target, buf);
		}

	}

	return NULL;
}

void variant_set_int(variant_t *target, int value)
{
	if (target->type == VARIANT_INT) {
		target->value.i_val = value;
		return;
	}

	/* string is the only type which needs to be freed */
	if (target->type == VARIANT_STRING) {
		if (target->value.s_val) free(target->value.s_val);
	}

	target->value.i_val = value;
	target->type = VARIANT_INT;	
}

int variant_get_int(variant_t *target, int nil)
{
	switch (target->type) {
	
		case (VARIANT_INT):
			return target->value.i_val;

		case (VARIANT_BOOL):
			return target->value.b_val;
		
		case (VARIANT_STRING):
			if (target->value.s_val == NULL)
				return nil;
			return atoi(target->value.s_val);
	
		case (VARIANT_TIMESTAMP):
			return (int)target->value.ts_val;
		
	}
	
	return nil;
}

void variant_set_bool(variant_t *target, int value)
{
	if (target->type == VARIANT_BOOL) {
		target->value.b_val = value;
		return;
	}

	/* string is the only type which needs to be freed */
	if (target->type == VARIANT_STRING) {
		if (target->value.s_val) free(target->value.s_val);
	}

	/* normalize to 0 or 1 */
	target->value.b_val = (value) ? 0 : 1;
	target->type = VARIANT_BOOL;
}

int variant_get_bool(variant_t *target, int nil)
{
	switch (target->type) {
	
		case (VARIANT_INT):
			return (target->value.i_val) ? 0 : 1;

		case (VARIANT_BOOL):
			return target->value.b_val;
		
		case (VARIANT_STRING):
			if (target->value.s_val == NULL)
				return nil;
			if (0 == strcmp(target->value.s_val, "yes")
				|| 0 == strcmp(target->value.s_val, "1"))
				return 1;
			else if (0 == strcmp(target->value.s_val, "no")
				|| 0 == strcmp(target->value.s_val, "0"))
				return 0;
			return nil;
	
		case (VARIANT_TIMESTAMP):
			return nil;
		
	}
	
	return nil;
}

void variant_set_ts(variant_t *target, time_t value)
{
	if (target->type == VARIANT_TIMESTAMP) {
		target->value.ts_val = value;
		return;
	}

	/* string is the only type which needs to be freed */
	if (target->type == VARIANT_STRING) {
		if (target->value.s_val)
			 free(target->value.s_val);
		target->value.s_val = NULL;
	}

	target->value.ts_val = value;
	target->type = VARIANT_TIMESTAMP;	
}

time_t variant_get_ts(variant_t *target, time_t nil)
{
	switch (target->type) {
	
		case (VARIANT_INT):
			return nil;

		case (VARIANT_BOOL):
			return nil;
		
		case (VARIANT_STRING):
			return (time_t)atof(target->value.s_val);
	
		case (VARIANT_TIMESTAMP):
			return target->value.ts_val;
		
	}
	
	return nil;
}
