/*
 * bg.h --
 */
/*
 * Copyright (C) 2000, 2001, 2002, 2003 Eggheads Development Team
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
 * $Id: bg.h,v 1.6 2003/01/02 21:33:15 wcc Exp $
 */

#ifndef _EGG_BG_H
#define _EGG_BG_H

typedef enum {
	BG_QUIT = 1,
	BG_ABORT
} bg_quit_t;


/*
 * Prototypes
 */
void bg_prepare_split(void);
void bg_send_quit(bg_quit_t);
void bg_do_split(void);

#endif				/* !_EGG_BG_H */
