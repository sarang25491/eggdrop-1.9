/* users.c: userfile, user records, etc
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
static const char rcsid[] = "$Id: users.c,v 1.53 2006/12/15 09:30:47 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>

/* When we walk along irchost_cache_ht, we pass along this struct so the
 * function can keep track of modified entries. */
typedef struct {
	user_t *u;
	const char *ircmask;
	const char **entries;
	int nentries, maxentries;
	int rand_delete;
} walker_info_t;

typedef struct {
	long start;
	long limit;
	partymember_t *pmember;
	const char *data;
	const char *channel;
	flags_t musthave[2];
	flags_t mustnothave[2];
} cmd_match_data_t;

#define BAD_HANDLE_CHARS " *?@:"

/* Keep track of the next available uid. Also keep track of when uid's wrap
 * around (probably won't happen), so that we know when we can trust g_uid. */
static int g_uid = 1, uid_wraparound = 0;

/* The number of users we have. */
static int nusers = 0;

/* Hash table to associate irchosts (nick!user@host) with users. */
static hash_table_t *irchost_cache_ht = NULL;

/* Hash table to associate handles with users. */
static hash_table_t *handle_ht = NULL;

/* Hash table to associate uid's with users. */
static hash_table_t *uid_ht = NULL;

/* List to keep all users' ircmasks in. */
static ircmask_list_t ircmask_list = {NULL};

/*!
 * \bind
 * This flag is triggered every time the flags of a user are changed.
 * \name uflags
 * \flags ignored
 * \match "channel oldflags newflags"
 * \param string The user's handle.
 * \param string The channel.
 * \param string The old flags.
 * \param string The new flags.
 * \stackable
 * \return The return value is broken.
 * \bug The flags are ignored, the return value is treated as if the bind is
 *      breakable, the user is passed as a string.
 */

static bind_table_t *BT_uflags = NULL;

/*!
 * \bind
 * This flag is triggered every time a user's settings are changed.
 * \name uset
 * \flags ignored
 * \match "channel setting"
 * \param string The user's handle.
 * \param string The channel.
 * \param string The setting.
 * \param string The value.
 * \stackable
 * \return The return value is broken.
 * \bug The flags are ignored, the return value is treated as if the bind is
 *      breakable, the user is passed as a string.
 */

static bind_table_t *BT_uset = NULL;

/*!
 * \bind
 * This flag is triggered every time a user is deleted.
 * \name udelete
 * \flags ignored
 * \match Nothing.
 * \param user The deleted user.
 * \stackable
 * \noreturn
 * \bug The flags are ignored, nothing is matched, the bind is triggered after
 *      the user was deleted.
 */

static bind_table_t *BT_udelete = NULL;

/* Prototypes for internal functions. */
static user_t *real_user_new(const char *handle, int uid);
static int user_get_uid();
static int cache_check_add(const void *key, void *dataptr, void *client_data);
static int cache_check_del(const void *key, void *dataptr, void *client_data);
static int cache_rand_cleanup(const char *event);
static int cache_user_del(user_t *u, const char *ircmask);
static void append_setting(user_t *u, const char *chan, const char *flag_str, xml_node_t *extended);
static int userlist_delete_walker(const void *key, void *dataptr, void *client_data);

int user_init(void)
{

	/* Create hash tables. */
	handle_ht = hash_table_create(NULL, NULL, USER_HASH_SIZE, HASH_TABLE_STRINGS);
	uid_ht = hash_table_create(NULL, NULL, USER_HASH_SIZE, HASH_TABLE_INTS);
	irchost_cache_ht = hash_table_create(NULL, NULL, HOST_HASH_SIZE, HASH_TABLE_STRINGS | HASH_TABLE_FREE_KEY);

	/* And bind tables. */
	BT_uflags = bind_table_add(BTN_USER_CHANGE_FLAGS, 4, "ssss", MATCH_MASK, BIND_STACKABLE);	/* DDD	*/
	BT_uset = bind_table_add(BTN_USER_CHANGE_SETTINGS, 4, "ssss", MATCH_MASK, BIND_STACKABLE);	/* DDD	*/
	BT_udelete = bind_table_add(BTN_USER_DELETE, 1, "U", MATCH_NONE, BIND_STACKABLE);	/* DDD	*/

	/* Add binds. */
	bind_add_simple("event", NULL, "hourly", cache_rand_cleanup);

	return(0);
}

