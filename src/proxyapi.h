#ifndef _PROXYAPI_H_
#define _PROXYAPI_H_

/* Interface definitions for proxy functions. */

typedef struct {
	char *name;
	int (*connect)(int idx, char *dest_host, int dest_port, proxy_t *proxy);
	int (*abort)(int idx);
} proxy_interface_t;

int proxy_register_module(proxy_interface_t *proxy);
int proxy_unregister_module(proxy_interface_t *proxy);

#endif /* _PROXYAPI_H_ */
