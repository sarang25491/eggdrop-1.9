/* telnetparty.h: header for telnetparty.c
 *
 * Copyright (C) 2003, 2004 Eggheads Development Team
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
 * $Id: telnetparty.h,v 1.4 2004/06/19 16:11:53 wingman Exp $
 */

#ifndef _EGG_MOD_TELNETPARTY_TELNETPARTY_H_
#define _EGG_MOD_TELNETPARTY_TELNETPARTY_H_

/* Possible states of the connection. */
#define STATE_RESOLVE	0
#define STATE_NICKNAME	1
#define STATE_PASSWORD	2
#define STATE_PARTYLINE	3

/* Telnet strings for controlling echo behavior. */
#define TELNET_ECHO	1
#define TELNET_AYT	246
#define TELNET_WILL	251
#define TELNET_WONT	252
#define TELNET_DO	253
#define TELNET_DONT	254
#define TELNET_CMD	255

#define TELNET_ECHO_OFF	"\377\373\001"
#define TELNET_ECHO_ON	"\377\374\001"

/* Flags for sessions. */
#define STEALTH_LOGIN	1
#define TFLAG_ECHO	2
#define TFLAG_PASSWORD	4

typedef struct {
	int idx;

	/* Pointer to our entry in the partyline. */
	partymember_t *party;

	/* Who we're connected to. */
	user_t *user;
	char *nick, *ident, *host, *ip;
	int port;
	int pid;

	/* Flags for this connection. */
	int flags;

	/* Connection state we're in. */
	int state, count;
} telnet_session_t;

typedef struct {
	char *vhost;
	int port;
	int stealth;
	int max_retries;
} telnet_config_t;

extern telnet_config_t telnet_config;

#endif /* !_EGG_MOD_TELNETPARTY_TELNETPARTY_H_ */