int user_shutdown(void)
{
	hash_table_walk(uid_ht, userlist_delete_walker, NULL);

	/* flush any pending user delete events */
	garbage_run();

	bind_table_del(BT_udelete);
	bind_table_del(BT_uset);
	bind_table_del(BT_uflags);

	hash_table_delete(irchost_cache_ht);
	hash_table_delete(uid_ht);
	hash_table_delete(handle_ht);

	return (0);
}


int user_load(const char *fname)
{
	int uid;
	xml_node_t *root, *user_node, *ircmask_node, *setting_node, *extended_node;
	char *handle, *ircmask, *chan, *flag_str;
	user_t *u;

	if (!(root = xml_parse_file(fname))) {
		putlog(LOG_MISC, "*", _("Failed to load userfile '%s': %s"), fname, xml_last_error());
		return -1;
	}

	xml_node_get_vars(root, "ii", "next_uid", &uid, "uid_wraparound", &uid_wraparound);
	g_uid = uid;

	user_node = xml_node_lookup(root, 0, "user", 0, 0);
	for (; user_node; user_node = user_node->next_sibling) {
		/* The only required user fields are 'handle' and 'uid'. */
		xml_node_get_vars(user_node, "sinn", "handle", &handle, "uid", &uid, "ircmask", &ircmask_node, "setting", &setting_node);

		if (!handle || !uid) continue;
		u = real_user_new(handle, uid);

		/* User already exists? */
		if (!u) continue;

		/* Irc masks. */
		for (; ircmask_node; ircmask_node = ircmask_node->next_sibling) {
			ircmask = xml_node_str(ircmask_node, NULL);
			if (!ircmask) continue;
			u->ircmasks = realloc(u->ircmasks, sizeof(char *) * (u->nircmasks+1));
			u->ircmasks[u->nircmasks] = strdup(ircmask);
			u->nircmasks++;
			ircmask_list_add(&ircmask_list, ircmask, u);
		}

		/* Settings. */
		/* They have to have at least 1 setting, the global one. */
		if (!setting_node) append_setting(u, NULL, NULL, NULL);
		for (; setting_node; setting_node = setting_node->next_sibling) {
			xml_node_get_vars(setting_node, "ssn", "chan", &chan, "flags", &flag_str, "extended", &extended_node);
			append_setting(u, chan, flag_str, extended_node);
		}
	}
	xml_doc_delete(root);

	if (nusers == 1) putlog(LOG_MISC, "*", _("Loaded 1 user"));
	else putlog(LOG_MISC, "*", _("Loaded %d users"), nusers);

	return(0);
}

static int save_walker(const void *key, void *dataptr, void *param)
{
	xml_node_t *root = param;
	user_t *u = *(user_t **)dataptr;
	user_setting_t *setting;
	xml_node_t *user_node, *setting_node;
	int i;
	char flag_str[128];

	user_node = xml_node_new();
	user_node->name = strdup("user");
	xml_node_set_str(u->handle, user_node, "handle", 0, 0);
	xml_node_set_int(u->uid, user_node, "uid", 0, 0);
	for (i = 0; i < u->nircmasks; i++) xml_node_set_str(u->ircmasks[i], user_node, "ircmask", i, 0);
	for (i = 0; i < u->nsettings; i++) {
		setting = u->settings+i;
		setting_node = xml_node_new();
		setting_node->name = strdup("setting");
		if (setting->chan) xml_node_set_str(setting->chan, setting_node, "chan", 0, 0);
		flag_to_str(&u->settings[i].flags, flag_str);
		xml_node_set_str(flag_str, setting_node, "flags", 0, 0);
		xml_node_append(setting_node, u->settings[i].extended);
		xml_node_append(user_node, setting_node);
	}
	xml_node_append(root, user_node);
	return(0);
}

