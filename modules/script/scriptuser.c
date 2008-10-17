/* scriptuser.c: user-related scripting commands
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
static const char rcsid[] = "$Id: scriptuser.c,v 1.13 2008/10/17 15:57:43 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include "egg_script_internal.h"

static char *script_uid_to_handle(int uid)
{
	user_t *u;

	u = user_lookup_by_uid(uid);
	if (u) return(u->handle);
	return(NULL);
}

static int script_handle_to_uid(user_t *u)
{
	if (u) return(u->uid);
	return(0);
}

static int script_validhandle(char *handle)
{
	if (user_lookup_by_handle(handle)) return(1);
	return(0);
}

static int script_validuid(int uid)
{
	if (user_lookup_by_uid(uid)) return(1);
	return(0);
}

static int script_adduser(char *handle)
{
	user_t *u;

	u = user_new(handle);
	if (!u) return(-1);
	return(0);
}

static int script_get_ircmasks(script_var_t *retval, user_t *u)
{
	retval->type = SCRIPT_ARRAY | SCRIPT_STRING;
	retval->len = u->nircmasks;
	retval->value = u->ircmasks;
	return(0);
}

static char *script_user_find(char *irchost)
{
	user_t *u;
	
	u = user_lookup_by_irchost_nocache(irchost);
	if (u) return(u->handle);
	return(NULL);
}

static char *script_user_get(int nargs, user_t *u, char *chan, char *setting)
{
	char *value;

	/* If there's a missing arg, it's the channel. */
	if (nargs == 2) {
		setting = chan;
		chan = NULL;
	}

	user_get_setting(u, chan, setting, &value);
	return(value);
}

static int script_user_set(int nargs, user_t *u, char *chan, char *setting, char *value)
{
	int i;

	/* If there's a missing arg, it's the channel. */
	if (nargs == 3) {
		value = setting;
		setting = chan;
		chan = NULL;
	}

	i = user_set_setting(u, chan, setting, value);
	return(i);
}

static int script_user_save(char *fname)
{
	if (!fname) fname = "_users.xml";
	user_save(fname);
	return(0);
}

static int script_setflags(user_t *u, char *chan, char *flags)
{
	if (!flags) {
		flags = chan;
		chan = NULL;
	}

	user_set_flags_str(u, chan, flags);
	return(0);
}

static char *script_getflags(user_t *u, char *chan)
{
	flags_t flags;
	char flagstr[64];

	if (user_get_flags(u, chan, &flags)) return(NULL);
	flag_to_str(&flags, flagstr);
	return strdup(flagstr);
}

static int script_matchflags(void *client_data, user_t *u, char *chan, char *flagstr)
{
	flags_t flags_left, flags_right;
	int r = 0;

	if (!flagstr) {
		flagstr = chan;
		chan = NULL;
	}
	if (user_get_flags(u, chan, &flags_right)) return(0);
	flag_from_str(&flags_left, flagstr);

	switch ((int)client_data) {
		case 0:
			/* Are all bits on in left also on in right? */
			r = flag_match_subset(&flags_left, &flags_right);
			break;
		case 1:
			/* Is at least one bit on in the left also on in right? */
			r = flag_match_partial(&flags_left, &flags_right);
			break;
	}
	return(r);
}

script_command_t script_new_user_cmds[] = {
	{"user", "uid_to_handle", script_uid_to_handle, NULL, 1, "i", "uid", SCRIPT_STRING, 0},	/* DDD */
	{"user", "handle_to_uid", script_handle_to_uid, NULL, 1, "U", "handle", SCRIPT_INTEGER, 0},	/* DDD */
	{"user", "validhandle", script_validhandle, NULL, 1, "s", "handle", SCRIPT_INTEGER, 0},	/* DDD */
	{"user", "validuid", script_validuid, NULL, 1, "i", "uid", SCRIPT_INTEGER, 0},			/* DDD */
	{"user", "add", script_adduser, NULL, 1, "s", "handle", SCRIPT_INTEGER, 0},			/* DDD */
	{"user", "delete", user_delete, NULL, 1, "U", "user", SCRIPT_INTEGER, 0},			/* DDD */
	{"user", "addmask", user_add_ircmask, NULL, 2, "Us", "user ircmask", SCRIPT_INTEGER, 0},	/* DDD */
	{"user", "getmasks", script_get_ircmasks, NULL, 1, "U", "user", 0, SCRIPT_PASS_RETVAL},	/* DDD */
	{"user", "delmask", user_del_ircmask, NULL, 2, "Us", "user ircmask", SCRIPT_INTEGER, 0},	/* DDD */
	{"user", "find", script_user_find, NULL, 1, "s", "irchost", SCRIPT_STRING, 0},			/* DDD */
	{"user", "get", script_user_get, NULL, 2, "Uss", "user ?channel? setting", SCRIPT_STRING, SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},	/* DDD */
	{"user", "set", script_user_set, NULL, 3, "Usss", "user ?channel? setting value", SCRIPT_INTEGER, SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},	/* DDD */
	{"user", "getflags", script_getflags, NULL, 1, "Us", "user ?chan?", SCRIPT_STRING | SCRIPT_FREE, SCRIPT_VAR_ARGS},	/* DDD */
	{"user", "setflags", script_setflags, NULL, 2, "Uss", "user ?chan? flags", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},	/* DDD */
	{"user", "matchflags", script_matchflags, 0, 2, "Uss", "user ?chan? flags", SCRIPT_INTEGER, SCRIPT_VAR_ARGS | SCRIPT_PASS_CDATA},	/* DDD */
	{"user", "matchflags_or", script_matchflags, (void *)1, 2, "Uss", "user ?chan? flags", SCRIPT_INTEGER, SCRIPT_VAR_ARGS | SCRIPT_PASS_CDATA},	/* DDD */
	{"user", "load", user_save, NULL, 0, "s", "?fname?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},	/* DDD */
	{"user", "save", script_user_save, NULL, 0, "s", "?fname?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},	/* DDD */
	{"user", "haspass", user_has_pass, NULL, 1, "U", "user", SCRIPT_INTEGER, 0},	/* DDD */
	{"user", "checkpass", user_check_pass, NULL, 2, "Us", "user pass", SCRIPT_INTEGER, 0},	/* DDD */
	{"user", "setpass", user_set_pass, NULL, 2, "Us", "user pass", SCRIPT_INTEGER, 0},	/* DDD */
	{0}
};
