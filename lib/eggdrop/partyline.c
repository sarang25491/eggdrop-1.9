#include <stdarg.h>
#include <eggdrop/eggdrop.h>

/* When does pid wrap around? This lets pids get up to 99999. */
#define PID_WRAPAROUND	100000

/* Flags for partyline members. */
#define PARTY_ECHO	1
#define PARTY_DELETED	2

struct partychan;

typedef struct partymember {
	struct partymember *next, *prev;
	int pid;
	user_t *user;
	char *nick, *ident, *host;
	int idx;
	int flags;
} partymember_t;

typedef struct partychan {
	struct partychan *next, *prev;
	int cid;
	partymember_t **members;
	int nmembers;
} partychan_t;

static char *partyline_command_chars = NULL;

static bind_table_t *BT_cmd = NULL;	/* Commands. */
static bind_table_t *BT_party_out = NULL;	/* Text sent to a user. */
static hash_table_t *pid_ht = NULL;
static partymember_t *party_head = NULL;
static partychan_t *chan_head = NULL;
static int g_pid = 0;	/* Keep track of next available pid. */

static int on_putlog(int flags, const char *chan, const char *text, int len);

static bind_list_t log_binds[] = {
	{"", on_putlog},
	{0}
};

int partyline_init()
{
	str_redup(&partyline_command_chars, "./");
	pid_ht = hash_table_create(NULL, NULL, 13, HASH_TABLE_INTS);
	BT_cmd = bind_table_add("party", 5, "isUss", MATCH_PARTIAL, 0);
	BT_party_out = bind_table_add("party_out", 5, "isUsi", MATCH_NONE, 0);
	bind_add_list("log", log_binds);
	return(0);
}

/* Deleting partymembers. */
static int partymember_cleanup_needed = 0;

static void partymember_delete(partymember_t *p)
{
	/* Mark it as deleted so it doesn't get reused before it's free. */
	p->flags |= PARTY_DELETED;
	partymember_cleanup_needed++;
}

static void partymember_really_delete(partymember_t *p)
{
	/* Get rid of it from the main list and the hash table. */
	if (p->prev) p->prev->next = p->next;
	else party_head = p->next;
	if (p->next) p->next->prev = p->prev;
	hash_table_delete(pid_ht, (void *)p->pid);

	/* Free! */
	free(p->nick);
	free(p->ident);
	free(p->host);
	free(p);
}

static void partymember_cleanup()
{
	partymember_t *p, *next;

	if (!partymember_cleanup_needed) return;
	for (p = party_head; p; p = next) {
		next = p->next;
		if (p->flags & PARTY_DELETED) partymember_really_delete(p);
	}
	partymember_cleanup_needed = 0;
}

static partymember_t *partymember_lookup(int pid)
{
	partymember_t *p = NULL;

	hash_table_find(pid_ht, (void *)pid, &p);
	if (p && p->flags & PARTY_DELETED) p = NULL;
	return(p);
}

/* Introduce a user to the partyline after they authenticate. If you pass -1 as
 * the pid, then we generate a new one.
 * Returns the pid (partyline id). */
int partyline_connect(int idx, int pid, user_t *u, const char *nick, const char *ident, const char *host)
{
	partymember_t *p;

	if (pid == -1) {
		while (partymember_lookup(g_pid)) {
			g_pid++;
			if (g_pid >= PID_WRAPAROUND) g_pid = 0;
		}
		pid = g_pid++;
		if (g_pid >= PID_WRAPAROUND) g_pid = 0;
	}
	p = calloc(1, sizeof(*p));
	p->user = u;
	p->nick = strdup(nick);
	p->ident = strdup(ident);
	p->host = strdup(host);
	p->pid = g_pid++;
	p->idx = idx;
	p->next = party_head;
	if (p->next) p->next->prev = p;
	p->prev = NULL;
	party_head = p;
	hash_table_insert(pid_ht, (void *)p->pid, p);
	return(p->pid);
}

int partyline_disconnect(int pid, const char *msg)
{
	partymember_t *p;
	char *ptr, buf[1024];
	int buflen;

	p = partymember_lookup(pid);
	if (!p) return(-1);

	partymember_delete(p);
	if (p->idx != -1) sockbuf_delete(p->idx);

	if (!msg) msg = "Client disconnected";

	ptr = egg_msprintf(buf, sizeof(buf), &buflen, "%s has quit: %s\n", p->nick, msg);
	for (p = party_head; p; p = p->next) {
		if (p->flags & PARTY_DELETED || p->idx == -1) continue;
		sockbuf_write(p->idx, ptr, buflen);
	}
	if (ptr != buf) free(ptr);
	return(0);
}

