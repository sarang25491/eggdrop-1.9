#ifndef _EGGNET_H_
#define _EGGNET_H_

int egg_net_init();
int egg_iprintf(int idx, const char *format, ...);
int egg_server(const char *vip, int port, int *real_port);
int egg_client(const char *ip, int port, const char *vip, int vport);
int egg_listen(int port, int *real_port);
int egg_connect(const char *host, int port, int timeout);
int egg_reconnect(int idx, const char *host, int port, int timeout);
int egg_ident_lookup(const char *ip, int their_port, int our_port, int timeout, int (*callback)(), void *client_data);
int egg_dns_lookup(const char *host_or_ip, int timeout, int (*callback)(), void *client_data);

#endif /* _EGGNET_H_ */