static int unlink_walker(const void *key, void *dataptr, void *param)
{
	user_t *u = *(user_t **)dataptr;
	int i;

	for (i = 0; i < u->nsettings; i++) {
		xml_node_unlink(u->settings[i].extended);
	}
	return(0);
}

int user_save(const char *fname)
{
	xml_node_t *root;

	root = xml_node_new();
	root->name = strdup("users");

	xml_node_set_int(g_uid, root, "next_uid", 0, 0);
	xml_node_set_int(uid_wraparound, root, "uid_wraparound", 0, 0);
	hash_table_walk(uid_ht, save_walker, root);

	xml_save_file((fname) ? fname : "users.xml", root, XML_INDENT);

	hash_table_walk(uid_ht, unlink_walker, NULL);

	xml_node_delete(root);
	return(0);
}

static int user_get_uid()
{
	user_t *u;
	int uid;

	if (g_uid <= 0) {
		g_uid = 1;
		uid_wraparound++;
	}

	/* If we've wrapped around on uids, we need to search for a free one. */
	if (uid_wraparound) {
		while (!hash_table_find(uid_ht, (void *)g_uid, &u)) g_uid++;
	}

	uid = g_uid;
	g_uid++;
	return(uid);
}

const char *user_invalid_handle(const char *handle)
{
	if (user_lookup_by_handle(handle)) return _("Handle already exists!");
	for (; *handle; ++handle) {
		if (*handle < 32 || strchr(BAD_HANDLE_CHARS, *handle)) {
			return _("Handle contains invalid characters!");
		}
	}
	return NULL;
}

static user_t *real_user_new(const char *handle, int uid)
{
	user_t *u;

	/* Make sure the handle is valid. */
	if (user_invalid_handle(handle)) return(NULL);

	u = calloc(1, sizeof(*u));
	u->handle = strdup(handle);
	if (!uid) uid = user_get_uid();
	u->uid = uid;

	hash_table_insert(handle_ht, u->handle, u);
	hash_table_insert(uid_ht, (void *)u->uid, u);
	nusers++;
	return(u);
}

user_t *user_new(const char *handle)
{
	int uid;
	user_t *u;

	uid = user_get_uid();
	u = real_user_new(handle, uid);
	if (!u) return(NULL);

	/* All users have the global setting by default. */
	u->settings = calloc(1, sizeof(*u->settings));
	u->nsettings = 1;
	u->settings[0].extended = xml_node_new();
	u->settings[0].extended->name = strdup("extended");

	return(u);
}

static int user_really_delete(void *client_data)
{
	int i;
	user_t *u = client_data;
	user_setting_t *setting;

	/* Free the ircmasks. */
	for (i = 0; i < u->nircmasks; i++) free(u->ircmasks[i]);
	if (u->ircmasks) free(u->ircmasks);

	/* And all of the settings. */
	for (i = 0; i < u->nsettings; i++) {
		setting = u->settings+i;
		xml_node_delete(setting->extended);
		if (setting->chan) free(setting->chan);
	}
	if (u->settings) free(u->settings);
	if (u->handle) free(u->handle);
	free(u);
	return(0);
}

int user_delete(user_t *u)
{
	int i;

	if (!u || (u->flags & USER_DELETED)) return(-1);

	nusers--;
	hash_table_remove(handle_ht, u->handle, NULL);
	hash_table_remove(uid_ht, (void *)u->uid, NULL);
	cache_user_del(u, "*");
	for (i = 0; i < u->nircmasks; i++) ircmask_list_del(&ircmask_list, u->ircmasks[i], u);
	u->flags |= USER_DELETED;
	bind_check(BT_udelete, NULL, NULL, u);
	garbage_add(user_really_delete, u, 0);
	return(0);
}

