#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <eggdrop/eggdrop.h>

/* When we walk along irchost_cache_ht, we pass along this struct so the
 * function can keep track of modified entries. */
typedef struct {
	user_t *u;
	const char *ircmask;
	const char **entries;
	int nentries;
} walker_info_t;

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

/* Bind tables. */
static bind_table_t *BT_uflags = NULL,	/* user flags */
	*BT_uset = NULL,	/* settings */
	*BT_udelete = NULL;	/* user got deleted */

/* Prototypes for internal functions. */
static user_t *real_user_new(const char *handle, int uid);
static int user_get_uid();
static int cache_check_add(const void *key, void *dataptr, void *client_data);
static int cache_check_del(const void *key, void *dataptr, void *client_data);
static int cache_user_del(user_t *u, const char *ircmask);

int user_init()
{

	/* Create hash tables. */
	handle_ht = hash_table_create(NULL, NULL, USER_HASH_SIZE, HASH_TABLE_STRINGS);
	uid_ht = hash_table_create(NULL, NULL, USER_HASH_SIZE, HASH_TABLE_INTS);
	irchost_cache_ht = hash_table_create(NULL, NULL, HOST_HASH_SIZE, HASH_TABLE_STRINGS);

	/* And bind tables. */
	BT_uflags = bind_table_add("uflag", 4, "ssss", MATCH_MASK, BIND_STACKABLE);
	BT_uset = bind_table_add("uset", 4, "ssss", MATCH_MASK, BIND_STACKABLE);
	BT_udelete = bind_table_add("udelete", 1, "U", MATCH_NONE, BIND_STACKABLE);
	return(0);
}

int user_load(const char *fname)
{
	int i, j, k, uid;
	xml_node_t *root, *user_node, *setting_node;
	user_setting_t *setting;
	char *handle, *ircmask, *chan, *name, *value, *flag_str;
	user_t *u;

	root = xml_node_new();
	xml_read(root, fname);
	if (xml_node_get_int(&uid, root, "next_uid", 0, 0)) {
		xml_node_destroy(root);
		return(0);
	}
	g_uid = uid;
	xml_node_get_int(&uid_wraparound, root, "uid_wraparound", 0, 0);
	for (i = 0; i < root->nchildren; i++) {
		user_node = root->children[i];
		if (strcasecmp(user_node->name, "user")) continue;

		/* The only required user fields are 'handle' and 'uid'. */
		xml_node_get_str(&handle, user_node, "handle", 0, 0);
		xml_node_get_int(&uid, user_node, "uid", 0, 0);
		if (!handle || !uid) break;
		u = real_user_new(handle, uid);

		/* Irc masks. */
		for (j = 0; ; j++) {
			xml_node_get_str(&ircmask, user_node, "ircmask", j, 0);
			if (!ircmask) break;
			u->ircmasks = realloc(u->ircmasks, sizeof(char *) * (u->nircmasks+1));
			u->ircmasks[u->nircmasks] = strdup(ircmask);
			u->nircmasks++;
			ircmask_list_add(&ircmask_list, ircmask, u);
		}

		/* Settings. */
		for (j = 0; ; j++) {
			setting_node = xml_node_lookup(user_node, 0, "setting", j, 0);
			if (!setting_node) break;
			u->settings = realloc(u->settings, sizeof(*u->settings) * (j+1));
			u->nsettings++;
			setting = u->settings+j;
			xml_node_get_str(&flag_str, setting_node, "flags", 0, 0);
			if (flag_str) flag_from_str(&setting->flags, flag_str);
			else memset(&setting->flags, 0, sizeof(setting->flags));

			setting->nextended = 0;
			setting->extended = NULL;
			xml_node_get_str(&chan, setting_node, "chan", 0, 0);

			if (chan) setting->chan = strdup(chan);
			else if (j) continue;
			else setting->chan = NULL;

			for (k = 0; ; k++) {
				xml_node_get_str(&name, setting_node, "extended", k, "name", 0, 0);
				xml_node_get_str(&value, setting_node, "extended", k, "value", 0, 0);
				if (!name || !value) break;
				setting->extended = realloc(setting->extended, sizeof(*setting->extended) * (k+1));
				setting->nextended++;
				setting->extended[k].name = strdup(name);
				setting->extended[k].value = strdup(value);
			}
		}
		/* They have to have at least 1 setting, the global one. */
		if (!j) {
			u->settings = calloc(1, sizeof(u->settings));
			u->nsettings = 1;
		}
	}
	xml_node_destroy(root);
	return(0);
}

