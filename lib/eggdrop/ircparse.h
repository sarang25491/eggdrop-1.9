/* ircparse.h: header for ircparse.c
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id: ircparse.h,v 1.2 2003/12/17 07:39:14 wcc Exp $
 */

#ifndef _EGG_IRCPARSE_H_
#define _EGG_IRCPARSE_H_

/* Structure to hold the parts of an irc message. NSTATIC_ARGS is how much
 * space we allocate statically for arguments. If we run out, more is
 * allocated dynamically. */
#define IRC_MSG_NSTATIC_ARGS 10
typedef struct irc_msg {
	char *prefix, *cmd, **args;
	char *static_args[IRC_MSG_NSTATIC_ARGS];
	int nargs;
} irc_msg_t;

void irc_msg_parse(char *text, irc_msg_t *msg);
void irc_msg_restore(irc_msg_t *msg);
void irc_msg_cleanup(irc_msg_t *msg);

#endif /* !_EGG_IRCPARSE_H_ */
