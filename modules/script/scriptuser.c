#include <eggdrop/eggdrop.h>
#include <eggdrop/flags.h>
#include <eggdrop/users.h>

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

static int script_user_addmask(user_t *u, char *ircmask)
{
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

	user_set_flag_str(u, chan, flags);
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

script_command_t script_new_user_cmds[] = {
	{"", "user_uid_to_handle", script_uid_to_handle, NULL, 1, "i", "uid", SCRIPT_STRING, 0},
	{"", "user_handle_to_uid", script_handle_to_uid, NULL, 1, "U", "handle", SCRIPT_INTEGER, 0},
	{"", "user_validhandle", script_validhandle, NULL, 1, "s", "handle", SCRIPT_INTEGER, 0},
	{"", "user_validuid", script_validuid, NULL, 1, "i", "uid", SCRIPT_INTEGER, 0},
	{"", "user_add", script_adduser, NULL, 1, "s", "handle", SCRIPT_INTEGER, 0},
	{"", "user_addmask", script_user_addmask, NULL, 2, "Us", "user ircmask", SCRIPT_INTEGER, 0},
	{"", "user_find", script_user_find, NULL, 1, "s", "irchost", SCRIPT_STRING, 0},
	{"", "user_get", script_user_get, NULL, 2, "Uss", "user ?channel? setting", SCRIPT_STRING, SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
	{"", "user_set", script_user_set, NULL, 3, "Usss", "user ?channel? setting value", SCRIPT_INTEGER, SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
	{"", "user_getflags", script_getflags, NULL, 1, "Us", "user ?chan?", SCRIPT_STRING | SCRIPT_FREE, SCRIPT_VAR_ARGS},
	{"", "user_setflags", script_setflags, NULL, 2, "Uss", "user ?chan? flags", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "user_load", user_save, NULL, 0, "s", "?fname?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "user_save", script_user_save, NULL, 0, "s", "?fname?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "user_haspass", user_has_pass, NULL, 1, "U", "user", SCRIPT_INTEGER, 0},
	{"", "user_checkpass", user_check_pass, NULL, 2, "Us", "user pass", SCRIPT_INTEGER, 0},
	{"", "user_setpass", user_set_pass, NULL, 2, "Us", "user pass", SCRIPT_INTEGER, 0},
	{0}
};
