/* ident.h: header for ident.c
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
 * $Id: ident.h,v 1.2 2006/10/03 04:02:12 sven Exp $
 */

#ifndef _EGG_IDENT_H_
#define _EGG_IDENT_H_

typedef int ident_callback_t(void *client_data, const char *ip, int port, const char *ident);

int egg_ident_lookup(const char *ip, int their_port, int our_port, int timeout, ident_callback_t *callback, void *client_data, event_owner_t *owner);
int egg_ident_cancel(int id, int issue_callback);
int egg_ident_cancel_by_owner(struct egg_module *module, void *script);

#endif /* !_EGG_IDENT_H_ */