static int save_walker(const void *key, void *dataptr, void *param)
{
	xml_node_t *root = param;
	user_t *u = *(void **)dataptr;
	user_setting_t *setting;
	xml_node_t *user_node;
	int i, j;
	char flag_str[128];

	user_node = xml_node_new();
	user_node->name = strdup("user");
	xml_node_set_str(u->handle, user_node, "handle", 0, 0);
	xml_node_set_int(u->uid, user_node, "uid", 0, 0);
	for (i = 0; i < u->nircmasks; i++) {
		xml_node_set_str(u->ircmasks[i], user_node, "ircmask", i, 0);
	}
	for (i = 0; i < u->nsettings; i++) {
		setting = u->settings+i;
		if (setting->chan) xml_node_set_str(setting->chan, user_node, "setting", i, "chan", 0, 0);
		flag_to_str(&u->settings[i].flags, flag_str);
		xml_node_set_str(flag_str, user_node, "setting", i, "flags", 0, 0);
		for (j = 0; j < setting->nextended; j++) {
			xml_node_set_str(setting->extended[j].name, user_node, "setting", i, "extended", j, "name", 0, 0);
			xml_node_set_str(setting->extended[j].value, user_node, "setting", i, "extended", j, "value", 0, 0);
		}
	}
	xml_node_add(root, user_node);
	return(0);
}

