#include <stdio.h>
#include <string.h> /* strlen(), memset(), memmove() */
#include <stdlib.h>

#include "lib/egglib/hash_table.h"
typedef int (*Function)();
#include "registry.h"

typedef struct registry_internal_entry_b {
	char *key;
	Function callback;
	void *client_data;
	registry_entry_t **main; /* Pointer to the "main" function(s). */
	int nmain;
	registry_entry_t **chains; /* Chain functions. */
	int nchains;
	Function **listeners; /* Addresses of those who are using this entry. */
	void ***listener_handles; /* Ditto. */
	int nlisteners;
} registry_internal_entry_t;

static int update_listeners(registry_internal_entry_t *internal);
static int arbitrator(registry_internal_entry_t *internal, ...);

static hash_table_t *registry_table = NULL;

int registry_add(registry_entry_t *entry)
{
	registry_internal_entry_t *internal;
	char *key;

	if (!registry_table) {
		registry_table = hash_table_create(NULL, NULL, 13, 0);
	}

	key = (char *)malloc(strlen(entry->class)+strlen(entry->name)+4);
	sprintf(key, "%s / %s", entry->class, entry->name);

	/* Look up registry entry. */
	if (hash_table_find(registry_table, key, &internal)) {
		/* Doesn't exist yet, get a new one. */
		internal = (registry_internal_entry_t *)malloc(sizeof(*internal));
		memset(internal, 0, sizeof(*internal));
		internal->key = key;
		hash_table_insert(registry_table, key, internal);
	}
	else free(key);

	if (entry->flags & REGISTRY_MAIN) {
		internal->nmain++;
		internal->main = (registry_entry_t **)realloc(internal->main, internal->nmain * sizeof(void *));
		internal->main[internal->nmain-1] = entry;
	}
	else if (entry->flags & REGISTRY_CHAIN) {
		internal->nchains++;
		internal->chains = (registry_entry_t **)realloc(internal->chains, internal->nchains * sizeof(void *));
		if (entry->flags & REGISTRY_PREPEND) {
			memmove(internal->chains+1, internal->chains, sizeof(void *) * (internal->nchains-1));
			internal->chains[0] = entry;
		}
		else {
			internal->chains[internal->nchains-1] = entry;
		}
	}

	update_listeners(internal);

	return(0);
}

int registry_add_table(registry_entry_t *entries)
{
	while (entries->callback) {
		registry_add(entries);
		entries++;
	}
	return(0);
}

int registry_add_simple_chains(registry_simple_chain_t *entries)
{
	registry_entry_t *entry;
	char *class;

	/* First entry gives the class. */
	class = entries->name;
	entries++;
	while (entries->name) {
		entry = (registry_entry_t *)calloc(1, sizeof(*entry));
		entry->class = class;
		entry->name = entries->name;
		entry->callback = entries->callback;
		entry->nargs = entries->nargs;
		entry->flags = REGISTRY_CHAIN;
		registry_add(entry);
		entries++;
	}
	return(0);
}

int registry_remove(registry_entry_t *entry)
{
	printf("registry_remove(%s, %s)\n", entry->class, entry->name);
	return(0);
}

int registry_remove_table(registry_entry_t *entries)
{
	while (entries->callback) {
		registry_remove(entries);
		entries++;
	}
	return(0);
}

int registry_lookup(const char *class, const char *name, Function *funcptr, void **handleptr)
{
	registry_internal_entry_t *internal;
	char *key;

	key = (char *)malloc(strlen(class)+strlen(name)+4);
	sprintf(key, "%s / %s", class, name);

        if (!registry_table) {
                registry_table = hash_table_create(NULL, NULL, 13, 0);
        }

	if (hash_table_find(registry_table, key, &internal)) {
		/* Doesn't exist -- create it. */
                internal = (registry_internal_entry_t *)malloc(sizeof(*internal));
                memset(internal, 0, sizeof(*internal));
                internal->key = key;
                hash_table_insert(registry_table, key, internal);
		internal->callback = (Function) arbitrator;
		internal->client_data = (void *)internal;
	}
	else free(key);

	*funcptr = internal->callback;
	*handleptr = internal->client_data;

	internal->nlisteners++;
	internal->listeners = (Function **)realloc(internal->listeners, internal->nlisteners * sizeof(void *));
	internal->listener_handles = (void ***)realloc(internal->listener_handles, internal->nlisteners * sizeof(void *));
	internal->listeners[internal->nlisteners-1] = funcptr;
	internal->listener_handles[internal->nlisteners-1] = handleptr;

	return(0);
}

