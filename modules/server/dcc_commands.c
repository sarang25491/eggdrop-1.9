#include "lib/eggdrop/module.h"

#include "server.h"
#include "serverlist.h"
#include "output.h"

static void parse_server(char *text, char **host, int *port, char **pass)
{
	char *sep, *sport = NULL;

	*pass = NULL;
	*port = 0;
	*host = text;
	sep = strchr(text, ' ');
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

static int cmd_plus_server(struct userrec *u, int idx, char *text)
{
	char *host, *pass;
	int port;

	parse_server(text, &host, &port, &pass);
	server_add(host, port, pass);
	dprintf(idx, "Added %s:%d\n", host, port ? port : config.default_port);
	return(0);
}

static int cmd_dump(struct userrec *u, int idx, char *par)
{
  if (!par[0]) {
    dprintf(idx, "Usage: dump <server stuff>\n");
    return(0);
  }
  putlog(LOG_CMDS, "*", "#%s# dump %s", dcc[idx].nick, par);
  printserv(SERVER_NORMAL, "%s\r\n", par);
  return(0);
}

cmd_t my_dcc_commands[] =
{
  {"dump",		"m",	(Function) cmd_dump,		NULL},
  {NULL,		NULL,	NULL,				NULL}
};