static int userlist_delete_walker(const void *key, void *dataptr, void *client_data)
{
	return user_delete(*(user_t **)dataptr);
}

user_t *user_lookup_by_handle(const char *handle)
{
	user_t *u = NULL;
	if (handle == NULL) return NULL;
	hash_table_find(handle_ht, handle, &u);
	return(u);
}

user_t *user_lookup_authed(const char *handle, const char *pass)
{
	user_t *u = user_lookup_by_handle(handle);
	if (!u) return(NULL);
	if (user_check_pass(u, pass)) return(u);
	return(NULL);
}

user_t *user_lookup_by_uid(int uid)
{
	user_t *u = NULL;

	hash_table_find(uid_ht, (void *)uid, &u);
	return(u);
}

user_t *user_lookup_by_irchost_nocache(const char *irchost)
{
	user_t *u;

	/* Check the ircmask cache. */
	if (!hash_table_find(irchost_cache_ht, irchost, &u)) return(u);

	/* Look for a match in the ircmask list. We don't cache the result. */
	ircmask_list_find(&ircmask_list, irchost, &u);
	return(u);
}

user_t *user_lookup_by_irchost(const char *irchost)
{
	user_t *u;

	/* Check the irchost cache. */
	if (!hash_table_find(irchost_cache_ht, irchost, &u)) return(u);

	/* Look for a match in the ircmask list. */
	ircmask_list_find(&ircmask_list, irchost, &u);

	/* Cache it, even if it's null (to prevent future lookups). */
	hash_table_insert(irchost_cache_ht, strdup(irchost), u);
	return(u);
}

static int cache_check_add(const void *key, void *dataptr, void *client_data)
{
	const char *irchost = key;
	user_t *u = *(user_t **)dataptr;
	walker_info_t *info = client_data;
	int i, strength, max_strength;

	/* Get the strength of the current match. */
	max_strength = 0;
	if (u) {
		for (i = 0; i < u->nircmasks; i++) {
			strength = wild_match(u->ircmasks[i], irchost);
			if (strength > max_strength) max_strength = strength;
		}
	}

	/* And now the strength of the the new mask. */
	strength = wild_match(info->ircmask, irchost);
	if (strength > max_strength) {
		/* Ok, replace it. */
		*(user_t **)dataptr = info->u;
	}
	return(0);
}

static int cache_check_del(const void *key, void *dataptr, void *client_data)
{
	const char *irchost = key;
	user_t *u = *(user_t **)dataptr;
	walker_info_t *info = client_data;

	if ((info->rand_delete && (rand() & 1)) || (!info->rand_delete && u == info->u && wild_match(info->ircmask, irchost))) {
		if (info->nentries >= info->maxentries) {
			info->maxentries = 2*info->maxentries+1;
			info->entries = realloc(info->entries, sizeof(*info->entries) * (info->maxentries+1));
		}
		info->entries[info->nentries] = irchost;
		info->nentries++;
	}
	return(0);
}

static int cache_user_del(user_t *u, const char *ircmask)
{
	walker_info_t info;
	int i;

	/* Check irchost_cache_ht for changes in the users. */
	info.u = u;
	info.ircmask = ircmask;
	info.entries = NULL;
	info.nentries = 0;
	info.maxentries = 0;
	info.rand_delete = 0;
	hash_table_walk(irchost_cache_ht, cache_check_del, &info);
	for (i = 0; i < info.nentries; i++) {
		hash_table_remove(irchost_cache_ht, info.entries[i], NULL);
	}
	if (info.entries) free(info.entries);

	/* And remove it from the ircmask_list. */
	ircmask_list_del(&ircmask_list, ircmask, u);
	return(0);
}

