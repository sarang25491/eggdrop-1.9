/* scriptcmds.c: server scripting commands
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
static const char rcsid[] = "$Id: scriptcmds.c,v 1.45 2004/10/01 15:31:18 stdarg Exp $";
#endif

#include "server.h"

/* From server.c */
extern int cycle_delay;
extern current_server_t current_server;

/* From output.c */
extern char *global_output_string;

/* From input.c */
extern char *global_input_string;

/* match_my_nick() is a macro so we can't put it right into the script command table. */
static int script_isbotnick(char *nick)
{
	return match_my_nick(nick);
}

static int script_irccmp(char *nick1, char *nick2)
{
	return (current_server.strcmp)(nick1, nick2);
}

static int name_to_priority(const char *queue, const char *next)
{
	int prio;

	/* Figure out which arguments they've given us. */
	if (!next) queue = NULL;
	else if (!queue) {
		/* If we only have 1 option, check to see if it's -next or
			a queue. */
		if (!strcasecmp(next, "next")) queue = NULL;
		else {
			queue = next;
			next = NULL;
		}
	}
	else if (!strcasecmp(queue, "next")) {
		/* They did it in reverse order, so swap them. */
		const char *temp = next;
		next = queue;
		queue = temp;
	}

	/* Figure out which queue they want. */
	if (!queue) prio = SERVER_NORMAL;
	else if (!strcasecmp(queue, "noqueue")) prio = SERVER_NOQUEUE;
	else if (!strcasecmp(queue, "slow")) prio = SERVER_SLOW;
	else if (!strcasecmp(queue, "quick")) prio = SERVER_QUICK;
	else prio = SERVER_NORMAL;

	if (next) prio |= SERVER_NEXT;

	return(prio);
}

static int script_putserv(char *queue, char *next, char *text)
{
	int prio;

	prio = name_to_priority(queue, next);
	printserv(prio, "%s\r\n", text);

	return(0);
}

static int script_jump(int nargs, int num)
{
  	if (nargs) server_set_next(num);

	cycle_delay = server_config.cycle_delay;
	kill_server("changing servers");

	return(0);
}

static int script_dcc_send_info(int idx, char *what)
{
	int field, value;

	if (!strcasecmp(what, "bytes_left"))
		field = DCC_SEND_LEFT;
	else if (!strcasecmp(what, "bytes_sent"))
		field = DCC_SEND_SENT;
	else if (!strcasecmp(what, "total_cps"))
		field = DCC_SEND_CPS_TOTAL;
	else if (!strcasecmp(what, "snapshot_cps"))
		field = DCC_SEND_CPS_SNAPSHOT;
	else if (!strcasecmp(what, "acks"))
		field = DCC_SEND_ACKS;
	else if (!strcasecmp(what, "bytes_acked"))
		field = DCC_SEND_BYTES_ACKED;
	else if (!strcasecmp(what, "resumed_at"))
		field = DCC_SEND_RESUMED_AT;
	else if (!strcasecmp(what, "request_time"))
		field = DCC_SEND_REQUEST_TIME;
	else if (!strcasecmp(what, "connect_time"))
		field = DCC_SEND_CONNECT_TIME;
	else
		return(-1);

	if (dcc_send_info(idx, field, &value) < 0)
		return(-1);

	return(value);
}

static int script_channel_list(script_var_t *retval)
{
	channel_t *chan;

	retval->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_VAR;
	retval->len = 0;

	for (chan = channel_head; chan; chan = chan->next) {
		script_list_append(retval, script_string(chan->name, -1));
	}
	return(0);
}

static int script_channel_members(script_var_t *retval, char *chan_name)
{
	channel_t *chan;
	channel_member_t *m;

	retval->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_VAR;
	retval->len = 0;

	channel_lookup(chan_name, 0, &chan, NULL);
	if (!chan) return(-1);

	for (m = chan->member_head; m; m = m->next) {
		script_list_append(retval, script_string(m->nick, -1));
	}

	return(0);
}

