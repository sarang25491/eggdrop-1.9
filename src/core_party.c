/* core_party.c: core partyline commands
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
static const char rcsid[] = "$Id: core_party.c,v 1.44 2004/10/10 04:55:11 stdarg Exp $";
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_UNAME
#  include <sys/utsname.h>
#endif

#include <eggdrop/eggdrop.h>

#include "core_config.h"
#include "core_binds.h"
#include "logfile.h"
#include "terminal.h"			/* TERMINAL_NICK				*/
#include "main.h"			/* SHUTDOWN_*, core_shutdown, core_restart	*/

static int party_help(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	help_summary_t *entry;
	help_search_t *search;
	int hits = 0;
	xml_node_t *node, *arg, *desc, *see;
	char *argname, *argdesc;
	int optional;


	if (!text || !*text) {
		partymember_printf(p, "Syntax: help <command|variable|*search*>");
		return(0);
	}
	/* First try to look up an exact match. */
	entry = help_lookup_summary(text);
	if (entry) {
		node = help_lookup_entry(entry);
		partymember_printf(p, "Syntax:");
		partymember_printf(p, "    %s", entry->syntax);
		partymember_printf(p, "");
		partymember_printf(p, "Summary:");
		partymember_printf(p, "    %s", entry->summary);
		partymember_printf(p, "");
		xml_node_get_vars(node, "nnn", "desc.line", &desc, "args.arg", &arg, "seealso.see", &see);
		if (arg) {
			partymember_printf(p, "Arguments:");
			for (; arg; arg = arg->next_sibling) {
				xml_node_get_vars(arg, "sis", "name", &argname, "optional", &optional, "desc", &argdesc);
				partymember_printf(p, "    %s%s - %s", argname, optional ? " (optional)" : "", argdesc);
			}
			partymember_printf(p, "");
		}
		partymember_printf(p, "Description:");
		for (; desc; desc = desc->next_sibling) {
			partymember_printf(p, "    %s", xml_node_str(desc, ""));
		}
		partymember_printf(p, "");
		if (see) {
			partymember_printf(p, "See also:");
			for (; see; see = see->next_sibling) {
				partymember_printf(p, "    %s", xml_node_str(see, ""));
			}
		}
		xml_node_delete(node);
		return(0);
	}

	/* No, do a search. */
	if (!strchr(text, '*') && !strchr(text, '?')) {
		partymember_printf(p, "No help was found! Try searching with wildcards, e.g. *%s*", text);
		return(0);
	}

	search = help_search_new(text);
	while ((entry = help_search_result(search))) {
		partymember_printf(p, "%s - %s", entry->syntax, entry->summary);
		hits++;
	}
	if (hits == 0) {
		partymember_printf(p, "No help was found! Try more wildcards...");
	}
	help_search_end(search);
	return(0);
}

static int party_join(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (!text || !*text) {
		partymember_printf(p, _("Syntax: join <channel>"));
		return(0);
	}
	partychan_join_name(text, p);
	return(0);
}

static int party_part(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	partychan_t *chan;

	if (!text || !*text) chan = partychan_get_default(p);
	else chan = partychan_lookup_name(text);
	partychan_part(chan, p, "parting");
	return(0);
}

static int party_quit(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (0 == strcmp (p->nick, TERMINAL_NICK)) {
		partymember_printf (p, "You can't leave the partyline in terminal mode.");
		return -1;
	}

	partymember_printf(p, "Goodbye!");
	if (!text || !*text) partymember_delete(p, "Quit");
	else partymember_delete(p, text);
	return(0);
}

static int party_whisper(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char *who;
	const char *next;
	partymember_t *dest;

	egg_get_arg(text, &next, &who);
	if (!who || !next || !*who || !*next) {
		partymember_printf(p, _("Syntax: whisper <partylineuser> <msg>"));
		goto done;
	}

	dest = partymember_lookup_nick(who);
	if (!dest) {
		partymember_printf(p, _("No such user '%s'."), who);
		goto done;
	}

	partymember_msg(dest, p, next, -1);
done:
	if (who) free(who);
	return(0);
}

static void *lookup_setting(partymember_t *p, const char *path)
{
	void *root;

	root = config_get_root("eggdrop");
	root = config_exists(root, path, 0, NULL);
	if (!root) partymember_printf(p, _("That setting does not exist."));
	return(root);
}

