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


/* Levels for filters. */
#define SOCKBUF_LEVEL_INTERNAL	-1
#define SOCKBUF_LEVEL_WRITE_INTERNAL	1000000
#define SOCKBUF_LEVEL_PROXY	1000
#define SOCKBUF_LEVEL_ENCRYPTION	2000
#define SOCKBUF_LEVEL_COMPRESSION	3000
#define SOCKBUF_LEVEL_TEXT_ALTERATION	4000
#define SOCKBUF_LEVEL_TEXT_BUFFER	5000

typedef struct {
	const char *name;
	int level;

	/* Connection-level events. */
	int (*on_connect)(void *client_data, int idx, const char *peer_ip, int peer_port);
	int (*on_eof)(void *client_data, int idx, int err, const char *errmsg);
	int (*on_newclient)(void *client_data, int idx, int newsock, const char *peer_ip, int peer_port);

	/* Data-level events. */
	int (*on_read)(void *client_data, int idx, char *data, int len);
	int (*on_write)(void *client_data, int idx, const char *data, int len);
	int (*on_written)(void *client_data, int idx, int len, int remaining);

	/* Command-level events. */
	int (*on_flush)(void *client_data, int idx);
	int (*on_delete)(void *client_data, int idx);
} sockbuf_filter_t;

typedef struct {
	const char *name;

	/* Connection-level events. */
	int (*on_connect)(void *client_data, int idx, const char *peer_ip, int peer_port);
	int (*on_eof)(void *client_data, int idx, int err, const char *errmsg);
	int (*on_newclient)(void *client_data, int idx, int newsock, const char *peer_ip, int peer_port);

	/* Data-level events. */
	int (*on_read)(void *client_data, int idx, char *data, int len);
	int (*on_written)(void *client_data, int idx, int len, int remaining);
} sockbuf_handler_t;

int sockbuf_write(int idx, const char *data, int len);
int sockbuf_new();
int sockbuf_delete(int idx);
int sockbuf_close(int idx);
int sockbuf_flush(int idx);
int sockbuf_set_handler(int idx, sockbuf_handler_t *handler, void *client_data);
int sockbuf_set_sock(int idx, int sock, int flags);
int sockbuf_attach_listener(int fd);
int sockbuf_detach_listener(int fd);
int sockbuf_attach_filter(int idx, sockbuf_filter_t *filter, void *client_data);
int sockbuf_detach_filter(int idx, sockbuf_filter_t *filter, void *client_data);
int sockbuf_update_all(int timeout);

#endif /* _SOCKBUF_H_ */
