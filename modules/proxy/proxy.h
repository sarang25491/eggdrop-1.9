#ifndef _PROXY_H_
#define _PROXY_H_

#define http_owner proxy_LTX_http_owner
#define socks5_owner proxy_LTX_socks5_owner

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
event_owner_t http_owner;

/* From socks5.c */
int socks5_reconnect(int idx, const char *host, int port);
event_owner_t socks5_owner;

#endif
