/* telnetparty.c: telnet partyline interface
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef lint
static const char rcsid[] = "$Id: telnetparty.c,v 1.15 2004/06/17 13:32:44 wingman Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "telnetparty.h"

telnet_config_t telnet_config = {0};

static config_var_t telnet_config_vars[] = {
	{"vhost", &telnet_config.vhost, CONFIG_STRING},
	{"port", &telnet_config.port, CONFIG_INT},
	{"stealth", &telnet_config.stealth, CONFIG_INT},
	{"max_retries", &telnet_config.max_retries, CONFIG_INT},
	{0}
};

EXPORT_SCOPE int telnetparty_LTX_start(egg_module_t *modinfo);
static int telnetparty_close(int why);

/* From events.c */
extern partyline_event_t telnet_party_handler;

static int telnet_idx = -1;
static int telnet_port = 0;
static int terminal_idx = -1;
static user_t *terminal_user = NULL;

static int telnet_on_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port);
static int telnet_on_read(void *client_data, int idx, char *data, int len);
static int telnet_on_eof(void *client_data, int idx, int err, const char *errmsg);
static int telnet_on_delete(void *client_data, int idx);
static int telnet_filter_read(void *client_data, int idx, char *data, int len);
static int telnet_filter_write(void *client_data, int idx, const char *data, int len);
static int telnet_filter_delete(void *client_data, int idx);

static int ident_result(void *client_data, const char *ip, int port, const char *reply);
static int dns_result(void *client_data, const char *ip, char **hosts);
static int process_results(telnet_session_t *session);

static sockbuf_handler_t server_handler = {
	"telnet server",
	NULL, NULL, telnet_on_newclient,
	NULL, NULL
};

#define TELNET_FILTER_LEVEL	SOCKBUF_LEVEL_TEXT_ALTERATION

static sockbuf_filter_t telnet_filter = {
	"telnet filter", TELNET_FILTER_LEVEL,
	NULL, NULL, NULL,
	telnet_filter_read, telnet_filter_write, NULL,
	NULL, telnet_filter_delete
};

static sockbuf_handler_t client_handler = {
	"telnet",
	NULL, telnet_on_eof, NULL,
	telnet_on_read, NULL,
	telnet_on_delete
};

static void
terminal_mode ()
{	
	putlog (LOG_MISC, "*", "Entering terminal mode.");
	
	terminal_idx = sockbuf_new ();
	sockbuf_set_sock (terminal_idx, fileno (stdin), SOCKBUF_CLIENT);
	telnet_on_newclient (NULL, telnet_idx, terminal_idx, "127.0.0.1", 0);	
}

int telnet_init()
{	
	/* Open our listening socket. */
	telnet_idx = egg_server(telnet_config.vhost, telnet_config.port, &telnet_port);
	sockbuf_set_handler(telnet_idx, &server_handler, NULL);
	
	/* XXX: invent something for better access of command line args
	 * XXX: in modules. */
	if (partyline_terminal_mode) {
		/* We don't go into terminal right now since not all modules might be
	    	 * loaded at this stage. So right before we enter our main loop we 
	  	 * enable our terminal. */
		bind_add_simple ("init", NULL, NULL, (Function)terminal_mode);	
	}
	
	return(0);
}

void telnet_code(int idx, int cmd, int what)
{
	char temp[3];
	temp[0] = TELNET_CMD;
	temp[1] = cmd;
	temp[2] = what;
	sockbuf_on_write(idx, TELNET_FILTER_LEVEL, temp, 3);
}

