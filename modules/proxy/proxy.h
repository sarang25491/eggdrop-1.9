#ifndef _PROXY_H_
#define _PROXY_H_

typedef struct {
	char *username;
	char *password;
	char *host;
	int port;
	char *type;
} proxy_config_t;

/* Defined in proxy.c */
extern proxy_config_t proxy_config;

/* From http.c */
int http_reconnect(int idx, const char *host, int port);

#endif
