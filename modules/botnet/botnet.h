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
 * $Id: botnet.h,v 1.1 2007/06/03 23:43:45 sven Exp $
 */

#ifndef _EGG_MOD_OLDBOTNET_OLDBOTNET_H_
#define _EGG_MOD_OLDBOTNET_OLDBOTNET_H_

#define bothandler   botnet_LTX_bothandler

typedef struct {
	botnet_bot_t *bot;
	user_t *user;
	int idx;
	char *password;
	int incoming;
	int linking;
	int version;
	int idle;
} bot_t;

/* From events.c */
botnet_handler_t bothandler;

#endif /* !_EGG_MOD_OLDBOTNET_OLDBOTNET_H_ */
