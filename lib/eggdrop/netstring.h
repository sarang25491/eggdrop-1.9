/* netstring.h: header for netstring.c
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
 * $Id: netstring.h,v 1.1 2007/06/03 23:43:45 sven Exp $
 */

#ifndef _EGG_NETSTRING_H_
#define _EGG_NETSTRING_H_

int netstring_on(int idx);
int netstring_off(int idx);
int netstring_check(int idx);

#endif /* !_EGG_NETSTRING_H_ */
