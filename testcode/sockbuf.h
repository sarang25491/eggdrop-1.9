#ifndef _SOCKBUF_H_
#define _SOCKBUF_H_

/* Flags for sockbuf_t structure. */
#define SOCKBUF_BLOCK	1
#define SOCKBUF_SERVER	2
#define SOCKBUF_CLIENT	4
#define SOCKBUF_DELETE	8
#define SOCKBUF_NOREAD	16
#define SOCKBUF_AVAIL	32
#define SOCKBUF_DELETED	64

/* The difference between SOCKBUF_AVAIL and SOCKBUF_DELETED is that DELETED
	means it's invalid but not available for use yet. That happens when you
	delete a sockbuf from within sockbuf_update_all(). Otherwise, when you
	create a new sockbuf after you delete one, you could get the same idx,
	and sockbuf_update_all() might trigger invalid events for it. */

/* Event types. */
enum {
	SOCKBUF_READ, SOCKBUF_EMPTY, SOCKBUF_EOF, SOCKBUF_ERR, SOCKBUF_CONNECT,
	SOCKBUF_WRITE,
	SOCKBUF_NEVENTS	/* Marker for how many events are defined. */
};

typedef struct sockbuf_iobuf_b {
	unsigned char *data;
	int len;
	int max;
} sockbuf_iobuf_t;

typedef int (*Function)();
typedef Function sockbuf_event_t[];
typedef Function *sockbuf_event_b;

int sockbuf_filter(int idx, int event, int level, void *arg);
int sockbuf_write(int idx, unsigned char *data, int len);
int sockbuf_write_filter(int idx, int level, unsigned char *data, int len);
int sockbuf_new(int sock, int flags);
int sockbuf_delete(int idx);
int sockbuf_set_handler(int idx, sockbuf_event_t handler, void *client_data);
int sockbuf_set_sock(int idx, int sock, int flags);
int sockbuf_attach_listener(int fd);
int sockbuf_detach_listener(int fd);
int sockbuf_attach_filter(int idx, sockbuf_event_t filter, void *client_data);
int sockbuf_detach_filter(int idx, sockbuf_event_t filter, void *client_data);
int sockbuf_update_all(int timeout);

#endif /* _SOCKBUF_H_ */
