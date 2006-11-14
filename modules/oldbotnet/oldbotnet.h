/* oldbotnet.h: header for oldbotnet.c
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
 * $Id: oldbotnet.h,v 1.4 2006/11/14 14:51:24 sven Exp $
 */

#ifndef _EGG_MOD_OLDBOTNET_OLDBOTNET_H_
#define _EGG_MOD_OLDBOTNET_OLDBOTNET_H_

#define BOT_PID_MULT	1000

typedef struct {
	char *host;
	int port;
	char *username, *password;
	int idx;
	user_t *obot;
} oldbotnet_session_t;

typedef struct {
	botnet_bot_t *bot;
	user_t *user;
	char *host;
	int port;
	int idx;
	char *name;
	char *password;
	int linking;
	int connected;
	int handlen;
	int version;
} oldbotnet_t;

/* From events.c */
extern int oldbotnet_events_init();

#endif /* !_EGG_MOD_OLDBOTNET_OLDBOTNET_H_ */