static int script_channel_topic(script_var_t *retval, char *chan_name)
{
	channel_t *chan;

	retval->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_VAR;
	retval->len = 0;

	channel_lookup(chan_name, 0, &chan, NULL);
	if (!chan) return(-1);

	script_list_append(retval, script_string(chan->topic, -1));
	script_list_append(retval, script_string(chan->topic_nick, -1));
	script_list_append(retval, script_int(chan->topic_time));
	return(0);
}

static int script_channel_mask_list(script_var_t *retval, char *chan_name, char *type)
{
	channel_t *chan;
	channel_mask_t *m;
	channel_mask_list_t *l;

	retval->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_VAR;
	retval->len = 0;

	channel_lookup(chan_name, 0, &chan, NULL);
	if (!chan) return(-1);

	l = channel_get_mask_list(chan, type[0]);
	if (!l) return(-1);

	for (m = l->head; m; m = m->next) {
		script_list_append(retval,
			script_list(3,
				script_string(m->mask, -1),
				script_string(m->set_by, -1),
				script_int(m->time)
			)
		);
	}
	return(0);
}

static char *script_channel_mode(char *chan, char *nick)
{
	char *buf;

	buf = malloc(64);
	channel_mode(chan, nick, buf);
	return(buf);
}

static const char *script_channel_mode_arg(char *chan, char *mode)
{
	const char *value = NULL;

	channel_mode_arg(chan, *mode, &value);
	return(value);
}

static char *script_channel_key(char *chan_name)
{
	channel_t *chan;

	channel_lookup(chan_name, 0, &chan, NULL);
	if (chan) return(chan->key);
	return(NULL);
}

static int script_channel_limit(char *chan_name)
{
	channel_t *chan;

	channel_lookup(chan_name, 0, &chan, NULL);
	if (chan) return(chan->limit);
	return(-1);
}

/* Schan commands. */

static char *script_schan_get(char *chan_name, char *setting)
{
	schan_t *chan = schan_lookup(chan_name, 0);
	char *value;

	if (!chan) return(NULL);
	schan_get(chan, &value, setting, 0, NULL);
	return(value);
}

static int script_schan_set(char *chan_name, char *setting, char *value)
{
	schan_t *chan = schan_lookup(chan_name, 1);

	if (!chan) return(-1);
	return schan_set(chan, value, setting, 0, NULL);
}

/* Output queue commands. */

static int script_queue_len(char *qname, char *next)
{
	int prio;
	queue_t *queue;

	/* Look up queue. */
	prio = name_to_priority(qname, next);
	queue = queue_get_by_priority(prio);

	return(queue->len);
}

static void get_queue_entry(char *qname, char *next, int num, queue_t **queue_ptr, queue_entry_t **queue_entry_ptr)
{
	int prio;
	queue_t *queue;
	queue_entry_t *q;

	/* Look up queue. */
	prio = name_to_priority(qname, next);
	queue = queue_get_by_priority(prio);

	/* Get entry. */
	if (num >= 0 && num < queue->len/2) {
		for (q = queue->queue_head; q && num > 0; q = q->next) {
			num--;
		}
	}
	else if (num >= queue->len/2 && num < queue->len) {
		num -= (queue->len/2);
		for (q = queue->queue_tail; q && num > 0; q = q->prev) {
			num--;
		}
	}
	else q = NULL;

	if (queue_ptr) *queue_ptr = queue;
	if (queue_entry_ptr) *queue_entry_ptr = q;
}

static char *script_queue_get(char *qname, char *next, int num)
{
	queue_entry_t *q;
	char buf[1024];
	int remaining;

	get_queue_entry(qname, next, num, NULL, &q);
	if (!q) return(NULL);

	remaining = sizeof(buf);
	queue_entry_to_text(q, buf, &remaining);
	if (remaining < sizeof(buf)) buf[sizeof(buf)-remaining-1] = 0;
	else buf[0] = 0;

	return strdup(buf);
}

