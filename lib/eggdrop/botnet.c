/* botnet.c: botnet stuff
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

/*!
 * \file
 *
 * \brief Contains all botnet related stuff.
 *
 * This file contains all functions dealing with other bots on the botnet.
 *
 * \todo A lot ... And then to write a module that actually uses this -_-
 */

#ifndef lint
static const char rcsid[] = "$Id: botnet.c,v 1.6 2007/04/14 15:21:11 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>

#define BAD_BOTNAME_CHARS " *?@:"   //!< A string of characters that aren't allowed in a botname.

#define BOT_DELETED 1

static char *botname;
static char *default_botname = "eggdrop";

static hash_table_t *bot_ht = NULL;
static botnet_bot_t *bot_head, *localbot_head;
static int nbots = 0;

static int botnet_cleanup(void *client_data);
static void botnet_really_delete(botnet_bot_t *bot);
static int botnet_clear_all(void);

//static int partymember_cleanup(void *client_data);
//static int on_udelete(user_t *u);

//static bind_list_t partymember_udelete_binds[] = {
//	{NULL, NULL, on_udelete},
//	{0}
//};

/*!
 * \bind
 * This bind is triggered every time the bot wants to link to another bot for
 * some reason.
 * \name request-link
 * \flags Ignored.
 * \match The bot's type.
 * \param user The user record of the bot to be linked.
 * \param string The type of the bot. (Same as the match.)
 * \breakable
 * \return A function should return ::BIND_RET_BREAK if it tried to establish a
 *         link or 0 otherwise.
 */

static bind_table_t *BT_request_link;

/*!
 * \bind
 * This bind is triggered every time a new bot is somehow linked to the botnet.
 * There is no way to determine if it happened because it was directly linked
 * or because another bot was linked and introduced it.
 * \name link
 * \flags Ignored.
 * \match The new bot's name.
 * \param bot The new bot.
 * \stackable
 * \noreturn
 */

static bind_table_t *BT_link;

/*!
 * \bind
 * This bind is triggered every time a bot is lost from the botnet. If a bot
 * gets disconnected this bind is triggered for all bots behind the lost one
 * first, beginning with the most distant bots. That means that for some bots
 * this bind is triggered for, the uplink still exists in this bots database
 * even though it has already been lost.
 * \name disc
 * \flags Ignored.
 * \match The lost bot's name.
 * \param bot The lost bot.
 * \param bot The bot that really got disconnected and caused the bots behind it to get lost.
 * \param string Some kind of reason for the disconnect.
 * \stackable
 * \noreturn
 */

static bind_table_t *BT_disc;

/*!
 * \bind
 * Triggered by a message coming from another bot in the botnet. The first
 * word is the command and the rest becomes the text argument.
 * \name bot
 * \flags Ignored.
 * \match The command.
 * \param bot The bot the message is from.
 * \param string The command.
 * \param string The rest of the line, possibly nothing.
 * \stackable
 * \noreturn
 */

static bind_table_t *BT_bot;

/*!
 * \brief Inits the whole botnet stuff
 *
 * \returns Always 0
 */

int botnet_init()
{
	nbots = 0;
	bot_head = NULL;
	localbot_head = NULL;

	botname = default_botname;
	bot_ht = hash_table_create(NULL, NULL, 13, HASH_TABLE_STRINGS);

	BT_request_link = bind_table_add(BTN_BOTNET_REQUEST_LINK, 2, "Us", 0, BIND_BREAKABLE);
	BT_link = bind_table_add(BTN_BOTNET_LINK, 1, "B", 0, BIND_STACKABLE);
	BT_disc = bind_table_add(BTN_BOTNET_DISC, 3, "BBs", 0, BIND_STACKABLE);
	BT_bot = bind_table_add(BTN_BOTNET_BOT, 2, "Bss", 0, 0);
	return 0;
}

/*!
 * \brief Shuts the whole botnet stuff down
 *
 * Frees all memory, deletes the binds, ...
 *
 * \returns Always 0
 */

