#include "main.h"
#include <eggdrop/users.h>

static char *script_uid_to_handle(int uid)
{
	user_t *u;

	u = user_lookup_by_uid(uid);
	if (u) return(u->handle);
	return(NULL);
}

static int script_handle_to_uid(char *handle)
{
	user_t *u;

	u = user_lookup_by_handle(handle);
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

static int script_user_addmask(int uid, char *ircmask)
{
	user_t *u = user_lookup_by_uid(uid);
	if (!u) return(-1);
	user_add_ircmask(u, ircmask);
	return(0);
}

static char *script_user_find(char *irchost)
{
	user_t *u;
	
	u = user_lookup_by_irchost_nocache(irchost);
	if (u) return(u->handle);
	return(NULL);
}

static char *script_user_get(int nargs, int uid, char *chan, char *setting)
{
	char *value;
	user_t *u;

	/* If there's a missing arg, it's the channel. */
	if (nargs == 2) {
		setting = chan;
		chan = NULL;
	}

	u = user_lookup_by_uid(uid);
	if (!u) return(NULL);

	user_get_setting(u, chan, setting, &value);
	return(value);
}

static int script_user_set(int nargs, int uid, char *chan, char *setting, char *value)
{
	int i;
	user_t *u;

	/* If there's a missing arg, it's the channel. */
	if (nargs == 3) {
		value = setting;
		setting = chan;
		chan = NULL;
	}

	u = user_lookup_by_uid(uid);
	if (!u) return(-1);
	i = user_set_setting(u, chan, setting, value);
	return(i);
}

static int script_user_save(char *fname)
{
	if (!fname) fname = "_users.xml";
	user_save(fname);
	return(0);
}

static int script_setflags(int uid, char *chan, char *flags)
{
	user_t *u;

	u = user_lookup_by_uid(uid);
	if (!u) return(-1);

	if (!flags) {
		flags = chan;
		chan = NULL;
	}

	//user_setflags(u, chan, flags);
	return(0);
}

static char *script_getflags(int uid, char *chan)
{
	user_t *u;

	u = user_lookup_by_uid(uid);
	if (!u) return(-1);
}

script_command_t script_new_user_cmds[] = {
	{"", "user_uid_to_handle", script_uid_to_handle, NULL, 1, "i", "uid", SCRIPT_STRING, 0},
	{"", "user_handle_to_uid", script_handle_to_uid, NULL, 1, "s", "handle", SCRIPT_INTEGER, 0},
	{"", "user_validhandle", script_validhandle, NULL, 1, "s", "handle", SCRIPT_INTEGER, 0},
	{"", "user_validuid", script_validuid, NULL, 1, "i", "uid", SCRIPT_INTEGER, 0},
	{"", "user_add", script_adduser, NULL, 1, "s", "handle", SCRIPT_INTEGER, 0},
	{"", "user_addmask", script_user_addmask, NULL, 2, "is", "uid ircmask", SCRIPT_INTEGER, 0},
	{"", "user_find", script_user_find, NULL, 1, "s", "irchost", SCRIPT_STRING, 0},
	{"", "user_get", script_user_get, NULL, 2, "iss", "uid ?channel? setting", SCRIPT_STRING, SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
	{"", "user_set", script_user_set, NULL, 3, "isss", "uid ?channel? setting value", SCRIPT_INTEGER, SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
	{"", "user_save", script_user_save, NULL, 0, "s", "?fname?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{0}
};
