#include "lib/eggdrop/module.h"
#include "lib/eggdrop/eggdrop.h"

#include "server.h"
#include "servsock.h"
#include "input.h"
#include "serverlist.h"
#include "channels.h"
#include "nicklist.h"
#include "output.h"
#include "dcc.h"

/* From server.c. */
extern int cycle_delay;

static void disconnect_server();

static int server_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);

static int server_on_eof(void *client_data, int idx, int err, const char *errmsg);

static int server_on_read(void *client_data, int idx, char *text, int len);

static int server_on_delete(void *client_data, int idx);

static sockbuf_handler_t server_handler = {
	"server",
	server_on_connect, server_on_eof, NULL,
	server_on_read, NULL,
	server_on_delete
};

/**********************************************************************
 * Connection management
 */

void connect_to_next_server() {
	server_t *serv;

	serv = server_get_next();
	if (!serv) {
		putlog(LOG_MISC, "*", "Error connecting to next server: no servers are defined!");
		return;
	}

	str_redup(&current_server.server_host, serv->host);
	str_redup(&current_server.server_self, serv->host);
	str_redup(&current_server.pass, serv->pass);
	current_server.strcmp = strcasecmp;
	if (!server_config.fake005) server_config.fake005 = strdup(":fakeserver 005 fakenick MODES=3 MAXCHANNELS=10 MAXBANS=100 NICKLEN=9 TOPICLEN=307 KICKLEN=307 CHANTYPES=#& PREFIX=(ov)@+ NETWORK=fakenetwork CASEMAPPING=rfc1459 CHANMODES=b,k,l,imnprst :are available on this server");

	if (serv->port) current_server.port = serv->port;
	else current_server.port = server_config.default_port;

	current_server.idx = egg_connect(current_server.server_host, current_server.port, server_config.connect_timeout);
	sockbuf_set_handler(current_server.idx, &server_handler, NULL);
}

/* Close the current server connection. */
void kill_server(const char *reason)
{
	if (reason && (current_server.idx > -1)) printserv(SERVER_NOQUEUE, "QUIT :%s\r\n", reason);
	disconnect_server();
}

/* Clean up stuff when we want to disconnect or are disconnected. */
static void disconnect_server()
{
	int connected = current_server.connected;

	cycle_delay = server_config.cycle_delay;

	current_server.connected = 0;
	current_server.registered = 0;
	current_server.got005 = 0;

	if (current_server.idx != -1) {
		sockbuf_delete(current_server.idx);
		current_server.idx = -1;
	}

	if (connected) check_bind_event("disconnect-server");
}











/****************************************************************
 * Sockbuf handlers
 */

/* When we get an eof or an error in the connection. */
static int server_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	putlog(LOG_SERV, "*", "Disconnected from %s", current_server.server_self);
	if (err && errmsg) putlog(LOG_SERV, "*", "Network error %d: %s", err, errmsg);
	disconnect_server();
	return(0);
}

static int server_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	/* If ip_lookup is 0, then we get our ip address from the socket we used
	 * to connect to the server. If it's 1, it's handled in got001(). */
	if (server_config.ip_lookup == 0) {
		int sock;
		char *ip;

		sock = sockbuf_get_sock(idx);
		socket_get_name(sock, &ip, NULL);
		dcc_dns_set(ip);
		free(ip);
	}

	linemode_on(current_server.idx);

	current_server.connected = 1;
	nick_list_on_connect();
	putlog(LOG_SERV, "*", "Connected to %s, logging in.", current_server.server_host);
	check_bind_event("connect-server");
	if (!current_server.connected) return(0);

	printserv(SERVER_NOQUEUE, "USER %s localhost %s :%s\r\n", server_config.user, current_server.server_host, server_config.realname);
	if (current_server.pass) printserv(SERVER_NOQUEUE, "PASS %s\r\n", current_server.pass);
	try_next_nick();
	return(0);
}

static int server_on_read(void *client_data, int idx, char *text, int len)
{
	if (!len) return(0);
	server_parse_input(text);
	return(0);
}

static int server_on_delete(void *client_data, int idx)
{
	return(0);
}