static int party_get(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	void *root;
	char *str = NULL;

	if (!text || !*text) {
		partymember_printf(p, _("Syntax: get <path>"));
		return(0);
	}

	root = lookup_setting(p, text);
	if (!root) return(0);

	config_get_str(&str, root, NULL);
	if (str) partymember_printf(p, "Current value: '%s'", str);
	else partymember_printf(p, _("Current value: null (unset)"));
	return(0);
}

static int party_set(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	void *root;
	char *path, *str;
	const char *next;

	egg_get_arg(text, &next, &path);
	if (!path) {
		partymember_printf(p, _("Syntax: set <path> [new value]"));
		return(0);
	}
	if (!next) {
		free(path);
		return party_get(p, nick, u, cmd, text);
	}

	root = lookup_setting(p, path);
	free(path);
	if (!root) return(0);

	config_get_str(&str, root, NULL);
	partymember_printf(p, _("Old value: '%s'"), str);
	config_set_str(next, root, NULL);
	config_get_str(&str, root, NULL);
	if (str) partymember_printf(p, _("New value: '%s'"), str);
	else partymember_printf(p, _("New value: null (unset)"));
	return(0);
}

static int party_unset(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	void *root;
	char *str;

	if (!text || !*text) {
		partymember_printf(p, _("Syntax: unset <path>"));
		return(0);
	}

	root = lookup_setting(p, text);
	if (!root) return(0);

	config_get_str(&str, root, NULL);
	if (str) partymember_printf(p, _("Old value: '%s'"), str);
	else partymember_printf(p, _("Old value: null (unset)"));
	config_set_str(NULL, root, NULL);
	config_get_str(&str, root, NULL);
	if (str) partymember_printf(p, _("New value: '%s'"), str);
	else partymember_printf(p, _("New value: null (unset)"));
	return(0);
}

static int party_status(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
#ifdef HAVE_UNAME
	struct utsname un;
#endif

	partymember_printf(p, _("I am %1$s, running Eggdrop %2$s."), core_config.botname, VERSION);
	partymember_printf(p, _("Owner: %s"), core_config.owner);
	if (core_config.admin) partymember_printf(p, _("Admin: %s"), core_config.admin);
#ifdef HAVE_UNAME
	if (!uname(&un)) partymember_printf(p, _("OS: %1$s %2$s"), un.sysname, un.release);
#endif
	partymember_printf(p, _("Help path: %s (%d entries, %d sections)"),
			core_config.help_path, 0, 0);
	partymember_printf(p, "");
	check_bind_status(p, text);
	return(0);
}

static int party_save(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	putlog(LOG_MISC, "*", _("Saving user file..."));
	user_save(core_config.userfile);
	putlog(LOG_MISC, "*", _("Saving config file..."));
	core_config_save();
	return(1);
}

static int party_newpass(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (!text || strlen(text) < 6) {
		partymember_printf(p, _("Please use at least 6 characters."));
		return(0);
	}
	user_set_pass(p->user, text);
	partymember_printf(p, _("Changed password to '%s'."), text);
	return(0);
}

static int intsorter(const void *left, const void *right)
{
	return(*(int *)left - *(int *)right);
}

static int party_who(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	partymember_t *who;
	int *pids, len, i, width = 0;

	partymember_printf(p, _("Partyline members:"));
	partymember_who(&pids, &len);
	qsort(pids, len, sizeof(int), intsorter);
	if (len > 0) {
		i = pids[len-1];
		if (!i) i++;
		while (i != 0) {
			i /= 10;
			width++;
		}
	}
	for (i = 0; i < len; i++) {
		who = partymember_lookup_pid(pids[i]);
		partymember_printf(p, "  [%*d] %s (%s@%s)", width, who->pid, who->nick, who->ident, who->host);
	}
	free(pids);
	return(0);
}

