#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include "my_socket.h"

#define DO_IPV6
#define DO_IPV4

/* Return values: */
/* -1: invalid ip address */
/* -2: socket() failure */
/* -3: bind() failure */
/* -4: connect() failure */
int socket_create(char *ipaddr, int port, int flags)
{
	int sock, pfamily, err, len;
	struct sockaddr *name;
	#ifdef DO_IPV4
	struct sockaddr_in ipv4_name;
	#endif
	#ifdef DO_IPV6
	struct sockaddr_in6 ipv6_name;
	#endif

	if (!ipaddr) {
		#ifdef DO_IPV6
		ipaddr = "0:0:0:0:0:0:0:0";
		#else
		ipaddr = "0.0.0.0";
		#endif
	}

	/* Resolve the ip address. */
	#ifdef DO_IPV6
	err = inet_pton(AF_INET6, ipaddr, &ipv6_name.sin6_addr);
	if (err <= 0) {
		#ifndef DO_IPV4
		return(err);
		#else
		err = inet_pton(AF_INET, ipaddr, &ipv4_name.sin_addr);
		if (err <= 0) return(-1);
		pfamily = PF_INET;
		name = (struct sockaddr *)&ipv4_name;
		len = sizeof(ipv4_name);
		ipv4_name.sin_port = htons(port);
		ipv4_name.sin_family = AF_INET;
		#endif
	}
	else {
		pfamily = PF_INET6;
		name = (struct sockaddr *)&ipv6_name;
		len = sizeof(ipv6_name);
		ipv6_name.sin6_port = htons(port);
		ipv6_name.sin6_family = AF_INET6;
	}
	#else
	err = inet_aton(ipaddr, &ipv4_name.sin_addr);
	if (err <= 0) return(-1);
	pfamily = PF_INET;
	name = (struct sockaddr *)&ipv4_name;
	len = sizeof(ipv4_name);
	ipv4_name.sin_port = htons(port);
	ipv4_name.sin_family = AF_INET;
	#endif

	/* Create the socket! */
	if (flags & SOCKET_UDP) sock = socket(pfamily, SOCK_DGRAM, 0);
	else sock = socket(pfamily, SOCK_STREAM, 0);
	if (sock < 0) return(-2);

	if (flags & SOCKET_NONBLOCK) socket_set_nonblock(sock, 1);

	if (flags & SOCKET_SERVER) {
		int yes = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		err = bind(sock, name, len);
		if (err < 0) return(-3);
		listen(sock, 50);
	}
	else {
		err = connect(sock, name, len);
		if (err < 0 && errno != EINPROGRESS) return(-4);
	}

	/* Yay, we're done. */
	return(sock);
}

int socket_set_nonblock(int sock, int value)
{
	int oldflags = fcntl(sock, F_GETFL, 0);
	if (oldflags == -1) return -1;
	if (value != 0) oldflags |= O_NONBLOCK;
	else oldflags &= ~O_NONBLOCK;

	return fcntl(sock, F_SETFL, oldflags);
}
