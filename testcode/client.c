#include <stdio.h>
#include <stdlib.h>

#include "my_socket.h"
#include "sockbuf.h"

int server_idx;

int server_err(int idx, int err, void *client_data)
{
	perror("server_err");
	exit(0);
}

int server_connect(int idx, void *client_data)
{
	static int baa = 0;

	if (!baa) {
		printf("Connected to server!\n");
		sslmode_on(idx, 0);
		zipmode_on(idx);
		linemode_on(idx);
		baa++;
	}
	return(0);
}

int server_read(int idx, sockbuf_iobuf_t *data, void *client_data)
{
	printf("%s\n", data->data);
	return(0);
}

int server_eof(sockbuf_t *sbuf, void *client_data)
{
	printf("Server closed connection!\n");
	exit(0);
}

static sockbuf_event_t server_event = {
	(Function) 4,
	(Function) "server",
	server_read,
	server_connect,
	server_eof,
	server_err
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
	int sock, port, stdin_idx;
	char *host;

	srand(time(0));
	if (argc < 2) {
		host = "127.0.0.1";
		port = 12345;
	}
	else if (argc == 2) {
		host = argv[1];
		port = 12345;
	}
	else {
		host = argv[1];
		port = atoi(argv[2]);
	}

	printf("Connecting to %s %d\n", host, port);
	sock = socket_create(host, port, SOCKET_CLIENT);
	if (sock < 0) {
		perror("socket_create");
		return(0);
	}
	socket_set_nonblock(sock, 1);
	socket_set_nonblock(0, 1);
	server_idx = sockbuf_new(sock, SOCKBUF_CLIENT);
	sockbuf_set_handler(server_idx, server_event, NULL);
	//zipmode_on(server_idx);
	//linemode_on(server_idx);
	sslmode_init();

	stdin_idx = sockbuf_new(0, 0);
	sockbuf_set_handler(stdin_idx, stdin_event, NULL);
	linemode_on(stdin_idx);
	while (1) {
		sockbuf_update_all(-1);
	}
	return(0);
}
