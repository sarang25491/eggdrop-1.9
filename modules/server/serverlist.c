#include <stdlib.h>
#include <string.h>

#include "serverlist.h"

/*
 * serverlist.c maintains the connection of servers and gets the next entry
 * when we want to connect.
 */

server_t *server_list = NULL;
int server_list_index = -1;
int server_list_len = 0;

/* Get the next server from the list. */
server_t *server_get_next()
{
	server_t *serv;
	int i;

	if (server_list_len <= 0) return(NULL);
	server_list_index++;
	if (server_list_index >= server_list_len) server_list_index = 0;

	serv = server_list;
	for (i = 0; i < server_list_index && serv; i++) {
		serv = serv->next;
	}
	return(serv);
}

void server_set_next(int next)
{
	server_list_index = next-1;
}

/* Add a server to the server list. */
int server_add(char *host, int port, char *pass)
{
	server_t *serv, *ptr;

	/* Fill out a server entry. */
	serv = calloc(1, sizeof(*serv));
	serv->host = strdup(host);
	serv->port = port;
	if (pass) serv->pass = strdup(pass);

	/* Append the server to the list. */
	for (ptr = server_list; ptr && ptr->next; ptr = ptr->next) {
		; /* empty */
	}
	if (ptr) ptr->next = serv;
	else server_list = serv;

	server_list_len++;

	return(0);
}

static void server_free(server_t *serv)
{
	free(serv->host);
	if (serv->pass) free(serv->pass);
	free(serv);
}

/* Remove a server from the server list based on its index. */
int server_del(int num)
{
	server_t *cur, *prev;
	int i;

	prev = NULL;
	i = 0;
	for (cur = server_list; cur; cur = cur->next) {
		if (i == num) break;
		prev = cur;
		i++;
	}
	if (!cur) return(-1);

	if (prev) prev->next = cur->next;
	else server_list = cur->next;

	server_free(cur);

	/* If we removed a server from underneath the current pointer, we got
	 * bumped down. The list also shrinks by 1. */
	if (num < server_list_index) server_list_index--;
	server_list_len--;

	return(0);
}

/* Return the index of the first matching server in the list. */
int server_find(const char *host, int port, const char *pass)
{
	server_t *serv;
	int i, found;

	i = 0;
	for (serv = server_list; serv; serv = serv->next) {
		found = 0;
		if (!host) found++;
		else if (!strcasecmp(serv->host, host)) found++;

		if (!port) found++;
		else if (serv->port == port) found++;

		if (!pass) found++;
		else if (!strcasecmp(serv->pass, pass)) found++;

		if (found == 3) return(i);
		i++;
	}
	return(-1);
}

/* Clear out the server list. */
int server_clear()
{
	server_t *cur, *next;

	for (cur = server_list; cur; cur = next) {
		next = cur->next;
		server_free(cur);
	}
	server_list = NULL;
	server_list_len = 0;
	server_list_index = -1;
	return(0);
}