int user_save(const char *fname)
{
	xml_node_t *root;
	FILE *fp;

	root = xml_node_new();
	xml_node_set_int(g_uid, root, "next_uid", 0, 0);
	xml_node_set_int(uid_wraparound, root, "uid_wraparound", 0, 0);
	hash_table_walk(uid_ht, save_walker, root);
	if (!fname) fname = "users.xml";
	fp = fopen(fname, "w");
	if (!fp) return(-1);
	xml_write_node(fp, root, 0);
	fclose(fp);
	xml_node_destroy(root);
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

static user_t *real_user_new(const char *handle, int uid)
{
	user_t *u;

	/* Make sure the handle is unique. */
	u = user_lookup_by_handle(handle);
	if (u) return(NULL);

	u = calloc(1, sizeof(*u));
	u->handle = strdup(handle);
	if (!uid) uid = user_get_uid();
	u->uid = uid;

	hash_table_insert(handle_ht, u->handle, u);
	hash_table_insert(uid_ht, (void *)u->uid, u);
	hash_table_check_resize(&handle_ht);
	hash_table_check_resize(&uid_ht);
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

	return(u);
}

static int user_really_delete(void *client_data)
{
	int i, j;
	user_t *u = client_data;
	user_setting_t *setting;

	/* Free the ircmasks. */
	for (i = 0; i < u->nircmasks; i++) free(u->ircmasks[i]);
	if (u->ircmasks) free(u->ircmasks);

	/* And all of the settings. */
	for (i = 0; i < u->nsettings; i++) {
		setting = u->settings+i;
		for (j = 0; j < setting->nextended; j++) {
			if (setting->extended[j].name) free(setting->extended[j].name);
			if (setting->extended[j].value) free(setting->extended[j].value);
		}
		if (setting->extended) free(setting->extended);
		if (setting->chan) free(setting->chan);
	}
	if (u->settings) free(u->settings);

	memset(u, 0, sizeof(*u));
	free(u);
	return(0);
}

int user_delete(user_t *u)
{
	int i;

	if (!u || (u->flags & USER_DELETED)) return(-1);

	nusers--;
	hash_table_delete(handle_ht, u->handle, NULL);
	hash_table_delete(uid_ht, (void *)u->uid, NULL);
	cache_user_del(u, "*");
	for (i = 0; i < u->nircmasks; i++) ircmask_list_del(&ircmask_list, u->ircmasks[i], u);
	u->flags |= USER_DELETED;
	bind_check(BT_udelete, NULL, NULL, u);
	garbage_add(user_really_delete, u, 0);
	return(0);
}

user_t *user_lookup_by_handle(const char *handle)
{
	user_t *u = NULL;

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

	if (u == info->u && wild_match(info->ircmask, irchost)) {
		info->entries = realloc(info->entries, sizeof(*info->entries) * (info->nentries+1));
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
	hash_table_walk(irchost_cache_ht, cache_check_del, &info);
	for (i = 0; i < info.nentries; i++) {
		hash_table_delete(irchost_cache_ht, info.entries[i], NULL);
		free((void *)info.entries[i]);
	}
	if (info.entries) free(info.entries);

	/* And remove it from the ircmask_list. */
	ircmask_list_del(&ircmask_list, ircmask, u);
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

	if (get_flags(u, chan, &oldflags)) return(-1);
	newflags.builtin = flags->builtin;
	newflags.udef = flags->udef;
	if (check_flag_change(u, chan, oldflags, &newflags)) return(0);
	oldflags->builtin = newflags.builtin;
	oldflags->udef = newflags.udef;
	return(0);
}

int user_set_flag_str(user_t *u, const char *chan, const char *flags)
{
	flags_t *oldflags, newflags;

	if (get_flags(u, chan, &oldflags)) return(-1);
	newflags.builtin = oldflags->builtin;
	newflags.udef = oldflags->udef;
	flag_merge_str(&newflags, flags);
	if (check_flag_change(u, chan, oldflags, &newflags)) return(0);
	oldflags->builtin = newflags.builtin;
	oldflags->udef = newflags.udef;
	return(0);
}

int user_check_flag_str(user_t *u, const char *flags, const char *chan, int method)
{
	return(0);
}

static int find_setting(user_t *u, const char *chan, const char *name, int *row, int *col)
{
	user_setting_t *setting;
	int i = 0;

	*row = -1;
	*col = -1;
	if (chan) for (i = 1; i < u->nsettings; i++) {
		if (!strcasecmp(chan, u->settings[i].chan)) break;
	}
	if (i >= u->nsettings) return(-1);
	*row = i;
	setting = u->settings+i;
	for (i = 0; i < setting->nextended; i++) {
		if (!strcasecmp(name, setting->extended[i].name)) {
			*col = i;
			return(i);
		}
	}
	return(-1);
}

int user_get_setting(user_t *u, const char *chan, const char *setting, char **valueptr)
{
	int i, j;

	if (find_setting(u, chan, setting, &i, &j) < 0) {
		*valueptr = NULL;
		return(-1);
	}
	*valueptr = u->settings[i].extended[j].value;
	return(0);
}

int user_set_setting(user_t *u, const char *chan, const char *setting, const char *newvalue)
{
	int i, j, r;
	char **value, *change;
	user_setting_t *setptr;

	change = egg_msprintf(NULL, 0, NULL, "%s %s", chan ? chan : "", setting);
	r = bind_check(BT_uset, NULL, change, u->handle, chan, setting, newvalue);
	free(change);

	if (r & BIND_RET_BREAK) return(0);

	if (find_setting(u, chan, setting, &i, &j) < 0) {
		/* See if we need to add the channel. */
		if (i < 0) {
			u->settings = realloc(u->settings, sizeof(*u->settings) * (u->nsettings+1));
			i = u->nsettings;
			u->nsettings++;
			memset(u->settings+i, 0, sizeof(*u->settings));
			u->settings[i].chan = strdup(chan);
		}
		setptr = u->settings+i;

		/* And then the setting. */
		if (j < 0) {
			setptr->extended = realloc(setptr->extended, sizeof(*setptr->extended) * (setptr->nextended+1));
			j = setptr->nextended;
			setptr->nextended++;
			setptr->extended[j].name = strdup(setting);
			setptr->extended[j].value = NULL;
		}
	}
	value = &(u->settings[i].extended[j].value);
	if (*value) free(*value);
	*value = strdup(newvalue);
	return(0);
}

int user_has_pass(user_t *u)
{
	char *hash, *salt;

	user_get_setting(u, NULL, "pass", &hash);
	user_get_setting(u, NULL, "salt", &salt);
	if (hash && salt) return(1);
	return(0);
}

int user_check_pass(user_t *u, const char *pass)
{
	char *hash, *salt, test[16], testhex[33];
	MD5_CTX ctx;

	user_get_setting(u, NULL, "pass", &hash);
	user_get_setting(u, NULL, "salt", &salt);
	if (!hash || !salt) return(0);

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
	int i;

	user_get_setting(u, NULL, "salt", &salt);
	if (!salt) {
		salt = new_salt;
		for (i = 0; i < 32; i++) {
			new_salt[i] = random() % 26 + 'A';
		}
		new_salt[i] = 0;
		user_set_setting(u, NULL, "salt", new_salt);
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
	if (!buf || bufsize < 0) return(-1);
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