void convert_ansi_code_to_mirc(int *tokens, int ntokens, char **dest, int *cur, int *max)
{
	int ansi_to_mirc_colors[] = {1, 4, 3, 8, 2, 5, 10, 0};
	int i, in_bold, in_uline, in_reverse, fg, bg;
	char *newline;
	char buf[64];

	/* User inserted some color stuff... */
	in_bold = in_uline = in_reverse = 0;
	fg = bg = -1;
	for (i = 0; i < ntokens; i++) {
		if (tokens[i] == 0) in_bold = in_uline = in_reverse = 0;
		else if (tokens[i] == 1) in_bold = 1;
		else if (tokens[i] == 4) in_uline = 1;
		else if (tokens[i] == 7) in_reverse = 1;
		else if (tokens[i] == 8) fg = bg = 99;
		else if (tokens[i] >= 30 && tokens[i] <= 37) {
			fg = ansi_to_mirc_colors[tokens[i] - 30];
		}
		else if (tokens[i] >= 40 && tokens[i] <= 47) {
			bg = ansi_to_mirc_colors[tokens[i] - 40];
		}
	}
	if (*max - *cur < 10) {
		*dest = realloc(*dest, *max + 10);
		*max += 10;
	}
	newline = buf;
	if (in_bold) *newline++ = '\002';
	if (in_uline) *newline++ = '\037';
	if (in_reverse) *newline++ = '\026';
	if (fg != -1 || bg != -1) {
		*newline++ = '\003';
		if (fg == -1) fg = 99;
		sprintf(newline, "%02d", fg);
		newline += 2;
		if (bg != -1) {
			sprintf(newline, ",%02d", bg);
			newline += 3;
		}
	}
	*newline = 0;
	egg_append_str(dest, cur, max, buf);
}

/* Get an int up to 2 digits from the string. Used for getting
 * mirc color values. */
int get_2(const char *data, int len, int *val)
{
	if (isdigit(*data)) {
		*val = *data++ - '0';
		if (len > 1 && isdigit(*data)) {
			*val *= 10;
			*val += *data - '0';
			return(2);
		}
		return(1);
	}
	return(0);
}

int convert_mirc_color_to_ansi(const char *data, int len, char **newline, int *cur, int *max)
{
	const char *orig;
	char add[64], temp[64];
	/* Convert a mirc color (0-15) to an ansi escape color (0-7). */
	int mirc_to_ansi[] = {7, 0, 4, 2, 1, 1, 5, 1, 3, 2, 6, 6, 4, 5, 0, 0};
	int c1, c2, n;

	orig = data;
	c1 = c2 = -1;
	n = get_2(data, len, &c1);
	if (n) {
		data += n; len -= n;
		if (len && *data == ',') {
			len--;
			data++;
			n = get_2(data, len, &c2);
			data += n;
			len -= n;
		}
	}
	strcpy(add, "\033[");
	if (c1 != -1) {
		if (c1 != 99) {
			c1 %= 16;
			sprintf(temp, "%d", 30+mirc_to_ansi[c1]);
			strcat(add, temp);
		}
		if (c2 != -1 && c2 != 99) {
			c2 %= 16;
			sprintf(temp, "%d", 40+mirc_to_ansi[c2]);
			if (c1 != 99) strcat(add, ";");
			strcat(add, temp);
		}
		strcat(add, "m");
	}
	else strcat(add, "0m");
	egg_append_str(newline, cur, max, add);
	return data - orig;
}

void handle_ansi_code(const char **src, char **dest, int *cur, int *max)
{
	const char *line = *src;
	int token, ntokens, *tokens;

	if (*line++ != 27) return;
	*src = line;
	if (*line++ != '[') return;

	/* Now get stuff in the form <num>;<num>;<num><alpha> */
	token = 0;
	ntokens = 0;
	tokens = NULL;
	while (*line) {
		if (*line >= '0' && *line <= '9') {
			/* Building this token still. */
			token *= 10;
			token += (*line - '0');
		}
		else if (*line == ';' || isalpha(*line)) {
			/* Done with this token. */
			tokens = realloc(tokens, sizeof(int) * (ntokens+1));
			tokens[ntokens++] = token;
			token = 0;
			if (isalpha(*line)) break;
		}
		else break;
		line++;
	}

	if (*line == 'm') {
		/* Graphic rendition (color, underline, etc)... */
		convert_ansi_code_to_mirc(tokens, ntokens, dest, cur, max);
		*src = line+1;
	}

	if (tokens) free(tokens);
}

