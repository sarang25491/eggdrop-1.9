/* sockbuf.h: header for sockbuf.c
 *
 * Copyright (C) 2002, 2003, 2004 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id: sockbuf.h,v 1.7 2004/06/22 23:20:23 wingman Exp $
 */

#ifndef _EGG_SOCKBUF_H_
#define _EGG_SOCKBUF_H_

/* Flags for sockbuf_t structure. */
#define SOCKBUF_BLOCK	1
#define SOCKBUF_SERVER	2
#define SOCKBUF_CLIENT	4
#define SOCKBUF_DELETE	8
#define SOCKBUF_NOREAD	16
/* The difference between SOCKBUF_AVAIL and SOCKBUF_DELETED is that DELETED
 * means it's invalid but not available for use yet. That happens when you
 * delete a sockbuf from within sockbuf_update_all(). Otherwise, when you
 * create a new sockbuf after you delete one, you could get the same idx,
 * and sockbuf_update_all() might trigger invalid events for it.
 */
#define SOCKBUF_AVAIL	32
#define SOCKBUF_DELETED	64


/* Levels for filters. */
#define SOCKBUF_LEVEL_INTERNAL	-1
#define SOCKBUF_LEVEL_WRITE_INTERNAL	1000000
#define SOCKBUF_LEVEL_PROXY	1000
#define SOCKBUF_LEVEL_THROTTLE	2000
#define SOCKBUF_LEVEL_ENCRYPTION	3000
#define SOCKBUF_LEVEL_COMPRESSION	4000
#define SOCKBUF_LEVEL_TEXT_ALTERATION	5000
#define SOCKBUF_LEVEL_TEXT_BUFFER	6000

/* Some connection stats for sockbufs. */
typedef struct {
	egg_timeval_t connected_at;
	egg_timeval_t last_input_at;
	egg_timeval_t last_output_at;

	uint64_t raw_bytes_in, raw_bytes_out, raw_bytes_left;
	uint64_t bytes_in, bytes_out;

	int total_in_cps, snapshot_in_cps;
	int total_out_cps, snapshot_out_cps;

	int snapshot_in_bytes[5], snapshot_out_bytes[5], snapshot_counter, last_snapshot;
} sockbuf_stats_t;

typedef struct {
	const char *name;
	int level;

	/* Connection-level events. */
	int (*on_connect)(void *client_data, int idx, const char *peer_ip, int peer_port);
	int (*on_eof)(void *client_data, int idx, int err, const char *errmsg);
	int (*on_newclient)(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port);

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
	int (*on_newclient)(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port);

	/* Data-level events. */
	int (*on_read)(void *client_data, int idx, char *data, int len);
	int (*on_written)(void *client_data, int idx, int len, int remaining);

	/* Command-level events. */
	int (*on_delete)(void *client_data, int idx);
} sockbuf_handler_t;

int sockbuf_init(void);
int sockbuf_shutdown(void);

int sockbuf_write(int idx, const char *data, int len);
int sockbuf_new();
int sockbuf_delete(int idx);
int sockbuf_isvalid(int idx);
int sockbuf_close(int idx);
int sockbuf_flush(int idx);
int sockbuf_set_handler(int idx, sockbuf_handler_t *handler, void *client_data);
int sockbuf_get_sock(int idx);
int sockbuf_set_sock(int idx, int sock, int flags);
int sockbuf_get_peer(int idx, const char **peer_ip, int *peer_port);
int sockbuf_get_self(int idx, const char **my_ip, int *my_port);
int sockbuf_get_stats(int idx, sockbuf_stats_t **stats);
int sockbuf_read(int idx);
int sockbuf_noread(int idx);
int sockbuf_attach_listener(int fd);
int sockbuf_get_filter_data(int idx, sockbuf_filter_t *filter, void *client_data_ptr);
int sockbuf_detach_listener(int fd);
int sockbuf_attach_filter(int idx, sockbuf_filter_t *filter, void *client_data);
int sockbuf_detach_filter(int idx, sockbuf_filter_t *filter, void *client_data_ptr);
int sockbuf_update_all(int timeout);

/* Filter functions. */
int sockbuf_on_connect(int idx, int level, const char *peer_ip, int peer_port);
int sockbuf_on_newclient(int idx, int level, int newidx, const char *peer_ip, int peer_port);
int sockbuf_on_eof(int idx, int level, int err, const char *errmsg);
int sockbuf_on_read(int idx, int level, char *data, int len);
int sockbuf_on_write(int idx, int level, const char *data, int len);
int sockbuf_on_written(int idx, int level, int len, int remaining);

#endif /* !_EGG_SOCKBUF_H_ */
