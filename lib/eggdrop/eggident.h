/* eggident.h: header for eggident.c
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
 * $Id: eggident.h,v 1.3 2003/12/17 07:39:14 wcc Exp $
 */

#ifndef _EGG_EGGIDENT_H_
#define _EGG_EGGIDENT_H_

int egg_ident_lookup(const char *ip, int their_port, int our_port, int timeout, int (*callback)(), void *client_data);
int egg_ident_cancel(int id, int issue_callback);

#endif /* !_EGG_EGGIDENT_H_ */