static int party_whois(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	user_t *who;
	flags_t flags;
	char *item, *chan, *setting, *value, flagstr[64];
	const char *next;
	int i;

	if (text && *text) who = user_lookup_by_handle(text);
	else who = u;

	if (!who) {
		partymember_printf(p, "No such user.");
		return(0);
	}

	next = core_config.whois_items;
	while (next && *next) {
		egg_get_arg(next, &next, &item);
		if (!strcasecmp(item, "handle")) {
			partymember_printf(p, "%s: '%s'", item, who->handle);
		}
		else if (!strcasecmp(item, "uid")) {
			partymember_printf(p, "%s: '%d'", item, who->uid);
		}
		else if (!strcasecmp(item, "ircmasks")) {
			partymember_printf(p, "%s:", item);
			for (i = 0; i < who->nircmasks; i++) {
				partymember_printf(p, "   %d. '%s'", i+1, who->ircmasks[i]);
			}
		}
		else {
			if ((setting = strchr(item, '.'))) {
				chan = item;
				*setting = 0;
				setting++;
			}
			else {
				chan = NULL;
				setting = item;
			}
			if (!strcasecmp(setting, "flags")) {
				user_get_flags(who, chan, &flags);
				flag_to_str(&flags, flagstr);
				value = flagstr;
			}
			else {
				user_get_setting(who, chan, setting, &value);
			}

			if (chan) partymember_printf(p, "%s.%s: %s", chan, setting, value);
			else partymember_printf(p, "%s: %s", setting, value);
		}
		free(item);
	}
	return(0);
}

static int party_die(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	/* XXX: should we really enable hard shutdowns?
	if (*text && 0 == strcmp(text, "force")) {
		return core_shutdown(SHUTDOWN_HARD, nick, text);
	} else
	*/
	return core_shutdown(SHUTDOWN_GRACEFULL, nick, text);
}

static int party_plus_user(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	user_t *newuser;

	if (!text || !*text) {
		partymember_printf(p, _("Syntax: +user <handle>"));
		return(0);
	}
	if (user_lookup_by_handle(text)) {
		partymember_printf(p, _("User '%s' already exists!"));
		return(0);
	}
	newuser = user_new(text);
	if (newuser) partymember_printf(p, _("User '%s' created."), text);
	else partymember_printf(p, _("Could not create user '%s'."), text);
	return(0);
}

static int party_minus_user(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	user_t *who;

	if (!text || !*text) {
		partymember_printf(p, _("Syntax: -user <handle>"));
		return(0);
	}
	who = user_lookup_by_handle(text);
	if (!who) partymember_printf(p, _("User '%s' not found."));
	else {
		partymember_printf(p, _("Deleting user '%s'."), who->handle);
		user_delete(who);
	}
	return(0);
}

static int party_plus_host(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	user_t *who;
	char *target, *newhost;

	egg_get_args(text, NULL, &target, &newhost, NULL);
	if (!target) {
		partymember_printf(p, _("Syntax: +host [handle] <host>"));
		return(0);
	}
	if (!newhost) {
		newhost = target;
		target = NULL;
	}
	if (target) {
		who = user_lookup_by_handle(target);
		if (!who) {
			partymember_printf(p, _("User '%s' not found."), target);
			goto done;
		}
	}
	else {
		who = u;
		if (!who) {
			partymember_printf(p, _("Only valid users can add hosts."));
			goto done;
		}
	}
	user_add_ircmask(who, newhost);
	partymember_printf(p, _("Added '%1$s' to user '%2$s'."), newhost, who->handle);

done:
	if (target) free(target);
	free(newhost);

	return(0);
}

static int party_minus_host(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	user_t *who;
	char *target, *host;

	egg_get_args(text, NULL, &target, &host, NULL);
	if (!target) {
		partymember_printf(p, _("Syntax: -host [handle] <host>"));
		return(0);
	}
	if (!host) {
		host = target;
		target = NULL;
	}
	if (target) {
		who = user_lookup_by_handle(target);
		if (!who) {
			partymember_printf(p, _("User '%s' not found."), target);
			goto done;
		}
	}
	else {
		who = u;
		if (!who) {
			partymember_printf(p, _("Only valid users can remove hosts."));
			goto done;
		}
	}
	if (user_del_ircmask(who, host)) {
		partymember_printf(p, _("Mask '%1$s' not found for user '%2$s'."), host, who->handle);
	}
	else {
		partymember_printf(p, _("Removed '%1$s' from user '%2$s'."), host, who->handle);
	}

done:
	if (target) free(target);
	free(host);

	return(0);
}

/* Syntax: chattr <user> [chan] <flags> */
static int party_chattr(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	const char *next;
	char *who, *flags, *chan;
	user_t *dest;
	flags_t flagstruct;
	char flagstr[64];
	int n;

	n = egg_get_args(text, &next, &who, &chan, &flags, NULL);
	if (!chan || !*chan) {
		if (who) free(who);
		partymember_printf(p, _("Syntax: chattr <handle> [channel] <+/-flags>"));
		return(0);
	}
	if (!flags || !*flags) {
		flags = chan;
		chan = NULL;
	}
	dest = user_lookup_by_handle(who);
	if (dest) {
		user_set_flags_str(dest, chan, flags);
		user_get_flags(dest, chan, &flagstruct);
		flag_to_str(&flagstruct, flagstr);
		partymember_printf(p, _("Flags for %s are now '%s'."), who, flagstr);
	}
	else partymember_printf(p, _("'%s' is not a valid user."), who);
	free(who);
	free(flags);
	if (chan) free(chan);
	return(0);
}

