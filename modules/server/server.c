/* server.c: irc server support
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Eggheads Development Team
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
static const char rcsid[] = "$Id: server.c,v 1.64 2005/06/21 02:55:34 stdarg Exp $";
#endif

#include "server.h"

current_server_t current_server = {0};
server_config_t server_config = {0};

int cycle_delay = 0;

/* From scriptcmds.c */
extern int server_script_init();
extern int server_script_destroy();

/* From servsock.c */
extern void connect_to_next_server();

/* From output.c */
extern void dequeue_messages();

/* From input.c */
extern bind_list_t server_raw_binds[];

/* From dcc_cmd.c */
extern bind_list_t server_party_commands[];

/* Look up the information we get from 005. */
int server_support(const char *name, const char **value)
{
	int i;

	for (i = 0; i < current_server.nsupport; i++) {
		if (!strcasecmp(name, current_server.support[i].name)) {
			*value = current_server.support[i].value;
			return(0);
		}
	}
	*value = NULL;
	return(-1);
}

/* Every second, we want to
 * 1. See if we're ready to connect to the next server (cycle_delay == 0)
 * 2. Dequeue some messages if we're connected. */

static int server_secondly()
{
	if (current_server.idx < 0 && cycle_delay >= 0) {
		/* If there's no idx, see if it's time to jump to the next
		 * server. */
		cycle_delay--;
		if (cycle_delay <= 0) {
			cycle_delay = -1;
			connect_to_next_server();
		}
		return(0);
	}

	/* Try to dequeue some stuff. */
	dequeue_messages();

	/* Check to see if we're pinged out once in a while. */
	if (current_server.idx >= 0) {
		if (current_server.time_to_ping == 0) {
			egg_timeval_t now, diff;

			timer_get_time(&now);
			timer_diff(&current_server.last_ping_sent, &now, &diff);

			/* Update ping time if it surpasses the old one, even
			 * though we haven't received the actual ping reply. */
			if (diff.sec > current_server.last_ping_time.sec || (diff.sec <= current_server.last_ping_time.sec && diff.usec > current_server.last_ping_time.usec)) {
				current_server.last_ping_time = diff;
			}
			if (diff.sec >= server_config.ping_timeout) {
				kill_server("ping timeout");
			}
		}
		else if (current_server.time_to_ping > 0) {
			current_server.time_to_ping--;
			if (current_server.time_to_ping == 0) {
				current_server.ping_id = random();
				timer_get_time(&current_server.last_ping_sent);
				printserv(SERVER_NOQUEUE, "PING :%d", current_server.ping_id);
			}
		}
	}
	return(0);
}

static int server_status(partymember_t *p, const char *text)
{
	int details = 0;
	channel_t *chan;

	if (text) {
		if (!strcasecmp(text, "all") || !strcasecmp(text, "server")) details = 1;
		else if (*text) return(0);
	}

	partymember_printf(p, "Server module:");
	if (!current_server.connected) {
		if (current_server.idx >= 0) partymember_printf(p, _("   Connecting to server %s/%d."), current_server.server_host, current_server.port);
		else partymember_printf(p, _("   Connecting to next server in %d seconds."), cycle_delay);
		return(0);
	}

	/* First line, who we've connected to. */
	partymember_printf(p, _("   Connected to %s/%d."), current_server.server_self ? current_server.server_self : current_server.server_host, current_server.port);

	/* Second line, who we're connected as. */
	if (current_server.registered) {
		if (current_server.user) partymember_printf(p, _("   Online as %s!%s@%s (%s)."), current_server.nick, current_server.user, current_server.host, current_server.real_name);
		else partymember_printf(p, _("   Online as %s (still waiting for WHOIS result)."), current_server.nick);
	}
	else partymember_printf(p, _("   Still logging in."));

	/* Third line, ping time. */
	if (current_server.time_to_ping >= 0 && current_server.npings > 0) {
		partymember_printf(p, _("   Last server ping %d.%03d seconds"), current_server.last_ping_time.sec, current_server.last_ping_time.usec / 1000);
	}

	/* Print the channel list if we have one. */
	for (chan = channel_head; chan; chan = chan->next) {
		partymember_printf(p, "   %s : %d member%s", chan->name, chan->nmembers, chan->nmembers == 1 ? "" : "s");
	}

	/* Some traffic stats. */
	if (details) {
		sockbuf_stats_t *stats;

		sockbuf_get_stats(current_server.idx, &stats);
		partymember_printf(p, "   Server traffic: %lld in / %lld out (raw), %lld in / %lld out (filtered)", stats->raw_bytes_in, stats->raw_bytes_out, stats->bytes_in, stats->bytes_out);
	}
	return(0);
}

static int server_config_save(const char *handle)
{
	server_t *server;
	void *root, *list, *node;
	int i;

	putlog(LOG_MISC, "*", "Saving server config...");
	root = config_get_root("eggdrop");

	/* Erase the server list. */
	list = config_exists(root, "server.serverlist", 0, NULL);
	if (list) config_destroy(list);

	/* Save the server list. */
	list = config_lookup_section(root, "server.serverlist", 0, NULL);
	i = 0;
	for (server = server_list; server; server = server->next) {
		node = config_lookup_section(list, "server", i, NULL);
		config_set_str(server->host, node, "host", 0, NULL);
		config_set_str(server->pass, node, "pass", 0, NULL);
		config_set_int(server->port, node, "port", 0, NULL);
		i++;
	}

	/* Erase the nick list. */
	list = config_exists(root, "server.nicklist", 0, NULL);
	if (list) config_destroy(list);

	/* Save the nick list. */
	list = config_lookup_section(root, "server.nicklist", 0, NULL);
	for (i = 0; i < nick_list_len; i++) {
		config_set_str(nick_list[i], list, "nick", i, NULL);
	}

	/* Save the channel file. */
	putlog(LOG_MISC, "*", "Saving channels file...");
	channel_save(server_config.chanfile);
	return(0);
}

