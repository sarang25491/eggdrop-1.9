/* binds.c: server bind tables
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
 */

#ifndef lint
static const char rcsid[] = "$Id: binds.c,v 1.16 2004/06/15 19:19:16 wingman Exp $";
#endif

#include "server.h"

bind_table_t *BT_wall = NULL,
             *BT_raw = NULL,
             *BT_server_input = NULL,
             *BT_server_output = NULL,
             *BT_notice = NULL,
             *BT_msg = NULL,
             *BT_msgm = NULL,
             *BT_pub = NULL,
             *BT_pubm = NULL,
             *BT_ctcp = NULL,
             *BT_ctcr = NULL,
             *BT_dcc_chat = NULL,
             *BT_dcc_recv = NULL,
             *BT_nick = NULL,
             *BT_join = NULL,
             *BT_part = NULL,
             *BT_quit = NULL,
             *BT_kick = NULL,
             *BT_leave = NULL,
             *BT_mode = NULL;

void server_binds_destroy()
{
	bind_table_del(BT_wall);
	bind_table_del(BT_raw);
	bind_table_del(BT_server_input);
	bind_table_del(BT_server_output);
	bind_table_del(BT_notice);
	bind_table_del(BT_msgm);
	bind_table_del(BT_msg);
	bind_table_del(BT_pubm);
	bind_table_del(BT_pub);
	bind_table_del(BT_ctcr);
	bind_table_del(BT_ctcp);
	bind_table_del(BT_dcc_chat);
	bind_table_del(BT_dcc_recv);
	bind_table_del(BT_nick);
	bind_table_del(BT_join);
	bind_table_del(BT_part);
	bind_table_del(BT_quit);
	bind_table_del(BT_kick);
	bind_table_del(BT_leave);
	bind_table_del(BT_mode);
}

void server_binds_init()
{
	/* Create our bind tables. */
	BT_wall = bind_table_add("wall", 2, "ss", MATCH_MASK, BIND_STACKABLE);					/* DDD */
	BT_raw = bind_table_add("raw", 6, "ssUsiS", MATCH_MASK, BIND_STACKABLE);				/* DDD */
	BT_server_input = bind_table_add("server_input", 1, "s", MATCH_NONE, BIND_STACKABLE);			/* DDD */
	BT_server_output = bind_table_add("server_output", 1, "s", MATCH_NONE, BIND_STACKABLE);			/* DDD */
	BT_notice = bind_table_add("notice", 5, "ssUss", MATCH_MASK | MATCH_FLAGS, BIND_STACKABLE);		/* DDD */
	BT_msg = bind_table_add("msg", 4, "ssUs", MATCH_EXACT, 0);						/* DDD */
	BT_msgm = bind_table_add("msgm", 4, "ssUs", MATCH_MASK | MATCH_FLAGS, BIND_STACKABLE);			/* DDD */
	BT_pub = bind_table_add("pub", 5, "ssUss", MATCH_EXACT, 0);						/* DDD */
	BT_pubm = bind_table_add("pubm", 5, "ssUss", MATCH_MASK | MATCH_FLAGS, BIND_STACKABLE);			/* DDD */
	BT_ctcr = bind_table_add("ctcr", 6, "ssUsss", MATCH_MASK | MATCH_FLAGS, BIND_STACKABLE);		/* DDD */
	BT_ctcp = bind_table_add("ctcp", 6, "ssUsss", MATCH_MASK | MATCH_FLAGS, BIND_STACKABLE);		/* DDD */
	BT_dcc_chat = bind_table_add("dcc_chat", 6, "ssUssi", MATCH_MASK | MATCH_FLAGS, BIND_STACKABLE);	/* DDD */
	BT_dcc_recv = bind_table_add("dcc_recv", 7, "ssUssii", MATCH_MASK | MATCH_FLAGS, BIND_STACKABLE);	/* DDD */
	BT_nick = bind_table_add("nick", 4, "ssUs", MATCH_MASK | MATCH_FLAGS, BIND_STACKABLE);			/* DDD */
	BT_join = bind_table_add("join", 4, "ssUs", MATCH_MASK | MATCH_FLAGS, BIND_STACKABLE);			/* DDD */
	BT_part = bind_table_add("part", 5, "ssUss", MATCH_MASK | MATCH_FLAGS, BIND_STACKABLE);			/* DDD */
	BT_quit = bind_table_add("quit", 4, "ssUs", MATCH_MASK | MATCH_FLAGS, BIND_STACKABLE);			/* DDD */
	BT_kick = bind_table_add("kick", 5, "ssUss", MATCH_MASK | MATCH_FLAGS, BIND_STACKABLE);			/* DDD */
	BT_leave = bind_table_add("leave", 4, "ssUs", MATCH_MASK | MATCH_FLAGS, BIND_STACKABLE);		/* DDD */
	BT_mode = bind_table_add("mode", 6, "ssUsss", MATCH_MASK | MATCH_FLAGS, BIND_STACKABLE);		/* DDD */

	bind_add_list("ctcp", ctcp_dcc_binds);
}
