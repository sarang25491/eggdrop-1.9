/* egg_server_api.h: header for server API
 *
 * Copyright (C) 2003, 2004 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: egg_server_api.h,v 1.2 2003/12/18 06:50:47 wcc Exp $
 */

#ifndef _EGG_MOD_SERVER_API_H_
#define _EGG_MOD_SERVER_API_H_

/* Version of this implementation. */
#define EGG_SERVER_API_MAJOR	0
#define EGG_SERVER_API_MINOR	0

/* Server output priorities. */
#define SERVER_NOQUEUE	1
#define SERVER_QUICK	2
#define SERVER_NORMAL	3
#define SERVER_SLOW	4
/* Can be OR'd with any priority queue. */
#define SERVER_NEXT	8

/* Queue structures. */
typedef struct queue_entry {
	struct queue_entry *next, *prev;
	int id;
	irc_msg_t msg;
} queue_entry_t;

typedef struct {
	queue_entry_t *queue_head, *queue_tail;
	int len;
	int next_id;
} queue_t;


/* API structure. */
typedef struct {
	int major, minor; /* Version of this instance. */

	/* Output functions. */
	int (*printserv)(int priority, const char *format, ...);
	queue_entry_t (*queue_new)(char *text);
	void (*queue_server)(int priority, char *text, int len);
	void (*queue_append)(queue_t *queue, queue_entry_t *q);
	void (*queue_unlink)(queue_t *queue, queue_entry_t *q);
	void (*queue_entry_from_text)(queue_entry_t *q, char *text);
	void (*queue_entry_to_text)(queue_entry_t *q, char *text, int *remaining);
	void (*queue_entry_cleanup)(queue_entry_t *q);
	queue_t *(*queue_get_by_priority)(int priority);
	void (*dequeue_messages)();

	/* Channel functions. */
	int (*channel_list)(const char ***chans);
	int (*channel_list_members)(const char *chan, const char ***members);
} egg_server_api_t;

#endif /* !_EGG_MOD_SERVER_API_H_ */
