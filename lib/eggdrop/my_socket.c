#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include "my_socket.h"

/* Apparently SHUT_RDWR is not defined on some systems. */
#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif

#ifdef AF_INET6
#define DO_IPV6
#endif
#define DO_IPV4

typedef struct {
	int len;
	int family;
	union {
		struct sockaddr addr;
		#ifdef DO_IPV4
		struct sockaddr_in ipv4;
		#endif
		#ifdef DO_IPV6
		struct sockaddr_in6 ipv6;
		#endif
	} u;
} sockname_t;

static int socket_name(sockname_t *name, const char *ipaddr, int port)
{
	memset(name, 0, sizeof(*name));

	#ifdef DO_IPV4
	if (inet_pton(AF_INET, ipaddr, &name->u.ipv4.sin_addr) > 0) {
		name->len = sizeof(name->u.ipv4);
		name->family = PF_INET;
		name->u.ipv4.sin_port = htons(port);
		name->u.ipv4.sin_family = AF_INET;
		return(0);
	}
	#endif

	#ifdef DO_IPV6
	if (inet_pton(AF_INET6, ipaddr, &name->u.ipv6.sin6_addr) > 0) {
		name->len = sizeof(name->u.ipv6);
		name->family = PF_INET6;
		name->u.ipv6.sin6_port = htons(port);
		name->u.ipv6.sin6_family = AF_INET6;
		return(0);
	}
	#endif

	/* Invalid name? Then use passive. */
	name->len = sizeof(name->u.ipv4);
	name->family = PF_INET;
	name->u.ipv4.sin_port = htons(port);
	name->u.ipv4.sin_family = AF_INET;
	return(0);
}

int socket_get_name(int sock, char **ip, int *port)
{
	sockname_t name;
	int namelen;

	if (ip) *ip = NULL;
	if (port) *port = 0;

	#ifdef DO_IPV4
	namelen = sizeof(name.u.ipv4);
	if (!getsockname(sock, &name.u.addr, &namelen) && namelen == sizeof(name.u.ipv4)) {
		if (ip) {
			*ip = (char *)malloc(32);
			inet_ntop(AF_INET, &name.u.ipv4.sin_addr, *ip, 32);
		}
		if (port) *port = ntohs(name.u.ipv4.sin_port);
		return(0);
	}
	#endif
	#ifdef DO_IPV6
	namelen = sizeof(name.u.ipv6);
	if (!getsockname(sock, &name.u.addr, &namelen) && namelen == sizeof(name.u.ipv6)) {
		if (ip) {
			*ip = (char *)malloc(128);
			inet_ntop(AF_INET6, &name.u.ipv6.sin6_addr, *ip, 128);
		}
		if (port) *port = ntohs(name.u.ipv6.sin6_port);
		return(0);
	}
	#endif

	return(-1);
}

int socket_get_peer_name(int sock, char **peer_ip, int *peer_port)
{
	sockname_t name;
	int namelen;

	if (peer_ip) *peer_ip = NULL;
	if (peer_port) *peer_port = 0;

	#ifdef DO_IPV4
	namelen = sizeof(name.u.ipv4);
	if (!getpeername(sock, &name.u.addr, &namelen) && namelen == sizeof(name.u.ipv4)) {
		if (peer_ip) {
			*peer_ip = (char *)malloc(32);
			inet_ntop(AF_INET, &name.u.ipv4.sin_addr, *peer_ip, 32);
		}
		if (peer_port) *peer_port = ntohs(name.u.ipv4.sin_port);
		return(0);
	}
	#endif
	#ifdef DO_IPV6
	namelen = sizeof(name.u.ipv6);
	if (!getpeername(sock, &name.u.addr, &namelen) && namelen == sizeof(name.u.ipv6)) {
		if (peer_ip) {
			*peer_ip = (char *)malloc(128);
			inet_ntop(AF_INET6, &name.u.ipv6.sin6_addr, *peer_ip, 128);
		}
		if (peer_port) *peer_port = ntohs(name.u.ipv6.sin6_port);
		return(0);
	}
	#endif

	return(-1);
}

int socket_get_error(int sock)
{
	int size, err;

	size = sizeof(int);
	err = 0;
	getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &size);
	return(err);
}