static int script_queue_set(char *qname, char *next, int num, char *msg)
{
	queue_entry_t *q;

	get_queue_entry(qname, next, num, NULL, &q);
	if (!q) return(-1);

	queue_entry_from_text(q, msg);
	return(0);
}

static int script_queue_insert(char *qname, char *next, int num, char *msg)
{
	queue_t *queue;
	queue_entry_t *q, *newq;

	get_queue_entry(qname, next, num, &queue, &q);
	newq = queue_new(msg);
	if (!q) queue_append(queue, newq);
	else {
		queue->len++;
		newq->next = q;
		newq->prev = q->prev;
		if (q->prev) q->prev->next = newq;
		else queue->queue_head = newq;
		q->prev = q;
	}

	return(0);
}

/* 005 support */
static int script_supports(const char *name)
{
	const char *value;

	if (!server_support(name, &value)) {
		/* Supported. */
		return(1);
	}
	return(0);
}

static const char *script_support_val(const char *name)
{
	const char *value;
	server_support(name, &value);
	return value;
}

static script_linked_var_t server_script_vars[] = {
	{"", "servidx", &current_server.idx, SCRIPT_INTEGER | SCRIPT_READONLY, NULL},	/* DDD	*/
	{"", "server", &server_list_index, SCRIPT_INTEGER | SCRIPT_READONLY, NULL},	/* DDD	*/
	{"", "botnick", &current_server.nick, SCRIPT_STRING | SCRIPT_READONLY, NULL},	/* DDD	*/
	{"", "myip", &current_server.myip, SCRIPT_STRING, NULL},			/* DDD	*/
	{"", "mylongip", &current_server.mylongip, SCRIPT_UNSIGNED, NULL},		/* DDD	*/
	{"", "server_input_string", &global_input_string, SCRIPT_STRING, NULL},		/* DDD	*/
	{"", "server_output_string", &global_output_string, SCRIPT_STRING, NULL},	/* DDD	*/
	{0}
};

