/* partymembers.c: partyline members
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
static const char rcsid[] = "$Id: partymember.c,v 1.19 2004/10/17 05:14:06 stdarg Exp $";
#endif

#include <eggdrop/eggdrop.h>

/* When does pid wrap around? This lets pids get up to 99999. */
#define PID_WRAPAROUND 100000

static hash_table_t *pid_ht = NULL;
static partymember_t *party_head = NULL;
static int npartymembers = 0;

static int partymember_cleanup(void *client_data);
static int on_udelete(user_t *u);

static bind_list_t partymember_udelete_binds[] = {
	{NULL, NULL, on_udelete},
	{0}
};

static bind_table_t *BT_nick = NULL;

int partymember_init(void)
{
	pid_ht = hash_table_create(NULL, NULL, 13, HASH_TABLE_INTS);
	bind_add_list(BTN_USER_DELETE, partymember_udelete_binds);
	BT_nick = bind_table_add(BTN_PARTYLINE_NICK, 3, "Pss", MATCH_NONE, BIND_STACKABLE);
	return(0);
}

int partymember_shutdown(void)
{
	bind_rem_list(BTN_USER_DELETE, partymember_udelete_binds);
	bind_table_del(BT_nick);

	/* force a garbage run since we might have some partymembers 
 	 * marked as deleted and w/o a garbage_run we may not destroy
	 * our hashtable */
	garbage_run();

	hash_table_delete(pid_ht);
	pid_ht = NULL;
	return (0);
}

static void partymember_really_delete(partymember_t *p)
{
	/* Get rid of it from the main list and the hash table. */
	if (p->prev) p->prev->next = p->next;
	else party_head = p->next;
	if (p->next) p->next->prev = p->prev;
	hash_table_remove(pid_ht, (void *)p->pid, NULL);

	/* Free! */
	if (p->nick) free(p->nick);
	if (p->ident) free(p->ident);
	if (p->host) free(p->host);
	if (p->channels) free(p->channels);

	/* Zero it out in case anybody has kept a reference (bad!). */
	memset(p, 0, sizeof(*p));

	free(p);
}

static int partymember_cleanup(void *client_data)
{
	partymember_t *p, *next;

	for (p = party_head; p; p = next) {
		next = p->next;
		if (p->flags & PARTY_DELETED) partymember_really_delete(p);
	}
	return(0);
}

partymember_t *partymember_lookup_pid(int pid)
{
	partymember_t *p = NULL;

	hash_table_find(pid_ht, (void *)pid, &p);
	if (p && p->flags & PARTY_DELETED) p = NULL;
	return(p);
}

int partymember_get_pid()
{
	int pid;

	for (pid = 1; partymember_lookup_pid(pid); pid++) {
		;
	}
	return(pid);
#if 0
	while (partymember_lookup_pid(g_pid)) {
		g_pid++;
		if (g_pid >= PID_WRAPAROUND) g_pid = 0;
	}
	pid = g_pid++;
	if (g_pid >= PID_WRAPAROUND) g_pid = 0;
	return(pid);
#endif
}

partymember_t *partymember_new(int pid, user_t *user, const char *nick, const char *ident, const char *host, partyline_event_t *handler, void *client_data)
{
	partymember_t *mem;

	mem = calloc(1, sizeof(*mem));
	if (pid == -1) pid = partymember_get_pid();
	mem->pid = pid;
	mem->user = user;
	mem->nick = strdup(nick);
	mem->ident = strdup(ident);
	mem->host = strdup(host);
	mem->handler = handler;
	mem->client_data = client_data;
	mem->next = party_head;
	if (party_head) party_head->prev = mem;
	party_head = mem;
	hash_table_insert(pid_ht, (void *)pid, mem);
	npartymembers++;
	return(mem);
}

