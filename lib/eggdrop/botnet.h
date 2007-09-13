/* botnet.h: header for botnet.c
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Eggheads Development Team
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
 *
 * $Id: botnet.h,v 1.9 2007/09/13 22:20:55 sven Exp $
 */

#ifndef _EGG_BOTNET_H_
#define _EGG_BOTNET_H_

#define BTN_BOTNET_REQUEST_LINK "request-link"
#define BTN_BOTNET_LINK         "link"
#define BTN_BOTNET_DISC         "disc"
#define BTN_BOTNET_BOT          "bot"
#define BTN_BOTNET_EXTENSION    "ext"

#define BOT_DELETED          1
#define BOT_LINKING          2

#define EXTENSION_ONE        1
#define EXTENSION_ALL        2
#define EXTENSION_NEIGHBOURS 3

typedef struct {
	/* Events that don't depend on a single chan. */
	int (*on_login)(void *client_data, partymember_t *new);
	int (*on_bcast)(void *client_data, botnet_entity_t *src, const char *text, int len);
	int (*on_privmsg)(void *client_data, botnet_entity_t *src, partymember_t *dst, const char *text, int len);
	int (*on_nick)(void *client_data, partymember_t *src, const char *newnick);
	int (*on_quit)(void *client_data, partymember_t *src, const char *text, int len);

	/* Channel events. */
	int (*on_chanmsg)(void *client_data, partychan_t *chan, botnet_entity_t *src, const char *text, int len);
	int (*on_join)(void *client_data, partychan_t *chan, partymember_t *src, int netburst);
	int (*on_part)(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);

	/* Botnet events. */
	int (*on_new_bot)(void *client_data, botnet_bot_t *bot, int netburst);
	int (*on_lost_bot)(void *client_data, botnet_bot_t *bot_t, const char *reason);
	int (*on_link)(void *client_data, botnet_entity_t *from, botnet_bot_t *dst, const char *target);
	int (*on_unlink)(void *client_data, botnet_entity_t *from, botnet_bot_t *bot, const char *reason);
	int (*on_botmsg)(void *client_data, botnet_bot_t *src, botnet_bot_t *dst, const char *command, const char *text, int len);
	int (*on_botbroadcast)(void *client_data, botnet_bot_t *src, const char *command, const char *text, int len);
	int (*on_extension)(void *client_data, botnet_entity_t *src, botnet_bot_t *dst, const char *cmd, const char *text, int len);

	/* Module/Script responsible for this bot */
} botnet_handler_t;

/*!
 * \brief This is a bot in our botnet
 *
 * This struct contains only the minimum information nesessary to
 * maintain the botnet. All the messy details are stored in the
 * module's or script's client_data.
 */

struct botnet_bot {
	char *name;                        //!< The name of the bot.
	unsigned flags;                    //!< Some flags.
	user_t *user;                      //!< The user entry for this bot. NULL if it's not local.
	struct botnet_bot *uplink;         //!< The bot directly linked to this bot. NULL if it's local.
	struct botnet_bot *direction;      //!< The bot we have to talk to to reach this bot. Must \a not be NULL.
	botnet_handler_t *handler;         //!< Handler struct containing functions to call to get stuff done. May be NULL if it's not local..
	void *client_data;                 //!< Some data passed back to every callback function.
	event_owner_t *owner;              //!< The owner of this bot.
	partymember_t *partys;             //!< Start of the list of partymembers on this bot.
	xml_node_t *info;                  //!< Save anything you want here.
	struct botnet_bot *prev;           //!< Previous bot in the list.
	struct botnet_bot *next;           //!< Next bot in the list.
	struct botnet_bot *prev_local;     //!< Previous bot in the list of local bots. NULL if this bot isn't local.
	struct botnet_bot *next_local;     //!< Next bot in the list of local bots. NULL if this bot isn't local.
};

#define ENTITY_PARTYMEMBER  1
#define ENTITY_BOT          2

/*!
 * \brief Creates an entity from a user.
 *
 * This macro can be used to create a ::botnet_entity_t from a ::partymember_t.
 * This workes only during the initialisation of automatic variables.
 *
 * Example:
 * \code
 * botnet_entity_t entity = user_entity(some_user);
 * \endcode
 */

