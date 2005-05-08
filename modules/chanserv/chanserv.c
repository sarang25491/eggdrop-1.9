/* chanserv.c: channel services
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
 */

#ifndef lint
static const char rcsid[] = "$Id: chanserv.c,v 1.3 2005/05/08 04:40:12 stdarg Exp $";
#endif

#include <eggdrop/eggdrop.h>

#include "chanserv.h"

chanserv_config_t chanserv_config;
egg_server_api_t *server = NULL;
static chanserv_channel_stats_t global_chan;

/* This array is in the same sequence as the CHANSERV_STAT_* constants. */
static const char *channel_config_names[] = {
	"join", "part", "quit", "cycle", "leave", "ctcp", "msg",
	"notice", "line", "nick", "kick", "mode", "modefight",
	"topic"
};

static config_var_t chanserv_config_vars[] = {
	{"channel.cycle_time", &chanserv_config.cycle_time, CONFIG_INT},

	{"channel.join.period", &(chanserv_config.channel_periods[CHANSERV_STAT_JOIN]), CONFIG_INT},
	{"channel.join.limit", &(chanserv_config.channel_limits[CHANSERV_STAT_JOIN]), CONFIG_INT},
	{"channel.part.period", &(chanserv_config.channel_periods[CHANSERV_STAT_PART]), CONFIG_INT},
	{"channel.part.limit", &(chanserv_config.channel_limits[CHANSERV_STAT_PART]), CONFIG_INT},
	{"channel.quit.period", &(chanserv_config.channel_periods[CHANSERV_STAT_QUIT]), CONFIG_INT},
	{"channel.quit.limit", &(chanserv_config.channel_limits[CHANSERV_STAT_QUIT]), CONFIG_INT},
	{"channel.cycle.period", &(chanserv_config.channel_periods[CHANSERV_STAT_CYCLE]), CONFIG_INT},
	{"channel.cycle.limit", &(chanserv_config.channel_limits[CHANSERV_STAT_CYCLE]), CONFIG_INT},
	{"channel.leave.period", &(chanserv_config.channel_periods[CHANSERV_STAT_LEAVE]), CONFIG_INT},
	{"channel.leave.limit", &(chanserv_config.channel_limits[CHANSERV_STAT_LEAVE]), CONFIG_INT},
	{"channel.ctcp.period", &(chanserv_config.channel_periods[CHANSERV_STAT_CTCP]), CONFIG_INT},
	{"channel.ctcp.limit", &(chanserv_config.channel_limits[CHANSERV_STAT_CTCP]), CONFIG_INT},
	{"channel.msg.period", &(chanserv_config.channel_periods[CHANSERV_STAT_MSG]), CONFIG_INT},
	{"channel.msg.limit", &(chanserv_config.channel_limits[CHANSERV_STAT_MSG]), CONFIG_INT},
	{"channel.notice.period", &(chanserv_config.channel_periods[CHANSERV_STAT_NOTICE]), CONFIG_INT},
	{"channel.notice.limit", &(chanserv_config.channel_limits[CHANSERV_STAT_NOTICE]), CONFIG_INT},
	{"channel.line.period", &(chanserv_config.channel_periods[CHANSERV_STAT_LINE]), CONFIG_INT},
	{"channel.line.limit", &(chanserv_config.channel_limits[CHANSERV_STAT_LINE]), CONFIG_INT},
	{"channel.nick.period", &(chanserv_config.channel_periods[CHANSERV_STAT_NICK]), CONFIG_INT},
	{"channel.nick.limit", &(chanserv_config.channel_limits[CHANSERV_STAT_NICK]), CONFIG_INT},
	{"channel.kick.period", &(chanserv_config.channel_periods[CHANSERV_STAT_KICK]), CONFIG_INT},
	{"channel.kick.limit", &(chanserv_config.channel_limits[CHANSERV_STAT_KICK]), CONFIG_INT},
	{"channel.mode.period", &(chanserv_config.channel_periods[CHANSERV_STAT_MODE]), CONFIG_INT},
	{"channel.mode.limit", &(chanserv_config.channel_limits[CHANSERV_STAT_MODE]), CONFIG_INT},
	{"channel.modefight.period", &(chanserv_config.channel_periods[CHANSERV_STAT_MODEFIGHT]), CONFIG_INT},
	{"channel.modefight.limit", &(chanserv_config.channel_limits[CHANSERV_STAT_MODEFIGHT]), CONFIG_INT},
	{"channel.topic.period", &(chanserv_config.channel_periods[CHANSERV_STAT_TOPIC]), CONFIG_INT},
	{"channel.topic.limit", &(chanserv_config.channel_limits[CHANSERV_STAT_TOPIC]), CONFIG_INT},

	{0}
};

