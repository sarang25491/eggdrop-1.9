#ifndef _USERS_H_
#define _USERS_H_

#define USER_HASH_SIZE	50
#define HOST_HASH_SIZE	50

#define USER_DELETED	1

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

int user_init();
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
int user_get_flags(user_t *u, const char *chan, flags_t *flags);
int user_set_flags(user_t *u, const char *chan, flags_t *flags);
int user_set_flag_str(user_t *u, const char *chan, const char *flags);
int user_has_pass(user_t *u);
int user_check_pass(user_t *u, const char *pass);
int user_set_pass(user_t *u, const char *pass);
int user_count();
int user_rand_pass(char *buf, int bufsize);

#endif