int botnet_shutdown(void)
{
	int leaked;

	/* Since at this point no scripts or modules should be loaded,
	 * there should be no botnet. But just to make sure ... */

	leaked = botnet_clear_all();
	if (leaked) putlog(LOG_MISC, "*", "Warning: Some botnet module leaked %d bots. Fix this!", leaked);

	bind_table_del(BT_request_link);
	bind_table_del(BT_link);
	bind_table_del(BT_disc);

	/* force a garbage run since we might have some partymembers 
 	 * marked as deleted and w/o a garbage_run we may not destroy
	 * our hashtable */
	garbage_run();

	hash_table_delete(bot_ht);
	bot_ht = NULL;

	if (botname != default_botname) free(botname);
	return 0;
}

/*!
 * \brief Set the botname of this bot.
 *
 * This will change the botname of this bot on the fly. It will fail and
 * report an error if there are any bots connected to the botnet or if
 * someone's on the partyline.
 *
 * \param name The new botname.
 *
 * \return 0 on success or -1 on error.
 */

int botnet_set_name(const char *name)
{
	if (bot_head || localbot_head || nbots || partymember_get_head()) return -1;
	if (botname != default_botname) free(botname);
	botname = strdup(name);
	return 0;
}

/*!
 * \brief Returns the bots name on the botnet.
 *
 * \return This bot's name.
 */

const char *botnet_get_name()
{
	return botname;
}

/*!
 * \brief Returns the first bot in the linked list.
 *
 * \return The first bot on the botnet.
 *
 * \todo There has to be a better way to do whatever needs to be done than
 *       messing with this list.
 */

const botnet_bot_t *botnet_get_head()
{
	return bot_head;
}

/*!
 * \brief Find a bot by its name
 *
 * \param name The name of the bot to find
 *
 * \return The bot or NULL if no such bot exists
 */

botnet_bot_t *botnet_lookup(const char *name)
{
	botnet_bot_t *bot = NULL;

	hash_table_find(bot_ht, name, &bot);
	if (bot && bot->flags & BOT_DELETED) return NULL;
	return bot;
}

/*!
 * \brief Adds a bot to the botnet
 *
 * Calling this function will add a bot to the net, inform the rest of the
 * net about it, log it and trigger the link bind.
 *
 * \param name The name of the bot.
 * \param user The user entry for this bot. NULL if it's not local.
 * \param uplink The uplink of this bot. NULL if it's local.
 * \param direction The bot we have to talk to to reach this bot. NULL for local bots, it'll be set to the bot itself.
 * \param handler Handler struct containing the functions to call to get stuff done. NULL if it's not local.
 * \param client_data Some data passed back to the callback functions.
 * \param owner Owner of this bot and it's callback functions.
 * \param netburst True if it's part of the syncing between two other linked bots, false if it's a real new link.
 *
 * \return A pointer to the new bot on success, NULL on error.
 *
 * \todo Find a better way to log this. But for now ::LOG_MISC is good enough.
 */

