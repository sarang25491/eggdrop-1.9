#include <eggdrop/eggdrop.h>
#include <ctype.h>
#include "core_config.h"

static int party_join(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (!text || !*text) {
		partymember_printf(p, "Syntax: join <channel>");
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
	partymember_printf(p, "Goodbye!");
	if (!text || !*text) partymember_delete(p, "Quit");
	else partymember_delete(p, text);
	return(0);
}

static int party_whisper(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return(0);
}

static void *lookup_and_check(partymember_t *p, const char *path)
{
	void *root;
	char *flags;
	flags_t uflags, rflags;

	root = config_get_root("eggdrop");
	root = config_exists(root, path, 0, NULL);
	if (!root) {
		partymember_printf(p, "That setting was not found.");
		return(NULL);
	}

	config_get_str(&flags, root, "flags", 0, NULL);
	if (flags) {
		flag_from_str(&rflags, flags);
		if (!p->user) goto nope;
		user_get_flags(p->user, NULL, &uflags);
		if (!flag_match_partial(&uflags, &rflags)) goto nope;
	}
	return(root);
nope:
	partymember_printf(p, "You cannot access that setting.");
	return(NULL);
}

static int party_get(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	void *root;
	char *str = NULL;

	if (!text || !*text) {
		partymember_printf(p, "Syntax: get <path>");
		return(0);
	}

	root = lookup_and_check(p, text);
	if (!root) return(0);

	config_get_str(&str, root, NULL);
	if (str) {
		partymember_printf(p, "==> '%s'", str);
	}
	else {
		partymember_printf(p, "Config setting not found.");
	}
	return(0);
}

static int party_set(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	void *root;
	char *path, *str;
	const char *next;

	egg_get_word(text, &next, &path);
	if (next) while (isspace(*next)) next++;
	if (!next || !path || !*path) {
		partymember_printf(p, "Syntax: set <path> <new value>");
		if (path) free(path);
		return(0);
	}
	root = lookup_and_check(p, path);
	free(path);
	if (!root) return(0);

	config_get_str(&str, root, NULL);
	partymember_printf(p, "old value: '%s'", str);
	config_set_str(next, root, NULL);
	config_get_str(&str, root, NULL);
	partymember_printf(p, "new value: '%s'", str);
	return(0);
}

static int party_save(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	putlog(LOG_MISC, "*", "Saving user file...");
	user_save(core_config.userfile);
	putlog(LOG_MISC, "*", "Saving config file...");
	core_config_save();
	return(1);
}

static int party_newpass(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (!text || strlen(text) < 6) {
		partymember_printf(p, "Please use at least 6 characters.");
		return(0);
	}
	user_set_pass(p->user, text);
	partymember_printf(p, "Changed password to '%s'.", text);
	return(0);
}

static int intsorter(const void *left, const void *right)
{
	return(*(int *)left - *(int *)right);
}

static int party_who(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	partymember_t *who;
	int *pids, len, i;

	partymember_printf(p, "Partyline members:");
	partymember_who(&pids, &len);
	qsort(pids, len, sizeof(int), intsorter);
	for (i = 0; i < len; i++) {
		who = partymember_lookup_pid(pids[i]);
		partymember_printf(p, "  [%5d] %s (%s@%s)", who->pid, who->nick, who->ident, who->host);
	}
	free(pids);
	return(0);
}

static int party_die(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char *reason;

	if (!text || !*text) reason = "No reason given.";
	else reason = text;
	putlog(LOG_MISC, "*", "Saving user file...");
	user_save(core_config.userfile);
	putlog(LOG_MISC, "*", "Saving config file...");
	core_config_save();
	putlog(LOG_MISC, "*", "Bot shutting down: %s", reason);
	exit(0);
}

static int party_plus_user(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	user_t *newuser;

	if (!text || !*text) {
		partymember_printf(p, "Syntax: +user <handle>");
		return(0);
	}
	if (user_lookup_by_handle(text)) {
		partymember_printf(p, "User '%s' already exists!");
		return(0);
	}
	newuser = user_new(text);
	if (newuser) partymember_printf(p, "User '%s' created.", text);
	else partymember_printf(p, "Could not create user '%s'.", text);
	return(0);
}

static int party_minus_user(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	user_t *who;

	if (!text || !*text) {
		partymember_printf(p, "Syntax: -user <handle>");
		return(0);
	}
	who = user_lookup_by_handle(text);
	if (!who) partymember_printf(p, "User '%s' not found.");
	else {
		partymember_printf(p, "Deleting user '%s'.", who->handle);
		user_delete(who);
	}
	return(0);
}

static int party_plus_host(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return(0);
}

static int party_minus_host(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return(0);
}

static int party_chattr(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	const char *next;
	char *who, *flags, *chan;
	user_t *dest;
	flags_t flagstruct;
	char flagstr[64];
	int n;

	n = egg_get_words(text, &next, &who, &chan, &flags, NULL);
	if (!chan) {
		if (who) free(who);
		partymember_printf(p, "Syntax: chattr <user> [channel] <+/-flags>");
		return(0);
	}
	if (!flags) {
		flags = chan;
		chan = NULL;
	}
	dest = user_lookup_by_handle(who);
	if (dest) {
		user_set_flag_str(dest, chan, flags);
		user_get_flags(dest, chan, &flagstruct);
		flag_to_str(&flagstruct, flagstr);
		partymember_printf(p, "Flags for %s are now '%s'.", who, flagstr);
	}
	else {
		partymember_printf(p, "'%s' is not a valid user.", who);
	}
	free(who);
	free(flags);
	if (chan) free(chan);
	return(0);
}

static int party_modules(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char **modules;
	int nummods, ctr;

	nummods = get_module_list(&modules);
	partymember_printf(p, "Loaded modules:");
	for (ctr = 0; ctr < nummods; ctr++) partymember_printf(p, "   %s", modules[ctr]);
	free(modules);
	return(0);
}

static bind_list_t core_party_binds[] = {
	{NULL, "join", party_join},
	{NULL, "whisper", party_whisper},
	{NULL, "newpass", party_newpass},
	{NULL, "part", party_part},
	{NULL, "quit", party_quit},
	{NULL, "who", party_who},
	{"n", "get", party_get},
	{"n", "set", party_set},
	{"n", "save", party_save},
	{"n", "die", party_die},
	{"n", "+user", party_plus_user},
	{"n", "-user", party_minus_user},
	{"n", "+host", party_plus_host},
	{"n", "-host", party_minus_host},
	{"n", "chattr", party_chattr},
	{"n", "modules", party_modules},
	{"m", "+host", party_plus_host},
	{"m", "-host", party_minus_host},
	{0}
};

void core_party_init()
{
	bind_add_list("party", core_party_binds);
}