int registry_unlookup(const char *class, const char *name, Function *funcptr, void **handleptr)
{
/*
	memmove(entry->listeners+i, entry->listeners+i+1, (entry->nlisteners-i-1) * sizeof(Function *));
	entry->nlisteners--;
	if (!entry->nlisteners) {
		free(entry->listeners);
		entry->listeners = NULL;
	}
*/
	return(0);
}

static int update_listeners(registry_internal_entry_t *internal)
{
	int i;

	if (internal->nmain != 0 && internal->nchains == 0) {
		/* If there's just a main function, give them that. */
		internal->callback = internal->main[internal->nmain-1]->callback;
		internal->client_data = internal->main[internal->nmain-1]->client_data;
	}
	else {
		/* Otherwise we use an arbitrator. */
		internal->callback = (Function) arbitrator;
		internal->client_data = (void *)internal;
	}

	for (i = 0; i < internal->nlisteners; i++) {
		*(internal->listeners[i]) = internal->callback;
		*(internal->listener_handles[i]) = internal->client_data;
	}
	return(0);
}

static int arbitrator(registry_internal_entry_t *internal, ...)
{
	registry_entry_t *entry;
	int i;
	int *al = (int *)&internal;

	/* Call chains backwards (first-in-last-out). */
	for (i = internal->nchains-1; i >= 0; i--) {
		entry = internal->chains[i];
		entry->action = 0;
		switch (entry->nargs) {
			case 1: entry->callback(entry); break;
			case 2: entry->callback(entry, al[1]); break;
			case 3: entry->callback(entry, al[1], al[2]); break;
			case 4: entry->callback(entry, al[1], al[2], al[3]); break;
			case 5: entry->callback(entry, al[1], al[2], al[3], al[4]); break;
			case 6: entry->callback(entry, al[1], al[2], al[3], al[4], al[5]); break;
			case 7: entry->callback(entry, al[1], al[2], al[3], al[4], al[5], al[6]); break;
			case 8: entry->callback(entry, al[1], al[2], al[3], al[4], al[5], al[6], al[7]); break;
			case 9: entry->callback(entry, al[1], al[2], al[3], al[4], al[5], al[6], al[7], al[8]); break;
			case 10: entry->callback(entry, al[1], al[2], al[3], al[4], al[5], al[6], al[7], al[8], al[9]);
		}
		if (entry->action & REGISTRY_HALT) break;
	}

	if (internal->nmain != 0) {
		entry = internal->main[internal->nmain-1];
		switch (entry->nargs) {
			case 1: entry->callback(entry); break;
			case 2: entry->callback(entry, al[1]); break;
			case 3: entry->callback(entry, al[1], al[2]); break;
			case 4: entry->callback(entry, al[1], al[2], al[3]); break;
			case 5: entry->callback(entry, al[1], al[2], al[3], al[4]); break;
			case 6: entry->callback(entry, al[1], al[2], al[3], al[4], al[5]); break;
			case 7: entry->callback(entry, al[1], al[2], al[3], al[4], al[5], al[6]); break;
			case 8: entry->callback(entry, al[1], al[2], al[3], al[4], al[5], al[6], al[7]); break;
			case 9: entry->callback(entry, al[1], al[2], al[3], al[4], al[5], al[6], al[7], al[8]); break;
			case 10: entry->callback(entry, al[1], al[2], al[3], al[4], al[5], al[6], al[7], al[8], al[9]);
		}
		return((int)entry->return_value);
	}
	return(0);
}
