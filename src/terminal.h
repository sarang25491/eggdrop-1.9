/* terminal.h: header for terminal.c
 *
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
 * $Id: terminal.h,v 1.3 2004/06/22 20:12:37 wingman Exp $
 */
#ifndef _EGG_TERMINAL_H
#define _EGG_TERMINAL_H

/* Terminal user settings */
#define TERMINAL_NICK	"HQ"
#define TERMINAL_USER	"hq"
#define TERMINAL_HANDLE	"HQ"
#define TERMINAL_HOST	"terminal"

int terminal_init(void);
int terminal_shutdown(void);

#endif /* _EGG_TERMINAL_H */
