/* server.h: header for server.c
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: server.h,v 1.21 2004/10/01 15:31:18 stdarg Exp $
 */

#ifndef _EGG_MOD_SERVER_SERVER_H_
#define _EGG_MOD_SERVER_SERVER_H_

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <eggdrop/eggdrop.h>

#include "egg_server_internal.h"

#define match_my_nick(test) (!((current_server.strcmp)(current_server.nick, test)))

/* Configuration data for this module. */
typedef struct {
	int trigger_on_ignore;
	int keepnick;
	int connect_timeout;
	int cycle_delay;
	int default_port;
	int ping_timeout;
	int dcc_timeout;
	char *user;
	char *realname;
	char *chanfile;
	int max_line_len;

	/* Override the 005 sent by the server. */
	char *fake005;

	int raw_log;

	int ip_lookup;	/* For dcc stuff, 0 - normal, 1 - server */
} server_config_t;

/* All the stuff we need to know about the currently connected server. */
typedef struct {
	/* Connection details. */
	int idx;		/* Sockbuf idx of this connection. */
	char *server_host;	/* The hostname we connected to. */
	char *server_self;	/* What the server calls itself (from 001). */
	int port;		/* The port we connected to. */
	int connected;		/* Are we connected yet? */

	/* Our info on the server. */
	int registered;		/* Has the server accepted our registration? */
	int got005;		/* Did we get the 005 message? */
	char *nick, *user, *host, *real_name;
	char *pass;

	/* Information about this server. */
	struct {
		char *name;
		char *value;
	} *support;
	int nsupport;
	char *chantypes;
	int (*strcmp)(const char *s1, const char *s2);
	char *type1modes, *type2modes, *type3modes, *type4modes;
	char *modeprefix, *whoprefix;

	/* Our dcc information for this server. */
	char *myip;
	unsigned int mylongip;
} current_server_t;

#include "egg_server_api.h"
#include "channels.h"
#include "input.h"
#include "output.h"
#include "dcc.h"
#include "binds.h"
#include "servsock.h"
#include "nicklist.h"
#include "serverlist.h"
#include "schan.h"

extern server_config_t server_config;
extern current_server_t current_server;

extern int server_support(const char *name, const char **value);
extern void *server_get_api();

#endif /* !_EGG_MOD_SERVER_SERVER_H_ */
