#include <eggdrop/eggdrop.h>
#include "core_config.h"

static int party_join(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (!text || !strlen(text)) {
		partymember_write(p, "Syntax: join <channel>", -1);
		return(0);
	}
	partychan_join_name(text, p);
	return(0);
}

static int party_part(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	partychan_t *chan;

	if (!text || !strlen(text)) chan = partychan_get_default(p);
	else chan = partychan_lookup_name(text);
	partychan_part(chan, p, "parting");
	return(0);
}

static int party_quit(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (!text) text = "Quit";
	partymember_write(p, "goodbye!", -1);
	partymember_delete(p, text);
	return(0);
}

static int party_msg(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return(0);
}

static int party_set(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return(0);
}

static int party_save(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	user_save(core_config.userfile);
	core_config_save();
	return(1);
}

static int party_newpass(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (!text || strlen(text) < 6) {
		partymember_write(p, "Please use at least 6 characters.", -1);
		return(0);
	}
	user_set_pass(p->user, text);
	partymember_printf(p, "Changed password to '%s'", text);
	return(0);
}

static bind_list_t core_party_binds[] = {
	{NULL, "join", party_join},
	{NULL, "msg", party_msg},
	{NULL, "newpass", party_newpass},
	{NULL, "part", party_part},
	{NULL, "quit", party_quit},
	{"n", "set", party_set},
	{"n", "save", party_save},
	{0}
};

void core_party_init()
{
	bind_add_list("party", core_party_binds);
}
