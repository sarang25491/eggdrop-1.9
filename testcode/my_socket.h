#ifndef MY_SOCKET_H
#define MY_SOCKET_H

#define SOCKET_CLIENT	1
#define SOCKET_SERVER	2
#define SOCKET_BIND	4
#define SOCKET_NONBLOCK	8
#define SOCKET_TCP	16
#define SOCKET_UDP	32

int socket_create(char *dest_ip, int dest_port, char *src_ip, int src_port, int flags);
int socket_set_nonblock(int desc, int value);

#endif
