/* chanserv.h: header for chanserv.c
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
 * $Id: chanserv.h,v 1.1 2004/12/13 15:02:29 stdarg Exp $
 */

#ifndef _EGG_MOD_CHANSERV_CHANSERV_H_
#define _EGG_MOD_CHANSERV_CHANSERV_H_

#include <modules/server/egg_server_api.h>

#define chan_config chanserv_LTX_chan_config
#define server chanserv_LTX_server
#define events_init chanserv_LTX_events_init

#include "events.h"

typedef struct {
	int short_joins, long_joins;
} chanserv_stats_t;

typedef struct {
	int short_join_period, long_join_period;
	int short_join_limit, long_join_limit;
} chan_config_t;

extern chan_config_t chan_config;
extern egg_server_api_t *server;

#endif