int partymember_delete(partymember_t *p, const char *text)
{
	int i, len;
	partymember_common_t *common;
	partymember_t *mem;

	if (p->flags & PARTY_DELETED) return(-1);

	/* Mark it as deleted so it doesn't get reused before it's free. */
	p->flags |= PARTY_DELETED;
	garbage_add(partymember_cleanup, NULL, GARBAGE_ONCE);
	npartymembers--;

	len = strlen(text);

	common = partychan_get_common(p);
	if (common) {
		for (i = 0; i < common->len; i++) {
			mem = common->members[i];
			if (mem->flags & PARTY_DELETED) continue;
			if (mem->handler->on_quit) (mem->handler->on_quit)(mem->client_data, p, text, len);
		}
		partychan_free_common(common);
	}

	/* Remove from any stray channels. */
	for (i = 0; i < p->nchannels; i++) {
		partychan_part(p->channels[i], p, text);
	}
	p->nchannels = 0;

	if (p->handler && p->handler->on_quit) {
		(p->handler->on_quit)(p->client_data, p, text, strlen(text));
	}
	return(0);
}

int partymember_set_nick(partymember_t *p, const char *nick)
{
	partymember_common_t *common;
	partymember_t *mem;
	char *oldnick;
	int i;

	oldnick = p->nick;
	p->nick = strdup(nick);
	if (p->handler && p->handler->on_nick)
		(p->handler->on_nick)(p->client_data, p, oldnick, nick);
	p->flags |= PARTY_SELECTED;
	common = partychan_get_common(p);
	if (common) {
		for (i = 0; i < common->len; i++) {
			mem = common->members[i];
			if (mem->flags & PARTY_DELETED) continue;
			if (mem->handler->on_nick) (mem->handler->on_nick)(mem->client_data, p, oldnick, nick);
		}
		partychan_free_common(common);
	}
	bind_check(BT_nick, &p->user->settings[0].flags, NULL, p, oldnick, p->nick);
	if (oldnick) free(oldnick);
	p->flags &= ~PARTY_SELECTED;
	return(0);
}

int partymember_update_info(partymember_t *p, const char *ident, const char *host)
{
	if (!p) return(-1);
	if (ident) str_redup(&p->ident, ident);
	if (host) str_redup(&p->host, host);
	return(0);
}

int partymember_who(int **pids, int *len)
{
	int i;
	partymember_t *p;

	*pids = malloc(sizeof(int) * (npartymembers+1));
	*len = npartymembers;
	i = 0;
	for (p = party_head; p; p = p->next) {
		if (p->flags & PARTY_DELETED) continue;
		(*pids)[i] = p->pid;
		i++;
	}
	(*pids)[i] = 0;
	return(0);
}

partymember_t *partymember_lookup_nick(const char *nick)
{
	partymember_t *p;

	for (p = party_head; p; p = p->next) {
		if (p->flags & PARTY_DELETED) continue;
		if (!strcmp(p->nick, nick)) break;
	}
	return(p);
}

int partymember_write_pid(int pid, const char *text, int len)
{
	partymember_t *p;

	p = partymember_lookup_pid(pid);
	return partymember_msg(p, NULL, text, len);
}

int partymember_write(partymember_t *p, const char *text, int len)
{
	return partymember_msg(p, NULL, text, len);
}

int partymember_msg(partymember_t *p, partymember_t *src, const char *text, int len)
{
	if (!p || p->flags & PARTY_DELETED) return(-1);

	if (len < 0) len = strlen(text);
	if (p->handler && p->handler->on_privmsg)
		(p->handler->on_privmsg)(p->client_data, p, src, text, len);
	return(0);
}

int partymember_printf_pid(int pid, const char *fmt, ...)
{
	va_list args;
	char *ptr, buf[1024];
	int len;

	va_start(args, fmt);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, fmt, args);
	va_end(args);

	partymember_write_pid(pid, ptr, len);

	if (ptr != buf) free(ptr);
	return(0);
}

int partymember_printf(partymember_t *p, const char *fmt, ...)
{
	va_list args;
	char *ptr, buf[1024];
	int len;

	va_start(args, fmt);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, fmt, args);
	va_end(args);

	partymember_write(p, ptr, len);

	if (ptr != buf) free(ptr);
	return(0);
}

static int on_udelete(user_t *u)
{
	partymember_t *p;

	for (p = party_head; p; p = p->next) {
		if (p->user == u) partymember_delete(p, "User deleted!");
	}
	return(0);
}