botnet_bot_t *botnet_new(const char *name, user_t *user, botnet_bot_t *uplink, botnet_bot_t *direction, botnet_handler_t *handler, void *client_data, event_owner_t *owner, int netburst)
{
	const char *temp;
	botnet_bot_t *bot, *tmp;

	if (!uplink && (!user || !handler)) return 0;
	if ((uplink && !direction) || (!uplink && direction)) return 0;
	if (botnet_lookup(name)) return 0;
	if (!strcmp(botname, name)) return 0;
	for (temp = name; *temp; ++temp) if (*temp < 32 || strchr(BAD_BOTNAME_CHARS, *temp)) return 0;

	bot = malloc(sizeof(*bot));
	if (!direction) direction = bot;

	bot->name = strdup(name);
	bot->flags = 0;
	bot->user = user;
	bot->uplink = uplink;
	bot->direction = direction;
	bot->partys = NULL;
	bot->handler = handler;
	bot->client_data = client_data;
	bot->owner = owner;

	hash_table_insert(bot_ht, bot->name, bot);
	
	bot->prev = 0;
	bot->next = bot_head;
	if (bot_head) bot_head->prev = bot;
	bot_head = bot;

	bot->prev_local = 0;
	if (!uplink) {
		bot->next_local = localbot_head;
		if (localbot_head) localbot_head->prev_local = bot;
		localbot_head = bot;
	} else {
		bot->next_local = 0;
	}

	++nbots;

	for (tmp = localbot_head; tmp; tmp = tmp->next_local) {
		if (tmp->flags & BOT_DELETED || tmp == bot->direction) continue;
		if (tmp->handler && tmp->handler->on_new_bot) tmp->handler->on_new_bot(tmp->client_data, bot, netburst);
	}

	bind_check(BT_link, NULL, bot->name, bot);

	if (!netburst) {
		if (!bot->uplink) putlog(LOG_MISC, "*", "Linked to %s.", bot->name);
		else putlog(LOG_MISC, "*", "(%s) Linked to %s.", bot->uplink->name, bot->name);
	}

	return bot;
}

/*!
 * \brief Deletes all bots.
 *
 * Deletes all bots without calling any binds or callbacks (except for on_delete).
 *
 * \return The number of killed bots.
 *
 * \warning Should probably never be called except during shutdowns and should
 *          never \e ever return a non-zero value.
 *
 * \todo Implement this!
 */

static int botnet_clear_all()
{
	int ret = 0;
	return ret;
}

/*!
 * \brief To request unlinking a bot.
 *
 * This function \a requests the unlinking of a bot on the botnet. If the bot
 * to unlink is not directly linked to this bot, the request is just sent on
 * in the bots direction.
 *
 * If the bot is directly linked, the link is broken and the unlinked event
 * is sent on in all directions.
 *
 * \param from Who did it.
 * \param bot The bot to get rid of.
 * \param reason Some kind of explanation.
 *
 * \return 0 on success, -1 if the bot's uplink doesn't have an on_unlink
 *         handler, -2 if we don't want to unlink.
 *
 * \todo Do some kind of check if we really want the unlink.
 */

int botnet_unlink(botnet_entity_t *from, botnet_bot_t *bot, const char *reason)
{
	char obuf[512];

	if (bot->direction != bot) {
		if (!bot->direction->handler || !bot->direction->handler->on_unlink) return -1;
		bot->direction->handler->on_unlink(bot->direction->client_data, from, bot, reason);
		return 0;
	}

	snprintf(obuf, sizeof(obuf), "%s (%s)", reason, from->full_name);

	if (bot->handler && bot->handler->on_lost_bot) bot->handler->on_lost_bot(bot->client_data, bot, obuf);
	if (from->what == ENTITY_PARTYMEMBER) partymember_msgf(from->user, 0, _("Unlinked from %s."), bot->name);
	botnet_delete(bot, reason);

	return 0;
}

/*!
 * \brief Completely delete a bot
 *
 * Unlike botnet_delete() this function doesn't call on_delete handlers
 * or trigger binds. It simply removes the bot from the hash table,
 * unlinks it from the ::bot_head and ::localbot_head lists and frees
 * the allocated memory.
 *
 * \param bot The bot to delete
 *
 * \warning Should never be called on a bot that hasn't been deleted
 *          by botnet_delete() earlier.
 */

static void botnet_really_delete(botnet_bot_t *bot)
{
	if (!(bot->flags & BOT_DELETED)) --nbots;

	if (bot->prev) bot->prev->next = bot->next;
	else bot_head = bot->next;
	if (bot->next) bot->next->prev = bot->prev;

	if (!bot->uplink) {
		if (bot->prev_local) bot->prev_local->next_local = bot->next_local;
		else localbot_head = bot->next_local;
		if (bot->next_local) bot->next_local->prev_local = bot->prev_local;
	}
	hash_table_remove(bot_ht, bot->name, NULL);

	free(bot->name);
	free(bot);
}