static int party_addlog(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	putlog(LOG_MISC, "*", "%s: %s", nick, text);

	return BIND_RET_LOG;
}

static int party_modules(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	const char **modules;
	int nummods, ctr;

	nummods = module_list(&modules);
	partymember_printf(p, _("Loaded modules:"));
	for (ctr = 0; ctr < nummods; ctr++) partymember_printf(p, "   %s", modules[ctr]);
	free(modules);

	return BIND_RET_LOG;
}

static int party_loadmod(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (!text || !*text) {
		partymember_printf(p, _("Syntax: loadmod <module name>"));
		return BIND_RET_BREAK;
	}
	switch (module_load(text)) {
		case 0:
			partymember_printf(p, _("Module '%s' loaded successfully."), text);
			break;
		case -1:
			partymember_printf(p, _("Module '%s' is already loaded."), text);
			break;
		case -2:
			partymember_printf(p, _("Module '%s' could not be loaded."), text);
			break;
		case -3:
			partymember_printf(p, _("Module '%s' does not have a valid initialization function. Perhaps it is not an eggdrop module?"), text);
			break;
	}
	return BIND_RET_LOG;
}

static int party_unloadmod(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (!text || !*text) {
		partymember_printf(p, _("Syntax: unloadmod <module name>"));
		return BIND_RET_BREAK;
	}
	switch (module_unload(text, MODULE_USER)) {
		case 0:
			partymember_printf(p, _("Module '%s' unloaded successfully."), text);
			break;
		case -1:
			partymember_printf(p, _("Module '%s' is not loaded."), text);
			break;
		case -2:
			partymember_printf(p, _("Module '%s' has dependencies that are still loaded. You must unload them first."), text);
			break;
		case -3:
			partymember_printf(p, _("Module '%s' refuses to be unloaded by you!"), text);
			break;
	}
	return BIND_RET_LOG;
}

static int party_binds(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	bind_table_t *table;
	bind_entry_t *entry;
	char flags[64];

	partymember_printf(p, "%-16s %-16s %-16s %-10s %-5s %s", _("TABLE"), _("SYNTAX"),
		 _("FUNCTION"), _("MASK"), _("FLAGS"), _("HITS"));
	for (table = bind_table_list(); table; table = table->next) {
		for (entry = table->entries; entry; entry = entry->next) {
			flag_to_str(&entry->user_flags, flags);
			partymember_printf(p, "%-16s %-16s %-16s %-10s %-5s %i", table->name, table->syntax,
				entry->function_name, entry->mask, flags, entry->nhits);
		}
	}

	return BIND_RET_LOG;
}

static int party_timers(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	egg_timer_t *timer;
	int remain, now;

	now = timer_get_now_sec(NULL);
	partymember_printf(p, "ID    SEC LEFT    NAME");
	for (timer = timer_list(); timer; timer = timer->next) {
		remain = timer->trigger_time.sec - now;
		partymember_printf(p, "%-5d %-8d    %s", timer->id, remain, timer->name);
	}
	return(BIND_RET_LOG);
}

static int print_net(partymember_t *p, const char *header, int flags)
{
	int *idx, len;
	int i, port, peer_port;
	const char *host, *peer_host;
	char *self_addr, *peer_addr;
	sockbuf_handler_t *handler;

	sockbuf_list(&idx, &len, flags);
	partymember_printf(p, "%s", header);
	partymember_printf(p, "   %3s %20s %20s   %s", _("Idx"), _("Local Address"), _("Foreign Address"), _("Description"));
	for (i = 0; i < len; i++) {
		sockbuf_get_self(idx[i], &host, &port);
		sockbuf_get_peer(idx[i], &peer_host, &peer_port);
		if (!host) host = "*";
		if (!peer_host) peer_host = "*";
		sockbuf_get_handler(idx[i], &handler, NULL);
		if (port) self_addr = egg_mprintf("%s/%d", host, port);
		else self_addr = egg_mprintf("%s/*", host);
		if (peer_port) peer_addr = egg_mprintf("%s/%d", peer_host, peer_port);
		else peer_addr = egg_mprintf("%s/*", peer_host);
		partymember_printf(p, "   %3d %20s %20s   %s", idx[i], self_addr, peer_addr, handler->name);
		free(self_addr);
		free(peer_addr);
	}
	free(idx);
	return(0);
}

