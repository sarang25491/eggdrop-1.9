/* binds.h: header for binds.c
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
 * $Id: binds.h,v 1.10 2005/03/03 18:45:26 stdarg Exp $
 */

#ifndef _EGG_MOD_SERVER_BINDS_H_
#define _EGG_MOD_SERVER_BINDS_H_

extern bind_table_t *BT_wall, *BT_raw, *BT_server_input, *BT_server_output,
	*BT_notice, *BT_msg, *BT_msgm, *BT_pub, *BT_pubm, *BT_ctcp, *BT_ctcr,
	*BT_dcc_chat, *BT_dcc_recv, *BT_nick, *BT_join, *BT_part, *BT_quit,
	*BT_kick, *BT_leave, *BT_mode, *BT_chanset;

extern void server_binds_destroy();
extern void server_binds_init();

#endif /* !_EGG_MOD_SERVER_BINDS_H_ */
