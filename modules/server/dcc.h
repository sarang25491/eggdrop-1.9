/* dcc.h: header for dcc.c
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: dcc.h,v 1.6 2003/12/18 06:50:47 wcc Exp $
 */

#ifndef _EGG_MOD_SERVER_DCC_H_
#define _EGG_MOD_SERVER_DCC_H_

/* Statistic types for dcc_send_info(). */
#define DCC_SEND_SENT		1
#define DCC_SEND_LEFT		2
#define DCC_SEND_CPS_TOTAL	3
#define DCC_SEND_CPS_SNAPSHOT	4
#define DCC_SEND_ACKS		5
#define DCC_SEND_BYTES_ACKED	6
#define DCC_SEND_RESUMED_AT	7
#define DCC_SEND_REQUEST_TIME	8
#define DCC_SEND_CONNECT_TIME	9

int dcc_dns_set(const char *host);
int dcc_start_chat(const char *nick, int timeout);
int dcc_start_send(const char *nick, const char *fname, int timeout);
int dcc_send_info(int idx, int field, void *valueptr);
int dcc_accept_send(char *nick, char *localfname, char *fname, int size, int resume, char *ip, int port, int timeout);

extern bind_list_t ctcp_dcc_binds[];

#endif /* !_EGG_MOD_SERVER_DCC_H_ */
