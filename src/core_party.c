#include <eggdrop/eggdrop.h>

static int party_quit(int pid, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (!strlen(text)) text = NULL;
	partyline_disconnect(pid, text);
	return(0);
}

static int party_msg(int pid, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return(0);
}

static int party_set(int pid, const char *nick, user_t *u, const char *cmd, const char *text)
{
	return(0);
}

static int party_save(int pid, const char *nick, user_t *u, const char *cmd, const char *text)
{
	core_config_save();
	return(1);
}

static bind_list_t core_party_binds[] = {
	{"quit", party_quit},
	{"msg", party_msg},
	{"set", party_set},
	{"save", party_save},
	{0}
};

void core_party_init()
{
	bind_add_list("party", core_party_binds);
}
