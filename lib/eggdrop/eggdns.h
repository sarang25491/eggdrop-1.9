#ifndef _EGGDNS_H_
#define _EGGDNS_H_

#define DNS_IPV4	1
#define DNS_IPV6	2
#define DNS_REVERSE	3

#define DNS_PORT	53

int egg_dns_init();
void egg_dns_send(char *query, int len);
int egg_dns_lookup(const char *host, int timeout, int (*callback)(), void *client_data);
int egg_dns_reverse(const char *ip, int timeout, int (*callback)(), void *client_data);
int egg_dns_cancel(int id, int issue_callback);

#endif /* _EGGDNS_H_ */