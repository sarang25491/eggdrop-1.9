#include <stdlib.h>
#include <eggdrop/eggdrop.h>

extern struct dcc_t *dcc;
extern int dcc_total;

/* The head of the bind table linked list. */
static bind_table_t *bind_table_list_head = NULL;

/* Garbage collection stuff. */
static void bind_table_really_del(bind_table_t *table);
static void bind_entry_really_del(bind_table_t *table, bind_entry_t *entry);

static int internal_bind_cleanup()
{
	bind_table_t *table, *next_table;
	bind_entry_t *entry, *next_entry;

	for (table = bind_table_list_head; table; table = next_table) {
		next_table = table->next;
		if (table->flags & BIND_DELETED) {
			bind_table_really_del(table);
			continue;
		}
		for (entry = table->entries; entry; entry = next_entry) {
			next_entry = entry->next;
			if (entry->flags & BIND_DELETED) bind_entry_really_del(table, entry);
		}
	}
	return(0);
}

static void schedule_bind_cleanup()
{
	garbage_add(internal_bind_cleanup, NULL, GARBAGE_ONCE);
}

void kill_binds(void)
{
	while (bind_table_list_head) bind_table_del(bind_table_list_head);
}

bind_table_t *bind_table_add(const char *name, int nargs, const char *syntax, int match_type, int flags)
{
	bind_table_t *table;

	for (table = bind_table_list_head; table; table = table->next) {
		if (!strcmp(table->name, name)) break;
	}

	/* If it doesn't exist, create it. */
	if (!table) {
		table = (bind_table_t *)calloc(1, sizeof(*table));
		table->name = strdup(name);
		table->next = bind_table_list_head;
		bind_table_list_head = table;
	}
	else if (!(table->flags & BIND_FAKE)) return(table);

	table->nargs = nargs;
	if (syntax) table->syntax = strdup(syntax);
	table->match_type = match_type;
	table->flags = flags;
	return(table);
}

void bind_table_del(bind_table_t *table)
{
	bind_table_t *cur, *prev;

	for (prev = NULL, cur = bind_table_list_head; cur; prev = cur, cur = cur->next) {
		if (!strcmp(table->name, cur->name)) break;
	}

	/* If it's found, remove it from the list. */
	if (cur) {
		if (prev) prev->next = cur->next;
		else bind_table_list_head = cur->next;
	}

	/* Now delete it. */
	table->flags |= BIND_DELETED;
	schedule_bind_cleanup();
}

static void bind_table_really_del(bind_table_t *table)
{
	bind_entry_t *entry, *next;

	free(table->name);
	for (entry = table->entries; entry; entry = next) {
		next = entry->next;
		if (entry->function_name) free(entry->function_name);
		if (entry->mask) free(entry->mask);
		free(entry);
	}
	free(table);
}

bind_table_t *bind_table_lookup(const char *name)
{
	bind_table_t *table;

	for (table = bind_table_list_head; table; table = table->next) {
		if (!(table->flags & BIND_DELETED) && !strcmp(table->name, name)) break;
	}
	return(table);
}

bind_table_t *bind_table_lookup_or_fake(const char *name)
{
	bind_table_t *table;

	table = bind_table_lookup(name);
	if (!table) table = bind_table_add(name, 0, NULL, 0, BIND_FAKE);
	return(table);
}

/* Look up a bind entry based on either function name or id. */
bind_entry_t *bind_entry_lookup(bind_table_t *table, int id, const char *mask, const char *function_name, Function callback)
{
	bind_entry_t *entry;
	int hit;

	for (entry = table->entries; entry; entry = entry->next) {
		if (entry->flags & BIND_DELETED) continue;
		if (entry->id >= 0) {
			if (entry->id == id) break;
		}
		else {
			hit = 0;
			if (entry->mask && !strcmp(entry->mask, mask)) hit++;
			else if (!entry->mask) hit++;
			if (entry->function_name && !strcmp(entry->function_name, function_name)) hit++;
			if (entry->callback == callback || !callback) hit++;
			if (hit == 3) break;
		}
	}
	return(entry);
}

