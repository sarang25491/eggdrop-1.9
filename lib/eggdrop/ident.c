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
static const char rcsid[] = "$Id: ident.c,v 1.7 2007/09/13 22:20:55 sven Exp $";
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
	ident_callback_t *callback;
	void *client_data;
	event_owner_t *owner;
} ident_info_t;

static int ident_on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int ident_on_read(void *client_data, int idx, char *data, int len);
static int ident_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int ident_result(ident_info_t *ident_info, const char *ident);

static sockbuf_handler_t ident_handler = {
	"ident",
	ident_on_connect, ident_on_eof, NULL,
	ident_on_read, NULL
};

static ident_info_t *ident_head = NULL;


/* Perform an async ident lookup and call 'callback' with the result. */
int egg_ident_lookup(const char *ip, int their_port, int our_port, int timeout, ident_callback_t *callback, void *client_data, event_owner_t *owner)
{
	static int ident_id = 0;
	int idx;
	ident_info_t *ident_info;
	
	idx = egg_connect(ip, 113, -1);
	if (idx < 0) {
		callback(client_data, ip, their_port, NULL);
		if (owner && owner->on_delete) owner->on_delete(owner, client_data);
		return(-1);
	}
	ident_info = calloc(1, sizeof(*ident_info));
	ident_info->ip = strdup(ip);
	ident_info->their_port = their_port;
	ident_info->our_port = our_port;
	ident_info->timeout = timeout;
	ident_info->callback = callback;
	ident_info->client_data = client_data;
	ident_info->owner = owner;
	ident_info->idx = idx;
	ident_info->id = ident_id++;
	ident_info->next = ident_head;
	ident_head = ident_info;
	sockbuf_set_handler(idx, &ident_handler, ident_info, NULL);
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
	sockbuf_delete(ptr->idx);
	if (issue_callback) {
		ptr->callback(ptr->client_data, ptr->ip, ptr->their_port, NULL);
	}
	if (ptr->owner && ptr->owner->on_delete) ptr->owner->on_delete(ptr->owner, ptr->client_data);
	free(ptr->ip);
	free(ptr);
	return(0);
}

int egg_ident_cancel_by_owner(egg_module_t *module, void *script)
{
	int removed = 0;
	ident_info_t *ptr, *prev = NULL, *next;

	for (ptr = ident_head; ptr; ptr = next) {
		next = ptr->next;
		if (!ptr->owner || ptr->owner->module != module || (script && ptr->owner->client_data != script)) {
			prev = ptr;
			continue;
		}
		if (prev) prev->next = ptr->next;
		else ident_head = ptr->next;
		++removed;
		sockbuf_delete(ptr->idx);
		if (ptr->owner && ptr->owner->on_delete) ptr->owner->on_delete(ptr->owner, ptr->client_data);
		free(ptr->ip);
		free(ptr);
	}
	return removed;
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
	while (*ident && isspace(*ident)) ident++;

	/* Replace bad chars. */
	len = strlen(ident);
	for (i = 0; i < len; i++) {
		if (ident[i] == '\r' || ident[i] == '\n') {
			ident[i] = 0;
			break;
		}
		if (!isalnum(ident[i]) && ident[i] != '_') ident[i] = 'x';
	}
	if (!*ident) goto ident_error;
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

static int ident_result(ident_info_t *ident_info, const char *ident)
{
	ident_info_t *ptr, *prev = NULL;
	
	for (ptr = ident_head; ptr; ptr = ptr->next) {
		if (ptr == ident_info) break;
		prev = ptr;
	}

	if (prev) prev->next = ptr->next;
	else ident_head = ptr->next;

	sockbuf_delete(ident_info->idx);
	ident_info->callback(ident_info->client_data, ident_info->ip, ident_info->their_port, ident);
	if (ident_info->owner && ident_info->owner->on_delete) ident_info->owner->on_delete(ident_info->owner, ident_info->client_data);
	free(ident_info->ip);
	free(ident_info);
	return(0);
}
