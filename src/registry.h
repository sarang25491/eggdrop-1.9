#ifndef _REGISTRY_H_
#define _REGISTRY_H_

#define REGISTRY_DONT_CREATE	1
#define REGISTRY_PREPEND	2
#define REGISTRY_CHAIN		4
#define REGISTRY_MAIN		8
#define REGISTRY_DEFAULT	8

#define REGISTRY_TEMP_LISTENER	1

#define REGISTRY_HALT		1
#define REGISTRY_SKIP_MAIN	2

typedef struct registry_entry_b {
	char *class;
	char *name;
	Function callback;
	void *client_data;
	int nargs;
	int flags;
	int action;
	void *return_value;
} registry_entry_t;

typedef struct registry_simple_chain_b {
	char *name;
	Function callback;
	int nargs;
} registry_simple_chain_t;

#ifndef MAKING_MODS
int registry_add(registry_entry_t *entry);
int registry_add_table(registry_entry_t *entries);
int registry_add_simple_chains(registry_simple_chain_t *table);
int registry_remove(registry_entry_t *entry);
int registry_remove_table(registry_entry_t *entries);
int registry_lookup(const char *class, const char *name, Function *funcptr, void **handeptr);
int registry_unlookup(const char *class, const char *name, Function *funcptr, void **handleptr);
#endif /* MAKING_MODS */

#endif /* _REGISTRY_H_ */
