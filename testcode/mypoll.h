#ifndef _MYPOLL_H_
#define _MYPOLL_H_

#define POLLIN	1
#define POLLOUT	2
#define POLLERR	4
#define POLLNVAL	8
#define POLLHUP	16

struct pollfd {
	int fd;
	short events;
	short revents;
};

int poll(struct pollfd *pollfds, int npollfds, int timeout);

#endif
