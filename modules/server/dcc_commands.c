#include <eggdrop/eggdrop.h>
#include <ctype.h>

#include "server.h"
#include "serverlist.h"
#include "output.h"
#include "servsock.h"

static int party_servers(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	server_t *s;

	if (server_list_len <= 0) partymember_printf(p, "The server list is empty.");
	else partymember_printf(p, "%d server%s:", server_list_len, (server_list_len != 1) ? "s" : "");
	for (s = server_list; s; s = s->next) {
		if (s->port) partymember_printf(p, "   %s (port %d)%s", s->host, s->port, s->pass ? " (password set)" : "");
		else partymember_printf(p, "   %s (default port)%s", s->host, s->pass ? " (password set)" : "");
	}
	return(0);
}

static void parse_server(const char *text, char **host, int *port, char **pass)
{
	char *sep, *sport = NULL;

	*pass = NULL;
	*port = 0;
	while (isspace(*text)) text++;
	*host = strdup(text);
	sep = strchr(*host, ' ');
	if (sep) {
		*sep++ = 0;
		while (isspace(*sep)) sep++;
		sport = sep;
		sep = strchr(sep, ' ');
		if (sep) {
			*sep++ = 0;
			while (isspace(*sep)) sep++;
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
	if (!strlen(host)) {
		partymember_printf(p, "Please specify a valid host.");
		free(host);
		return(0);
	}
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
	free(host);
	if (i < 0) partymember_printf(p, "No matching servers.");
	else {
		server_del(i);
		partymember_printf(p, "Deleted server %d", i+1);
	}
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

static int party_jump(partymember_t *p, const char *nick, user_t *u, const char *cmd, const char *text)
{
	char *host, *pass;
	int port, num;

	if (text && *text) {
		parse_server(text, &host, &port, &pass);
		num = server_find(host, port, pass);
		if (num < 0) {
			server_add(host, port, pass);
			partymember_printf(p, "Added %s:%d\n", host, port ? port : server_config.default_port);
			num = 0;
		}
		free(host);
	}
	else num = server_list_index+1;
	server_set_next(num);
	kill_server("changing servers");
	return(0);
}

bind_list_t server_party_commands[] = {
	{"", "servers", party_servers},
	{"m", "+server", party_plus_server},
	{"m", "-server", party_minus_server},
	{"m", "dump", party_dump},
	{"m", "jump", party_jump},
	{0}
};