static int cache_rand_cleanup(const char *event)
{
	walker_info_t info;
	int i;

	/* Check irchost_cache_ht for changes in the users. */
	info.u = NULL;
	info.ircmask = NULL;
	info.entries = NULL;
	info.nentries = 0;
	info.maxentries = 0;
	info.rand_delete = 1;
	hash_table_walk(irchost_cache_ht, cache_check_del, &info);
	putlog(LOG_MISC, "*", "user_rand_cleanup: deleting %d entries from cache", info.nentries);
	for (i = 0; i < info.nentries; i++) {
		hash_table_remove(irchost_cache_ht, info.entries[i], NULL);
	}
	if (info.entries) free(info.entries);
	return(0);
}

int user_add_ircmask(user_t *u, const char *ircmask)
{
	walker_info_t info;

	/* Add the ircmask to the user entry. */
	u->ircmasks = (char **)realloc(u->ircmasks, sizeof(char *) * (u->nircmasks+1));
	u->ircmasks[u->nircmasks] = strdup(ircmask);
	u->nircmasks++;

	/* Put it in the big list. */
	ircmask_list_add(&ircmask_list, ircmask, u);

	/* Check irchost_cache_ht for changes in the users. */
	info.u = u;
	info.ircmask = ircmask;
	hash_table_walk(irchost_cache_ht, cache_check_add, &info);
	return(0);
}

int user_del_ircmask(user_t *u, const char *ircmask)
{
	int i;

	/* Find the ircmask in the user entry. */
	for (i = 0; i < u->nircmasks; i++) {
		if (!strcasecmp(u->ircmasks[i], ircmask)) break;
	}
	if (i == u->nircmasks) return(-1);

	/* Get rid of it. */
	memmove(u->ircmasks+i, u->ircmasks+i+1, sizeof(char *) * (u->nircmasks - i - 1));
	u->nircmasks--;

	/* Delete matching entries of this user in the host cache. */
	cache_user_del(u, ircmask);

	return(0);
}

static int get_flags(user_t *u, const char *chan, flags_t **flags)
{
	int i;

	if (!chan) i = 0;
	else {
		for (i = 1; i < u->nsettings; i++) {
			if (!strcasecmp(chan, u->settings[i].chan)) break;
		}
		if (i == u->nsettings) return(-1);
	}
	*flags = &u->settings[i].flags;
	return(0);
}

int user_get_flags(user_t *u, const char *chan, flags_t *flags)
{
	flags_t *uflags;

	if (get_flags(u, chan, &uflags)) return(-1);

	flags->builtin = uflags->builtin;
	flags->udef = uflags->udef;
	return(0);
}

static int check_flag_change(user_t *u, const char *chan, flags_t *oldflags, flags_t *newflags)
{
	char oldstr[64], newstr[64], *change;
	int r;

	flag_to_str(oldflags, oldstr);
	flag_to_str(newflags, newstr);
	change = egg_msprintf(NULL, 0, NULL, "%s %s %s", chan ? chan : "", oldstr, newstr);
	r = bind_check(BT_uflags, NULL, change, u->handle, chan, oldstr, newstr);
	free(change);

	/* Does a callback want to cancel this flag change? */
	if (r & BIND_RET_BREAK) return(-1);
	return(0);
}

int user_set_flags(user_t *u, const char *chan, flags_t *flags)
{
	flags_t *oldflags, newflags;

	if (get_flags(u, chan, &oldflags)) {
		append_setting(u, chan, NULL, NULL);
		if (get_flags(u, chan, &oldflags)) return -1;
	}
	newflags.builtin = flags->builtin;
	newflags.udef = flags->udef;
	if (check_flag_change(u, chan, oldflags, &newflags)) return(0);
	oldflags->builtin = newflags.builtin;
	oldflags->udef = newflags.udef;
	if (chan)
		channel_sanity_check(oldflags);
	else
		global_sanity_check(oldflags);
	return(0);
}

