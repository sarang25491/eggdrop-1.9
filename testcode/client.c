#include <stdio.h>
#include <stdlib.h>

#include "my_socket.h"
#include "sockbuf.h"

int server_idx;

int server_read(void *client_data, int idx, char *data, int len)
{
	printf("%s\n", data);
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
	sslmode_on(idx, 0); /* 0 means client, 1 means server */
	zipmode_on(idx);
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
	sock = socket_create(host, port, NULL, 0, SOCKET_CLIENT | SOCKET_NONBLOCK);
	if (sock < 0) {
		perror("socket_create");
		return(0);
	}
	server_idx = sockbuf_new();
	sockbuf_set_sock(server_idx, sock, SOCKBUF_CLIENT);
	sockbuf_set_handler(server_idx, &server_event, NULL);
	sslmode_init();

	stdin_idx = sockbuf_new();
	socket_set_nonblock(0, 1);
	sockbuf_set_sock(stdin_idx, 0, 0);
	sockbuf_set_handler(stdin_idx, &stdin_event, NULL);
	linemode_on(stdin_idx);
	while (1) {
		sockbuf_update_all(1000);
	}
	return(0);
}
