#ifndef MY_SOCKET_H
#define MY_SOCKET_H

#define SOCKET_CLIENT	1
#define SOCKET_SERVER	2
#define SOCKET_NONBLOCK	4
#define SOCKET_TCP	8
#define SOCKET_UDP	16

int socket_create(char *ipaddr, int port, int flags);
int socket_set_nonblock(int desc, int value);

#endif
