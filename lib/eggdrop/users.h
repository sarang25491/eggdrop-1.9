/* users.h: header for users.c
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
 *
 * $Id: users.h,v 1.13 2004/07/17 20:59:38 darko Exp $
 */

#ifndef _EGG_USERS_H_
#define _EGG_USERS_H_

#define USER_HASH_SIZE	50
#define HOST_HASH_SIZE	50

#define USER_DELETED 1

/* Bind table names for user events */
#define BTN_USER_CHANGE_FLAGS		"uflags"
#define BTN_USER_CHANGE_SETTINGS	"uset"
#define BTN_USER_DELETE			"udelete"

typedef struct {
	char *name;
	char *value;
} extended_setting_t;

typedef struct {
	/* The channel these settings apply to, or NULL for global. */
	char *chan;

	/* Builtin flags and user defined flags. */
	flags_t flags;

	/* Extended settings done by modules/scripts. */
	extended_setting_t *extended;
	int nextended;
} user_setting_t;

typedef struct user {
	/* Each user has a unique handle and a permanent uid. */
	char *handle;
	int uid;

	/* Masks that the user is recognized by on irc (nick!user@host). */
	char **ircmasks;
	int nircmasks;

	/* Settings for this user. */
	user_setting_t *settings;
	int nsettings;

	/* Flags for the structure, e.g. USER_DELETED. */
	int flags;
} user_t;

int user_init(void);
int user_shutdown(void);

int user_load(const char *fname);
int user_save(const char *fname);
user_t *user_new(const char *handle);
int user_delete(user_t *u);
user_t *user_lookup_by_handle(const char *handle);
user_t *user_lookup_authed(const char *handle, const char *pass);
user_t *user_lookup_by_uid(int uid);
user_t *user_lookup_by_irchost_nocache(const char *irchost);
user_t *user_lookup_by_irchost(const char *irchost);
int user_add_ircmask(user_t *u, const char *ircmask);
int user_del_ircmask(user_t *u, const char *ircmask);
int user_get_setting(user_t *u, const char *chan, const char *setting, char **valueptr);
int user_set_setting(user_t *u, const char *chan, const char *setting, const char *newvalue);

int	user_get_flags		(user_t *u, const char *chan, flags_t *flags);
int	user_set_flags		(user_t *u, const char *chan, flags_t *flags);
int	user_set_flags_str	(user_t *u, const char *chan, const char *flags);

int	user_check_flags	(user_t *u, const char *chan, flags_t *flags);
int	user_check_flags_str	(user_t *u, const char *chan, const char *flags);

int user_has_pass(user_t *u);
int user_check_pass(user_t *u, const char *pass);
int user_set_pass(user_t *u, const char *pass);
int user_count();
int user_rand_pass(char *buf, int bufsize);
int user_change_handle(user_t *u, const char *old, const char *new);
int partyline_cmd_match_ircmask(void *p, const char *mask, long start, long limit);
int partyline_cmd_match_attr(void *p, const char *attr, const char *chan, long start, long limit);

#endif /* !_EGG_USERS_H_ */
