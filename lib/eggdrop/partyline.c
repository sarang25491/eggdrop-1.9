#include <eggdrop/eggdrop.h>

typedef struct {
	int pid;
	user_t *user;
	char *nick, *ident, *host;
	int idx;
} partymember_t;

static char *partyline_command_chars = NULL;

static bind_table_t *BT_cmd = NULL;
static hash_table_t *pid_ht = NULL;
static int g_pid = 0;

int partyline_init()
{
	str_redup(&partyline_command_chars, "./");
	pid_ht = hash_table_create(NULL, NULL, 13, HASH_TABLE_INTS);
	BT_cmd = bind_table_add("party", 5, "isUss", MATCH_PARTIAL, 0);
	return(0);
}

static partymember_t *partymember_lookup(int pid)
{
	partymember_t *p;

	hash_table_find(pid_ht, (void *)pid, &p);
	return(p);
}

/* Introduce a user to the partyline after they authenticate.
 * Returns a pid (partyline id). */
int partyline_on_connect(int idx, user_t *u, const char *nick, const char *ident, const char *host)
{
	partymember_t *p;

	p = calloc(1, sizeof(*p));
	p->user = u;
	p->nick = strdup(nick);
	p->ident = strdup(ident);
	p->host = strdup(host);
	p->pid = g_pid++;
	p->idx = idx;
	hash_table_insert(pid_ht, (void *)p->pid, p);
	return(p->pid);
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
	int r;

	p = partymember_lookup(pid);
	if (!p) return(-2);
	if (p->idx >= 0) r = sockbuf_write(p->idx, text, -1);
	else r = -3;
	return(r);
}
