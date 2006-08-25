/* scriptdns.c: dns-related scripting commands
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
static const char rcsid[] = "$Id: scriptdns.c,v 1.1 2006/08/25 17:22:50 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include "egg_script_internal.h"

typedef struct {
	script_callback_t *callback;
	int id;
	char *text;
	int len;
} script_dns_callback_data_t;

static int do_callback = 1;
static char *empty_string = "";

static int script_dns_callback(void *client_data, const char *query, char **result)
{
	byte_array_t bytes;
	script_dns_callback_data_t *data = client_data;

	bytes.bytes = data->text;
	bytes.len = data->len;
	bytes.do_free = 0;

	if (!bytes.bytes) bytes.bytes = empty_string;
	if (bytes.len <= 0) bytes.len = strlen(bytes.bytes);

	if (do_callback) data->callback->callback(data->callback, data->id, query, result, &bytes);

	if (data->text) free(data->text);
	free(data);

	return 0; /* what exactly should this function return? the return value is always ignored */
}

static int script_dns_lookup(const char *host, script_callback_t *callback, char *text, int len)
{
	int id;
	script_dns_callback_data_t *data;

	callback->syntax = strdup("isSb");
	data = malloc(sizeof(*data));
	data->callback = callback;
	data->id = -1;
	data->text = text;
	data->len = len;
	id = egg_dns_lookup(host, -1, script_dns_callback, data);
	if (id == -1) {
		/* the query was cached, the callback has already been called and data has been freed */
		return -1;
	}
	data->id = id;
	return id;
}

static int script_dns_reverse(const char *ip, script_callback_t *callback, char *text, int len)
{
	int id;
	script_dns_callback_data_t *data;

	callback->syntax = strdup("isSb");
	data = malloc(sizeof(*data));
	data->callback = callback;
	data->id = -1;
	data->text = text;
	data->len = len;
	id = egg_dns_reverse(ip, -1, script_dns_callback, data);
	if (id == -1) {
		/* the query was cached, the callback has already been called and data has been freed */
		return -1;
	}
	data->id = id;
	return id;
}

static int script_dns_cancel(int id, int callback)
{
	do_callback = callback;
	egg_dns_cancel(id, 1);
	do_callback = 1;
}

script_command_t script_dns_cmds[] = {
	{"", "dns_lookup", script_dns_lookup, NULL, 2, "scsi", "host callback ?callbackdata? ?len?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},	/* DDD */
	{"", "dns_reverse", script_dns_reverse, NULL, 2, "scsi", "ip  callback ?callbackdata? ?len?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},	/* DDD */
	{"", "dns_cancel", script_dns_cancel, NULL, 1, "ii", "id ?docallback?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},	/* DDD */
	{0}
};
