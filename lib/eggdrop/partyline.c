/* partyline.c: partyline functions
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
 * alon	with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef lint
static const char rcsid[] = "$Id: partyline.c,v 1.23 2004/06/22 20:12:37 wingman Exp $";
#endif

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <eggdrop/eggdrop.h>

static char *partyline_command_chars = NULL;

static bind_table_t *BT_cmd = NULL; /* Commands. */
static bind_table_t *BT_party_out = NULL; /* Text sent to a user. */

static int on_putlog(int flags, const char *chan, const char *text, int len);

extern int partychan_init(void);
extern int partychan_shutdown(void);

extern int partymember_init(void);
extern int partymember_shutdown(void);

static bind_list_t log_binds[] = {
	{NULL, NULL, on_putlog},
	{0}
};

int partyline_init(void)
{
	partychan_init();
	partymember_init();
	str_redup(&partyline_command_chars, "./");
	BT_cmd = bind_table_add(BTN_PARTYLINE_CMD, 5, "PsUss", MATCH_PARTIAL | MATCH_FLAGS, 0);	/* DDD	*/
	BT_party_out = bind_table_add(BTN_PARTYLINE_OUT, 5, "isUsi", MATCH_NONE, 0);		/* DDD	*/
	bind_add_list(BTN_LOG, log_binds);
	return(0);
}

int partyline_shutdown(void)
{
	bind_rem_list(BTN_LOG, log_binds);
	bind_table_del(BT_party_out);
	bind_table_del(BT_cmd);

	if (partyline_command_chars) 
		free(partyline_command_chars);
	partyline_command_chars = NULL;

	partymember_shutdown();
	partychan_shutdown();

	return (0);
}

/* 1 if the text starts with a recognized command character or 0 if not */
int partyline_is_command(const char *text)
{
	return strchr(partyline_command_chars, *text) ? 1 : 0;
}

/* Process any input from someone on the partyline (not bots, just users).
 * Everything is either a command or public text. Private messages and all that
 * are handled via commands. */
int partyline_on_input(partychan_t *chan, partymember_t *p, const char *text, int len)
{
	if (!p || p->flags & PARTY_DELETED || !len) return(0);
	if (!chan) chan = partychan_get_default(p);

	/* See if we should interpret it as a command. */
	if (strchr(partyline_command_chars, *text)) {
		text++;
		len--;
		/* If there are 2 command chars in a row, then it's not a
		 * command. We skip the command char and treat it as text. */
		if (*text != *(text-1)) {
			const char *space;

			space = strchr(text, ' ');
			if (space) {
				char *cmd;

				len = space-text;
				cmd = malloc(len+1);
				memcpy(cmd, text, len);
				cmd[len] = 0;
				while (isspace(*space)) space++;
				partyline_on_command(p, cmd, space);
				free(cmd);
			}
			else {
				partyline_on_command(p, text, NULL);
			}
			return(0);
		}
	}
	/* It's not a command, so it's public text. */
	partychan_msg(chan, p, text, len);
	return(0);
}

/* Processes input as a command. The first word is the command, and the rest
 * is the argument. */
int partyline_on_command(partymember_t *p, const char *cmd, const char *text)
{
	int r, hits;

	if (!p) return(-1);

	r = bind_check_hits(BT_cmd, &p->user->settings[0].flags, cmd, &hits, p, p->nick, p->user, cmd, text);

	/* if we have no hits then there's no such command */
	if (hits == 0)
		partymember_printf (p, _("Unknown command '%s', perhaps you need '.help'?"),
			cmd);
	else {
		switch (r) {
			/* logs everything */
			case (BIND_RET_LOG):
				putlog(LOG_MISC, "*", "#%s# %s %s", p->nick, cmd, (text) ? text : "");
				break;
			/* usefull if e.g. a .chpass is issued */
			case (BIND_RET_LOG_COMMAND):
				putlog(LOG_MISC, "*", "#%s# %s ...", p->nick, cmd);
				break;
			/* command doesn't want any lo	entry at all */
			case (BIND_RET_BREAK):
				break;
		}
		
	}

	return(r);
}

int partyline_update_info(partymember_t *p, const char *ident, const char *host)
{
	if (!p) return(-1);
	if (ident) str_redup(&p->ident, ident);
	if (host) str_redup(&p->host, host);
	return(0);
}

#if 0
int partyline_write_all_but(int pid, const char *text, int len)
{
	partymember_t *p;

	if (len < 0) len = strlen(text);
	for (p = partyhead; p; p = p->next) {
		if (p->pid != pid) partyline_write_internal(p, text, len);
	}
	return(0);
}

int partyline_printf(int pid, const char *fmt, ...)
{
	va_list args;
	char *ptr, buf[1024];
	int len;

	va_start(args, fmt);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, fmt, args);
	va_end(args);

	partyline_write(pid, ptr, len);

	if (ptr != buf) free(ptr);
	return(0);
}

int partyline_printf_chan_but(int cid, int pid, const char *fmt, ...)
{
	va_list args;
	char *ptr, buf[1024];
	int len;

	va_start(args, fmt);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, fmt, args);
	va_end(args);

	partyline_write_chan_but(cid, pid, ptr, len);

	if (ptr != buf) free(ptr);
	return(0);
}

int partyline_printf_all_but(int pid, const char *fmt, ...)
{
	va_list args;
	char *ptr, buf[1024];
	int len;

	va_start(args, fmt);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, fmt, args);
	va_end(args);

	partyline_write_all_but(pid, ptr, len);

	if (ptr != buf) free(ptr);
	return(0);
}
#endif

/* Loggin	stuff. */
static int on_putlog(int flags, const char *chan, const char *text, int len)
{
	partychan_t *chanptr;

	chanptr = partychan_lookup_name(chan);
	if (!chanptr)
		return(0);

	partychan_msg(chanptr, NULL, text, len);
	return(0);
}

int partyline_idx_privmsg(int idx, partymember_t *dest, partymember_t *src, const char *text, int len)
{
	if (src) egg_iprintf(idx, "[%s] %s\r\n", src->nick, text);
	else egg_iprintf(idx, "%s\r\n", text);
	return 0;
}

int partyline_idx_nick(int idx, partymember_t *src, const char *oldnick, const char *newnick)
{
	egg_iprintf(idx, "* %s*** %s is now known as %s.\n", timer_get_timestamp(), oldnick, newnick);
	return 0;
}

int partyline_idx_quit(int idx, partymember_t *src, const char *text, int len)
{
	egg_iprintf(idx, "* %s*** %s (%s@%s) has quit: %s\n", timer_get_timestamp(), src->nick, src->ident, src->host, text);
	return 0;
}

int partyline_idx_chanmsg(int idx, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	char *ts;

	ts = timer_get_timestamp();

	if (src) egg_iprintf(idx, "%s %s<%s> %s\r\n", chan->name, ts, src->nick, text);
	else egg_iprintf(idx, "%s %s%s\r\n", chan->name, ts, text);
	return 0;
}

int partyline_idx_join(int idx, partychan_t *chan, partymember_t *src)
{
	egg_iprintf(idx, "%s %s*** %s (%s@%s) has joined the channel.\r\n", chan->name, timer_get_timestamp(), src->nick, src->ident, src->host);
	return 0;
}

int partyline_idx_part(int idx, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	egg_iprintf(idx, "%s %s*** (%s@%s) has left %s: %s\r\n", chan->name, timer_get_timestamp(), src->nick, src->ident, src->host, chan->name, text);
	return 0;
}