int socket_accept(int sock, char **peer_ip, int *peer_port)
{
	int newsock, len;
	sockname_t name;

	*peer_ip = NULL;
	*peer_port = 0;
	memset(&name, 0, sizeof(name));
	len = sizeof(name.u);
	newsock = accept(sock, &name.u.addr, &len);
	#ifdef DO_IPV4
	if (len == sizeof(name.u.ipv4)) {
		*peer_ip = (char *)malloc(32);
		*peer_port = ntohs(name.u.ipv4.sin_port);
		inet_ntop(AF_INET, &name.u.ipv4.sin_addr, *peer_ip, 32);
	}
	#endif
	#ifdef DO_IPV6
	if (len == sizeof(name.u.ipv6)) {
		*peer_ip = (char *)malloc(128);
		*peer_port = ntohs(name.u.ipv6.sin6_port);
		inet_ntop(AF_INET6, &name.u.ipv6.sin6_addr, *peer_ip, 128);
	}
	#endif
	return(newsock);
}

/* Return values: */
/* -1: invalid ip address */
/* -2: socket() failure */
/* -3: bind() failure */
/* -4: connect() failure */
int socket_create(const char *dest_ip, int dest_port, const char *src_ip, int src_port, int flags)
{
	char *passive[] = {"::", "0.0.0.0"};
	int sock = -1, pfamily, try;
	sockname_t dest_name, src_name;

	/* If no source ip address is given, try :: and 0.0.0.0 (passive). */
	for (try = 0; try < 2; try++) {
		/* Resolve the ip addresses. */
		socket_name(&dest_name, dest_ip ? dest_ip : passive[try], dest_port);
		socket_name(&src_name, src_ip ? src_ip : passive[try], src_port);

		if (src_ip || src_port) flags |= SOCKET_BIND;

		if (flags & SOCKET_CLIENT) pfamily = dest_name.family;
		else if (flags & SOCKET_SERVER) pfamily = src_name.family;
		else {
			errno = EADDRNOTAVAIL;
			return(-1);
		}

		/* Create the socket. */
		if (flags & SOCKET_UDP) sock = socket(pfamily, SOCK_DGRAM, 0);
		else sock = socket(pfamily, SOCK_STREAM, 0);

		if (sock >= 0) break;
	}

	if (sock < 0) return(-2);

	if (flags & SOCKET_NONBLOCK) socket_set_nonblock(sock, 1);

	/* Do the bind if necessary. */
	if (flags & (SOCKET_SERVER|SOCKET_BIND)) {
		int yes = 1;

		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		if (bind(sock, &src_name.u.addr, src_name.len) != 0) return(-3);
		if (flags & SOCKET_SERVER) listen(sock, 50);
	}

	if (flags & SOCKET_CLIENT) {
		if (connect(sock, &dest_name.u.addr, dest_name.len) != 0) {
			if (errno != EINPROGRESS) return(-4);
		}
	}

	errno = 0;

	/* Yay, we're done. */
	return(sock);
}

int socket_close(int sock)
{
	shutdown(sock, SHUT_RDWR);
	close(sock);
	return(0);
}

int socket_set_nonblock(int sock, int value)
{
	int oldflags = fcntl(sock, F_GETFL, 0);
	if (oldflags == -1) return -1;
	if (value != 0) oldflags |= O_NONBLOCK;
	else oldflags &= ~O_NONBLOCK;

	return fcntl(sock, F_SETFL, oldflags);
}

int socket_valid_ip(const char *ip)
{
	char buf[512];

#ifdef DO_IPV6
	if (inet_pton(AF_INET6, ip, buf) > 0) return(1);
#endif
#ifdef DO_IPV4
	if (inet_pton(AF_INET, ip, buf) > 0) return(1);
#endif
	return(0);
}

int socket_ip_to_uint(const char *ip, unsigned int *longip)
{
	struct in_addr addr;

	inet_pton(AF_INET, ip, &addr);
	*longip = htonl(addr.s_addr);
	return(0);
}

/* Converts shorthand ipv6 notation (123:456::789) into long dotted-decimal
 * notation. 'dots' must be 16*4+1 = 65 bytes long. */
int socket_ipv6_to_dots(const char *ip, char *dots)
{
#ifndef DO_IPV6
	dots[0] = 0;
	return(-1);
#else
	struct in6_addr buf;

	dots[0] = 0;
	if (inet_pton(AF_INET6, ip, &buf) <= 0) return(-1);
	sprintf(dots, "%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u",
		buf.s6_addr[0], buf.s6_addr[1],
		buf.s6_addr[2], buf.s6_addr[3],
		buf.s6_addr[4], buf.s6_addr[5],
		buf.s6_addr[6], buf.s6_addr[7],
		buf.s6_addr[8], buf.s6_addr[9],
		buf.s6_addr[10], buf.s6_addr[11],
		buf.s6_addr[12], buf.s6_addr[13],
		buf.s6_addr[14], buf.s6_addr[15]
	);
	return(0);
#endif
}
