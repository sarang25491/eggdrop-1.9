/*
 * scriptcmds.c --
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Eggheads Development Team
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

/* FIXME: #include mess
#ifndef lint
static const char rcsid[] = "$Id: scriptcmds.c,v 1.16 2003/02/18 10:13:17 stdarg Exp $";
#endif
*/

#include "lib/eggdrop/module.h"

#include "server.h"
#include "serverlist.h"
#include "nicklist.h"
#include "output.h"
#include "servsock.h"
#include "dcc.h"

static int script_isbotnick (char *nick);
static int script_putserv(char *queue, char *next, char *text);
static int script_jump (int nargs, int num);
static int script_dcc_send_info(int idx, char *what);

/* From serverlist.c */
extern int server_list_index;

/* From server.c */
extern int cycle_delay;
extern current_server_t current_server;

script_linked_var_t server_script_vars[] = {
	{"", "servidx", &current_server.idx, SCRIPT_INTEGER | SCRIPT_READONLY, NULL},
	{"", "server", &server_list_index, SCRIPT_INTEGER | SCRIPT_READONLY, NULL},
	{"", "botnick", &current_server.nick, SCRIPT_STRING | SCRIPT_READONLY, NULL},
	{0}
};

script_command_t server_script_cmds[] = {
        {"", "jump", script_jump, NULL, 0, "i", "num", SCRIPT_INTEGER, SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
        {"", "isbotnick", script_isbotnick, NULL, 1, "s", "nick", SCRIPT_INTEGER, 0},
        {"", "putserv", script_putserv, NULL, 1, "sss", "?-queuetype? ?-next? text", SCRIPT_INTEGER, SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
	{"", "server_add", server_add, NULL, 1, "sis", "host ?port? ?pass?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "server_del", server_del, NULL, 1, "i", "server-num", SCRIPT_INTEGER, 0},
	{"", "server_clear", server_clear, NULL, 0, "", "", SCRIPT_INTEGER, 0},
	{"", "nick_add", nick_add, NULL, 1, "s", "nick", SCRIPT_INTEGER, 0},
	{"", "nick_del", nick_del, NULL, 1, "i", "nick-num", SCRIPT_INTEGER, 0},
	{"", "nick_clear", nick_clear, NULL, 0, "", "", SCRIPT_INTEGER, 0},
	{"", "dcc_chat", dcc_start_chat, NULL, 1, "si", "nick ?timeout?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "dcc_send", dcc_start_send, NULL, 2, "ssi", "nick filename ?timeout?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
	{"", "dcc_send_info", script_dcc_send_info, NULL, 2, "is", "idx stat", SCRIPT_INTEGER, 0},
	{"", "dcc_accept_send", dcc_accept_send, NULL, 7, "sssiisii", "nick localfile remotefile size resume ip port ?timeout?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS},
        {0}
};

/* match_my_nick() is a macro so we can't put it right into the script command table. */
static int script_isbotnick(char *nick)
{
	return match_my_nick(nick);
}

static int script_putserv(char *queue, char *next, char *text)
{
	int prio;

	/* Figure out which arguments they've given us. */
	if (!next) queue = "-serv";
	else if (!queue) {
		/* If we only have 1 option, check to see if it's -next or
			a queue. */
		if (!strcasecmp(next, "-next")) queue = "-serv";
		else {
			queue = next;
			next = NULL;
		}
	}
	else if (!strcasecmp(queue, "-next")) {
		/* They did it in reverse order, so swap them. */
		char *temp = next;
		next = queue;
		queue = temp;
	}

	/* Figure out which queue they want. */
	if (!strcasecmp(queue, "-noqueue")) {
		prio = SERVER_NOQUEUE;
	}
	else if (!strcasecmp(queue, "-help")) {
		prio = SERVER_HELP;
	}
	else if (!strcasecmp(queue, "-quick")) {
		prio = SERVER_MODE;
	}
	else {
		prio = SERVER_NORMAL;
	}

	if (next) prio |= SERVER_NEXT;

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

	if (!strcasecmp(what, "bytes_left")) field = DCC_SEND_LEFT;
	else if (!strcasecmp(what, "bytes_sent")) field = DCC_SEND_SENT;
	else if (!strcasecmp(what, "total_cps")) field = DCC_SEND_CPS_TOTAL;
	else if (!strcasecmp(what, "snapshot_cps")) field = DCC_SEND_CPS_SNAPSHOT;
	else return(-1);

	if (dcc_send_info(idx, field, &value) < 0) return(-1);
	return(value);
}
