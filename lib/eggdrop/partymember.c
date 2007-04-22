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
static const char rcsid[] = "$Id: partymember.c,v 1.27 2007/04/22 13:18:32 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>

static partymember_t *party_head = NULL;
static partymember_t *local_party_head = NULL;
static int npartymembers = 0;

static int partymember_cleanup(void *client_data);
static int on_udelete(user_t *u);

static bind_list_t partymember_udelete_binds[] = {
	{NULL, NULL, on_udelete},
	{0}
};

static bind_table_t *BT_nick = NULL, *BT_new = NULL, *BT_quit = NULL;

int partymember_init(void)
{
	bind_add_list(BTN_USER_DELETE, partymember_udelete_binds);
	BT_nick = bind_table_add(BTN_PARTYLINE_NICK, 3, "Pss", MATCH_NONE, BIND_STACKABLE);
	BT_new = bind_table_add(BTN_PARTYLINE_NEW, 1, "P", MATCH_MASK, BIND_STACKABLE);
	BT_quit = bind_table_add(BTN_PARTYLINE_QUIT, 2, "Ps", MATCH_MASK, BIND_STACKABLE);
	return(0);
}

int partymember_shutdown(void)
{
	while (party_head) partymember_delete(party_head, NULL, _("Bot shutdown"));
	bind_rem_list(BTN_USER_DELETE, partymember_udelete_binds);
	bind_table_del(BT_nick);
	bind_table_del(BT_new);
	bind_table_del(BT_quit);

	/* force a garbage run since we might have some partymembers 
 	 * marked as deleted and w/o a garbage_run we may not destroy
	 * our hashtable */
	garbage_run();

	return (0);
}

