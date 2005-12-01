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
 * $Id: chanserv.h,v 1.4 2005/12/01 21:22:11 stdarg Exp $
 */

#ifndef _EGG_MOD_CHANSERV_CHANSERV_H_
#define _EGG_MOD_CHANSERV_CHANSERV_H_

#include <modules/server/egg_server_api.h>
#include "egg_chanserv_internal.h"
#include "events.h"

#define CHANSERV_CLEANUP_TIME	120
#define CHANSERV_CLEANUP_TIMER	60

enum {
	CHANSERV_STAT_JOIN = 0,
	CHANSERV_STAT_PART,
	CHANSERV_STAT_QUIT,
	CHANSERV_STAT_CYCLE,
	CHANSERV_STAT_LEAVE,
	CHANSERV_STAT_CTCP,
	CHANSERV_STAT_MSG,
	CHANSERV_STAT_NOTICE,
	CHANSERV_STAT_LINE,
	CHANSERV_STAT_NICK,
	CHANSERV_STAT_KICK,
	CHANSERV_STAT_MODE,
	CHANSERV_STAT_MODEFIGHT,
	CHANSERV_STAT_TOPIC,
	CHANSERV_STAT_LEN
};

typedef struct {
	char *who;
	int stats[CHANSERV_STAT_LEN];
	time_t last_event_time[CHANSERV_STAT_LEN];
	time_t last_event;
} chanserv_member_stats_t;

typedef struct chanserv_channel_stats {
	struct chanserv_channel_stats *next, *prev;

	char *name;
	int stats[CHANSERV_STAT_LEN];
	int periods[CHANSERV_STAT_LEN];
	int limits[CHANSERV_STAT_LEN];
	time_t last_event_time[CHANSERV_STAT_LEN];

	chanserv_member_stats_t *members;
	int nmembers;
} chanserv_channel_stats_t;

typedef struct {
	int channel_periods[CHANSERV_STAT_LEN];
	int channel_limits[CHANSERV_STAT_LEN];
	int individual_periods[CHANSERV_STAT_LEN];
	int individual_limits[CHANSERV_STAT_LEN];
	int cycle_time;
} chanserv_config_t;

extern chanserv_config_t chanserv_config;
extern egg_server_api_t *server;

extern int chanserv_lookup_config(const char *chan, int stat, int *limit, int *period);
extern chanserv_channel_stats_t *chanserv_probe_chan(const char *chan, int create);
extern int chanserv_update_stats(int stat, const char *chan, const char *nick, const char *uhost);

#endif
