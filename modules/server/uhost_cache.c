#include "server.h"

#define UHOST_CACHE_SIZE 47

static hash_table_t *uhost_cache_ht = NULL;

/* Prototypes. */
static int uhost_cache_delete(const void *key, void *data, void *param);

void uhost_cache_init()
{
	uhost_cache_ht = hash_table_create(NULL, NULL, UHOST_CACHE_SIZE, HASH_TABLE_STRINGS);
}

/* Reset everything when we disconnect from the server. */
void uhost_cache_reset()
{
	uhost_cache_destroy();
	uhost_cache_init();
}

void uhost_cache_destroy()
{
	hash_table_walk(uhost_cache_ht, uhost_cache_delete, NULL);
	hash_table_delete(uhost_cache_ht);
}

static int uhost_cache_delete(const void *key, void *dataptr, void *param)
{
	uhost_cache_entry_t *cache = *(void **)dataptr;

	if (cache->nick) free(cache->nick);
	if (cache->uhost) free(cache->uhost);
	free(cache);
	return(0);
}

static uhost_cache_entry_t *cache_lookup(const char *nick)
{
	char buf[64], *lnick;
	uhost_cache_entry_t *cache = NULL;

	lnick = egg_msprintf(buf, sizeof(buf), NULL, "%s", nick);
	str_tolower(lnick);
	hash_table_find(uhost_cache_ht, lnick, &cache);
	if (lnick != buf) free(lnick);
	return(cache);
}

char *uhost_cache_lookup(const char *nick)
{
	uhost_cache_entry_t *cache;

	cache = cache_lookup(nick);
	if (cache) return(cache->uhost);
	return(NULL);
}

void uhost_cache_addref(const char *nick, const char *uhost)
{
	char buf[64], *lnick;
	uhost_cache_entry_t *cache = NULL;

	lnick = egg_msprintf(buf, sizeof(buf), NULL, "%s", nick);
	str_tolower(lnick);
	hash_table_find(uhost_cache_ht, lnick, &cache);
	if (!cache) {
		cache = calloc(1, sizeof(*cache));
		cache->nick = strdup(nick);
		str_tolower(cache->nick);
		if (uhost) cache->uhost = strdup(uhost);
		hash_table_insert(uhost_cache_ht, cache->nick, cache);
	}
	cache->ref_count++;
	if (lnick != buf) free(lnick);
}

void uhost_cache_decref(const char *nick)
{
	uhost_cache_entry_t *cache;

	/* We don't decrement ourselves.. we always know our own host. */
	if (match_my_nick(nick)) return;

	cache = cache_lookup(nick);
	if (!cache) return;

	cache->ref_count--;
	if (cache->ref_count <= 0) {
		hash_table_remove(uhost_cache_ht, cache->nick, NULL);
		uhost_cache_delete(NULL, cache, NULL);
	}
}

void uhost_cache_swap(const char *old_nick, const char *new_nick)
{
	uhost_cache_entry_t *cache;
	char buf[64], *lnick;

	lnick = egg_msprintf(buf, sizeof(buf), NULL, "%s", old_nick);
	str_tolower(lnick);
	hash_table_remove(uhost_cache_ht, lnick, &cache);
	if (lnick != buf) free(lnick);

	if (cache) {
		str_redup(&cache->nick, new_nick);
		str_tolower(cache->nick);
		hash_table_insert(uhost_cache_ht, cache->nick, cache);
	}
}
