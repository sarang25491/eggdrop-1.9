#include <stdlib.h>
#include <string.h>

#include "lib/eggdrop/module.h"
#include "nicklist.h"
#include "output.h"

/*
 * serverlist.c maintains the connection of servers and gets the next entry
 * when we want to connect.
 */

char **nick_list = NULL;
int nick_list_index = -1;
int nick_list_len = 0;
int nick_list_cycled = 0;

void try_next_nick()
{
	const char *nick;

	nick = nick_get_next();
	if (!nick) {
		putlog(LOG_MISC, "*", "try_next_nick: using random nick because %s.", nick_list_len ? "none of the defined nicks are valid" : "there are no nicks defined");
		try_random_nick();
		return;
	}
	printserv(SERVER_MODE, "NICK %s\r\n", nick);
}

void try_random_nick()
{
	printserv(SERVER_MODE, "NICK egg%d\r\n", random());
}

void nick_list_on_connect()
{
	nick_list_index = -1;
	nick_list_cycled = 0;
}

/* Get the next server from the list. */
const char *nick_get_next()
{
	if (nick_list_len <= 0) return(NULL);
	nick_list_index++;
	if (nick_list_index >= nick_list_len) {
		if (nick_list_cycled) return(NULL);
		nick_list_cycled = 1;
		nick_list_index = 0;
	}

	return(nick_list[nick_list_index]);
}

/* Add a server to the server list. */
int nick_add(const char *nick)
{
	nick_list = realloc(nick_list, sizeof(char *) * (nick_list_len+1));
	nick_list[nick_list_len] = strdup(nick);
	nick_list_len++;
	return(0);
}

/* Remove a server from the server list based on its index. */
int nick_del(int num)
{
	if (num >= 0 && num < nick_list_len) {
		free(nick_list[num]);
		memmove(nick_list+num, nick_list+num+1, nick_list_len-num-1);
	}
	if (num < nick_list_index) nick_list_index--;
	nick_list_len--;

	return(0);
}

/* Clear out the server list. */
int nick_clear()
{
	int i;

	for (i = 0; i < nick_list_len; i++) {
		free(nick_list[i]);
	}
	if (nick_list) free(nick_list);
	return(0);
}
