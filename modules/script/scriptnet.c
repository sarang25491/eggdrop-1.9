/* scriptnet.c: networking-related scripting commands
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
static const char rcsid[] = "$Id: scriptnet.c,v 1.5 2004/01/10 01:43:18 stdarg Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct script_net_info {
	struct script_net_info *prev, *next;
	int idx;
	script_callback_t *on_connect, *on_eof, *on_newclient;
	script_callback_t *on_read, *on_written;
	script_callback_t *on_delete;
} script_net_info_t;

/* We keep a list of script-controlled sockbufs. */
static script_net_info_t *script_net_info_head = NULL;

static void script_net_info_add(script_net_info_t *info);
static void script_net_info_remove(script_net_info_t *info);

/* Script interface prototypes. */
static int script_net_takeover(int idx);
static int script_net_listen(script_var_t *retval, int port);
static int script_net_open(const char *host, int port, int timeout);
static int script_net_close(int idx);
static int script_net_write(int idx, const char *text, int len);
static int script_net_linemode(int idx, int onoff);
static int script_net_handler(int idx, const char *event, script_callback_t *handler);
static int script_net_info(script_var_t *retval, int idx, char *what);
static int script_net_throttle(int idx, int speedin, int speedout);
static int script_net_throttle_set(void *client_data, int idx, int speed);

/* Sockbuf handler prototypes. */
static int on_connect(void *client_data, int idx, const char *peer_ip, int peer_port);
static int on_eof(void *client_data, int idx, int err, const char *errmsg);
static int on_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port);
static int on_read(void *client_data, int idx, char *data, int len);
static int on_written(void *client_data, int idx, int len, int remaining);
static int on_delete(void *client_data, int idx);

static sockbuf_handler_t sock_handler = {
	"script",
	on_connect, on_eof, on_newclient,
	on_read, on_written,
	on_delete
};

/* Put an idx under script control. */
static int script_net_takeover(int idx)
{
	script_net_info_t *info;

	info = calloc(1, sizeof(*info));
	info->idx = idx;
	script_net_info_add(info);
	sockbuf_set_handler(info->idx, &sock_handler, info);
	return(0);
}

/* Look up info based on the idx. */
static script_net_info_t *script_net_info_lookup(int idx)
{
	script_net_info_t *info;

	for (info = script_net_info_head; info; info = info->next) {
		if (info->idx == idx) break;
	}
	return(info);
}

static void script_net_info_add(script_net_info_t *info)
{
	info->next = script_net_info_head;
	info->prev = NULL;
	if (script_net_info_head) script_net_info_head->prev = info;
	script_net_info_head = info;
}

static void script_net_info_remove(script_net_info_t *info)
{
	if (info->prev) info->prev->next = info->next;
	else script_net_info_head = info->next;

	if (info->next) info->next->prev = info->prev;
}

/* Script interface functions. */
static int script_net_listen(script_var_t *retval, int port)
{
	int idx, real_port;

	retval->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_VAR;
	retval->len = 0;

	idx = egg_listen(port, &real_port);
	script_net_takeover(idx);
	script_list_append(retval, script_int(idx));
	script_list_append(retval, script_int(real_port));
	return(0);
}

static int script_net_open(const char *host, int port, int timeout)
{
	int idx;

	idx = egg_connect(host, port, timeout);
	script_net_takeover(idx);
	return(idx);
}

static int script_net_close(int idx)
{
	return sockbuf_delete(idx);
}

static int script_net_write(int idx, const char *text, int len)
{
	if (len <= 0) len = strlen(text);

	return sockbuf_write(idx, text, len);
}

static int script_net_linemode(int idx, int onoff)
{
	if (onoff) linemode_on(idx);
	else linemode_off(idx);
	return(0);
}

static int script_net_handler(int idx, const char *event, script_callback_t *callback)
{
	script_net_info_t *info;
	script_callback_t **replace = NULL;
	const char *newsyntax;

	/* See if it's already script-controlled. */
	info = script_net_info_lookup(idx);
	if (!info) {
		/* Nope, take it over. */
		script_net_takeover(idx);
		info = script_net_info_lookup(idx);
		if (!info) return(-1);
	}

	if (!strcasecmp(event, "connect")) {
		replace = &info->on_connect;
		newsyntax = "isi";
	}
	else if (!strcasecmp(event, "eof")) {
		replace = &info->on_eof;
		newsyntax = "iis";
	}
	else if (!strcasecmp(event, "newclient")) {
		replace = &info->on_newclient;
		newsyntax = "iisi";
	}
	else if (!strcasecmp(event, "read")) {
		replace = &info->on_read;
		newsyntax = "ibi";
	}
	else if (!strcasecmp(event, "written")) {
		replace = &info->on_written;
		newsyntax = "iii";
	}
	else if (!strcasecmp(event, "delete")) {
		replace = &info->on_delete;
		newsyntax = "i";
	}
	else {
		/* Invalid event type. */
		return(-2);
	}

	if (*replace) (*replace)->del(*replace);
	if (callback) callback->syntax = strdup(newsyntax);
	*replace = callback;
	return(0);
}

