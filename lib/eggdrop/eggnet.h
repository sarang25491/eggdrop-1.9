/* eggnet.h: header for eggnet.c
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
 * $Id: eggnet.h,v 1.6 2003/12/17 07:39:14 wcc Exp $
 */

#ifndef _EGG_EGGNET_H_
#define _EGG_EGGNET_H_

int egg_net_init();
int egg_iprintf(int idx, const char *format, ...);
int egg_server(const char *vip, int port, int *real_port);
int egg_client(const char *ip, int port, const char *vip, int vport);
int egg_listen(int port, int *real_port);
int egg_connect(const char *host, int port, int timeout);
int egg_reconnect(int idx, const char *host, int port, int timeout);
int egg_ident_lookup(const char *ip, int their_port, int our_port, int timeout, int (*callback)(), void *client_data);

#endif /* !_EGG_EGGNET_H_ */
