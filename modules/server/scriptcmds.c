/*
 * scriptcmds.c -- part of server.mod
 *
 * $Id: scriptcmds.c,v 1.2 2002/05/01 05:31:04 stdarg Exp $
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001, 2002 Eggheads Development Team
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

#include <eggdrop/eggdrop.h> /* Eggdrop API */

static int script_isbotnick (char *nick);
static int script_putserv(char *queue, char *next, char *text);
static int script_jump (int nargs, char *optserver, int optport, char *optpassword);
static int script_clearqueue (script_var_t *retval, char *queuetype);
static int script_queuesize (script_var_t *retval, int nargs, char *queuetype, int flags);

static script_command_t server_script_cmds[] = {
        {"", "jump", script_jump, NULL, 0, "sis", "server port password", SCRIPT_INTEGER, SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT},
        {"", "isbotnick", script_isbotnick, NULL, 1, "s", "nick", SCRIPT_INTEGER, 0},
        {"", "clearqueue", script_clearqueue, NULL, 1, "s", "queuetype", SCRIPT_INTEGER, SCRIPT_PASS_RETVAL},
        {"", "queuesize", script_queuesize, NULL, 0, "s", "?queuetype?", SCRIPT_INTEGER, SCRIPT_VAR_ARGS | SCRIPT_PASS_COUNT | SCRIPT_PASS_RETVAL},
        {"", "putserv", script_putserv, NULL, 1, "sss", "?-queuetype? ?-next? text", SCRIPT_INTEGER, SCRIPT_VAR_ARGS | SCRIPT_VAR_FRONT},
        {0}
};



static int script_isbotnick (char *nick)
{
	return match_my_nick(nick);
}

static int script_putserv (char *queue, char *next, char *text)
{
	char s[511];
	int queue_num = 0;
	int len = strlen(text);

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
		if (serv >= 0) {
			/* We'll add a \r\n to the end before we send it. */
			char *copy = msprintf("%s\r\n", text);
			tputs(serv, copy, len+2);
			free(copy);
		}
		return(0);
	}
	else if (!strcasecmp(queue, "-help")) {
		queue_num = next ? DP_HELP_NEXT : DP_HELP;
	}
	else if (!strcasecmp(queue, "-quick")) {
		queue_num = next ? DP_MODE_NEXT : DP_MODE;
	}
	else {
		queue_num = next ? DP_SERVER_NEXT : DP_SERVER;
	}

	/* We trim the data to make it IRC-compatible */
	if (len > 510) {
		strncpy(s, text, 510);
		s[510] = 0;

		dprintf(queue_num, "%s\n", s);
	}
	else dprintf(queue_num, "%s\n", text);

	return(0);
}

static int script_jump (int nargs, char *optserver, int optport, char *optpassword)
{
  	if (nargs >= 1) {
    		strlcpy(newserver, optserver, sizeof(newserver));
    		
		if (nargs >= 2)
      			newserverport = optport;
    		else
      			newserverport = default_port;
    
		if (nargs == 3)
      			strlcpy(newserverpass, optpassword, sizeof(newserverpass) );
  	}
  
	cycle_time = 0;
	
  	nuke_server("changing servers\n");
	
	return(1);
}

static int script_clearqueue (script_var_t *retval, char *queuetype)
{
	struct msgq *q, *qq;
	int msgs = 0;
	retval->type = SCRIPT_INTEGER;
	
	if (!strcmp(queuetype,"all")) {
		msgs = (int) (modeq.tot + mq.tot + hq.tot);
		for (q = modeq.head; q; q = qq) { 
			qq = q->next;
			free(q->msg);
			free(q);
		}
		for (q = mq.head; q; q = qq) {
			qq = q->next;
			free(q->msg);
			free(q);
		}
		for (q = hq.head; q; q = qq) {
			qq = q->next;
			free(q->msg);
			free(q);
		}
		modeq.tot = mq.tot = hq.tot = modeq.warned = mq.warned = hq.warned = 0;
		mq.head = hq.head = modeq.head = mq.last = hq.last = modeq.last = 0;
		double_warned = 0;
		burst = 0;
		
		retval->value = (void *)msgs;
		return(1);
	
	} else if (!strncmp(queuetype,"serv", 4)) {
		msgs = mq.tot;
		for (q = mq.head; q; q = qq) {
			qq = q->next;
			free(q->msg);
			free(q);
		}
		mq.tot = mq.warned = 0;
		mq.head = mq.last = 0;
		if (modeq.tot == 0)
			burst = 0;
		double_warned = 0;
		mq.tot = mq.warned = 0;
		mq.head = mq.last = 0;
		
		retval->value = (void *)msgs;
		return(1);
	
	} else if (!strcmp(queuetype,"mode")) {
		msgs = modeq.tot;
		for (q = modeq.head; q; q = qq) { 
			qq = q->next;
			free(q->msg);
			free(q);
		}
		if (mq.tot == 0)
			burst = 0;
		double_warned = 0;
		modeq.tot = modeq.warned = 0;
		modeq.head = modeq.last = 0;
		
		retval->value = (void *)msgs;	
		return(1);
		
	} else if (!strcmp(queuetype,"help")) {
		msgs = hq.tot;
		for (q = hq.head; q; q = qq) {
			qq = q->next;
			free(q->msg);
			free(q);
		}
		double_warned = 0;
		hq.tot = hq.warned = 0;
		hq.head = hq.last = 0;
	
		retval->value = (void *)msgs;
		return(1);
	}
	
	retval->type = SCRIPT_STRING | SCRIPT_ERROR;
	retval->value = "bad option: must be mode, server, help, or all";
	
	return(0);
}

static int script_queuesize (script_var_t *retval, int nargs, char *queuetype, int flags)
{	
	int x = 0;

	retval->type = SCRIPT_INTEGER;
	if (nargs == 0) {
		x = (int) (modeq.tot + hq.tot + mq.tot);
		retval->value = (void *)x;
		return(1);
	} else if (!strncmp(queuetype, "serv", 4)) {
		x = (int) (mq.tot);
		retval->value = (void *)x;
		return(1);
	} else if (!strcmp(queuetype, "mode")) {
		x = (int) (modeq.tot);
		retval->value = (void *)x;
		return(1);
	} else if (!strcmp(queuetype, "help")) {
		x = (int) (hq.tot);
		retval->value = (void *)x;
		return(1);
	}
	
	retval->type = SCRIPT_STRING | SCRIPT_ERROR;
	retval->value = "bad option: must be mode, server, or help";
	return(0);
}

