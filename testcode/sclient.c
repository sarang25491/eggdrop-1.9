#include <stdio.h>
#include <stdlib.h>

#include "my_socket.h"
#include "sockbuf.h"

int server_idx;

int server_read(int idx, sockbuf_iobuf_t *data, void *client_data)
{
	data->data[data->len] = 0;
	printf("%s\n", data->data);
	return(0);
}

int server_eof(int idx, void *ignore, void *client_data)
{
	printf("Server closed connection!\n");
	exit(0);
}

int server_err(int idx, void *err, void *client_data)
{
	perror("server_err");
	exit(0);
}

int server_connect(int idx, void *ignore, void *client_data)
{
	printf("Connected to server!\n");
	//sslmode_on(idx, 0); /* 0 means client, 1 means server */
	//zipmode_on(idx);
	//linemode_on(idx);
	return(0);
}

static sockbuf_event_t server_event = {
	(Function) 5,
	(Function) "server",
	server_read,
	NULL,
	server_eof,
	server_err,
	server_connect
};

int stdin_read(int idx, sockbuf_iobuf_t *data, void *client_data)
{
	data->data[data->len] = '\n';
	sockbuf_write(server_idx, data->data, data->len+1);
	return(0);
}

int stdin_eof(int idx, void *client_data)
{
	sockbuf_delete(idx);
	return(0);
}

static sockbuf_event_t stdin_event = {
	(Function) 4,
	(Function) "stdin",
	stdin_read,
	NULL,
	stdin_eof,
	NULL
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

	sockbuf_set_handler(server_idx, server_event, NULL);
	sslmode_init();

	stdin_idx = sockbuf_new(0, 0);
	sockbuf_set_handler(stdin_idx, stdin_event, NULL);
	linemode_on(stdin_idx);
	while (1) {
		sockbuf_update_all(-1);
	}
	return(0);
}
