#include <stdlib.h>
#include <eggdrop/garbage.h>

typedef struct garbage_list {
	struct garbage_list *next;
	garbage_proc_t garbage_proc;
	void *client_data;
} garbage_list_t;

static garbage_list_t *garbage_list_head = NULL;

int garbage_add(garbage_proc_t garbage_proc, void *client_data, int flags)
{
	garbage_list_t *g;

	/* Make sure it's not a duplicate (helpful for cleanup functions). */
	if (flags & GARBAGE_ONCE) {
		for (g = garbage_list_head; g; g = g->next) {
			if (g->garbage_proc == garbage_proc && g->client_data == client_data) return(0);
		}
	}

	/* Create the entry. */
	g = malloc(sizeof(*g));
	g->garbage_proc = garbage_proc;
	g->client_data = client_data;

	/* Add to list. */
	g->next = garbage_list_head;
	garbage_list_head = g;

	return(0);
}

/* Should be called only from the main loop. */
int garbage_run()
{
	garbage_list_t *g;

	for (g = garbage_list_head; g; g = garbage_list_head) {
		garbage_list_head = g->next;
		(g->garbage_proc)(g->client_data);
		free(g);
	}
	return(0);
}