int user_set_flags_str(user_t *u, const char *chan, const char *flags)
{
	flags_t *oldflags, newflags;

	if (get_flags(u, chan, &oldflags)) {
		append_setting(u, chan, NULL, NULL);
		if (get_flags(u, chan, &oldflags)) return(-1);
	}
	newflags.builtin = oldflags->builtin;
	newflags.udef = oldflags->udef;
	flag_merge_str(&newflags, flags);
	if (check_flag_change(u, chan, oldflags, &newflags)) return(0);
	oldflags->builtin = newflags.builtin;
	oldflags->udef = newflags.udef;
	if (chan)
		channel_sanity_check(oldflags);
	else
		global_sanity_check(oldflags);
	return(0);
}

static void append_setting(user_t *u, const char *chan, const char *flag_str, xml_node_t *extended)
{
	user_setting_t *setting;
	u->settings = realloc(u->settings, sizeof(*u->settings) * (u->nsettings+1));
	setting = u->settings + u->nsettings;
	u->nsettings++;
	memset(setting, 0, sizeof(*setting));
	if (chan) setting->chan = strdup(chan);
	if (flag_str) flag_from_str(&setting->flags, flag_str);
	if (extended) {
		xml_node_unlink(extended);
		setting->extended = extended;
	}
	else {
		setting->extended = xml_node_new();
		setting->extended->name = strdup("extended");
	}
}

static xml_node_t *find_setting(user_t *u, const char *chan, const char *name, int create)
{
	user_setting_t *setting;
	int i = 0;

	if (chan) for (i = 1; i < u->nsettings; i++) {
		if (!strcasecmp(chan, u->settings[i].chan)) break;
	}
	if (i >= u->nsettings) {
		if (!create) return(NULL);
		append_setting(u, chan, NULL, NULL);
	}
	setting = u->settings+i;
	return xml_node_lookup(setting->extended, create, name, 0, NULL);
}

int user_get_setting(user_t *u, const char *chan, const char *setting, char **valueptr)
{
	xml_node_t *node = find_setting(u, chan, setting, 0);

	return xml_node_get_str(valueptr, node, NULL);
}

int user_set_setting(user_t *u, const char *chan, const char *setting, const char *value)
{
	int r;
	char *change, buf[64];
	xml_node_t *node;

	change = egg_msprintf(buf, sizeof(buf), NULL, "%s %s", chan ? chan : "", setting);
	r = bind_check(BT_uset, NULL, change, u->handle, chan, setting, value);
	if (change != buf) free(change);

	if (r & BIND_RET_BREAK) return(0);

	node = find_setting(u, chan, setting, 1);
	return xml_node_set_str(value, node, NULL);
}

int user_has_pass(user_t *u)
{
	char *hash, *salt;

	user_get_setting(u, NULL, "pass", &hash);
	user_get_setting(u, NULL, "salt", &salt);
	if (hash && salt && strcmp(hash, "none")) return(1);
	return(0);
}

int user_check_pass(user_t *u, const char *pass)
{
	char *hash, *salt, test[16], testhex[33];
	MD5_CTX ctx;

	user_get_setting(u, NULL, "pass", &hash);
	user_get_setting(u, NULL, "salt", &salt);
	if (!hash || !salt || !strcmp(hash, "none")) return(0);

	MD5_Init(&ctx);
	MD5_Update(&ctx, salt, strlen(salt));
	MD5_Update(&ctx, pass, strlen(pass));
	MD5_Final(test, &ctx);
	MD5_Hex(test, testhex);
	return !strcasecmp(testhex, hash);
}

int user_set_pass(user_t *u, const char *pass)
{
	char hash[16], hashhex[33], *salt, new_salt[33];
	MD5_CTX ctx;

	user_get_setting(u, NULL, "salt", &salt);
	if (!salt) {
		salt = new_salt;
		user_rand_pass(salt, sizeof(new_salt));
		user_set_setting(u, NULL, "salt", salt);
	}

	if (!pass || !*pass) {
		user_set_setting(u, NULL, "pass", "none");
		return(1);
	}
	MD5_Init(&ctx);
	MD5_Update(&ctx, salt, strlen(salt));
	MD5_Update(&ctx, pass, strlen(pass));
	MD5_Final(hash, &ctx);
	MD5_Hex(hash, hashhex);
	user_set_setting(u, NULL, "pass", hashhex);
	return(0);
}