static int party_netstats(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	print_net(p, "Server Sockets", SOCKBUF_SERVER);
	print_net(p, "Incoming Connections", SOCKBUF_INBOUND);
	print_net(p, "Outgoing Connections", SOCKBUF_CLIENT);
	return(0);
}

static int party_restart(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	core_restart (nick);

	return BIND_RET_LOG;
}

/* Syntax: chhandle <old_handle> <new_handle> */
static int party_chhandle(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char *old = NULL, *newh = NULL;
	user_t *dest;

	egg_get_args(text, NULL, &old, &newh, NULL);
	if (!old || !newh || !*old || !*newh) {
		if (old) free(old);
		if (newh) free(newh);
		partymember_printf(p, _("Syntax: chhandle <old_handle> <new_handle>"));
		goto chhandleend;
	}

	dest = user_lookup_by_handle(old);
	if (!dest) {
		free(old);
		free(newh);
		partymember_printf(p, _("Error: User '%s' does not exist."), old);
		goto chhandleend;
	}

	if (user_lookup_by_handle(newh)) {
		free(old);
		free(newh);
		partymember_printf(p, _("Error: User '%s' already exists."), newh);
		return 0;
	}

	if (!user_change_handle(dest, newh))
		partymember_printf(p, _("Ok, changed."));

chhandleend:
	free(newh);
	free(old);

	return BIND_RET_LOG;
}

/* Syntax: chpass <handle> [new_pass] */
static int party_chpass(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char *user = NULL, *pass = NULL;
	user_t *dest;

	egg_get_args(text, NULL, &user, &pass, NULL);

	if (!user || !*user) {
		partymember_printf(p, _("Syntax: chpass <handle> [pass]"));
		goto chpassend;
	}

	dest = user_lookup_by_handle(user);
	if (!dest) {
		partymember_printf(p, _("Error: User '%s' does not exist."), user);
		goto chpassend;
	}

	if (pass && *pass && strlen(pass) < 6) {
		partymember_printf(p, _("Error: Please use at least 6 characters."));
		goto chpassend;
	}

	if (user_set_pass(dest, pass))
		partymember_printf(p, _("Removed password for %s."), user);
	else
		partymember_printf(p, _("Password for %s is now '%s'."), user, pass);

chpassend:
	free(user);
	free(pass);

	return BIND_RET_LOG_COMMAND;
}

/* Makes sure 'start' and 'limit' arguments for .match are reasonable, or else sets them -1 */
static int party_match_getbounds(const char *strstart, const char *strlimit, long *start, long *limit)
{
	char *tmpptr;

	if (strstart) {
		*start = strtol(strstart, &tmpptr, 10);
		if (!*strstart || *tmpptr || *start < 1) { /* Invalid input*/
			*start = -1;
			return 0;
		}

		if (strlimit) { /* 'start' was really a start and this is now 'limit' */
			*limit = strtol(strlimit, &tmpptr, 10);
			if (!*strlimit || *tmpptr || *limit < 1) { /* Invalid input*/
				*limit = -1;
				return 0;
			}
		}
		else { /* Ah, no, the only argument specified was the 'limit' */
			*limit = *start;
			*start = 0;
		}
	}
	else {
		*limit = 20;
		*start = 0;
	}

	return 0;
}

/* Handles case where .match was given mask to match against */
static int party_matchwild(partymember_t *p, const char *mask, const char *rest)
{
	char *strstart = NULL, *strlimit = NULL;
	long start, limit;

	egg_get_args(rest, NULL, &strstart, &strlimit, NULL);

	party_match_getbounds(strstart, strlimit, &start, &limit);
	if (start == -1 || limit == -1)
		partymember_printf(p, _("Error: 'start' and 'limit' must be positive integers"));
	else
		partyline_cmd_match_ircmask(p, mask, start, limit);

	free(strstart);
	free(strlimit);

	return 0;
}