int bind_entry_del(bind_table_t *table, int id, const char *mask, const char *function_name, Function callback, void *cdataptr)
{
	bind_entry_t *entry;

	entry = bind_entry_lookup(table, id, mask, function_name, callback);
	if (!entry) return(-1);

	if (cdataptr) *(void **)cdataptr = entry->client_data;

	/* Delete it. */
	entry->flags |= BIND_DELETED;
	schedule_bind_cleanup();
	return(0);
}

static void bind_entry_really_del(bind_table_t *table, bind_entry_t *entry)
{
	if (entry->next) entry->next->prev = entry->prev;
	if (entry->prev) entry->prev->next = entry->next;
	else table->entries = entry->next;
	if (entry->function_name) free(entry->function_name);
	if (entry->mask) free(entry->mask);
	memset(entry, 0, sizeof(*entry));
	free(entry);
}

/* Modify a bind entry's flags and mask. */
int bind_entry_modify(bind_table_t *table, int id, const char *mask, const char *function_name, const char *newflags, const char *newmask)
{
	bind_entry_t *entry;

	entry = bind_entry_lookup(table, id, mask, function_name, NULL);
	if (!entry) return(-1);

	/* Modify it. */
	if (newflags) flag_from_str(&entry->user_flags, newflags);
	if (newmask) str_redup(&entry->mask, newmask);

	return(0);
}

/* Overwrite a bind entry's callback and client_data. */
int bind_entry_overwrite(bind_table_t *table, int id, const char *mask, const char *function_name, Function callback, void *client_data)
{
	bind_entry_t *entry;

	entry = bind_entry_lookup(table, id, mask, function_name, NULL);
	if (!entry) return(-1);

	entry->callback = callback;
	entry->client_data = client_data;
	return(0);
}

int bind_entry_add(bind_table_t *table, const char *flags, const char *mask, const char *function_name, int bind_flags, Function callback, void *client_data)
{
	bind_entry_t *entry, *old_entry;

	old_entry = bind_entry_lookup(table, -1, mask, function_name, NULL);

	if (old_entry) {
		if (table->flags & BIND_STACKABLE) {
			entry = (bind_entry_t *)calloc(1, sizeof(*entry));
			entry->prev = old_entry;
			entry->next = old_entry->next;
			old_entry->next = entry;
			if (entry->next) entry->next->prev = entry;
		}
		else {
			entry = old_entry;
			if (entry->function_name) free(entry->function_name);
			if (entry->mask) free(entry->mask);
		}
	}
	else {
		for (old_entry = table->entries; old_entry && old_entry->next; old_entry = old_entry->next) {
			; /* empty loop */
		}
		entry = (bind_entry_t *)calloc(1, sizeof(*entry));
		if (old_entry) old_entry->next = entry;
		else table->entries = entry;
		entry->prev = old_entry;
	}

	if (flags) flag_from_str(&entry->user_flags, flags);
	if (mask) entry->mask = strdup(mask);
	if (function_name) entry->function_name = strdup(function_name);
	entry->callback = callback;
	entry->client_data = client_data;
	entry->flags = bind_flags;

	return(0);
}

/* Execute a bind entry with the given argument list. */
static int bind_entry_exec(bind_table_t *table, bind_entry_t *entry, void **al)
{
	bind_entry_t *prev;
	int retval;

	/* Give this entry a hit. */
	entry->nhits++;

	/* Does the callback want client data? */
	if (entry->flags & BIND_WANTS_CD) {
		*al = entry->client_data;
	}
	else al++;

	retval = entry->callback(al[0], al[1], al[2], al[3], al[4], al[5], al[6], al[7], al[8], al[9]);

	if (table->match_type & (MATCH_MASK | MATCH_PARTIAL | MATCH_NONE)) return(retval);

	/* Search for the last entry that isn't deleted. */
	for (prev = entry->prev; prev; prev = prev->prev) {
		if (!(prev->flags & BIND_DELETED) && (prev->nhits >= entry->nhits)) break;
	}

	/* See if this entry is more popular than the preceding one. */
	if (entry->prev != prev) {
		/* Remove entry. */
		if (entry->prev) entry->prev->next = entry->next;
		else table->entries = entry->next;
		if (entry->next) entry->next->prev = entry->prev;

		/* Re-add in correct position. */
		if (prev) {
			entry->next = prev->next;
			if (prev->next) prev->next->prev = entry;
			prev->next = entry;
		}
		else {
			entry->next = table->entries;
			table->entries = entry;
		}
		entry->prev = prev;
		if (entry->next) entry->next->prev = entry;
	}
	return(retval);
}

