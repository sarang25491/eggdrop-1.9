#include <stdio.h>
#include <stdlib.h>

#include "my_socket.h"
#include "sockbuf.h"

int server_idx;

int server_read(void *client_data, int idx, char *data, int len)
{
	printf("server: [%s]\n", data);
	return(0);
}

int server_eof(void *client_data, int idx, int err, const char *errmsg)
{
	if (err && errmsg) printf("Error: %s\n", errmsg);
	printf("Server closed connection!\n");
	exit(0);
}

int server_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	printf("Connected to server (%s %d)!\n", peer_ip, peer_port);
	//sslmode_on(idx, 0); /* 0 means client, 1 means server */
	//zipmode_on(idx);
	linemode_on(idx);
	return(0);
}

static sockbuf_handler_t server_event = {
	"server",
	server_connect, server_eof, NULL,
	server_read, NULL
};

int stdin_read(void *client_data, int idx, char *data, int len)
{
	printf("you said: [%s]\n", data);
	data[len] = '\n';
	sockbuf_write(server_idx, data, len+1);
	return(0);
}

int stdin_eof(void *client_data, int idx, int err, const char *errmsg)
{
	sockbuf_delete(idx);
	return(0);
}

static sockbuf_handler_t stdin_event = {
	"stdin",
	NULL, stdin_eof, NULL,
	stdin_read, NULL
};

main (int argc, char *argv[])
{
	int sock, port, proxy_port, stdin_idx, proxy, i;
	char *user, *pass, *host, *proxy_host;

	if (argc != 8) {
		printf("usage: %s proxy-type proxy-host proxy-port user pass host port\n", argv[0]);
		return(0);
	}

	srand(time(0));
	proxy = atoi(argv[1]);
	proxy_host = argv[2];
	proxy_port = atoi(argv[3]);
	user = argv[4];
	pass = argv[5];
	host = argv[6];
	port = atoi(argv[7]);

	printf("Connecting to %s %d by way of %s %d\n", host, port, proxy_host, proxy_port);

	if (proxy == 1) server_idx = socks5_connect(-1, proxy_host, proxy_port, user, pass, host, port);
	else if (proxy == 2) server_idx = socks4_connect(-1, proxy_host, proxy_port, user, host, port);
	else if (proxy == 3) server_idx = http_connect(-1, proxy_host, proxy_port, user, pass, host, port);
	else {
		printf("invalid proxy type\n");
		printf("1 - socks5, 2 - socks4, 3 - http\n");
		return(0);
	}

	sockbuf_set_handler(server_idx, &server_event, NULL);
//	sslmode_init();

	stdin_idx = sockbuf_new();
	sockbuf_set_sock(stdin_idx, 0, 0);
	sockbuf_set_handler(stdin_idx, &stdin_event, NULL);
	linemode_on(stdin_idx);
	while (1) {
		sockbuf_update_all(-1);
	}
	return(0);
}
