/*
 * userent.h
 *
 * $Id: userent.h,v 1.1 2002/05/05 15:21:31 wingman Exp $
 */
/*
 * Copyright (C) 2000, 2001, 2002 Eggheads Development Team
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

#ifndef _EGG_USERENT_H
#define _EGG_USERENT_H

#include "users.h"	/* list_type		*/

void list_type_kill(struct list_type *);
int xtra_set();

#endif	/* _EGG_USERENT_H	*/
