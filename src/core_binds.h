/* core_binds.h: header for core_binds.c
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
 *
 * $Id: core_binds.h,v 1.12 2004/06/22 23:20:23 wingman Exp $
 */

#ifndef _EGG_CORE_BINDS_H_
#define _EGG_CORE_BINDS_H_

/* Bind table names for core events */
#define BTN_CORE_INIT		"init"
#define BTN_CORE_SHUTDOWN	"shutdown"
#define BTN_CORE_TIME		"time"
#define BTN_CORE_SECONDLY	"secondly"
#define BTN_CORE_STATUS		"status"

int core_binds_init(void);
int core_binds_shutdown(void);

void check_bind_time(struct tm *tm);
void check_bind_init (void);
void check_bind_shutdown (void);
void check_bind_secondly();
void check_bind_status(partymember_t *p, const char *text);

#endif /* !_EGG_CORE_BINDS_H_ */