/* Returns a newly allocated line with all proper substitutions made. */
char *telnet_fix_line(const char *line)
{
	char *newline;
	int cur, max;

	cur = 0;
	max = 128;
	newline = malloc(max+1);

	while (*line) {
		switch (*line) {
			case 7:
				/* Get rid of beeps. */
				line++;
				break;
			case 8:
				/* Fix backspaces. */
				if (cur > 0) cur--;
				line++;
				break;
			case 27:
				/* Handle ansi escape sequences if possible. */
				handle_ansi_code(&line, &newline, &cur, &max);
				break;
			default:
				if (cur >= max) {
					newline = realloc(newline, max+128);
					max += 128;
				}
				newline[cur++] = *line++;
		}
	}
	newline[cur] = 0;

	return(newline);
}

static void kill_session(telnet_session_t *session)
{
	if (session->ip) free(session->ip);
	if (session->host) free(session->host);
	if (session->ident) free(session->ident);
	if (session->nick) free(session->nick);
	free(session);
}

static int telnet_on_newclient(void *client_data, int idx, int newidx, const char *peer_ip, int peer_port)
{
	telnet_session_t *session;

	session = calloc(1, sizeof(*session));
	session->ip = strdup(peer_ip);
	session->port = peer_port;
	session->idx = newidx;

	sockbuf_set_handler(newidx, &client_handler, session);
	sockbuf_attach_filter(newidx, &telnet_filter, session);
	linemode_on(newidx);

	if (newidx == terminal_idx) {
		if (terminal_user == NULL) {
			terminal_user = user_lookup_by_handle (PARTY_TERMINAL_NICK);
			if (terminal_user == NULL) {
				terminal_user = user_new (PARTY_TERMINAL_NICK);
				user_set_flags_str (terminal_user, NULL, "n");
 			}
		}

			
		session->state = STATE_PARTYLINE;
		session->party = partymember_new (-1,
			terminal_user, PARTY_TERMINAL_NICK, PARTY_TERMINAL_USER,
			PARTY_TERMINAL_HOST, &telnet_party_handler, session);
			
		/* Put them on the main channel. */
		partychan_join_name("*", session->party);		
		return 0;		
	}
		
	/* Stealth logins are where we don't say anything until we know they
	 * are a valid user. */
	if (telnet_config.stealth) {
		session->state = STATE_RESOLVE;
		session->flags |= STEALTH_LOGIN;
	}
	else {
		egg_iprintf(newidx, "Hello %s/%d!\r\n", peer_ip, peer_port);
		sockbuf_write(newidx, "\r\nPlease enter your nickname.\r\n", -1);
		session->state = STATE_NICKNAME;
		session->count = 0;
	}

	/* Start lookups. */
	egg_ident_lookup(peer_ip, peer_port, telnet_port, -1, ident_result, session);
	egg_dns_reverse(peer_ip, -1, dns_result, session);

	return(0);
}

static int ident_result(void *client_data, const char *ip, int port, const char *reply)
{
	telnet_session_t *session = client_data;

	if (reply) session->ident = strdup(reply);
	else session->ident = strdup("~telnet");
	process_results(session);
	return(0);
}

static int dns_result(void *client_data, const char *ip, char **hosts)
{
	telnet_session_t *session = client_data;
	const char *host;

	if (!hosts || !hosts[0]) host = ip;
	else host = hosts[0];

	session->host = strdup(host);
	process_results(session);
	return(0);
}

static int process_results(telnet_session_t *session)
{
	char fakehost[512];

	if (!session->ident || !session->host) return(0);

	if (session->state == STATE_PARTYLINE) {
		partyline_update_info(session->party, session->ident, session->host);
		return(0);
	}
	if (session->flags & STEALTH_LOGIN) {
		snprintf(fakehost, sizeof(fakehost), "-telnet!%s@%s", session->ident, session->host);
		fakehost[sizeof(fakehost)-1] = 0;
		if (!user_lookup_by_irchost(fakehost)) {
			sockbuf_delete(session->idx);
			return(0);
		}
		sockbuf_write(session->idx, "\r\nPlease enter your nickname.\r\n", -1);
		session->state = STATE_NICKNAME;
	}
	return(0);
}