/*!
 * \brief Deletes a subtree of the botnet.
 *
 * This function deletes a bot, all partymembers on this bot, all bots linked
 * via this bot and all partymembers on any of these bots.
 *
 * \param bot The bot to delete.
 * \param root The bot that was originally unlinked. Should ne the same as
 *             \a bot when it's originally called from another function.
 * \param reason Some text explaining the How and Why of the unlink.
 */

static void botnet_recursive_delete(botnet_bot_t *bot, botnet_bot_t *root, const char *reason)
{
	botnet_bot_t *tmp;

	if (bot->flags & BOT_DELETED) return;

	/* First delete all bots linked via this bot */
	for (tmp = bot_head; tmp; tmp = tmp->next) {
		if (tmp->flags & BOT_DELETED) continue;
		if (tmp->uplink == bot) botnet_recursive_delete(tmp, root, reason);
	}

	/* Then delete all partymembers on this bot */
	partymember_delete_by_bot(bot, root, reason);

	/* And finally this bot */
	bind_check(BT_disc, NULL, bot->name, bot, root, reason);
	bot->flags |= BOT_DELETED;
	if (bot->owner && bot->owner->on_delete) bot->owner->on_delete(bot->owner, bot->client_data);
	--nbots;
}

/*!
 * \brief Count the number of bots and partymembers in a subtree.
 *
 * Counts bots and partymembers on and behind a certain bot in the current botnet.
 *
 * \param bot The bot to start from. Must not be NULL.
 * \param bots A pointer to the variable to contain the number of bots. May be NULL.
 * \param members A pointer to the variable to contain the number of partymembers. May be NULL.
 *
 * \warning This function never initializes the parameters \a bots and \a members!
 *          Do it yourself before every call unless you want to add up more subtrees!
 */

void botnet_count_subtree(botnet_bot_t *bot, int *bots, int *members)
{
	int m;
	botnet_bot_t *tmp;

	for (tmp = bot_head; tmp; tmp = tmp->next) {
		if (tmp->flags & BOT_DELETED) continue;
		if (tmp->uplink == bot) botnet_count_subtree(tmp, bots, members);
	}

	m = partymember_count_by_bot(bot);
	if (bots) ++*bots;
	if (members) *members += m;
}

/*!
 * \brief Deletes a bot.
 *
 * Marks a bot as deleted, clears out anything linked via this bot, triggers
 * the binds, logs it and sends it out to the botnet.
 *
 * \param bot The bot to delete.
 * \param reason Some text explaining the Hows and Whys of the unlink.
 *
 * \return 0 on success, -1 on failure.
 */

int botnet_delete(botnet_bot_t *bot, const char *reason)
{
	char obuf[512];
	const botnet_bot_t *tmp;

	if (bot->flags & BOT_DELETED) return -1;

	if (!bot->uplink) {
		int bots = 0, users = 0;
		botnet_count_subtree(bot, &bots, &users);
		snprintf(obuf, sizeof(obuf), "Unlinked from: %s (%s) (lost %d bots and %d users)", bot->name, reason, bots, users);
		reason = obuf;
	}

	for (tmp = localbot_head; tmp; tmp = tmp->next_local) {
		if (tmp->flags & BOT_DELETED || tmp == bot->direction) continue;
		if (tmp->handler && tmp->handler->on_lost_bot) tmp->handler->on_lost_bot(tmp->client_data, bot, reason);
	}

	botnet_recursive_delete(bot, bot, reason);

	if (!bot->uplink) putlog(LOG_MISC, "*", "%s", reason);
	else putlog(LOG_MISC, "*", "(%s) %s", bot->uplink->name, reason);

	garbage_add(botnet_cleanup, NULL, GARBAGE_ONCE);

	return 0;
}