static int server_close(int why)
{
	kill_server(_("server module unloading"));
	cycle_delay = 100;

	bind_rem_list("raw", server_raw_binds);
	//bind_rem_list("party", server_party_commands);
	bind_rem_simple("secondly", NULL, NULL, server_secondly);
	bind_rem_simple("status", NULL, NULL, server_status);
	bind_rem_simple("config_save", NULL, "eggdrop", server_config_save);

	/* Clear the server and nick lists. */
	server_clear();
	nick_clear();

	server_binds_destroy();

	channel_destroy();
	channel_events_destroy();
	uhost_cache_destroy();

	server_script_destroy();

	return(0);
}

static config_var_t server_config_vars[] = {
	{"chanfile", &server_config.chanfile, CONFIG_STRING},
	/* Registration information. */
	{"user", &server_config.user, CONFIG_STRING},
	{"realname", &server_config.realname, CONFIG_STRING},

	/* Timeouts. */
	{"connect_timeout", &server_config.connect_timeout, CONFIG_INT},
	{"ping_timeout", &server_config.ping_timeout, CONFIG_INT},
	{"dcc_timeout", &server_config.dcc_timeout, CONFIG_INT},

	{"trigger_on_ignore", &server_config.trigger_on_ignore, CONFIG_INT},
	{"keepnick", &server_config.keepnick, CONFIG_INT},
	{"cycle_delay", &server_config.cycle_delay, CONFIG_INT},
	{"default_port", &server_config.default_port, CONFIG_INT},
	{"max_line_len", &server_config.max_line_len, CONFIG_INT},

	{"chaninfo_items", &server_config.chaninfo_items, CONFIG_STRING},

	{"fake005", &server_config.fake005, CONFIG_STRING},

	{"raw_log", &server_config.raw_log, CONFIG_INT},

	{"ip_lookup", &server_config.ip_lookup, CONFIG_INT},
	{0}
};

static void server_config_init()
{
	int i;
	char *host, *pass, *nickstr;
	int port;
	void *config_root, *server, *nick, *list;

	/* Set default values. */
	server_config.chanfile = strdup("channels.xml");
	server_config.user = strdup("user");
	server_config.realname = strdup("real name");
	server_config.connect_timeout = 30;
	server_config.ping_timeout = 30;
	server_config.dcc_timeout = 30;
	server_config.trigger_on_ignore = 0;
	server_config.keepnick = 0;
	server_config.cycle_delay = 10;
	server_config.default_port = 6667;
	server_config.fake005 = NULL;
	server_config.max_line_len = 510;
	server_config.chaninfo_items = strdup("Inactive");

	/* Link our config vars. */
	config_root = config_get_root("eggdrop");
	config_link_table(server_config_vars, config_root, "server", 0, NULL);
	config_update_table(server_config_vars, config_root, "server", 0, NULL);

	/* Get the server list. */
	list = config_exists(config_root, "server", 0, "serverlist", 0, NULL);
	for (i = 0; (server = config_exists(list, "server", i, NULL)); i++) {
		host = pass = NULL;
		port = 0;
		config_get_str(&host, server, "host", 0, NULL);
		config_get_str(&pass, server, "pass", 0, NULL);
		config_get_int(&port, server, "port", 0, NULL);
		if (host) server_add(host, port, pass);
	}

	/* And the nick list. */
	list = config_exists(config_root, "server", 0, "nicklist", 0, NULL);
	for (i = 0; (nick = config_exists(list, "nick", i, NULL)); i++) {
		nickstr = NULL;
		config_get_str(&nickstr, nick, NULL);
		if (nickstr) nick_add(nickstr);
	}
}

EXPORT_SCOPE int server_start(egg_module_t *modinfo);

int server_start(egg_module_t *modinfo)
{
	modinfo->name = "server";
	modinfo->author = "eggdev";
	modinfo->version = "1.0.0";
	modinfo->description = "normal irc server support";
	modinfo->close_func = server_close;
	modinfo->major = EGG_SERVER_API_MAJOR;
	modinfo->minor = EGG_SERVER_API_MINOR;
	modinfo->module_api = server_get_api();

	memset(&current_server, 0, sizeof(current_server));
	current_server.idx = -1;
	cycle_delay = 0;

	server_config_init();

	/* Create our bind tables. */
	server_binds_init();
	bind_add_list("raw", server_raw_binds);
	bind_add_list("party", server_party_commands);
	bind_add_simple("secondly", NULL, NULL, server_secondly);
	bind_add_simple("status", NULL, NULL, server_status);
	bind_add_simple("config_save", NULL, "eggdrop", server_config_save);

	/* Initialize channels. */
	channel_init();
	channel_events_init();
	uhost_cache_init();

	/* Initialize script interface. */
	server_script_init();

	return(0);
}