static chanserv_channel_stats_t *chanstats_head = NULL;
static bind_table_t *BT_chanflood = NULL;
static bind_table_t *BT_memflood = NULL;

EXPORT_SCOPE int chanserv_LTX_start(egg_module_t *modinfo);
static int chanserv_close(int why);
static int on_chanset(const char *chan, const char *setting, const char *oldvalue, const char *newvalue);

static int chanserv_init()
{
	memset(&global_chan, 0, sizeof(global_chan));
	BT_chanflood = bind_table_add("chanflood", 5, "ssiii", MATCH_MASK, BIND_STACKABLE);
	BT_memflood = bind_table_add("memflood", 7, "ssssiii", MATCH_MASK, BIND_STACKABLE);
	server = module_get_api("server", 1, 0);
	bind_add_simple("chanset", NULL, "settings.chanserv.*", on_chanset);
	events_init();
	return(0);
}

static int chanserv_shutdown()
{
	bind_table_del(BT_chanflood);
	bind_table_del(BT_memflood);
	events_shutdown();
	return(0);
}

int chanserv_refresh_channel_config(chanserv_channel_stats_t *chanstats)
{
	channel_t *chan = server->channel_lookup(chanstats->name);
	int i, setting;

	if (!chan) return(0);
	for (i = 0; i < CHANSERV_STAT_LEN; i++) {
		/* If the setting isn't found, default to -1, which means
		 * we use the global default. This happens when a channel
		 * is created. */
		if (server->channel_get_int(chan, &setting, "chanserv", 0, channel_config_names[i], 0, "period", 0, NULL)) setting = -1;

		/* If the period changes, we clear the stats. */
		if (setting != chanstats->periods[i]) {
			chanstats->periods[i] = setting;
			chanstats->stats[i] = 0;
		}

		if (server->channel_get_int(chan, &setting, "chanserv", 0, channel_config_names[i], 0, "limit", 0, NULL)) setting = -1;
		chanstats->limits[i] = setting;
	}

	return(0);
}

static int on_chanset(const char *chan, const char *setting, const char *oldvalue, const char *newvalue)
{
	chanserv_channel_stats_t *chanstats = chanserv_probe_chan(chan, 1);
	garbage_add(chanserv_refresh_channel_config, chanstats, GARBAGE_ONCE);
	return(0);
}

chanserv_channel_stats_t *chanserv_probe_chan(const char *chan, int create)
{
	chanserv_channel_stats_t *chanstats, *prev;

	prev = NULL;
	for (chanstats = chanstats_head; chanstats; chanstats = chanstats->next) {
		if (!strcasecmp(chanstats->name, chan)) return(chanstats);
		prev = chanstats;
	}
	if (!create) return(NULL);

	/* Create new channel. */
	chanstats = calloc(1, sizeof(*chanstats));
	chanstats->name = strdup(chan);
	chanserv_refresh_channel_config(chanstats);

	if (prev) prev->next = chanstats;
	else chanstats_head = chanstats;
	chanstats->prev = prev;
	return(chanstats);
}

chanserv_member_stats_t *chanserv_probe_member(chanserv_channel_stats_t *chan, const char *nick, const char *uhost, int create)
{
	char *who, buf[256];
	int i;

	if (!chan) chan = &global_chan;

	who = egg_msprintf(buf, sizeof(buf), NULL, "%s!%s", nick, uhost);

	for (i = 0; i < chan->nmembers; i++) {
		if (wild_match(chan->members[i].who, who)) {
			if (who != buf) free(who);
			return(chan->members+i);
		}
	}
	if (who != buf) free(who);
	if (!create) return(NULL);

	/* Add new member. */
	chan->members = realloc(chan->members, sizeof(*chan->members) * (chan->nmembers+1));
	chan->nmembers++;
	chan->members[i].who = ircmask_create(5, nick, uhost);
	return(chan->members+i);
}