int bind_check(bind_table_t *table, flags_t *user_flags, const char *match, ...)
{
	void *args[11];
	bind_entry_t *entry, *next, *winner = NULL;
	int i, cmp, retval;
	int tie = 0, matchlen = 0;
	va_list ap;

	va_start(ap, match);
	for (i = 1; i <= table->nargs; i++) {
		args[i] = va_arg(ap, void *);
	}
	va_end(ap);

	/* Default return value is 0 */
	retval = 0;

	/* Check if we're searching for a partial match. */
	if (table->match_type & MATCH_PARTIAL) matchlen = strlen(match);

	for (entry = table->entries; entry; entry = next) {
		next = entry->next;
		if (entry->flags & BIND_DELETED) continue;

		/* Check flags. */
		if (table->match_type & MATCH_FLAGS) {
			if (!(entry->user_flags.builtin | entry->user_flags.udef)) cmp = 1;
			else if (!user_flags) cmp = 0;
			else if (entry->flags & MATCH_FLAGS_AND) cmp = flag_match_subset(&entry->user_flags, user_flags);
			else cmp = flag_match_partial(user_flags, &entry->user_flags);
			if (!cmp) continue;
		}

		if (table->match_type & MATCH_MASK) {
			cmp = !wild_match_per((unsigned char *)entry->mask, (unsigned char *)match);
		}
		else if (table->match_type & MATCH_NONE) {
			cmp = 0;
		}
		else if (table->match_type & MATCH_PARTIAL) {
			cmp = 1;
			if (!strncasecmp(match, entry->mask, matchlen)) {
				winner = entry;
				/* Is it an exact match? */
				if (!entry->mask[matchlen]) {
					tie = 1;
					break;
				}
				else tie++;
			}
		}
		else {
			if (table->match_type & MATCH_CASE) cmp = strcmp(entry->mask, match);
			else cmp = strcasecmp(entry->mask, match);
		}
		if (cmp) continue; /* Doesn't match. */

		retval = bind_entry_exec(table, entry, args);
		if ((table->flags & BIND_BREAKABLE) && (retval & BIND_RET_BREAK)) break;
	}
	/* If it's a partial match table, see if we have 1 winner. */
	if (winner && tie == 1) {
		retval = bind_entry_exec(table, winner, args);
	}
	return(retval);
}

void bind_add_list(const char *table_name, bind_list_t *cmds)
{
	char name[50];
	bind_table_t *table;

	table = bind_table_lookup_or_fake(table_name);

	for (; cmds->callback; cmds++) {
		snprintf(name, 50, "*%s:%s", table->name, cmds->mask ? cmds->mask : "");
		name[49] = 0;
		bind_entry_add(table, cmds->user_flags, cmds->mask, name, 0, cmds->callback, NULL);
	}
}

void bind_add_simple(const char *table_name, const char *flags, const char *mask, Function callback)
{
	char name[50];
	bind_table_t *table;

	table = bind_table_lookup_or_fake(table_name);

	snprintf(name, 50, "*%s:%s", table->name, mask ? mask : "");
	name[49] = 0;
	bind_entry_add(table, flags, mask, name, 0, callback, NULL);
}

void bind_rem_list(const char *table_name, bind_list_t *cmds)
{
	char name[50];
	bind_table_t *table;

	table = bind_table_lookup(table_name);
	if (!table) return;

	for (; cmds->mask; cmds++) {
		snprintf(name, 50, "*%s:%s", table->name, cmds->mask);
		name[49] = 0;
		bind_entry_del(table, -1, cmds->mask, name, cmds->callback, NULL);
	}
}

void bind_rem_simple(const char *table_name, const char *flags, const char *mask, Function callback)
{
	char name[50];
	bind_table_t *table;

	table = bind_table_lookup(table_name);
	if (!table) return;

	snprintf(name, 50, "*%s:%s", table->name, mask);
	name[49] = 0;
	bind_entry_del(table, -1, mask, name, callback, NULL);
}