/*!
 * \brief Deletes all bots with a given owner.
 *
 * Calls botnet_delete() for every bot with a given owner. In the first search
 * only the list of local bots is searched. After that the whole list is searched
 * but it'd be a really wyrd botnet if a match was found there.
 *
 * \param module The module whose bots should be deleted.
 * \param script The script whose bots should be deleted. NULL matches everything.
 *
 * \return The number of deleted bots. (Not counting recursive deletes.)
 */

int botnet_delete_by_owner(struct egg_module *module, void *script)
{
	int ret = 0;
	botnet_bot_t *tmp;

	for (tmp = localbot_head; tmp; tmp = tmp->next_local) {
		if (tmp->flags & BOT_DELETED) continue;
		if (tmp->owner && tmp->owner->module == module && (!script || tmp->owner->client_data == script)) {
			++ret;
			botnet_delete(tmp, _("Module unloaded"));
		}
	}

	for (tmp = bot_head; tmp; tmp = tmp->next_local) {
		if (tmp->flags & BOT_DELETED) continue;
		if (tmp->owner && tmp->owner->module == module && (!script || tmp->owner->client_data == script)) {
			++ret;
			botnet_delete(tmp, _("Module unloaded"));
		}
	}

	return ret;
}

/*!
 * \brief For internal use by botnet_replay_net().
 *
 * \warning Don't call this unless you're botnet_replay_net().
 */

static void botnet_recursive_replay_net(botnet_bot_t *dst, botnet_bot_t *start, partymember_t *members)
{
	int i;
	botnet_bot_t *tmp;
	partymember_t *p;

	if (start && dst->handler->on_new_bot) dst->handler->on_new_bot(dst->client_data, start, 1);
	
	if (dst->handler->on_join) {
		for (p = members; p; p = p->next) {
			if (p->flags & PARTY_DELETED) continue;
			if (p->bot != start) continue;
			for (i = 0; i < p->nchannels; ++i) {
				dst->handler->on_join(dst->client_data, p->channels[i], p, 1);
			}
		}
	}

	for (tmp = bot_head; tmp; tmp = tmp->next) {
		if (tmp->flags & BOT_DELETED || tmp == dst) continue;
		if (tmp->uplink == start) botnet_recursive_replay_net(dst, tmp, members);
	}
}

/*!
 * \brief Sends a replay of the current botnet to a bot.
 *
 * This function calls the botnet_handler_t::on_new_bot and botnet_handler_t::on_join
 * callbacks of a bot for every bot and partymember on the botnet. The callbacks
 * are always called with the \a linking parameter true.
 *
 * \param dst The bot to send it to.
 */

void botnet_replay_net(botnet_bot_t *dst)
{
	botnet_recursive_replay_net(dst, NULL, partymember_get_head());
}

int botnet_check_direction(botnet_bot_t *direction, botnet_bot_t *src)
{
	if (!src || src->direction != direction) {
		putlog(LOG_MISC, "*", _("Fake message from %s rejected! (direction != %s)."), direction->name, src ? src->name : "non-existant bot");
		return 1;
	}
	return 0;
}

/*!
 * \brief Deletes all bots marked as deleted
 *
 * \param client_data Ignored.
 *
 * \return Always 0.
 *
 * \todo Return something useful or change it to void.
 */

static int botnet_cleanup(void *client_data)
{
	botnet_bot_t *bot, *next;

	for (bot = bot_head; bot; bot = next) {
		next = bot->next;
		if (bot->flags & PARTY_DELETED) botnet_really_delete(bot);
	}
	return 0;
}

int botnet_link(user_t *user)
{
	char *type;

	if (!user_check_flags_str(user, NULL, "b")) return -1;
	if (user_get_setting(user, NULL, "bot-type", &type)) return -2;

	if (!bind_check(BT_request_link, NULL, type, user, type)) return -3;

	return 0;
}