int chanserv_lookup_config(const char *chan, int stat, int *limit, int *period)
{
	chanserv_channel_stats_t *chanstats;

	chanstats = chanserv_probe_chan(chan, 1);
	if (limit) {
		*limit = chanstats->limits[stat];
		if (*limit < 0) *limit = chanserv_config.channel_limits[stat];
	}
	if (period) {
		*period = chanstats->periods[stat];
		if (*period < 0) *period = chanserv_config.channel_periods[stat];
	}
	return(0);
}

int chanserv_update_stats(int stat, const char *chan, const char *nick, const char *uhost)
{
	chanserv_channel_stats_t *chanstats = NULL;
	chanserv_member_stats_t *m = NULL;
	int period, limit;
	time_t now;
	int diff;

	if (stat < 0 || stat >= CHANSERV_STAT_LEN) return(-1);
	if (!nick || !uhost) return(-1);

	if (chan) chanstats = chanserv_probe_chan(chan, 1);
	m = chanserv_probe_member(chanstats, nick, uhost, 1);
	if (!m) return(-1);

	/* Take care of channel stats first. */

	now = timer_get_now_sec(NULL);

	if (chanstats) {
		/* Figure out the period and limit for this stat. */
		period = chanstats->periods[stat];
		if (period < 0) period = chanserv_config.channel_periods[stat];
		if (period > 0) {
			limit = chanstats->limits[stat];
			if (limit < 0) limit = chanserv_config.channel_limits[stat];

			/* Adjust stats for time difference of this event
			 * relative to the last one. */
			diff = now - chanstats->last_event_time[stat];
			if (diff > period) {
				chanstats->last_event_time[stat] = now;
				chanstats->stats[stat] = 0;
				diff = 0;
			}
			chanstats->stats[stat]++;

			if (chanstats->stats[stat] > limit) {
				bind_check(BT_chanflood, NULL, channel_config_names[stat], channel_config_names[stat], chan, limit, period, diff);
				chanstats->last_event_time[stat] = now;
				chanstats->stats[stat] = 0;
			}
		}
	}

	/* Now update member stats. */
	if (m) {
		/* Figure out the period and limit for this stat. */
		period = global_chan.periods[stat];
		if (period > 0) {
			limit = global_chan.limits[stat];

			/* Adjust stats for time difference of this event
			 * relative to the last one. */
			diff = now - m->last_event_time[stat];
			if (diff > period) {
				m->last_event_time[stat] = now;
				m->stats[stat] = 0;
				diff = 0;
			}
			m->stats[stat]++;

			if (m->stats[stat] > limit) {
				bind_check(BT_memflood, NULL, channel_config_names[stat], channel_config_names[stat], chan, nick, uhost, limit, period, diff);
				m->last_event_time[stat] = now;
				m->stats[stat] = 0;
			}
		}
		
		switch (stat) {
			case CHANSERV_STAT_JOIN:
				m->join_time = now;
				break;
			case CHANSERV_STAT_LEAVE:
				if (now - m->join_time < chanserv_config.cycle_time) {
					chanserv_update_stats(CHANSERV_STAT_CYCLE, chan, nick, uhost);
				}
				break;
		}
	}
	return(0);
}

static int chanserv_close(int why)
{
	void *config_root;

	chanserv_shutdown();
	config_root = config_get_root("eggdrop");
	config_unlink_table(chanserv_config_vars, config_root, "chanserv", 0, NULL);
	return(0);
}

int chanserv_LTX_start(egg_module_t *modinfo)
{
	void *config_root;

	modinfo->name = "chanserv";
	modinfo->author = "eggdev";
	modinfo->version = "1.0.0";
	modinfo->description = "channel services";
	modinfo->close_func = chanserv_close;

	/* Set defaults. */
	memset(&chanserv_config, 0, sizeof(chanserv_config));

	/* Link our vars in. */
	config_root = config_get_root("eggdrop");
	config_link_table(chanserv_config_vars, config_root, "chanserv", 0, NULL);
	config_update_table(chanserv_config_vars, config_root, "chanserv", 0, NULL);

	chanserv_init();
	return(0);
}
