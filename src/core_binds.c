/* core_binds.c: core bind tables
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Eggheads Development Team
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

#include <eggdrop/eggdrop.h>
#include "core_binds.h"			/* prototypes		*/

static bind_table_t *BT_time = NULL,
	*BT_secondly = NULL,
	*BT_status = NULL, *BT_init = NULL, *BT_shutdown = NULL;

void core_binds_init()
{
	BT_init = bind_table_add (BTN_CORE_INIT, 0, "", MATCH_NONE, BIND_STACKABLE);		/* DDD	*/
	BT_shutdown = bind_table_add (BTN_CORE_SHUTDOWN, 0, "", MATCH_NONE, BIND_STACKABLE);	/* DDD	*/
	BT_time = bind_table_add(BTN_CORE_TIME, 5, "iiiii", MATCH_MASK, BIND_STACKABLE);	/* DDD	*/
	BT_secondly = bind_table_add(BTN_CORE_SECONDLY, 0, "", MATCH_NONE, BIND_STACKABLE);	/* DDD	*/
	BT_status = bind_table_add(BTN_CORE_STATUS, 2, "Ps", MATCH_NONE, BIND_STACKABLE);	/* DDD	*/
}

void check_bind_init(void)
{
	bind_check (BT_init, NULL, NULL);
}

void check_bind_shutdown (void)
{
	bind_check (BT_shutdown, NULL, NULL);
}

void check_bind_time(struct tm *tm)
{
	char full[32];

	sprintf(full, "%02d %02d %02d %02d %04d", tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon, tm->tm_year + 1900);
	bind_check(BT_time, NULL, full, tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon, tm->tm_year + 1900);
}

void check_bind_secondly()
{
	bind_check(BT_secondly, NULL, NULL);
}

void check_bind_status(partymember_t *p, const char *text)
{
	bind_check(BT_status, NULL, NULL, p, text);
}