/* 1 if the text starts with a recognized command character or 0 if not */
int partyline_is_command(const char *text)
{
	return strchr(partyline_command_chars, *text) ? 1 : 0;
}

/* Process any input from someone on the partyline (not bots, just users).
 * Everything is either a command or public text. Private messages and all that
 * are handled via commands. */
int partyline_on_input(int pid, const char *text, int len)
{
	if (!len) return(0);
	/* See if we should interpret it as a command. */
	if (strchr(partyline_command_chars, *text)) {
		text++;
		len--;
		/* If there are 2 command chars in a row, then it's not a
		 * command. We skip the command char and treat it as text. */
		if (*text != *(text-1)) {
			partyline_on_command(pid, text, len);
			return(0);
		}
	}
	/* It's not a command, so it's public text. */
	partyline_on_public(pid, text, len);
	return(0);
}

/* Processes input as a command. The first word is the command, and the rest
 * is the argument. */
int partyline_on_command(int pid, const char *text, int len)
{
	partymember_t *party;
	char *space, *cmd, *trailing;
	int r;

	party = partymember_lookup(pid);
	if (!party) return(0);

	space = strchr(text, ' ');
	if (space) {
		int cmdlen = space-text;
		cmd = malloc(cmdlen+1);
		memcpy(cmd, text, cmdlen);
		cmd[cmdlen] = 0;
		trailing = (char *)text+cmdlen+1;
	}
	else {
		cmd = (char *)text;
		trailing = "";
	}
	r = bind_check(BT_cmd, cmd, pid, party->nick, party->user, cmd, trailing);
	if (r & BIND_RET_LOG) {
		egg_iprintf(party->idx, "#%s# %s\n", party->nick, cmd);
	}
	if (cmd != text) free(cmd);
	return(0);
}

/* Process the input as public text on some channel. */
int partyline_on_public(int pid, const char *text, int len)
{
	partymember_t *p;
	char *ptr, buf[1024];
	int buflen;

	p = partymember_lookup(pid);
	if (!p) return(0);

	ptr = egg_msprintf(buf, sizeof(buf), &buflen, "<%s> %s\n", p->nick, text);

	for (p = party_head; p; p = p->next) {
		if (p->flags & PARTY_DELETED || p->idx == -1 || (p->pid == pid && !(p->flags & PARTY_ECHO))) continue;
		sockbuf_write(p->idx, ptr, buflen);
	}
	if (ptr != buf) free(ptr);
	//bind_check(BT_public, NULL, pid, party->nick, party->user, party->chan, text, len);
	return(0);
}

int partyline_update_info(int pid, const char *ident, const char *host)
{
	partymember_t *p;

	p = partymember_lookup(pid);
	if (!p) return(-1);
	if (ident) str_redup(&p->ident, ident);
	if (host) str_redup(&p->host, host);
	return(0);
}

/* If it has an idx associated with it, write it. Otherwise we just
 * execute the party_write bind and let anybody interested catch it
 * (like botnet). */
int partyline_write(int pid, const char *text)
{
	partymember_t *p;
	int r, len;

	p = partymember_lookup(pid);
	if (!p) return(-2);
	len = strlen(text);
	if (p->idx >= 0) r = sockbuf_write(p->idx, text, len);
	else r = -3;
	bind_check(BT_party_out, NULL, pid, p->nick, p->user, text, len);
	return(r);
}

int partyline_printf(int pid, const char *fmt, ...)
{
	va_list args;
	partymember_t *p;
	char *ptr, buf[1024];
	int len;

	p = partymember_lookup(pid);
	if (!p) return(-2);

	va_start(args, fmt);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, fmt, args);
	va_end(args);

	if (p->idx != -1) sockbuf_write(p->idx, ptr, len);
	bind_check(BT_party_out, NULL, pid, p->nick, p->user, ptr, len);
	if (ptr != buf) free(ptr);
	return(0);
}

/* Logging stuff. */
static int on_putlog(int flags, const char *chan, const char *text, int len)
{
	char *ptr, buf[1024];
	int buflen;
	partymember_t *p;

	ptr = egg_msprintf(buf, sizeof(buf), &buflen, "%s\n", text);

	for (p = party_head; p; p = p->next) {
		if ((p->flags & PARTY_DELETED) || p->idx == -1) continue;
		sockbuf_write(p->idx, ptr, buflen);
	}
	if (ptr != buf) free(ptr);
	return(0);
}

/* Channel stuff. */

int partyline_chan;
