#ifndef _SOCKBUF_H_
#define _SOCKBUF_H_

/* Flags for sockbuf_t structure. */
#define SOCKBUF_BLOCK	1
#define SOCKBUF_SERVER	2
#define SOCKBUF_CLIENT	4
#define SOCKBUF_DELETE	8
#define SOCKBUF_NOREAD	16

/* Event types. */
enum {
	SOCKBUF_READ, SOCKBUF_EMPTY, SOCKBUF_EOF, SOCKBUF_ERR,
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

typedef struct sockbuf_b {
	int sock;	/* Underlying socket descriptor */
	int flags;	/* Keep track of blocked status, client/server */

	int idx;	/* Self-referencing idx */

	sockbuf_iobuf_t outbuf;	/* Output buffer. */

	sockbuf_event_b *filters;	/* Line-mode, gzip, ssl... */
	void **filter_client_data;	/* Client data for filters */
	int nfilters;	/* Number of filters */

	sockbuf_event_b on;	/* User's event handlers */
	void *client_data;	/* User's client data */
} sockbuf_t;

int sockbuf_filter(int idx, int event, int level, void *arg);
int sockbuf_write(int idx, unsigned char *data, int len);
int sockbuf_new(int sock, int flags);
int sockbuf_delete(int idx);
int sockbuf_set_handler(int idx, sockbuf_event_t handler, void *client_data);
int sockbuf_attach_listener(int fd);
int sockbuf_detach_listener(int fd);
int sockbuf_attach_filter(int idx, sockbuf_event_t filter, void *client_data);
int sockbuf_detach_filter(int idx, sockbuf_event_t filter, void *client_data);
int sockbuf_update_all(int timeout);

#endif /* _SOCKBUF_H_ */