/*!
 * \brief Sends a partyline channel join out to the botnet
 *
 * This function simply calls the on_join callback for all directly linked
 * bots except the one the event came from. It should only be called by
 * exactly one function.
 *
 * \param chan The partyline channel the user has joined.
 * \param p The user that has joined the channel.
 * \param linking True if the join is part of the linking process between two bots.
 *
 * \warning If this function is referenced by \e any other function
 *          except partychan_join() it is a bug.
 */

void botnet_member_join(partychan_t *chan, partymember_t *p, int linking)
{
	botnet_bot_t *tmp;

	for (tmp = localbot_head; tmp; tmp = tmp->next_local) {
		if (tmp->flags & BOT_DELETED || (p->bot && tmp == p->bot->direction)) continue;
		if (tmp->handler && tmp->handler->on_join) tmp->handler->on_join(tmp->client_data, chan, p, linking);
	}
}

void botnet_member_part(partychan_t *chan, partymember_t *p, const char *reason, int len)
{
	botnet_bot_t *tmp;

	for (tmp = localbot_head; tmp; tmp = tmp->next_local) {
		if (tmp->flags & BOT_DELETED || (p->bot && tmp == p->bot->direction)) continue;
		if (tmp->handler && tmp->handler->on_part) tmp->handler->on_part(tmp->client_data, chan, p, reason, len);
	}
}

void botnet_chanmsg(partychan_t *chan, botnet_entity_t *src, const char *text, int len)
{
	botnet_bot_t *tmp, *srcbot;

	if (src->what == ENTITY_PARTYMEMBER) srcbot = src->user->bot;
	else srcbot = src->bot;

	for (tmp = localbot_head; tmp; tmp = tmp->next_local) {
		if (tmp->flags & BOT_DELETED || (srcbot && tmp == srcbot->direction)) continue;
		if (tmp->handler && tmp->handler->on_chanmsg) tmp->handler->on_chanmsg(tmp->client_data, chan, src, text, len);
	}
}

void botnet_broadcast(botnet_entity_t *src, const char *text, int len)
{
	botnet_bot_t *tmp;

	if (len < 0) len = strlen(text);
	for (tmp = localbot_head; tmp; tmp = tmp->next_local) {
		if (tmp->flags & BOT_DELETED || (src && src->bot && tmp == src->bot->direction)) continue;
		if (tmp->handler && tmp->handler->on_bcast) tmp->handler->on_bcast(tmp->client_data, src, text, len);
	}
	partymember_local_broadcast(src, text, len);
}

void botnet_member_quit(partymember_t *p, const char *reason, int len)
{
	botnet_bot_t *tmp;

	for (tmp = localbot_head; tmp; tmp = tmp->next_local) {
		if (tmp->flags & BOT_DELETED || (p->bot && tmp == p->bot->direction)) continue;
		if (tmp->handler && tmp->handler->on_quit) tmp->handler->on_quit(tmp->client_data, p, reason, len);
	}
}

void botnet_botmsg(botnet_bot_t *src, botnet_bot_t *dst, const char *command, const char *text, int len)
{
	if (!text) len = 0;
	else if (len < 0) len = strlen(text);

	if (dst) {
		botnet_bot_t *dir = dst->direction;
		if (dir->handler && dir->handler->on_botmsg) dir->handler->on_botmsg(dir->client_data, src, dst, command, text, len);
		return;
	}
	bind_check(BT_bot, NULL, command, command, text);
}

void botnet_botbroadcast(botnet_bot_t *src, const char *command, const char *text, int len)
{
	botnet_bot_t *tmp;

	if (!text) len = 0;
	else if (len < 0) len = strlen(text);

	bind_check(BT_bot, NULL, command, command, text);

	for (tmp = localbot_head; tmp; tmp = tmp->next_local) {
		if (tmp->flags & BOT_DELETED || (src && tmp == src->direction)) continue;
		if (tmp->handler && tmp->handler->on_botbroadcast) tmp->handler->on_botbroadcast(tmp->client_data, src, command, text, len);
	}
}