/* Handles case where .match was given attributes to match against */
static int party_matchattr(partymember_t *p, const char *mask, const char *rest)
{
	char *channel = NULL, *strstart = NULL, *strlimit = NULL;
	long start, limit;
	int ischan = 0;

	egg_get_args(rest, NULL, &channel, &strstart, &strlimit, NULL);

	/* This is probably the easiest way to conclude if content of 'channel'
	is *NOT* a number, and thus it is a candidate for a valid channel name */
	if (channel && (*channel < '0' || *channel > '9'))
		ischan = 1;


	if (strlimit) /* .match <flags> <channel> <start> <limit> */
		party_match_getbounds(strstart, strlimit, &start, &limit);
	else if (strstart) /* .match <flags> <channel|start> <limit> */
		party_match_getbounds(ischan?strstart:channel, ischan?NULL:strstart, &start, &limit);
	else if (ischan) { /* .match <flags> <channel> */
		start = 0;
		limit = 20;
	}
	else /* .match <flags> [limit] */
		party_match_getbounds(channel, NULL, &start, &limit);

	free(strstart);
	free(strlimit);

	if (start == -1 || limit == -1)
		partymember_printf(p, _("Error: 'start' and 'limit' must be positive integers"));
	else
		partyline_cmd_match_attr(p, mask, ischan?channel:NULL, start, limit);

	free(channel);

	return 0;
}

/* match <attr> [channel] [[start] limit] */
/* match <mask> [[start] limit] */
static int party_match(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{

	char *attr = NULL;
	const char *rest = NULL;

/* FIXME - Check if user is allowed to see results.. if !chan && !glob_master && -> error
	I have left it available to everyone because 'whois' was that way too.
	We should update both or neither */

	egg_get_args(text, &rest, &attr, NULL);

	if (!attr) {
		partymember_printf(p, _("Syntax: match <attr> [channel] [[start] limit]"));
		partymember_printf(p, _("    or: match <mask> [[start] limit]"));
		free(attr);
		return 0;
	}

	if (*attr == '+' || *attr == '-' || *attr == '|')
		party_matchattr(p, attr, rest);
	else if (*attr != '&')
		party_matchwild(p, attr, rest);

	free(attr);

	return BIND_RET_LOG;
}

static bind_list_t core_party_binds[] = {		/* Old flags requirement */
	{NULL, "help", party_help},
	{NULL, "join", party_join},		/* DDD	*/
	{NULL, "whisper", party_whisper},	/* DDD	*/
	{NULL, "newpass", party_newpass},	/* DDC	*/ /* -|- */
	{NULL, "help", party_help},		/* DDC	*/ /* -|- */
	{NULL, "part", party_part},		/* DDD	*/
	{NULL, "quit", party_quit},		/* DDD	*/ /* -|- */
	{NULL, "who", party_who},		/* DDD	*/
	{NULL, "whois", party_whois},		/* DDC	*/ /* ot|o */
	{NULL, "match", party_match},		/* DDC	*/ /* ot|o */
	{"n", "addlog", party_addlog},		/* DDD	*/ /* ot|o */
	{"n", "get", party_get},		/* DDC	*/
	{"n", "set", party_set},		/* DDC	*/
	{"n", "unset", party_unset},		/* DDC	*/
	{"n", "status", party_status},		/* DDC	*/ /* m|m */
	{"n", "save", party_save},		/* DDD	*/ /* m|m */
	{"n", "die", party_die},		/* DDD	*/ /* n|- */
	{"n", "restart", party_restart},	/* DDD	*/ /* m|- */
	{"n", "+user", party_plus_user},	/* DDC	*/ /* m|- */
	{"n", "-user", party_minus_user},	/* DDC	*/ /* m|- */
	{"n", "chattr", party_chattr},		/* DDC	*/ /* m|m */
	{"n", "modules", party_modules},	/* DDD	*/ /* n|- */
	{"n", "loadmod", party_loadmod},	/* DDD	*/ /* n|- */
	{"n", "unloadmod", party_unloadmod},	/* DDD	*/ /* n|- */
	{"n", "binds", party_binds},		/* DDD 	*/ /* m|- */
	{"n", "timers", party_timers},
	{"n", "netstats", party_netstats},
	{"m", "+host", party_plus_host},	/* DDC	*/ /* t|m */
	{"m", "-host", party_minus_host},	/* DDC	*/ /* -|- */
	{"t", "chhandle", party_chhandle},	/* DDC	*/ /* t|- */
	{"t", "chpass", party_chpass},		/* DDC	*/ /* t|- */
	{0}
};

void core_party_init(void)
{
	bind_add_list("party", core_party_binds);
}

