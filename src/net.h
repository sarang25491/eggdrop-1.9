/*
 * net.h --
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
 * $Id: net.h,v 1.3 2003/01/02 21:33:16 wcc Exp $
 */

#ifndef _EGG_NET_H
#define _EGG_NET_H

#include "egg.h"			/* IP				*/

#define iptolong(a)             (0xffffffff &                           \
                                 (long) (htonl((unsigned long) a)))

#ifdef BORGCUBES
#  define O_NONBLOCK    00000004 	/* POSIX non-blocking I/O	*/
#endif /* BORGUBES */

IP getmyip();
struct in6_addr getmyip6();
void neterror(char *);
void setsock(int, int);
int allocsock(int, int);
int getsock(int);
void killsock(int);
int answer(int, char *, char *, unsigned short *, int);
inline int open_listen(int *, int);
int open_address_listen(char *, int *);
int open_telnet(char *, int);
int open_telnet_dcc(int, char *, char *);
int open_telnet_raw(int, char *, int);
void tputs(int, char *, unsigned int);
void dequeue_sockets();
int sockgets(char *, int *);
void tell_netdebug(int);
char *iptostr(IP);
char *getlocaladdr(int);
int sock_has_data(int, int);
int sockoptions(int sock, int operation, int sock_options);
int flush_inbuf(int idx);

#endif				/* !_EGG_NET_H */
