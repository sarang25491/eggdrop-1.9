#ifndef _SERVERLIST_H_
#define _SERVERLIST_H_

typedef struct server {
	struct server *next;
	char *host;
	int port;
	char *pass;
} server_t;

extern server_t *server_list;
extern int server_list_index;
extern int server_list_len;

server_t *server_get_next();
void server_set_next(int next);
int server_add(char *host, int port, char *pass);
int server_del(int num);
int server_find(const char *host, int port, const char *pass);
int server_clear();

#endif
