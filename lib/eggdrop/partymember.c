#include <stdarg.h>
#include <eggdrop/eggdrop.h>

/* When does pid wrap around? This lets pids get up to 99999. */
#define PID_WRAPAROUND	100000

static hash_table_t *pid_ht = NULL;
static partymember_t *party_head = NULL;
static int g_pid = 0;	/* Keep track of next available pid. */

static int partymember_cleanup(void *client_data);

int partymember_init()
{
	pid_ht = hash_table_create(NULL, NULL, 13, HASH_TABLE_INTS);
	return(0);
}

static void partymember_really_delete(partymember_t *p)
{
	/* Get rid of it from the main list and the hash table. */
	if (p->prev) p->prev->next = p->next;
	else party_head = p->next;
	if (p->next) p->next->prev = p->prev;
	hash_table_delete(pid_ht, (void *)p->pid, NULL);

	/* Free! */
	free(p->nick);
	free(p->ident);
	free(p->host);
	free(p);
}

static int partymember_cleanup(void *client_data)
{
	partymember_t *p, *next;

	for (p = party_head; p; p = next) {
		next = p->next;
		if (p->flags & PARTY_DELETED) partymember_really_delete(p);
	}
	return(0);
}

partymember_t *partymember_lookup_pid(int pid)
{
	partymember_t *p = NULL;

	hash_table_find(pid_ht, (void *)pid, &p);
	if (p && p->flags & PARTY_DELETED) p = NULL;
	return(p);
}

int partymember_get_pid()
{
	int pid;

	while (partymember_lookup_pid(g_pid)) {
		g_pid++;
		if (g_pid >= PID_WRAPAROUND) g_pid = 0;
	}
	pid = g_pid++;
	if (g_pid >= PID_WRAPAROUND) g_pid = 0;
	return(pid);
}

partymember_t *partymember_new(int pid, user_t *user, const char *nick, const char *ident, const char *host, partyline_event_t *handler, void *client_data)
{
	partymember_t *mem;

	mem = calloc(1, sizeof(*mem));
	if (pid == -1) pid = partymember_get_pid();
	mem->pid = pid;
	mem->user = user;
	mem->nick = strdup(nick);
	mem->ident = strdup(ident);
	mem->host = strdup(host);
	mem->handler = handler;
	mem->client_data = client_data;
	mem->next = party_head;
	if (party_head) party_head->prev = mem;
	party_head = mem;
	return(mem);
}

int partymember_delete(partymember_t *p, const char *text)
{
	int i;

	if (p->flags & PARTY_DELETED) return(-1);

	/* Mark it as deleted so it doesn't get reused before it's free. */
	p->flags |= PARTY_DELETED;
	garbage_add(partymember_cleanup, NULL, GARBAGE_ONCE);

	/* Remove from any stray channels. */
	for (i = p->nchannels-1; i >= 0; i--) {
		partychan_part(p->channels[i], p, text);
	}
	p->nchannels = 0;

	if (p->handler->on_delete) {
		(p->handler->on_delete)(p->client_data);
	}
	return(0);
}

int partymember_update_info(partymember_t *p, const char *ident, const char *host)
{
	if (!p) return(-1);
	if (ident) str_redup(&p->ident, ident);
	if (host) str_redup(&p->host, host);
	return(0);
}

int partymember_write_pid(int pid, const char *text, int len)
{
	partymember_t *p;

	p = partymember_lookup_pid(pid);
	return partymember_write(p, text, len);
}

int partymember_write(partymember_t *p, const char *text, int len)
{
	if (!p || p->flags & PARTY_DELETED) return(-1);

	if (len < 0) len = strlen(text);
	(p->handler->on_privmsg)(p->client_data, p, NULL, text, len);
	return(0);
}

int partymember_printf_pid(int pid, const char *fmt, ...)
{
	va_list args;
	char *ptr, buf[1024];
	int len;

	va_start(args, fmt);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, fmt, args);
	va_end(args);

	partymember_write_pid(pid, ptr, len);

	if (ptr != buf) free(ptr);
	return(0);
}

int partymember_printf(partymember_t *p, const char *fmt, ...)
{
	va_list args;
	char *ptr, buf[1024];
	int len;

	va_start(args, fmt);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, fmt, args);
	va_end(args);

	partymember_write(p, ptr, len);

	if (ptr != buf) free(ptr);
	return(0);
}