#define user_entity(user) {ENTITY_PARTYMEMBER, (user), 0}

/*!
 * \brief Creates an entity from a bot.
 *
 * This macro can be used to create a ::botnet_entity_t from a ::botnet_bot_t.
 * This workes only during the initialisation of automatic variables.
 *
 * Example:
 * \code
 * botnet_entity_t other = bot_entity(some_bot);
 * botnet_entity_t me = bot_entity((botnet_bot *) 0);
 * \endcode
 */

#define bot_entity(bot) {ENTITY_BOT, 0, (bot)}

#define set_user_entity(e, u) do {\
	(e)->what = ENTITY_PARTYMEMBER;\
	(e)->user = (u);\
} while (0)

#define set_bot_entity(e, b) do {\
	(e)->what = ENTITY_BOT;\
	(e)->bot = (b);\
} while (0)

struct botnet_entity {
	int what;                          //!< ::ENTITY_PARTYMEMBER or ::ENTITY_BOT
	partymember_t *user;
	botnet_bot_t *bot;
};

#define entity_full_id_name(e) ((e)->what == ENTITY_BOT ? (e)->bot ? (e)->bot->name : botnet_get_name() : (e)->user->full_id_name)
#define entity_full_name(e) ((e)->what == ENTITY_BOT ? (e)->bot ? (e)->bot->name : botnet_get_name() : (e)->user->full_name)
#define entity_common_name(e) ((e)->what == ENTITY_BOT ? (e)->bot ? (e)->bot->name : botnet_get_name() : (e)->user->bot ? (e)->user->full_name : (e)->user->nick)
#define entity_net_name(e) ((e)->what == ENTITY_BOT ? (e)->bot ? (e)->bot->name : botnet_get_name() : (e)->user->net_name)

int botnet_init(void);
int botnet_shutdown(void);

void botnet_autolink(void);
int botnet_set_name(const char *name);
const char *botnet_get_name(void);
const botnet_bot_t *botnet_get_head(void);

botnet_bot_t *botnet_lookup(const char *name);
botnet_bot_t *botnet_new(const char *name, user_t *user, botnet_bot_t *uplink, botnet_bot_t *direction, xml_node_t *info, botnet_handler_t *handler, void *client_data, event_owner_t *owner, int netburst);
void botnet_count_subtree(botnet_bot_t *bot, int *bots, int *members);
void botnet_link_failed(user_t *user, const char *reason);
void botnet_link_success(botnet_bot_t *bot);
int botnet_delete_by_owner(struct egg_module *module, void *script);
int botnet_delete(botnet_bot_t *bot, const char *reason);
int botnet_unlink(botnet_entity_t *from, botnet_bot_t *bot, const char *reason);
int botnet_link(botnet_entity_t *src, botnet_bot_t *dst, const char *target);

const char *botnet_get_info(botnet_bot_t *bot, const char *name);
void botnet_set_info(botnet_bot_t *bot, const char *name, const char *value);
void botnet_set_info_int(botnet_bot_t *bot, const char *name, int value);

void botnet_replay_net(botnet_bot_t *bot);
int botnet_check_direction(botnet_bot_t *direction, botnet_bot_t *src);

void botnet_broadcast(botnet_entity_t *src, const char *text, int len);
void botnet_announce_login(partymember_t *p);
void botnet_member_join(partychan_t *chan, partymember_t *p, int linking);
void botnet_member_part(partychan_t *chan, partymember_t *p, const char *reason, int len);
void botnet_chanmsg(partychan_t *chan, botnet_entity_t *src, const char *reason, int len);
void botnet_member_quit(partymember_t *p, const char *reason, int len);
void botnet_set_nick(partymember_t *p, const char *oldnick);
void botnet_botmsg(botnet_bot_t *src, botnet_bot_t *dst, const char *command, const char *text, int len);
void botnet_botbroadcast(botnet_bot_t *src, const char *command, const char *text, int len);
void botnet_extension(int mode, botnet_entity_t *src, botnet_bot_t *dst, egg_module_t *mod, const char *cmd, const char *text, int len);

#endif /* !_EGG_BOTNET_H_ */
