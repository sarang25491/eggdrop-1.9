/* ident.c: ident functions
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
 */

#ifndef lint
static const char rcsid[] = "$Id: ident.c,v 1.2 2004/06/29 21:28:16 stdarg Exp $";
#endif

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <eggdrop/eggdrop.h>

typedef struct _ident_info {
	struct _ident_info *next;
	int id;
	char *ip;
	int their_port, our_port;
	int idx;
	int timeout;
	int (*callback)();
	void *client_data;
} ident_info_t;

static int ident_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int ident_on_read(void *client_data, int idx, char *data, int len);
static int ident_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int ident_result(void *client_data, const char *ident);

static sockbuf_handler_t ident_handler = {
	"ident",
	ident_on_connect, ident_on_eof, NULL,
	ident_on_read, NULL,
	NULL
};

static ident_info_t *ident_head = NULL;


/* Perform an async ident lookup and call 'callback' with the result. */
int egg_ident_lookup(const char *ip, int their_port, int our_port, int timeout, int (*callback)(), void *client_data)
{
	static int ident_id = 0;
	int idx;
	ident_info_t *ident_info;
	
	idx = egg_connect(ip, 113, -1);
	if (idx < 0) {
		callback(client_data, ip, their_port, NULL);
		return(-1);
	}
	ident_info = calloc(1, sizeof(*ident_info));
	ident_info->ip = strdup(ip);
	ident_info->their_port = their_port;
	ident_info->our_port = our_port;
	ident_info->timeout = timeout;
	ident_info->callback = callback;
	ident_info->client_data = client_data;
	ident_info->idx = idx;
	ident_info->id = ident_id++;
	ident_info->next = ident_head;
	ident_head = ident_info;
	sockbuf_set_handler(idx, &ident_handler, ident_info);
	return(ident_info->id);
}

/* Cancel an ident lookup, optionally triggering the callback. */
int egg_ident_cancel(int id, int issue_callback)
{
	ident_info_t *ptr, *prev;

	prev = NULL;
	for (ptr = ident_head; ptr; ptr = ptr->next) {
		if (ptr->id == id) break;
		prev = ptr;
	}
	if (!ptr) return(-1);
	if (prev) prev->next = ptr->next;
	else ident_head = ptr->next;
	if (issue_callback) {
		ptr->callback(ptr->client_data, ptr->ip, ptr->their_port, NULL);
	}
	free(ptr->ip);
	free(ptr);
	return(0);
}

static int ident_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	ident_info_t *ident_info = client_data;

	egg_iprintf(idx, "%d,%d\r\n", ident_info->their_port, ident_info->our_port);
	return(0);
}

static int ident_on_read(void *client_data, int idx, char *data, int len)
{
	char *colon, *ident;
	int i;

	colon = strchr(data, ':');
	if (!colon) goto ident_error;

	/* See if it's a successful lookup. */
	colon++;
	while (isspace(*colon)) colon++;
	if (strncasecmp(colon, "USERID", 6)) goto ident_error;

	/* Go to the OS identifier. */
	colon = strchr(colon, ':');
	if (!colon) goto ident_error;
	colon++;

	/* Go to the user id. */
	ident = strchr(colon, ':');
	if (!ident) goto ident_error;
	ident++;
	while (isspace(*ident)) ident++;

	/* Replace bad chars. */
	len = strlen(ident);
	for (i = 0; i < len; i++) {
		if (ident[i] == '\r' || ident[i] == '\n') {
			ident[i] = 0;
			break;
		}
		if (!isalnum(ident[i]) && ident[i] != '_') ident[i] = 'x';
	}
	if (len > 20) ident[20] = 0;
	ident_result(client_data, ident);
	return(0);

ident_error:
	ident_result(client_data, NULL);
	return(0);
}

static int ident_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	ident_result(client_data, NULL);
	return(0);
}

static int ident_result(void *client_data, const char *ident)
{
	ident_info_t *ident_info = client_data;
	sockbuf_delete(ident_info->idx);
	ident_info->callback(ident_info->client_data, ident_info->ip, ident_info->their_port, ident);
	free(ident_info->ip);
	free(ident_info);
	return(0);
}
