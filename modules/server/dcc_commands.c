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

static int party_plus_server(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char *host, *pass;
	int port;

	if (!text || !*text) {
		partymember_printf(p, "Syntax: +server <host> [port] [pass]");
		return(0);
	}
	parse_server(text, &host, &port, &pass);
	server_add(host, port, pass);
	partymember_printf(p, "Added %s:%d\n", host, port ? port : server_config.default_port);
	free(host);
	return(0);
}

static int party_minus_server(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char *host, *pass;
	int port, i;

	if (!text || !*text) {
		partymember_printf(p, "Syntax: -server <host> [port] [pass]");
		return(0);
	}

	parse_server(text, &host, &port, &pass);
	i = server_find(host, port, pass);
	if (i < 0) partymember_printf(p, "No matching servers.");
	else {
		server_del(i);
		partymember_printf(p, "Deleted server %d", i+1);
	}
	free(host);
	return(0);
}

static int party_dump(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	if (!text || !*text) {
		partymember_printf(p, "Syntax: dump <server stuff>");
		return(0);
	}
	printserv(SERVER_NORMAL, "%s\r\n", text);
	return(BIND_RET_LOG);
}

bind_list_t server_party_commands[] = {
	{"m", "+server", party_plus_server},
	{"m", "-server", party_minus_server},
	{"m", "dump", party_dump},
	{0}
};
