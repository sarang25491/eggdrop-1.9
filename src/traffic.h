/*
 * traffic.h --
 */
/*
 * Copyright (C) 2002, 2003 Eggheads Development Team
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
 * $Id: traffic.h,v 1.4 2003/01/02 21:33:17 wcc Exp $
 */

#ifndef _EGG_TRAFFIC_H
#define _EGG_TRAFFIC_H

#include "main.h"		/* FILE			*/
#include "egg.h"		/* dcc_t		*/

typedef struct {
	struct {
		unsigned long irc;
		unsigned long bn;
		unsigned long dcc;
		unsigned long filesys;
		unsigned long trans;
		unsigned long unknown;
	} in_total, in_today, out_total, out_today;
} egg_traffic_t;

extern egg_traffic_t traffic;

/* init our traffic stats (register hooks, ...)
 */
void traffic_init();

/* resets our traffic stats
 */
void traffic_reset();

/* update incoming traffic stats (used in main.c loop)
 */
void traffic_update_in(struct dcc_table *type, int size);

/* update outgoing traffic stats
 */
void traffic_update_out(struct dcc_table *type, int size);

/* traffic dcc command
 */
int cmd_traffic(struct userrec *u, int idx, char *par);

#endif				/* !_EGG_TRAFFIC_H */
