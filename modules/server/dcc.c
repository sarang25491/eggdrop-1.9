/*
 * dcc.c -- take care of dcc stuff
 */

#include "lib/eggdrop/module.h"
#include "server.h"
#include "output.h"
#include "parse.h"
#include "binds.h"
#include <netinet/in.h>

typedef struct dcc_chat {
	int serv, idx;
	int timer_id, timeout;
} dcc_chat_t;

static int dcc_chat_server_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port);
static int dcc_chat_server_delete(void *client_data, int idx);
static int dcc_chat_client_delete(void *client_data, int idx);
static int dcc_chat_timeout(void *client_data);

static sockbuf_handler_t dcc_chat_server_handler = {
	"DCC chat server",
	NULL, NULL, dcc_chat_server_newclient,
	NULL, NULL,
	dcc_chat_server_delete
};

#define DCC_FILTER_LEVEL	(SOCKBUF_LEVEL_INTERNAL+1)
static sockbuf_filter_t dcc_chat_client_filter = {
	"DCC chat client", DCC_FILTER_LEVEL,
	NULL, NULL, NULL,
	NULL, NULL,
	NULL, dcc_chat_client_delete
};

/* Starts a chat with the specified nick.
 * If timeout is 0, the default timeout is used.
 * If timeout is -1, there is no timeout.
 * We don't make sure the nick is valid, so if it's not, you won't know until
 * the chat times out. */
int dcc_start_chat(const char *nick, int timeout)
{
	dcc_chat_t *chat;
	int serv, idx, longip, port;
	egg_timeval_t howlong;

	/* Create a listening idx to accept the chat connection. */
	serv = egg_listen(0, &port);
	if (serv < 0) return(serv);
	chat = calloc(1, sizeof(*chat));
	sockbuf_set_handler(serv, &dcc_chat_server_handler, chat);

	/* Create an empty sockbuf that will eventually be used for the
	 * dcc chat (when they connect to us). */
	idx = sockbuf_new();
	sockbuf_attach_filter(idx, &dcc_chat_client_filter, chat);

	chat->serv = serv;
	chat->idx = idx;

	/* See if they want the default timeout. */
	if (!timeout) timeout = config.dcc_timeout;

	if (timeout > 0) {
		howlong.sec = timeout;
		howlong.usec = 0;
		chat->timer_id = timer_create_complex(&howlong, dcc_chat_timeout, chat, 0);
	}

	longip = htonl(getmyip());
	printserv(SERVER_NORMAL, "PRIVMSG %s :%cDCC CHAT chat %u %d%c", nick, 1, longip, port, 1);
	return(idx);
}

static int dcc_chat_server_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port)
{
	int sock;
	dcc_chat_t *chat = client_data;

	putlog(LOG_MISC, "*", "dcc_chat_server: got new client (%s:%d)", peer_ip, peer_port);

	sock = sockbuf_get_sock(newidx);
	sockbuf_set_sock(newidx, -1, 0);
	sockbuf_delete(newidx);

	/* Give the waiting sockbuf the new socket and mark it as a client
	 * so that it fires the on_connect event. */
	sockbuf_set_sock(chat->idx, sock, SOCKBUF_CLIENT);

	/* Delete the listening idx since it's not needed. */
	chat->idx = -1;
	sockbuf_delete(idx);
	return(0);
}

static int dcc_chat_server_delete(void *client_data, int idx)
{
	dcc_chat_t *chat = client_data;

	/* If the timer is still going, kill it. */
	if (chat->timer_id != -1) {
		timer_destroy(chat->timer_id);
		chat->timer_id = -1;
	}

	/* If we're still waiting for the connection to come in, then send
	 * an error to the associated client sock. */
	if (chat->idx != -1) {
		sockbuf_detach_filter(chat->idx, &dcc_chat_client_filter, NULL);
		if (chat->timeout) {
			sockbuf_on_eof(chat->idx, DCC_FILTER_LEVEL, -1, "DCC chat timed out");
		}
		else {
			sockbuf_on_eof(chat->idx, DCC_FILTER_LEVEL, -1, "DCC chat listening socket unexpectedly closed");
		}
	}
	free(chat);
	return(0);
}

static int dcc_chat_client_delete(void *client_data, int idx)
{
	dcc_chat_t *chat = client_data;

	/* If the listening idx is still open, close it down. */
	if (chat->serv != -1) {
		chat->idx = -1;
		sockbuf_delete(chat->serv);
	}
	return(0);
}

static int dcc_chat_timeout(void *client_data)
{
	dcc_chat_t *chat = client_data;

	chat->timeout = 1;
	chat->timer_id = -1;
	sockbuf_delete(chat->serv);
	return(0);
}

static void got_chat(char *nick, char *uhost, user_t *u, char *text)
{
	char type[256], ip[256], port[32];
	int nport;

	sscanf(text, "%255s %255s %31s", type, ip, port);

	/* Check if the ip is 'new-style' (dotted decimal or ipv6). If not,
	 * it's the old-style long ip. */
	if (!strchr(ip, '.') && !strchr(ip, ':')) {
		int longip;
		longip = atoi(ip);
		sprintf(ip, "%d.%d.%d.%d", longip & 255, (longip >> 8) & 255, (longip >> 16) & 255, (longip >> 24) & 255);
	}

	nport = atoi(port);

	check_bind(BT_dcc_chat, nick, NULL, nick, uhost, u, type, ip, nport);
}

/* PRIVMSG ((target) :^ADCC CHAT ((type) ((longip) ((port)^A */
static int got_dcc(char *nick, char *uhost, struct userrec *u, char *cmd, char *text)
{
	if (!strncasecmp(text, "chat ", 5)) {
		got_chat(nick, uhost, u, text+5);
	}
	else if (!strncasecmp(text, "send ", 5)) {
		//got_send(nick, uhost, u, text+5);
	}
	return(0);
}

cmd_t my_ctcp_binds[] = {
	{"DCC", "", (Function) got_dcc, NULL},
	{NULL, NULL, NULL, NULL}
};