int user_count()
{
	return(nusers);
}

/* Generate a password out of digits, uppercase, and lowercase letters. */
int user_rand_pass(char *buf, int bufsize)
{
	int i, c;

	bufsize--;
	if (!buf || bufsize <= 0) return(-1);
	for (i = 0; i < bufsize; i++) {
		c = (random() + (random() >> 16) + (random() >> 24)) % 62;
		if (c < 10) c += 48;	/* Digits. */
		else if (c < 36) c += 55;	/* Uppercase. */
		else c += 61;	/* Lowercase. */
		buf[i] = c;
	}
	buf[i] = 0;
	return(0);
}

int user_check_flags(user_t *u, const char *chan, flags_t *flags)
{
	flags_t f;

	user_get_flags(u, chan, &f);
	return flag_match_subset(flags, &f);
}

int user_check_flags_str(user_t *u, const char *chan, const char *flags)
{
	flags_t f;

	flag_from_str(&f, flags);
	return user_check_flags(u, chan, &f);
}

int user_check_partial_flags(user_t *u, const char *chan, flags_t *flags)
{
	flags_t f;

	user_get_flags(u, chan, &f);
	return flag_match_partial(flags, &f);
}

int user_check_partial_flags_str(user_t *u, const char *chan, const char *flags)
{
	flags_t f;

	flag_from_str(&f, flags);
	return user_check_partial_flags(u, chan, &f);
}

int user_change_handle(user_t *u, const char *newhandle)
{
	char *old = u->handle;
	partymember_t *p = NULL;

	if (user_invalid_handle(newhandle)) return 1;
	hash_table_remove(handle_ht, u->handle, NULL);
	u->handle = strdup(newhandle);
	hash_table_insert(handle_ht, u->handle, u);

	while ((p = partymember_lookup(old, NULL, -1))) {
		partymember_set_nick(p, u->handle);
	}

	free(u->handle);
	return(0);
}

static int ircmask_matches_user(user_t *u, const char *wild)
{
	int i;

	for (i = 0; i < u->nircmasks; i++)
		if (wild_match(wild, u->ircmasks[i]))
			return 1;
	return 0;
}

static int
attr_matches_user(user_t *u, cmd_match_data_t *mdata)
{
	int matched = 0, haschanflags = 0;
	flags_t globflags, chanflags;

	chanflags.builtin = chanflags.udef = 0;

	if (user_get_flags(u, NULL, &globflags))
		return 0;

	if (mdata->channel) {
		haschanflags = !user_get_flags(u, mdata->channel, &chanflags);
		if (!haschanflags)
			return 0;
	}

	if ((mdata->musthave[0].builtin | mdata->musthave[0].udef) && /* Non empty needed flags */
		flag_match_subset(&mdata->musthave[0], &globflags) && /* ..AND they match */
		(!(mdata->mustnothave[0].builtin | mdata->mustnothave[0].udef) || /* AND (empty forbiden flags */
		!flag_match_partial(&globflags, &mdata->mustnothave[0])) /* OR forbiden flags don't match) */
		)
		matched = 1;

	if (mdata->data) { /* mdata->data points to '|' or '&' if it existed,
				which means channel flags are needed too */
		int tmpyes = mdata->musthave[1].builtin | mdata->musthave[1].udef;
		int tmpnot = mdata->mustnothave[1].builtin | mdata->mustnothave[1].udef;
		if (*mdata->data == '|') {

			return (matched || /* Either we already matched OR .. */
				((tmpyes && flag_match_subset(&mdata->musthave[1], &chanflags)) && /*Non-empty needed
												flags match, AND */
				(!tmpnot || !flag_match_partial(&chanflags, &mdata->mustnothave[1]))) /* There are no
													forbidden flags OR
													they don't match */
				);
		}
		else if (*mdata->data == '&')
			return (matched && /* Global flags match AND .. */
				((tmpyes && flag_match_subset(&mdata->musthave[1], &chanflags)) &&
				(!tmpnot || !flag_match_partial(&chanflags, &mdata->mustnothave[1])))
				);
		else
			return 0;
	}

	return matched;
}