static script_command_t server_script_cmds[] = {
	/* Misc commands. */
	{"", "irccmp", script_irccmp, NULL, 2, "ss", "str1 str2", SCRIPT_INTEGER, 0},	/* DDC */
	{"", "get_uhost", uhost_cache_lookup, NULL, 1, "s", "nick", SCRIPT_STRING, 0},	/* DDC */

	/* Botnick commands. */
	{"", "nick_add", nick_add, NULL, 1, "s", "nick", SCRIPT_INTEGER, 0},		/* DDC */
	{"", "nick_del", nick_del, NULL, 1, "i", "nick-num", SCRIPT_INTEGER, 0},	/* DDC */
	{"", "nick_clear", nick_clear, NULL, 0, "", "", SCRIPT_INTEGER, 0},		/* DDC */
	{"", "nick_find", nick_find, NULL, 0, "s", "nick", SCRIPT_INTEGER, 0},		/* DDC */
	{"", "isbotnick", script_isbotnick, NULL, 1, "s", "nick", SCRIPT_INTEGER, 0},	/* DDC */

	/* Server List commands. */
	{"", "jump", script_jump, NULL, 0, "i", "num", SCRIPT_INTEGER, SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},	/* DDC */
	{"", "server_add", server_add, NULL, 1, "sis", "host ?port? ?pass?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},	/* DDC */
	{"", "server_del", server_del, NULL, 1, "i", "server-num", SCRIPT_INTEGER, 0},				/* DDC */
	{"", "server_clear", server_clear, NULL, 0, "", "", SCRIPT_INTEGER, 0},					/* DDC */
	{"", "server_find", server_find, NULL, 1, "sis", "host ?port? ?pass?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},/* DDC */

	/* 005 Commands */
	{"", "server_supports", script_supports, NULL, 1, "s", "name", SCRIPT_INTEGER, 0},			/* DDC */
	{"", "server_support_val", script_support_val, NULL, 1, "s", "name", SCRIPT_STRING, 0},			/* DDC */

	/* DCC commands. */
	{"", "dcc_chat", dcc_start_chat, NULL, 1, "si", "nick ?timeout?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},		/* DDC */
	{"", "dcc_send", dcc_start_send, NULL, 2, "ssi", "nick filename ?timeout?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},	/* DDC */
	{"", "dcc_send_info", script_dcc_send_info, NULL, 2, "is", "idx stat", SCRIPT_INTEGER, 0}, /* DDD */
	{"", "dcc_accept_send", dcc_accept_send, NULL, 7, "sssiisii", "nick localfile remotefile size resume ip port ?timeout?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS}, /* DDD */

	/* Channel commands. */
	{"", "channel_list", script_channel_list, NULL, 0, "", "", 0, SCRIPT_PASS_RETVAL},					/* DDC */
	{"", "channel_members", script_channel_members, NULL, 1, "s", "channel", 0, SCRIPT_PASS_RETVAL},			/* DDC */
	{"", "channel_topic", script_channel_topic, NULL, 1, "s", "channel", 0, SCRIPT_PASS_RETVAL},				/* DDC */
	{"", "channel_mask_list", script_channel_mask_list, NULL, 2, "ss", "channel type", 0, SCRIPT_PASS_RETVAL},		/* DDC */
	{"", "channel_mode", script_channel_mode, NULL, 1, "ss", "channel ?nick?", SCRIPT_STRING|SCRIPT_FREE, SCRIPT_VAR_ARGS},	/* DDC */
	{"", "channel_mode_arg", script_channel_mode_arg, NULL, 2, "ss", "channel mode", SCRIPT_STRING, 0},	/* DDC */
	{"", "channel_key", script_channel_key, NULL, 1, "s", "channel", SCRIPT_STRING, 0},					/* DDC */
	{"", "channel_limit", script_channel_limit, NULL, 1, "s", "channel", SCRIPT_INTEGER, 0},				/* DDC */

	/* Schan commands. */
	{"", "channel_get", script_schan_get, NULL, 2, "ss", "channel setting", SCRIPT_STRING, 0},
	{"", "channel_set", script_schan_set, NULL, 3, "sss", "channel setting value", SCRIPT_INTEGER, 0},

	/* Input / Output commands. */
	{"", "server_fake_input", server_parse_input, NULL, 1, "s", "text", SCRIPT_INTEGER, 0},						/* DDC */
	{"", "putserv", script_putserv, NULL, 1, "sss", "?queue? ?next? text", SCRIPT_INTEGER, SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},	/* DDC */

	/* Output queue commands. */
	{"", "server_queue_len", script_queue_len, NULL, 1, "ssi", "queue ?next?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},						/* DDC */
	{"", "server_queue_get", script_queue_get, NULL, 1, "ssi", "?queue? ?next? msgnum", SCRIPT_STRING|SCRIPT_FREE, SCRIPT_VAR_ARGS|SCRIPT_VAR_FRONT},	/* DDC */
	{"", "server_queue_set", script_queue_set, NULL, 2, "ssis", "?queue? ?next? msgnum msg", SCRIPT_INTEGER, SCRIPT_VAR_ARGS|SCRIPT_VAR_FRONT},		/* DDC */
	{"", "server_queue_insert", script_queue_insert, NULL, 2, "ssis", "?queue? ?next? msgnum msg", SCRIPT_INTEGER, SCRIPT_VAR_ARGS|SCRIPT_VAR_FRONT},	/* DDC */

        {0}
};

int server_script_init()
{
	script_create_commands(server_script_cmds);
	script_link_vars(server_script_vars);
	return(0);
}

int server_script_destroy()
{
	script_delete_commands(server_script_cmds);
	script_unlink_vars(server_script_vars);
	return(0);
}
