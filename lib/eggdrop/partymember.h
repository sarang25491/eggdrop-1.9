/* partymember.h: header for partyline.c
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
 *
 * $Id: partymember.h,v 1.8 2007/08/18 22:32:23 sven Exp $
 */

#ifndef _EGG_PARTYMEMBER_H
#define _EGG_PARTYMEMBER_H

struct partymember {
  partymember_t *next;
	partymember_t *prev;
	partymember_t *next_on_bot;
	partymember_t *prev_on_bot;

	int id;
	char *nick, *ident, *host;
	user_t *user;
	struct botnet_bot *bot;
	int flags;

	char *net_name;
	char *full_id_name;
	const char *full_name;
	const char *common_name;

	partychan_t **channels;
	int nchannels;

	partyline_event_t *handler;
	void *client_data;
	event_owner_t *owner;
};

partymember_t *partymember_lookup(const char *name, struct botnet_bot *bot, int id);
partymember_t *partymember_new(int id, user_t *user, struct botnet_bot *bot, const char *nick, const char *ident, const char *host, partyline_event_t *handler, void *client_data, event_owner_t *owner);
partymember_t *partymember_get_head(void);
int partymember_delete(partymember_t *p, const struct botnet_bot *lost_bot, const char *text);
int partymember_delete_by_owner(struct egg_module *module, void *script);
int partymember_update_info(partymember_t *p, const char *ident, const char *host);
int partymember_who(int **ids, int *len);
int partymember_write_id(int id, const char *text, int len);
int partymember_write(partymember_t *p, const char *text, int len);
int partymember_msg(partymember_t *p, botnet_entity_t *src, const char *text, int len);
int partymember_msgf(partymember_t *p, botnet_entity_t *src, const char *fmt, ...);
int partymember_printf_id(int id, const char *fmt, ...);
int partymember_printf(partymember_t *p, const char *fmt, ...);
int partymember_set_nick(partymember_t *p, const char *nick);
int partymember_local_broadcast(botnet_entity_t *src, const char *text, int len);

int partymember_count_by_bot(const struct botnet_bot *bot);
int partymember_delete_by_bot(const struct botnet_bot *bot, const struct botnet_bot *lost_bot, const char *reason);

#endif /* !_EGG_PARTYMEMBER_H */

