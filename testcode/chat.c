#include <stdio.h>
#include <signal.h>
#include "my_socket.h"
#include "sockbuf.h"

static int *idx_array = NULL;
static int array_len = 0;

static int client_read(void *client_data, int idx, char *data, int len)
{
	char buf[512];
	int i, buflen;

	//printf("read %d from %d\n", iobuf->len, idx);
	data[len] = 0;
	snprintf(buf, sizeof(buf), "<%d> %s\n", idx, data);
	buf[sizeof(buf)-1] = 0;
	//printf("%s", buf);
	buflen = strlen(buf);
	for (i = 0; i < array_len; i++) {
		if (idx_array[i] != idx) {
			sockbuf_write(idx_array[i], buf, buflen);
		}
	}
	return(0);
}

static int client_eof(void *client_data, int idx, int err, const char *errmsg)
{
	char buf[128];
	int i, buflen;

	printf("eof from %d (%s)\n", idx, errmsg);
	for (i = 0; i < array_len; i++) {
		if (idx_array[i] == idx) break;
	}
	array_len--;
	memmove(idx_array+i, idx_array+i+1, sizeof(int) * (array_len-i));
	sockbuf_delete(idx);
	sprintf(buf, "%d disconnected\n", idx);
	buflen = strlen(buf);
	for (i = 0; i < array_len; i++) {
		sockbuf_write(idx_array[i], buf, buflen);
	}
	return(0);
}

static sockbuf_handler_t client_handler = {
	"client",
	NULL, client_eof, NULL,
	client_read, NULL
};

static int server_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port)
{
	char buf[512];
	int buflen, i;

	//sslmode_on(newidx, 1);
	//zipmode_on(newidx);
	linemode_on(newidx);
	sockbuf_set_handler(newidx, &client_handler, NULL);

	sockbuf_write(newidx, "Hello!\n", 7);
	printf("New connection %d\n", newidx);
	sprintf(buf, "New connection %d from %s %d\n", newidx, peer_ip, peer_port);
	buflen = strlen(buf);
	for (i = 0; i < array_len; i++) {
		sockbuf_write(idx_array[i], buf, buflen);
	}
	idx_array = (int *)realloc(idx_array, sizeof(int) * (array_len+1));
	idx_array[array_len] = newidx;
	array_len++;
	return(0);
}

static sockbuf_handler_t server_handler = {
	"server",
	NULL, NULL, server_newclient,
	NULL, NULL
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
	idx = sockbuf_new();
	sockbuf_set_sock(idx, sock, SOCKBUF_SERVER);
	sockbuf_set_handler(idx, &server_handler, NULL);
	while (1) {
		sockbuf_update_all(-1);
	}
	return(0);
}