/* Maybe we should come up with a way to tell hash_table_walk to stop traversing?
   Callback functions already return an int - it's just a question of making sure
   other code doesn't get broken.
   As it is now, callback_match_attr will continue to be called even if no more
   results are needed. No big deal usually, but some people are known to have
   hundreds and even thousands of users.
*/

static int
callback_match_attr(const char *key, char **data, cmd_match_data_t *mdata)
{
	user_t *u = user_lookup_by_handle(key);

	if (!u)
		return 0;

	if (mdata->start && attr_matches_user(u, mdata)) {
		mdata->start--;
		return 0;
	}

	if (mdata->limit && attr_matches_user(u, mdata)) {
		mdata->limit--;
		partymember_printf(mdata->pmember, _("  %s"), key);
	}

	return 0;
}

static int
callback_match_ircmask(const char *key, char **data, cmd_match_data_t *mdata)
{
	user_t *u = user_lookup_by_handle(key);

	if (!u)
		return 0;

	if (mdata->start && (wild_match(mdata->data, key) || ircmask_matches_user(u, mdata->data))) {
		mdata->start--;
		return 0;
	}

	if (mdata->limit && (wild_match(mdata->data, key) || ircmask_matches_user(u, mdata->data))) {
		mdata->limit--;
		partymember_printf(mdata->pmember, _("  %s"), key);
	}

	return 0;
}

int
partyline_cmd_match_ircmask(void *p, const char *mask, long start, long limit)
{
	cmd_match_data_t mdata;

	mdata.start = start;
	mdata.limit = limit;
/* FIXME - Once we sort #includes, 'p' should be partymember_t in function declaration */
	mdata.pmember = (partymember_t *)p;
	mdata.data = mask;

	hash_table_walk(handle_ht, (hash_table_node_func)callback_match_ircmask, &mdata);

	return 0;
}

int
partyline_cmd_match_attr(void *p, const char *attr, const char *chan, long start, long limit)
{
	cmd_match_data_t mdata;
	int plsmns = 1, globchan = 0;
	char flagshack[] = "+ "; /* Hack to allow us to use flag_merge_str */

	mdata.start = start;
	mdata.limit = limit;
/* FIXME - Once we sort #includes, 'p' should be partymember_t in function declaration */
	mdata.pmember = (partymember_t *)p;
	mdata.data = NULL;
	mdata.channel = chan;
	mdata.musthave[0].builtin = mdata.musthave[0].udef = 0;
	mdata.mustnothave[0].builtin = mdata.mustnothave[0].udef = 0;
	mdata.musthave[1].builtin = mdata.musthave[1].udef = 0;
	mdata.mustnothave[1].builtin = mdata.mustnothave[1].udef = 0;

	while (*attr) {
		switch (*attr) {
			case '+':
				plsmns = 1;
				break;
			case '-':
				plsmns = 0;
				break;
			case '&':
			case '|':
				if (globchan == 0) { /* Consider only the first & or | */
					globchan++;
					mdata.data = attr;
				}
				break;
			default:
				flagshack[1] = *attr;
				flag_merge_str(plsmns ? &mdata.musthave[globchan] :
							&mdata.mustnothave[globchan], flagshack);
				break;
		}
		attr++;
	}

	hash_table_walk(handle_ht, (hash_table_node_func)callback_match_attr, &mdata);

	return 0;
}
