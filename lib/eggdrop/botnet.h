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
 * $Id: botnet.h,v 1.1 2006/11/14 14:51:23 sven Exp $
 */

#ifndef _EGG_BOTNET_H_
#define _EGG_BOTNET_H_

#define BTN_BOTNET_REQUEST_LINK "request-link"
#define BTN_BOTNET_LINK "link"
#define BTN_BOTNET_DISC "disc"

typedef struct {
	/* Events that don't depend on a single chan. */
	int (*on_nick)(void *client_data, partymember_t *src, const char *oldnick, const char *newnick);
	int (*on_quit)(void *client_data, partymember_t *src, const char *text, int len);

	/* Channel events. */
	int (*on_chanmsg)(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
	int (*on_join)(void *client_data, partychan_t *chan, partymember_t *src, int netburst);
	int (*on_part)(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);

	/* Botnet events. */
	int (*on_new_bot)(void *client_data, struct botnet_bot *bot, int netburst);
	int (*on_lost_bot)(void *client_data, struct botnet_bot *bot, const char *reason);
	int (*on_unlink)(void *client_data, partymember_t *from, struct botnet_bot *bot, const char *reason);
	int (*on_botmsg)(void *client_data, ...);

	/* Module/Script responsible for this bot */
} botnet_handler_t;

/*!
 * \brief This is a bot in our botnet
 *
 * This struct contains only the minimum information nesessary to
 * maintain the botnet. All the messy details are stored in the
 * module's or script's client_data.
 */

typedef struct botnet_bot {
	char *name;                        //!< The name of the bot.
	unsigned flags;                    //!< Some flags.
	user_t *user;                      //!< The user entry for this bot. NULL if it's not local.
	struct botnet_bot *uplink;         //!< The bot directly linked to this bot. NULL if it's local.
	struct botnet_bot *direction;      //!< The bot we have to talk to to reach this bot. Must \a not be NULL.
	botnet_handler_t *handler;         //!< Handler struct containing functions to call to get stuff done. May be NULL if it's not local..
	void *client_data;                 //!< Some data passed back to every callback function.
	event_owner_t *owner;              //!< The owner of this bot.
	partymember_t *partys;             //!< Start of the list of partymembers on this bot.
	struct botnet_bot *prev;           //!< Previous bot in the list.
	struct botnet_bot *next;           //!< Next bot in the list.
	struct botnet_bot *prev_local;     //!< Previous bot in the list of local bots. NULL if this bot isn't local.
	struct botnet_bot *next_local;     //!< Next bot in the list of local bots. NULL if this bot isn't local.
} botnet_bot_t;

int botnet_init(void);
int botnet_shutdown(void);

int botnet_set_name(const char *name);
const char *botnet_get_name(void);
const botnet_bot_t *botnet_get_head(void);

botnet_bot_t *botnet_lookup(const char *name);
botnet_bot_t *botnet_new(const char *name, user_t *user, botnet_bot_t *uplink, botnet_bot_t *direction, botnet_handler_t *handler, void *client_data, event_owner_t *owner, int netburst);
void botnet_count_subtree(botnet_bot_t *bot, int *bots, int *members);
int botnet_delete(botnet_bot_t *bot, const char *reason);
int botnet_unlink(partymember_t *from, botnet_bot_t *bot, const char *reason);
int botnet_link(user_t *user);

void botnet_replay_net(botnet_bot_t *bot);
int botnet_check_direction(botnet_bot_t *direction, botnet_bot_t *src);

void botnet_member_join(partychan_t *chan, partymember_t *p, int linking);
void botnet_member_part(partychan_t *chan, partymember_t *p, const char *reason, int len);
void botnet_chanmsg(partychan_t *chan, partymember_t *p, const char *reason, int len);
void botnet_member_quit(partymember_t *p, const char *reason, int len);

#endif /* !_EGG_BOTNET_H_ */
