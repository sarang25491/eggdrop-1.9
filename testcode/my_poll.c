#include <stdio.h>
#include <errno.h>
#include <sys/select.h>
#include "my_poll.h"

int my_poll(struct pollfd *pollfds, int npollfds, int timeout)
{
	fd_set reads, writes, excepts;
	int i, n;
	int highest = -1;
	int events;

	FD_ZERO(&reads); FD_ZERO(&writes); FD_ZERO(&excepts);

	/* Convert pollfds array to fd_set's. */
	for (i = 0; i < npollfds; i++) {
		/* Ignore invalid descriptors. */
		n = pollfds[i].fd;
		if (n < 0) continue;

		events = pollfds[i].events;
		if (events & POLLIN) FD_SET(n, &reads);
		if (events & POLLOUT) FD_SET(n, &writes);
		FD_SET(n, &excepts);
		if (n > highest) highest = n;
	}

	if (timeout >= 0) {
		struct timeval tv;
		tv.tv_sec = (timeout / 1000);
		timeout -= 1000 * tv.tv_sec;
		tv.tv_usec = timeout * 1000;
		events = select(highest+1, &reads, &writes, &excepts, &tv);
	}
	else events = select(highest+1, &reads, &writes, &excepts, NULL);

	if (events < 0) {
		/* There is a bad descriptor among us... find it! */
		char temp[1];
		events = 0;
		for (i = 0; i < npollfds; i++) {
			if (pollfds[i].fd < 0) continue;
			errno = 0;
			n = write(pollfds[i].fd, temp,  0);
			if (n < 0 && errno != EINVAL) {
				pollfds[i].revents = POLLNVAL;
				events++;
			}
			else pollfds[i].revents = 0;
		}
	}
	else for (i = 0; i < npollfds; i++) {
		n = pollfds[i].fd;
		pollfds[i].revents = 0;
		if (FD_ISSET(n, &reads)) pollfds[i].revents |= POLLIN;
		if (FD_ISSET(n, &writes)) pollfds[i].revents |= POLLOUT;
		if (FD_ISSET(n, &excepts)) pollfds[i].revents |= POLLERR;
	}

	return(events);
}