static int telnet_on_read(void *client_data, int idx, char *data, int len)
{
	telnet_session_t *session = client_data;
	char *newline;

	switch (session->state) {
		case STATE_PARTYLINE:
			newline = telnet_fix_line(data);
			if (newline) {
				partyline_on_input(NULL, session->party, newline, strlen(newline));
				free(newline);
			}
			break;
		case STATE_NICKNAME:
			session->nick = strdup(data);
			session->state = STATE_PASSWORD;
			session->flags |= TFLAG_PASSWORD;
			telnet_code(session->idx, TELNET_WILL, TELNET_ECHO);
			sockbuf_write(session->idx, "Please enter your password.\r\n", -1);
			break;
		case STATE_PASSWORD:
			session->flags &= ~TFLAG_PASSWORD;
			telnet_code(session->idx, TELNET_WONT, TELNET_ECHO);
			session->user = user_lookup_authed(session->nick, data);
			if (!session->user) {
				sockbuf_write(session->idx, "Invalid username/password.\r\n\r\n", -1);
				session->count++;
				if (session->count > telnet_config.max_retries) {
					sockbuf_delete(session->idx);
					break;
				}
				free(session->nick);
				session->nick = NULL;
				sockbuf_write(session->idx, "Please enter your nickname.\r\n", -1);
				session->state = STATE_NICKNAME;
			}
			else {
				session->party = partymember_new(-1, session->user, session->nick, session->ident ? session->ident : "~telnet", session->host ? session->host : session->ip, &telnet_party_handler, session);
				session->state = STATE_PARTYLINE;
				egg_iprintf(idx, "\r\nWelcome to the telnet partyline interface!\r\n");
				if (session->ident) egg_iprintf(idx, "Your ident is: %s\r\n", session->ident);
				if (session->host) egg_iprintf(idx, "Your hostname is: %s\r\n", session->host);
				/* Put them on the main channel. */
				partychan_join_name("*", session->party);
			}
			break;
	}
	return(0);
}

static int telnet_on_eof(void *client_data, int idx, int err, const char *errmsg)
{
	telnet_session_t *session = client_data;

	if (session->party) {
		if (!err) errmsg = "Client disconnected";
		else if (!errmsg) errmsg = "Unknown error";
		partymember_delete(session->party, errmsg);
		session->party = NULL;
	}
	sockbuf_delete(idx);
	return(0);
}

static int telnet_on_delete(void *client_data, int idx)
{
	telnet_session_t *session = client_data;

	if (session->party) {
		partymember_delete(session->party, "Deleted!");
		session->party = NULL;
	}
	kill_session(session);
	return(0);
}

static int telnet_filter_read(void *client_data, int idx, char *data, int len)
{
	unsigned char *cmd;
	int type, arg, remove;
	telnet_session_t *session = client_data;

	cmd = data;
	while ((cmd = memchr(cmd, TELNET_CMD, len))) {
		type = *(cmd+1);
		switch (type) {
			case TELNET_AYT:
				sockbuf_write(idx, "[YES]\r\n", 7);
				remove = 1;
				break;
			case TELNET_CMD:
				cmd++;
				remove = 1;
				break;
			case TELNET_DO:
				arg = *(cmd+2);
				if (arg == TELNET_ECHO) {
					if (!(session->flags & TFLAG_ECHO)) {
						telnet_code(idx, TELNET_WILL, arg);
						session->flags |= TFLAG_ECHO;
					}
				}
				else telnet_code(idx, TELNET_WONT, arg);
				remove = 3;
				break;
			case TELNET_DONT:
				arg = *(cmd+2);
				if (arg == TELNET_ECHO) {
					if (session->flags & TFLAG_ECHO) {
						telnet_code(idx, TELNET_WONT, arg);
						session->flags &= ~TFLAG_ECHO;
					}
				}
				remove = 3;
				break;
			case TELNET_WILL:
				remove = 3;
				break;
			case TELNET_WONT:
				remove = 3;
				break;
			default:
				remove = 2;
		}
		memmove(cmd, cmd+remove, len - (cmd - (unsigned char *)data) - remove + 1);
		len -= remove;
	}

	if (len) {
		if (session->flags & TFLAG_ECHO && !(session->flags & TFLAG_PASSWORD)) sockbuf_on_write(idx, TELNET_FILTER_LEVEL, data, len);
		sockbuf_on_read(idx, TELNET_FILTER_LEVEL, data, len);
	}
	return(0);
}

