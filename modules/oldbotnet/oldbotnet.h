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
 * $Id: oldbotnet.h,v 1.3 2003/12/18 06:50:47 wcc Exp $
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
	user_t *obot;
	char *host;
	int port;
	int idx;
	char *name;
	char *password;
	int connected;
	int handlen;
	int oversion;
} oldbotnet_t;

/* From events.c */
extern int oldbotnet_events_init();

/* From oldbotnet.c */
extern oldbotnet_t oldbotnet;

#endif /* !_EGG_MOD_OLDBOTNET_OLDBOTNET_H_ */