static int script_net_info(script_var_t *retval, int idx, char *what)
{
	sockbuf_stats_t *stats = NULL;

	sockbuf_get_stats(idx, &stats);
	retval->type = SCRIPT_INTEGER;
	if (!stats) return(0);

	if (!strcasecmp(what, "all")) {
		retval->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_VAR;
		retval->len = 0;
		script_list_append(retval, script_int(stats->connected_at.sec));
		script_list_append(retval, script_int(stats->last_input_at.sec));
		script_list_append(retval, script_int(stats->last_output_at.sec));
		script_list_append(retval, script_int(stats->raw_bytes_in));
		script_list_append(retval, script_int(stats->raw_bytes_out));
		script_list_append(retval, script_int(stats->raw_bytes_left));
		script_list_append(retval, script_int(stats->bytes_in));
		script_list_append(retval, script_int(stats->bytes_out));
		script_list_append(retval, script_int(stats->total_in_cps));
		script_list_append(retval, script_int(stats->snapshot_in_cps));
		script_list_append(retval, script_int(stats->total_out_cps));
		script_list_append(retval, script_int(stats->snapshot_out_cps));
	}
	else if (!strcasecmp(what, "raw_bytes_left")) retval->value = (void *)(int)stats->raw_bytes_left;
	else if (!strcasecmp(what, "raw_bytes_in")) retval->value = (void *)(int)stats->raw_bytes_in;
	else if (!strcasecmp(what, "raw_bytes_out")) retval->value = (void *)(int)stats->raw_bytes_out;
	return(0);
}

static int script_net_throttle(int idx, int speedin, int speedout)
{
	throttle_on(idx);
	if (speedin) throttle_set(idx, 0, speedin);
	if (speedout) throttle_set(idx, 1, speedout);
	return(0);
}

static int script_net_throttle_set(void *client_data, int idx, int speed)
{
	return throttle_set(idx, (int) client_data, speed);
}

/* Sockbuf handler functions. */
static int on_connect(void *client_data, int idx, const char *peer_ip, int peer_port)
{
	script_net_info_t *info = client_data;

	if (info->on_connect) info->on_connect->callback(info->on_connect, idx, peer_ip, peer_port);
	return(0);
}

static int on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	script_net_info_t *info = client_data;

	if (info->on_eof) info->on_eof->callback(info->on_eof, idx, err, errmsg);
	sockbuf_delete(idx);
	return(0);
}

static int on_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port)
{
	script_net_info_t *info = client_data;

	if (info->on_newclient) info->on_newclient->callback(info->on_newclient, idx, newidx, peer_ip, peer_port);
	return(0);
}

static int on_read(void *client_data, int idx, char *data, int len)
{
	script_net_info_t *info = client_data;
	byte_array_t bytes;

	if (info->on_read) {
		bytes.bytes = data;
		bytes.len = len;
		bytes.do_free = 0;
		info->on_read->callback(info->on_read, idx, &bytes, len);
	}
	return(0);
}

static int on_written(void *client_data, int idx, int len, int remaining)
{
	script_net_info_t *info = client_data;

	if (info->on_written) info->on_written->callback(info->on_written, idx, len, remaining);
	return(0);
}

static int on_delete(void *client_data, int idx)
{
	script_net_info_t *info = client_data;

	if (info->on_connect) info->on_connect->del(info->on_connect);
	if (info->on_eof) info->on_eof->del(info->on_eof);
	if (info->on_newclient) info->on_newclient->del(info->on_newclient);
	if (info->on_read) info->on_read->del(info->on_read);
	if (info->on_written) info->on_written->del(info->on_written);

	if (info->on_delete) {
		info->on_delete->callback(info->on_delete, idx);
		info->on_delete->del(info->on_delete);
	}
	script_net_info_remove(info);
	free(info);
	return(0);
}

script_command_t script_net_cmds[] = {
	{"", "net_takeover", script_net_takeover, NULL, 1, "i", "idx", SCRIPT_INTEGER, 0},
	{"", "net_listen", script_net_listen, NULL, 1, "i", "port", 0, SCRIPT_PASS_RETVAL},
	{"", "net_open", script_net_open, NULL, 2, "sii", "host port ?timeout?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "net_close", script_net_close, NULL, 1, "i", "idx", SCRIPT_INTEGER, 0},
	{"", "net_write", script_net_write, NULL, 2, "isi", "idx text ?len?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "net_handler", script_net_handler, NULL, 2, "isc", "idx event callback", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "net_linemode", script_net_linemode, NULL, 2, "ii", "idx on-off", SCRIPT_INTEGER, 0},
	{"", "net_info", script_net_info, NULL, 2, "is", "idx what", 0, SCRIPT_PASS_RETVAL},
	{"", "net_throttle", script_net_throttle, NULL, 3, "iii", "idx speed-in speed-out", SCRIPT_INTEGER, 0},
	{"", "net_throttle_in", script_net_throttle_set, (void *)0, 2, "ii", "idx speed", SCRIPT_INTEGER, SCRIPT_PASS_CDATA},
	{"", "net_throttle_out", script_net_throttle_set, (void *)1, 2, "ii", "idx speed", SCRIPT_INTEGER, SCRIPT_PASS_CDATA},
	{"", "net_throttle_off", throttle_off, NULL, 1, "i", "idx", SCRIPT_INTEGER, 0},
	{0}
};