/* We replace \n with \r\n and \255 with \255\255. */
static int telnet_filter_write(void *client_data, int idx, const char *data, int len)
{
	char *newline;
	const char *filterchars = "\n\r\003\002\007\017\026\037\255";
	char temp[64];
	int i, cur, max, n, code, esc;

	cur = 0;
	max = 128;
	newline = malloc(max+1);

	code = 0;
	esc = -1;
	while (len > 0) {
		switch (*data) {
			case '\002':
				esc = 1;
				break;
			case '\003':
				code = 1;
				data++; len--;
				n = convert_mirc_color_to_ansi(data, len, &newline, &cur, &max);
				data += n;
				len -= n;
				break;
			case '\007':
				/* Skip. */
				data++; len--;
				break;
			case '\017':
				esc = 0;
				break;
			case '\026':
				esc = 7;
				break;
			case '\037':
				esc = 4;
				break;
			case '\r':
			case '\n':
				if (code) {
					/* If we've used an escape code, we
					 * append the 'reset' code right before
					 * the end of the line. */
					egg_append_str(&newline, &cur, &max, "\033[0m");
					code = 0;
				}
				if (*data != '\r') {
					/* Skip \r and wait for \n, where we
					 * add both (prevents stray \r's). */
					egg_append_str(&newline, &cur, &max, "\r\n");
				}
				data++;
				len--;
				break;
			case '\255':
				/* This is the telnet command char, so we have
				 * to escape it. */
				egg_append_str(&newline, &cur, &max, "\255\255");
				data++;
				len--;
				break;
			default:
				for (i = 0; i < sizeof(temp)-1 && i < len; i++) {
					if (strchr(filterchars, *data)) break;
					temp[i] = *data++;
				}
				temp[i] = 0;
				egg_append_str(&newline, &cur, &max, temp);
				len -= i;
		}

		if (esc != -1) {
			/* Add the escape code to the line. */
			sprintf(temp, "\033[%dm", esc);
			egg_append_str(&newline, &cur, &max, temp);
			data++; len--;
			esc = -1;

			/* Remember that we're in a code so we can reset it
			 * at the end of the line. */
			code = 1;
		}
	}
	newline[cur] = 0;
	if (cur > 0) {
		n = sockbuf_on_write(idx, TELNET_FILTER_LEVEL, newline, cur);
	}
	else n = 0;
	free(newline);
	return(n);
}

static int telnet_filter_delete(void *client_data, int idx)
{
	/* The client data is a telnet_session_t, which will be freed
	 * by telnet_on_delete. So we do nothing. */
	return(0);
}

static int telnetparty_close(int why)
{	
	if (terminal_user != NULL)
		user_delete (terminal_user);
		
	if (terminal_idx != -1)
		sockbuf_delete (terminal_idx);
		
	sockbuf_delete(telnet_idx);
	return(0);
}

int telnetparty_LTX_start(egg_module_t *modinfo)
{
	void *config_root;

	modinfo->name = "telnetparty";
	modinfo->author = "eggdev";
	modinfo->version = "1.0.0";
	modinfo->description = "telnet support for the partyline";
	modinfo->close_func = telnetparty_close;

	/* Set defaults. */
	memset(&telnet_config, 0, sizeof(telnet_config));
	telnet_config.max_retries = 3;

	/* Link our vars in. */
	config_root = config_get_root("eggdrop");
	config_link_table(telnet_config_vars, config_root, "telnetparty", 0, NULL);
	config_update_table(telnet_config_vars, config_root, "telnetparty", 0, NULL);

	telnet_init();
	return(0);
}
