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
static const char rcsid[] = "$Id: ircmasks.c,v 1.14 2005/05/08 04:40:12 stdarg Exp $";
#endif

#include <eggdrop/eggdrop.h>

static int compute_hash(const char *str, int *hash1, int *hash2);

int ircmask_list_clear(ircmask_list_t *list)
{
	ircmask_list_entry_t *entry, *next;

	if (list == NULL) return -1;

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
	compute_hash(ircmask, &entry->hash1, &entry->hash2);
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
	int hash1, hash2;
	// ################ see if this is effective... remove later
	static int searches = 0, stops = 0, misses = 0;

	compute_hash(irchost, &hash1, &hash2);
	for (entry = list->head; entry; entry = entry->next) {
		searches++;
		if (searches % 1000 == 0) putlog(LOG_MISC, "*", "ircmask_list_find: %d successful stops, %d false hits, %d total searches... efficiency = %0.1f%%", stops, misses, searches, 100.0*(float)(searches-misses)/(float)searches);
		if ((entry->hash1 & hash1) != entry->hash1 || (entry->hash2 & hash2) != entry->hash2) {
			stops++;
			continue;
		}
		if (wild_match(entry->ircmask, irchost) > 0) {
			*(void **)dataptr = entry->data;
			return(0);
		}
		misses++;
	}
	*(void **)dataptr = NULL;
	return(-1);
}

/* Type corresponds to the mIRC mask types. */
char *ircmask_create_separate(int type, const char *nick, const char *user, const char *host)
{
	char *mask;
	char ustar[2] = {0, 0};
	char *domain;
/*
	int replace_numbers = 0;

	if (type >= 10) {
		replace_numbers = 1;
		type -= 10;
	}
*/
	if (type < 5) nick = "*";
	if (type == 2 || type == 4 || type == 7 || type == 9) user = "*";
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
	/* replaces numbers with '?' 
	if (replace_numbers) {
		char *c = NULL;

		for (c = mask; *c; ++c)
			if (isdigit((unsigned char) *c)) *c = '?';
	}
	*/
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

static int compute_hash(const char *str, int *hash1, int *hash2)
{
	unsigned int pair;
	int last = 0;

	*hash1 = *hash2 = 0;
	for (; *str; str++) {
		if (*str == '*' || *str == '?') {
			last = -1;
			continue;
		}
		if (last != -1) {
			pair = (last * 71317 + ((unsigned)tolower(*str)) * 1937) % 64;
			if (pair > 31) *hash2 |= 1 << (pair-31);
			else *hash1 |= 1 << pair;
		}
		last = (unsigned) tolower(*str);
	}
	if (last > 0) {
		pair = (last * 3133719) % 64;
		if (pair > 31) *hash2 |= 1 << (pair-31);
		else *hash1 |= 1 << pair;
	}
	return(0);
}
