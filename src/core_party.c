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

static bind_list_t core_party_binds[] = {
	{"quit", party_quit},
	{"msg", party_msg},
	{0}
};

void core_party_init()
{
	bind_add_list("party", core_party_binds);
}
