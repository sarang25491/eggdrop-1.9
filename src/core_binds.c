/*
 * core_binds.c --
 */
/*
 * Copyright (C) 2002 Eggheads Development Team
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
static const char rcsid[] = "$Id: core_binds.c,v 1.4 2002/05/17 07:29:25 stdarg Exp $";
#endif

#include "main.h"
#include "cmdt.h"		/* cmd_t				*/
#include "tclhash.h"		/* BT_time, BT_event, add_bind_table2,
				   check_bind				*/

static bind_table_t *BT_time, *BT_event;

void core_binds_init()
{
	BT_time = bind_table_add("time", 5, "iiiii", MATCH_MASK, BIND_STACKABLE);
	BT_event = bind_table_add("event", 1, "s", MATCH_MASK, BIND_STACKABLE);
}

void check_bind_time(struct tm *tm)
{
	char full[32];
	snprintf(full, sizeof(full), "%02d %02d %02d %02d %04d", tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon, tm->tm_year + 1900);
	check_bind(BT_time, full, NULL, tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon, tm->tm_year + 1900);
}

void check_bind_event(char *event)
{
	check_bind(BT_event, event, NULL, event);
}
