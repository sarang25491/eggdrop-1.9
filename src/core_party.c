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
static const char rcsid[] = "$Id: core_party.c,v 1.27 2004/06/15 11:24:46 wingman Exp $";
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

/* from main.c */
extern int term_z;
extern char pid_file[];

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
	if (term_z && 0 == strcmp (p->nick, PARTY_TERMINAL_NICK)) {
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
	partymember_printf(p, _("Help path: %s (%d help entries loaded)"), core_config.help_path, help_count());
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
	putlog(LOG_MISC, "*", _("Saving user file..."));
	user_save(core_config.userfile);
	if (text && *text) putlog(LOG_MISC, "*", _("Bot shutting down: %s"), text);
	else putlog(LOG_MISC, "*", _("Bot shutting down."));
	flushlogs();
	unlink(pid_file);
	exit(0);
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
		user_set_flag_str(dest, chan, flags);
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

static int party_help(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	help_t *h;

	if (!text || !*text) {
		partymember_printf(p, _("Syntax: help <section>"));
		return(0);
	}

	h = help_lookup_by_name(text);
	if (!h) {
		partymember_printf(p, _("No help found for '%s'."), text);
		return(0);
	}

	help_print_party(p, h);
	return(0);
}

static int party_addlog(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	putlog(LOG_MISC, "*", "%s: %s", nick, text);
	return(0);
}

static int party_modules(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	const char **modules;
	int nummods, ctr;

	nummods = module_list(&modules);
	partymember_printf(p, _("Loaded modules:"));
	for (ctr = 0; ctr < nummods; ctr++) partymember_printf(p, "   %s", modules[ctr]);
	free(modules);
	return(0);
}

static int party_loadmod(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (!text || !*text) {
		partymember_printf(p, _("Syntax: loadmod <module name>"));
		return(0);
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
	return(0);
}

static int party_unloadmod(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (!text || !*text) {
		partymember_printf(p, _("Syntax: unloadmod <module name>"));
		return(0);
	}
	switch (module_unload(text, EGGMOD_USER)) {
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
	return(0);
}

static bind_list_t core_party_binds[] = {
	{NULL, "join", party_join},
	{NULL, "whisper", party_whisper},
	{NULL, "newpass", party_newpass},
	{NULL, "help", party_help},
	{NULL, "part", party_part},
	{NULL, "quit", party_quit},
	{NULL, "who", party_who},
	{NULL, "whois", party_whois},
	{"n", "addlog", party_addlog},
	{"n", "get", party_get},
	{"n", "set", party_set},
	{"n", "unset", party_unset},
	{"n", "status", party_status},
	{"n", "save", party_save},
	{"n", "die", party_die},
	{"n", "+user", party_plus_user},
	{"n", "-user", party_minus_user},
	{"n", "chattr", party_chattr},
	{"n", "modules", party_modules},
	{"n", "loadmod", party_loadmod},
	{"n", "unloadmod", party_unloadmod},
	{"m", "+host", party_plus_host},
	{"m", "-host", party_minus_host},
	{0}
};

void core_party_init(void)
{
	bind_add_list("party", core_party_binds);
}
