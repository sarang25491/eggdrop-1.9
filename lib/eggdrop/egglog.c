/* egglog.c: logging functions
 *
 * Copyright (C) 2003, 2004 Eggheads Development Team
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
static const char rcsid[] = "$Id: egglog.c,v 1.9 2004/06/21 11:33:40 wingman Exp $";
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <eggdrop/eggdrop.h>

static bind_table_t *BT_log = NULL;

void egglog_init(void)
{
	BT_log = bind_table_add(BTN_LOG, 4, "issi", MATCH_NONE, BIND_STACKABLE);	/* DDD	*/
}

void egglog_shutdown(void)
{
	/* XXX: bind_table_del(BT_LOG); */
}

int putlog(int flags, const char *chan, const char *format, ...)
{
	va_list args;
	char *ptr, buf[1024];
	int len;

	va_start(args, format);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, format, args);
	va_end(args);
	bind_check(BT_log, NULL, NULL, flags, chan, ptr, len);
	if (ptr != buf) free(ptr);
	return(len);
}
