/* ircmasks.c: functions for working with ircmask lists
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
static const char rcsid[] = "$Id: ircmasks.c,v 1.10 2005/03/03 18:44:47 stdarg Exp $";
#endif

#include <eggdrop/eggdrop.h>

int ircmask_list_clear(ircmask_list_t *list)
{
	ircmask_list_entry_t *entry, *next;

	if (list == NULL)
		return -1;

	for (entry = list->head; entry; ) {
		next = entry->next;

		free(entry->ircmask);
		free(entry);

		entry = next;
	}

	return (0);
}

int ircmask_list_add(ircmask_list_t *list, const char *ircmask, void *data)
{
	int i, stars, len;
	ircmask_list_entry_t *entry, *prev;

	stars = 0;
	len = strlen(ircmask);
	for (i = 0; i < len; i++) {
		if (ircmask[i] == '*') stars++;
	}
	len -= stars;
	prev = NULL;
	for (entry = list->head; entry; entry = entry->next) {
		if (len >= entry->len) break;
		prev = entry;
	}
	entry = malloc(sizeof(*entry));
	entry->ircmask = strdup(ircmask);
	entry->len = len;
	entry->data = data;
	if (prev) {
		entry->next = prev->next;
		prev->next = entry;
	}
	else {
		entry->next = list->head;
		list->head = entry;
	}
	return(0);
}

int ircmask_list_del(ircmask_list_t *list, const char *ircmask, void *data)
{
	ircmask_list_entry_t *entry, *prev;

	prev = NULL;
	for (entry = list->head; entry; entry = entry->next) {
		if (!strcasecmp(ircmask, entry->ircmask)) break;
		prev = entry;
	}
	if (entry) {
		if (prev) prev->next = entry->next;
		else list->head = entry->next;
		free(entry->ircmask);
		free(entry);
	}
	return(0);
}

int ircmask_list_find(ircmask_list_t *list, const char *irchost, void *dataptr)
{
	ircmask_list_entry_t *entry;

	for (entry = list->head; entry; entry = entry->next) {
		if (wild_match(entry->ircmask, irchost) > 0) {
			*(void **)dataptr = entry->data;
			return(0);
		}
	}
	*(void **)dataptr = NULL;
	return(-1);
}

/* Type corresponds to the mirc mask types. */
char *ircmask_create_separate(int type, const char *nick, const char *user, const char *host)
{
	char *mask;
	char ustar[2] = {0, 0};
	char *domain;

	if (type < 5) nick = "*";
	if (type == 2 || type == 7) user = "*";
	else if (type == 1 || type == 3 || type == 6 || type == 8) {
		while (*user && !isalnum(*user)) user++;
		ustar[0] = '*';
	}
	else {
		user = "";
		ustar[0] = '*';
	}

	domain = strdup(host);
	if (type == 3 || type == 4 || type == 8 || type == 9) {
		char *dot = strrchr(host, '.');
		char *colon = strrchr(host, ':');
		if (colon || (dot && isdigit(*(dot+1)))) {
			/* ipv6 or dotted decimal. */
			if (dot < colon) dot = colon;
			domain[host-dot+1] = '*';
			domain[host-dot+2] = 0;
		}
		else if (dot) {
			char *dot2;
			dot = strchr(host, '.');
			if ((dot2 = strchr(dot+1, '.'))) {
				if (strlen(dot2+1) > 2) {
					free(domain);
					domain = egg_mprintf("*%s", dot);
				}
			}
		}
	}
	mask = egg_mprintf("%s!%s%s@%s", nick, ustar, user, domain);
	free(domain);
	return(mask);
}

char *ircmask_create(int type, const char *nick, const char *uhost)
{
	char *user = strdup(uhost);
	char *at = strchr(uhost, '@');
	char *mask;

	if (at) {
		user[at-uhost] = 0;
		uhost = at+1;
	}
	else user[0] = 0;
	mask = ircmask_create_separate(type, nick, user, uhost);
	free(user);
	return(mask);
}

char *ircmask_create_full(int type, const char *nuhost)
{
	char *nick, *user, *host, *buf, *mask;

	buf = strdup(nuhost);
	nick = buf;
	user = strchr(nick, '!');
	if (user) *user++ = 0;
	else user = "";
	host = strchr(user, '@');
	if (host) *host++ = 0;
	else host = "";
	mask = ircmask_create_separate(type, nick, user, host);
	free(buf);
	return(mask);
}
