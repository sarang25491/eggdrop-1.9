#include <eggdrop/eggdrop.h>

#include "server.h"
#include "serverlist.h"
#include "output.h"

static void parse_server(const char *text, char **host, int *port, char **pass)
{
	char *sep, *sport = NULL;

	*pass = NULL;
	*port = 0;
	*host = strdup(text);
	sep = strchr(*host, ' ');
	if (sep) {
		*sep++ = 0;
		sport = sep;
		sep = strchr(sep, ' ');
		if (sep) {
			*sep++ = 0;
			*pass = sep;
		}
		*port = atoi(sport);
	}
}

static int party_plus_server(int pid, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char *host, *pass;
	int port;

	parse_server(text, &host, &port, &pass);
	server_add(host, port, pass);
	partyline_printf(pid, "Added %s:%d\n", host, port ? port : config.default_port);
	free(host);
	return(0);
}

static int party_minus_server(int pid, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char *host, *pass;
	int port, i;

	parse_server(text, &host, &port, &pass);
	i = server_find(host, port, pass);
	if (i < 0) partyline_printf(pid, "No matching servers.\n");
	else {
		server_del(i);
		partyline_printf(pid, "Deleted server %d\n", i+1);
	}
	free(host);
	return(0);
}

static int party_dump(int pid, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (!*text) {
		partyline_printf(pid, "Usage: dump <server stuff>\n");
		return(0);
	}
	printserv(SERVER_NORMAL, "%s\r\n", text);
	return(BIND_RET_LOG);
}

bind_list_t server_party_commands[] = {
	{"+server", party_plus_server},
	{"-server", party_minus_server},
	{"dump", party_dump},
	{0}
};
