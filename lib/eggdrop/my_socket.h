/* my_socket.h: header for my_socket.c
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
 * $Id: my_socket.h,v 1.5 2003/12/17 07:39:14 wcc Exp $
 */

#ifndef _EGG_MY_SOCKET_H_
#define _EGG_MY_SOCKET_H_

#define SOCKET_CLIENT	1
#define SOCKET_SERVER	2
#define SOCKET_BIND	4
#define SOCKET_NONBLOCK	8
#define SOCKET_TCP	16
#define SOCKET_UDP	32

int socket_create(const char *dest_ip, int dest_port, const char *src_ip, int src_port, int flags);
int socket_close(int sock);
int socket_set_nonblock(int desc, int value);
int socket_get_name(int sock, char **ip, int *port);
int socket_get_peer_name(int sock, char **peer_ip, int *peer_port);
int socket_get_error(int sock);
int socket_accept(int sock, char **peer_ip, int *peer_port);
int socket_valid_ip(const char *ip);
int socket_ip_to_uint(const char *ip, unsigned int *longip);
int socket_ipv6_to_dots(const char *ip, char *dots);

#endif /* !_EGG_MY_SOCKET_H_ */