static void partymember_really_delete(partymember_t *p)
{
	/* Get rid of it from the main list and the local list. */
	if (p->prev) p->prev->next = p->next;
	else party_head = p->next;
	if (p->next) p->next->prev = p->prev;

	if (p->prev_on_bot) p->prev_on_bot->next_on_bot = p->next_on_bot;
	else if (p->bot) p->bot->partys = p->next_on_bot;
	else local_party_head = p->next_on_bot;
	if (p->next_on_bot) p->next_on_bot->prev_on_bot = p->prev_on_bot;

	/* Free! */
	if (p->nick) free(p->nick);
	if (p->ident) free(p->ident);
	if (p->host) free(p->host);
	if (p->full_id_name) free(p->full_id_name);
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

partymember_t *partymember_get_head()
{
	return party_head;
}

static int partymember_get_id(botnet_bot_t *bot)
{
	int id;

	for (id = 1; partymember_lookup(NULL, bot, id); id++) {
		;
	}
	return(id);
}

partymember_t *partymember_new(int id, user_t *user, botnet_bot_t *bot, const char *nick, const char *ident, const char *host, partyline_event_t *handler, void *client_data, event_owner_t *owner)
{
	partymember_t *mem;

	mem = calloc(1, sizeof(*mem));
	if (id == -1) id = partymember_get_id(bot);
	mem->id = id;
	mem->user = user;
	mem->bot = bot;
	mem->nick = strdup(nick);
	mem->ident = strdup(ident);
	mem->host = strdup(host);
	mem->full_id_name = egg_mprintf("%d:%s@%s", id, nick, bot ? bot->name : botnet_get_name());
	mem->full_name = strchr(mem->full_id_name, ':') + 1;
	mem->common_name = bot ? mem->full_name : mem->nick;
	mem->handler = handler;
	mem->client_data = client_data;
	mem->owner = owner;

	mem->next = party_head;
	if (party_head) party_head->prev = mem;
	party_head = mem;

	mem->prev_on_bot = NULL;
	if (bot) {
		mem->next_on_bot = bot->partys;
		if (bot->partys) bot->partys->prev_on_bot = mem;
		bot->partys = mem;
	} else {
		mem->next_on_bot = local_party_head;
		if (local_party_head) local_party_head->prev_on_bot = mem;
		local_party_head = mem;
	}

	npartymembers++;
	bind_check(BT_new, NULL, nick, mem);
	return(mem);
}

/*!
 * \brief Deletes all partymembers with a given owner.
 *
 * Calls partymember_delete() for every partymember with a given owner.
 *
 * \param module The module whose partymembers should be deleted.
 * \param script The script whose partymembers should be deleted. NULL matches everything.
 *
 * \return The number of deleted partymembers.
 */

int partymember_delete_by_owner(struct egg_module *module, void *script)
{
	int ret = 0;
	partymember_t *p;

	for (p = party_head; p; p = p->next) {
		if (p->flags & PARTY_DELETED) continue;
		if (p->owner && p->owner->module == module && (!script || p->owner->client_data == script)) {
			partymember_delete(p, NULL, "Module unloaded");
			++ret;
		}
	}

	return ret;
}

int partymember_delete(partymember_t *p, const botnet_bot_t *lost_bot, const char *text)
{
	int i, len;
	partymember_common_t *common;
	partymember_t *mem;

	if (p->flags & PARTY_DELETED) return(-1);

	len = strlen(text);

	if (!lost_bot && p->nchannels) botnet_member_quit(p, text, len);

	bind_check(BT_quit, NULL, p->nick, p, text);

	/* Mark it as deleted so it doesn't get reused before it's free. */
	p->flags |= PARTY_DELETED;
	garbage_add(partymember_cleanup, NULL, GARBAGE_ONCE);
	npartymembers--;

	common = partychan_get_common(p);
	if (common) {
		for (i = 0; i < common->len; i++) {
			mem = common->members[i];
			if (mem->flags & PARTY_DELETED) continue;
			if (mem->handler->on_quit) (mem->handler->on_quit)(mem->client_data, p, lost_bot, text, len);
		}
		partychan_free_common(common);
	}

	/* Remove from any stray channels. */
	for (i = 0; i < p->nchannels; i++) {
		partychan_part(p->channels[i], p, text);
	}
	p->nchannels = 0;

	if (p->handler && p->handler->on_quit) {
		(p->handler->on_quit)(p->client_data, p, lost_bot, text, strlen(text));
	}
	if (p->owner && p->owner->on_delete) p->owner->on_delete(p->owner, p->client_data);
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
	botnet_set_nick(p, oldnick);
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
	bind_check(BT_nick, NULL, NULL, p, oldnick, p->nick);
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

int partymember_who(int **ids, int *len)
{
	int i;
	partymember_t *p;

	*ids = malloc(sizeof(int) * (npartymembers+1));
	i = 0;
	for (p = local_party_head; p; p = p->next_on_bot) {
		if (p->flags & PARTY_DELETED) continue;
		(*ids)[i] = p->id;
		i++;
	}
	(*ids)[i] = 0;
	*len = i;
	return(0);
}

partymember_t *partymember_lookup(const char *full_name, botnet_bot_t *bot, int id)
{
	int i = id;
	char *name = NULL;
	const char *ptr, *nick = full_name;
	botnet_bot_t *b = bot;
	partymember_t *p;

	if (!full_name && id == -1) return NULL;

	if (full_name) {
		ptr = strchr(full_name, '@');
		if (ptr) {
			bot = botnet_lookup(ptr + 1);
			if (!bot && strcmp(ptr + 1, botnet_get_name())) return NULL;
			if (b && bot != b) return NULL;
			name = strdup(full_name);
			name[ptr - full_name] = 0;
			nick = name;
		}

		ptr = strchr(nick, ':');
		if (ptr) {
			id = atoi(nick);
			if (i != -1 && i != id) {
				if (name) free(name);
				return NULL;
			}
			nick = ptr + 1;
		}
	}

	for (p = bot ? bot->partys : local_party_head; p; p = p->next_on_bot) {
		if (p->flags & PARTY_DELETED) continue;
		if (id != -1) {
			if (p->id == id) break;
		} else {
			if (!strcmp(p->nick, nick)) break;
		}
	}

	if (name) free(name);
	if (p && id != -1 && nick && strcmp(p->nick, nick)) return NULL;
	return p;
}

/*int partymember_write_id(int id, const char *text, int len)
{
	partymember_t *p;

	p = partymember_lookup_pid(pid);
	return partymember_msg(p, NULL, text, len);
}*/

int partymember_write(partymember_t *p, const char *text, int len)
{
	return partymember_msg(p, NULL, text, len);
}

int partymember_msg(partymember_t *p, botnet_entity_t *src, const char *text, int len)
{
	if (!p || p->flags & PARTY_DELETED) return(-1);

	if (len < 0) len = strlen(text);
	if (p->handler && p->handler->on_privmsg)
		p->handler->on_privmsg(p->client_data, p, src, text, len);
	else if (p->bot && p->bot->direction->handler && p->bot->direction->handler->on_privmsg)
		p->bot->direction->handler->on_privmsg(p->bot->direction->client_data, src, p, text, len);
	return 0;
}

int partymember_local_broadcast(botnet_entity_t *src, const char *text, int len)
{
	partymember_t *p;

	if (len < 0) len = strlen(text);
	for (p = local_party_head; p; p = p->next_on_bot) {
		if (p->flags & PARTY_DELETED) continue;
		partymember_msg(p, src, text, len);
	}
	return 0;
}

/*int partymember_printf_id(int id, const char *fmt, ...)
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
}*/

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

int partymember_msgf(partymember_t *p, botnet_entity_t *src, const char *fmt, ...)
{
	va_list args;
	char *ptr, buf[1024];
	int len;

	va_start(args, fmt);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, fmt, args);
	va_end(args);

	partymember_msg(p, src, ptr, len);

	if (ptr != buf) free(ptr);
	return(0);
}

static int on_udelete(user_t *u)
{
	partymember_t *p;

	for (p = local_party_head; p; p = p->next_on_bot) {
		if (p->user == u) partymember_delete(p, NULL, "User deleted!");
	}
	return(0);
}

/*!
 * \brief Counts the number of users on a bot.
 *
 * Counts the number of users on any channels on a bot.
 *
 * \param bot The bot the users are on or NULL for the number of users on \a this bot.
 *
 * \return The number of users.
 */

int partymember_count_by_bot(const struct botnet_bot *bot)
{
	int ret = 0;
	partymember_t *p;

	for (p = bot ? bot->partys : local_party_head; p; p = p->next_on_bot) {
		if (p->flags & PARTY_DELETED) continue;
		++ret;
	}
	return ret;
}

/*!
 * \brief Deletes all users on a bot.
 *
 * Deletes all partymembers on a certain bot.
 *
 * \param bot The bot the users are on or NULL for all users on \a this bot.
 * \param lost_bot The bot that was really unlinked to cause this.
 * \param reason Some kind of explanation why it happened.
 *
 * \return The number of users deleted.
 */

int partymember_delete_by_bot(const botnet_bot_t *bot, const botnet_bot_t *lost_bot, const char *reason)
{
	int ret = 0;
	partymember_t *p;

	for (p = bot ? bot->partys : local_party_head; p; p = p->next_on_bot) {
		if (p->flags & PARTY_DELETED) continue;
		partymember_delete(p, lost_bot, reason);
		++ret;
	}
	return ret;
}
