/* main.h: header for main.c
 *
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Eggheads Development Team
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
 * $Id: main.h,v 1.39 2004/06/19 18:07:01 wingman Exp $
 */

#ifndef _EGG_MAIN_H
#define _EGG_MAIN_H

void fatal(const char *, int);

#define SHUTDOWN_GRACEFULL	0
#define SHUTDOWN_HARD		1

int core_init();
int core_restart(const char *nick);
int core_shutdown(int how, const char *nick, const char *reason);

#endif /* !_EGG_MAIN_H */
