#include <stdio.h>
#include <signal.h>
#include "my_socket.h"
#include "sockbuf.h"

static int *idx_array = NULL;
static int array_len = 0;

static int client_read(int idx, sockbuf_iobuf_t *iobuf, void *client_data)
{
	char buf[128];
	int i, buflen;

	//printf("read %d from %d\n", iobuf->len, idx);
	iobuf->data[iobuf->len] = 0;
	snprintf(buf, 128, "<%d> %s\n", idx, iobuf->data);
	//printf("%s", buf);
	buflen = strlen(buf);
	iobuf->data[iobuf->len++] = '\n';
	for (i = 0; i < array_len; i++) {
		if (idx_array[i] != idx) {
			sockbuf_write(idx_array[i], buf, buflen);
		}
	}
	return(0);
}

static int client_eof(int idx, void *ignore, void *client_data)
{
	char buf[128];
	int i, buflen;

	printf("eof from %d\n", idx);
	for (i = 0; i < array_len; i++) {
		if (idx_array[i] == idx) break;
	}
	array_len--;
	memmove(idx_array+i, idx_array+i+1, sizeof(int) * (array_len-i));
	sockbuf_delete(idx);
	snprintf(buf, 128, "%d disconnected\n", idx);
	buflen = strlen(buf);
	for (i = 0; i < array_len; i++) {
		sockbuf_write(idx_array[i], buf, buflen);
	}
	return(0);
}

static sockbuf_event_t client_handler = {
	(Function) 4,
	(Function) "client",
	client_read,
	NULL,
	client_eof,
	NULL
};

static int server_read(int idx, int serversock, void *client_data)
{
	char buf[128];
	int buflen, newsock, newidx, i;

	newsock = accept(serversock, NULL, NULL);
	if (newsock < 0) return(0);

	socket_set_nonblock(newsock, 1);
	newidx = sockbuf_new(newsock, 0);
	sslmode_on(newidx, 1);
	zipmode_on(newidx);
	linemode_on(newidx);
	sockbuf_set_handler(newidx, client_handler, NULL);

	sockbuf_write(newidx, "Hello!\n", 7);
	printf("New connection %d\n", newidx);
	snprintf(buf, 128, "New connection %d\n", newidx);
	buflen = strlen(buf);
	for (i = 0; i < array_len; i++) {
		sockbuf_write(idx_array[i], buf, buflen);
	}
	idx_array = (int *)realloc(idx_array, sizeof(int) * (array_len+1));
	idx_array[array_len] = newidx;
	array_len++;
	return(0);
}

static sockbuf_event_t server_handler = {
	(Function) 1,
	(Function) "server",
	server_read
};

main (int argc, char *argv[])
{
	int sock, idx, port;
	char *host;

	if (argc > 1) host = argv[1];
	else host = NULL;
	if (argc > 2) port = atoi(argv[2]);
	else port = 12345;

	if (host && !strcmp(host, "-")) host = NULL;

	signal(SIGPIPE, SIG_IGN);
	srand(time(0));
	sslmode_init();
	sock = socket_create(NULL, 0, host, port, SOCKET_SERVER);
	if (sock < 0) {
		perror("socket_create");
		return(0);
	}
	idx = sockbuf_new(sock, SOCKBUF_SERVER);
	sockbuf_set_handler(idx, server_handler, NULL);
	while (1) {
		sockbuf_update_all(-1);
	}
	return(0);
}
